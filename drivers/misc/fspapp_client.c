// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. */
/*
 * fspapp_client.c - FSPApp QTEE Service Client Driver
 */
#define pr_fmt(fmt) "fspapp: %s: " fmt, __func__

#include <linux/version.h>
#include <linux/dma-buf.h>
#include <linux/firmware.h>
#include <linux/key.h>
#include <linux/key-type.h>
#include <keys/user-type.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/qtee_shmbridge.h>
#include <linux/firmware/qcom/si_object.h>
#include <linux/firmware/qcom/si_core_xts.h>

/* ========================================================================= */
/* IFSPApp definitions                                                        */
/* ========================================================================= */

#define IFSPApp_WIDEVINE_FEATURE_ID               1
#define IFSPApp_HDCP1_FEATURE_ID                  2
#define IFSPApp_HDCP2_FEATURE_ID                  3
#define IFSPApp_UIE_KEY_0_FEATURE_ID              4
#define IFSPApp_UIE_KEY_1_FEATURE_ID              5
#define IFSPApp_KEYMASTER_FEATURE_ID              6
#define IFSPApp_FSP_WRAP_KEY_FEATURE_ID           7

#define IFSPApp_ERROR_INVALID_PARAM               10
#define IFSPApp_ERROR_INVALID_SIZE                11
#define IFSPApp_ERROR_CMD_NOT_AVAILABLE           12
#define IFSPApp_ERROR_UNSUPPORTED_FEATURE_ID      13
#define IFSPApp_ERROR_UIE_PROVISIONING_DISABLED   14
#define IFSPApp_ERROR_OEM_IMAGE_ENCRYPTION_WRITE_DISABLED 15
#define IFSPApp_ERROR_QFPROM_STATUS_CHECK         16
#define IFSPApp_ERROR_NO_MEMORY                   17
#define IFSPApp_ERROR_BAD_DATA                    18
#define IFSPApp_ERROR_NOT_ALLOWED                 19
#define IFSPApp_ERROR_BUFFER_TOO_SMALL            20

#define IFSPApp_OP_get_public_key                 0
#define IFSPApp_OP_decrypt_payload                1
#define IFSPApp_OP_decrypt_and_provision_key      2
#define IFSPApp_OP_unwrap_wrapped_key             3

#define FSPAPP_SERVICE_UID  0x000001c1

#define WRAPPED_KEY_FW "wrapped_key.mbn"

struct ifspapp_wrap_key_blob {
	uint32_t feature_id;
	uint32_t is_encrypted;
	uint8_t  wrapkey[12288];
	uint64_t wrapkey_size;
};

/* ========================================================================= */
/* Globals                                                                    */
/* ========================================================================= */

static struct si_object *fspapp_service;
static struct si_object_invoke_ctx fspapp_oic;

static DEFINE_MUTEX(fspapp_lock);

struct key *dmcrypt_keyring;
EXPORT_SYMBOL_GPL(dmcrypt_keyring);

/* ========================================================================= */
/* Keyring management                                                         */
/* ========================================================================= */

static int add_cek_to_keyring(const u8 *cek, size_t cek_len)
{
	key_ref_t key_ref;
	key_ref_t search_ref;
	int ret;

	static const char desc[] = "crypt:rootfs_key";

	if (!cek || !cek_len)
		return -EINVAL;

	if (!dmcrypt_keyring) {
		dmcrypt_keyring = keyring_alloc(
				"dmcrypt",
				GLOBAL_ROOT_UID,
				GLOBAL_ROOT_GID,
				current_cred(),
				KEY_POS_ALL,
				KEY_ALLOC_NOT_IN_QUOTA,
				NULL,
				NULL);

		if (IS_ERR(dmcrypt_keyring)) {
			ret = PTR_ERR(dmcrypt_keyring);
			dmcrypt_keyring = NULL;
			pr_err("keyring_alloc failed rc=%d\n", ret);
			return ret;
		}
	}

	search_ref = keyring_search(
			make_key_ref(dmcrypt_keyring, true),
			&key_type_logon,
			desc,
			false);

	if (!IS_ERR(search_ref)) {
		key_ref_put(search_ref);
		return 0;
	}

	key_ref = key_create_or_update(
			make_key_ref(dmcrypt_keyring, true),
			"logon",
			desc,
			cek,
			cek_len,
			KEY_POS_ALL,
			KEY_ALLOC_IN_QUOTA);

	if (IS_ERR(key_ref)) {
		ret = PTR_ERR(key_ref);
		pr_err("logon key create failed rc=%d\n", ret);
		return ret;
	}

	key_ref_put(key_ref);
	return 0;
}

/* ========================================================================= */
/* Wrapped key loading                                                        */
/* ========================================================================= */

static int load_wrapped_key(struct ifspapp_wrap_key_blob *blob, struct device *dev)
{
	const struct firmware *fw;
	int ret;

	if (!blob || !dev)
		return -EINVAL;

	memset(blob, 0, sizeof(*blob));

	ret = request_firmware(&fw, WRAPPED_KEY_FW, dev);
	if (ret) {
		dev_err(dev, "request_firmware(%s) failed rc=%d\n",
			WRAPPED_KEY_FW, ret);
		return ret;
	}

	if (fw->size > sizeof(blob->wrapkey)) {
		dev_err(dev, "wrapped key too large (%zu > %zu)\n",
			fw->size, sizeof(blob->wrapkey));
		release_firmware(fw);
		return -EOVERFLOW;
	}

	blob->feature_id  = IFSPApp_FSP_WRAP_KEY_FEATURE_ID;
	blob->is_encrypted = 1;
	memcpy(blob->wrapkey, fw->data, fw->size);
	blob->wrapkey_size = fw->size;

	release_firmware(fw);
	return 0;
}

