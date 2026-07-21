// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/device.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/soc/qcom/smem.h>

#define SMEM_READ_MAX_BYTES	32

#ifndef SMEM_BYTE_LOG
#define SMEM_BYTE_LOG	0
#endif

#if SMEM_BYTE_LOG
#define byte_dbg(_dev, _fmt, ...) dev_dbg((_dev), (_fmt), ##__VA_ARGS__)
#else
#define byte_dbg(_dev, _fmt, ...) do { } while (0)
#endif

struct smem_bridge {
	struct mutex lock;
	u32 smem_host;
	u32 smem_item;
	u32 smem_offset;
};

/**
 * smem_get_entry() - Resolve and validate the configured SMEM entry.
 * @dev: Device used for logging and driver context.
 * @bridge: Driver state containing the SMEM host, item ID, and offset.
 * @base: Output pointer that receives the base address of the SMEM entry.
 * @smem_size: Output pointer that receives the total size of the SMEM entry.
 *
 * Fetches the SMEM item described by @bridge, verifies that the returned
 * pointer is valid, and ensures the configured offset lies within the entry.
 *
 * Return: 0 on success, or a negative errno value on failure.
 */
static int smem_get_entry(struct device *dev,
			      struct smem_bridge *bridge,
			      void **base, size_t *smem_size)
{
	dev_dbg(dev, "requesting SMEM entry (host=%u item=%u)\n",
		bridge->smem_host, bridge->smem_item);

	*base = qcom_smem_get(bridge->smem_host, bridge->smem_item, smem_size);
	if (IS_ERR(*base)) {
		dev_err(dev, "qcom_smem_get failed (host=%u item=%u err=%ld)\n",
			bridge->smem_host, bridge->smem_item, PTR_ERR(*base));
		return PTR_ERR(*base);
	}

	if (!*base) {
		dev_err(dev, "qcom_smem_get returned NULL (host=%u item=%u)\n",
			bridge->smem_host, bridge->smem_item);
		return -ENODEV;
	}

	if (bridge->smem_offset >= *smem_size) {
		dev_err(dev,
			"invalid smem offset %u for item=%u (size=%zu)\n",
			bridge->smem_offset, bridge->smem_item, *smem_size);
		return -EINVAL;
	}

	dev_dbg(dev, "SMEM entry ready (host=%u item=%u size=%zu offset=%u)\n",
		bridge->smem_host, bridge->smem_item, *smem_size,
		bridge->smem_offset);

	return 0;
}

/**
 * smem_id_show() - Show the currently selected SMEM item ID.
 * @dev: Device owning the sysfs attribute.
 * @attr: Sysfs attribute metadata; unused because this show routine serves a
 *	single attribute.
 * @buf: PAGE_SIZE sysfs output buffer receiving the numeric item ID.
 *
 * Serializes access to the bridge state, copies the configured SMEM item ID,
 * and returns it as a decimal string with a trailing newline.
 *
 * Return: Number of bytes written to @buf.
 */
static ssize_t smem_id_show(struct device *dev,
			    struct device_attribute *attr,
			    char *buf)
{
	struct smem_bridge *bridge = dev_get_drvdata(dev);
	u32 smem_item;

	mutex_lock(&bridge->lock);
	smem_item = bridge->smem_item;
	mutex_unlock(&bridge->lock);

	dev_dbg(dev, "smem_id read, item=%u\n", smem_item);

	return scnprintf(buf, PAGE_SIZE, "%u\n", smem_item);
}
static DEVICE_ATTR_RO(smem_id);

/**
 * smem_data_show() - Dump a small window of SMEM data through sysfs.
 * @dev: Device owning the sysfs attribute.
 * @attr: Sysfs attribute metadata; unused because this show routine serves a
 *	single attribute.
 * @buf: PAGE_SIZE sysfs output buffer receiving metadata and hex bytes.
 *
 * Reads the configured SMEM entry at the configured offset and emits
 * SMEM_READ_MAX_BYTES bytes formatted as hexadecimal.
 *
 * Return: Number of bytes written to @buf, or a negative errno value.
 */
static ssize_t smem_data_show(struct device *dev,
			      struct device_attribute *attr,
			      char *buf)
{
	struct smem_bridge *bridge = dev_get_drvdata(dev);
	u8 *base;
	size_t smem_size = 0;
	size_t avail;
	size_t read_len;
	ssize_t len = 0;
	size_t i;
	int ret;

	dev_dbg(dev, "smem_data read requested\n");

	mutex_lock(&bridge->lock);

	ret = smem_get_entry(dev, bridge, (void **)&base, &smem_size);
	if (ret) {
		len = ret;
		goto out;
	}

	base += bridge->smem_offset;
	avail = smem_size - bridge->smem_offset;
	read_len = min_t(size_t, avail, SMEM_READ_MAX_BYTES);
	dev_dbg(dev,
		"smem_data read window: item=%u size=%zu offset=%u avail=%zu selected=%zu\n",
		bridge->smem_item, smem_size, bridge->smem_offset, avail, read_len);

	for (i = 0; i < read_len && len < PAGE_SIZE; i++) {
		byte_dbg(dev, "smem_data read byte[%zu]=0x%02x\n", i, base[i]);
		len += scnprintf(buf + len, PAGE_SIZE - len, "%02x%s", base[i],
				 (i == (read_len - 1)) ? "\n" : " ");
	}

	dev_dbg(dev,
		"smem_data read complete, item=%u read_len=%zu bytes=%zd\n",
		bridge->smem_item, read_len, len);

out:
	mutex_unlock(&bridge->lock);
	return len;
}
static DEVICE_ATTR_RO(smem_data);

static struct attribute *smem_bridge_attrs[] = {
	&dev_attr_smem_id.attr,
	&dev_attr_smem_data.attr,
	NULL,
};
ATTRIBUTE_GROUPS(smem_bridge);

/**
 * smem_bridge_probe() - Initialize the SMEM bridge platform device.
 * @pdev: Platform device matched from device tree.
 *
 * Allocates and initializes driver state, reads device-tree properties that
 * identify the SMEM host/item and offset, stores the state as
 * device driver data, and creates the sysfs attributes exposed by this driver.
 *
 * Return: 0 on success, or a negative errno value on failure.
 */
static int smem_bridge_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct smem_bridge *bridge;
	int ret;

	bridge = devm_kzalloc(dev, sizeof(*bridge), GFP_KERNEL);
	if (!bridge)
		return -ENOMEM;

	mutex_init(&bridge->lock);

	bridge->smem_host = QCOM_SMEM_HOST_ANY;

	ret = of_property_read_u32(dev->of_node, "qcom,smem-item",
				   &bridge->smem_item);
	if (ret) {
		dev_err(dev, "missing qcom,smem-item\n");
		return ret;
	}

	bridge->smem_offset = 0;
	of_property_read_u32(dev->of_node, "qcom,smem-offset",
			     &bridge->smem_offset);

	platform_set_drvdata(pdev, bridge);

	dev_info(dev,
		 "SMEM bridge ready (host=%u item=%u offset=%u)\n",
		 bridge->smem_host, bridge->smem_item,
		 bridge->smem_offset);

	return 0;
}

static const struct of_device_id smem_bridge_of_match[] = {
	{ .compatible = "qcom,smem-bridge" },
	{ }
};
MODULE_DEVICE_TABLE(of, smem_bridge_of_match);

static struct platform_driver smem_bridge_driver = {
	.probe = smem_bridge_probe,
	.driver = {
		.name = "qcom-smem-bridge",
		.of_match_table = smem_bridge_of_match,
		.dev_groups = smem_bridge_groups,
	},
};
module_platform_driver(smem_bridge_driver);

MODULE_DESCRIPTION("Qualcomm SMEM bridge driver");
MODULE_LICENSE("GPL");