/* ========================================================================= */
/* Service lifecycle                                                          */
/* ========================================================================= */

static int open_fspapp_service(struct device *dev)
{
	struct si_object *client_env = NULL_SI_OBJECT;
	int ret;

	ret = si_core_get_client_env(&fspapp_oic, &client_env);
	if (ret) {
		dev_err(dev, "si_core_get_client_env failed rc=%d\n", ret);
		return ret;
	}

	if (!client_env || client_env == NULL_SI_OBJECT) {
		dev_err(dev, "si_core_get_client_env returned invalid env\n");
		return -ENODEV;
	}

	ret = si_core_client_env_open(
			&fspapp_oic,
			client_env,
			FSPAPP_SERVICE_UID,
			&fspapp_service);

	if (ret) {
		dev_err(dev, "si_core_client_env_open failed uid=0x%x rc=%d\n",
			FSPAPP_SERVICE_UID, ret);
		fspapp_service = NULL;
		return ret;
	}

	return 0;
}

static void close_fspapp_service(void)
{
	mutex_lock(&fspapp_lock);

	if (fspapp_service) {
		put_si_object(fspapp_service);
		fspapp_service = NULL;
	}

	mutex_unlock(&fspapp_lock);
}

/* ========================================================================= */
/* Exported unwrap API                                                        */
/* ========================================================================= */

int32_t fspapp_unwrap_wrapped_key(
	const struct ifspapp_wrap_key_blob *wrapped_key,
	void *cek_key,
	size_t cek_key_sz,
	size_t *cek_key_len_out)
{
	struct si_arg args[3] = { };
	int ret;
	int result;

	mutex_lock(&fspapp_lock);

	if (!fspapp_service) {
		mutex_unlock(&fspapp_lock);
		return -ENODEV;
	}

	if (!wrapped_key || !cek_key || !cek_key_sz) {
		mutex_unlock(&fspapp_lock);
		return -EINVAL;
	}

	if (!wrapped_key->wrapkey_size ||
	    wrapped_key->wrapkey_size > sizeof(wrapped_key->wrapkey)) {
		mutex_unlock(&fspapp_lock);
		return -EINVAL;
	}

	args[0].type   = SI_AT_IB;
	args[0].b.addr = (void *)wrapped_key;
	args[0].b.size = sizeof(*wrapped_key);

	args[1].type   = SI_AT_OB;
	args[1].b.addr = cek_key;
	args[1].b.size = cek_key_sz;

	args[2].type   = SI_AT_END;

	ret = si_object_do_invoke(
			&fspapp_oic,
			fspapp_service,
			IFSPApp_OP_unwrap_wrapped_key,
			args,
			&result);

	mutex_unlock(&fspapp_lock);

	if (ret) {
		pr_err("transport error=%d\n", ret);
		return ret;
	}

	if (result) {
		pr_err("service returned error=%d\n", result);
		return -EINVAL;
	}

	if (args[1].b.size > cek_key_sz) {
		pr_err("output size overflow %zu > %zu\n",
		       args[1].b.size, cek_key_sz);
		return -EOVERFLOW;
	}

	if (cek_key_len_out)
		*cek_key_len_out = args[1].b.size;

	return 0;
}
EXPORT_SYMBOL_GPL(fspapp_unwrap_wrapped_key);

/* ========================================================================= */
/* Platform driver                                                            */
/* ========================================================================= */

static int fspapp_probe(struct platform_device *pdev)
{
	struct ifspapp_wrap_key_blob *blob;
	u8 cek[128];
	size_t cek_len = 0;
	int ret;

	ret = open_fspapp_service(&pdev->dev);
	if (ret) {
		dev_err(&pdev->dev, "open_fspapp_service failed rc=%d\n", ret);
		return ret;
	}

	blob = kzalloc(sizeof(*blob), GFP_KERNEL);
	if (!blob)
		return -ENOMEM;

	ret = load_wrapped_key(blob, &pdev->dev);
	if (ret == -ENOENT) {
		dev_info(&pdev->dev,
			 "wrapped_key.mbn not yet available, deferring probe\n");
		ret = -EPROBE_DEFER;
		goto free_blob;
	}
	if (ret) {
		dev_err(&pdev->dev, "load_wrapped_key failed rc=%d\n", ret);
		goto free_blob;
	}

	ret = fspapp_unwrap_wrapped_key(blob, cek, sizeof(cek), &cek_len);
	if (ret) {
		dev_err(&pdev->dev, "fspapp_unwrap_wrapped_key failed rc=%d\n", ret);
		goto free_blob;
	}

	ret = add_cek_to_keyring(cek, cek_len);
	if (ret) {
		dev_err(&pdev->dev, "add_cek_to_keyring failed rc=%d\n", ret);
		goto free_blob;
	}

free_blob:
	memzero_explicit(cek, sizeof(cek));
	kfree_sensitive(blob);
	return ret;
}

static int fspapp_remove(struct platform_device *pdev)
{
	close_fspapp_service();
	return 0;
}

static const struct of_device_id fspapp_of_match[] = {
	{ .compatible = "qcom,fspapp-client" },
	{ }
};
MODULE_DEVICE_TABLE(of, fspapp_of_match);

static struct platform_driver fspapp_driver = {
	.probe  = fspapp_probe,
	.remove = fspapp_remove,
	.driver = {
		.name           = "fspapp-client",
		.of_match_table = fspapp_of_match,
	},
};
module_platform_driver(fspapp_driver);

/* ========================================================================= */

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("FSPApp QTEE Service Client");
MODULE_SOFTDEP("pre: si_core_module");
MODULE_IMPORT_NS(DMA_BUF);
