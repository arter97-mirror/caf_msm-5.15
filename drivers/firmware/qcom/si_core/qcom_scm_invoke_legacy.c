// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/delay.h>
#include <linux/firmware/qcom/qcom_scm.h>

#include "si_core.h"

/* 6 Sec. retry seems reasonable!? */
#define SCM_EBUSY_WAIT_MS 30
#define SCM_EBUSY_MAX_RETRY 200

int si_object_invoke_ctx_invoke(struct si_object_invoke_ctx *oic,
	int *result, u64 *response_type, unsigned int *data)
{
	int ret, i = 0;

	/* TODO. buffers always coherent!? */

	do {
		/* Direct invocation of callback!? */
		if (!(oic->flags & OIC_FLAG_BUSY)) {
			qtee_shmbridge_flush_shm_buf(&oic->in_shm);
			qtee_shmbridge_flush_shm_buf(&oic->out_shm);
			ret = qcom_scm_invoke_smc_legacy(oic->in.paddr,
				oic->in.msg.size,
				oic->out.paddr,
				oic->out.msg.size,
				result,
				response_type,
				data);
			qtee_shmbridge_inv_shm_buf(&oic->in_shm);
			qtee_shmbridge_inv_shm_buf(&oic->out_shm);

		} else {
			qtee_shmbridge_flush_shm_buf(&oic->out_shm);
			ret = qcom_scm_invoke_callback_response(oic->out.paddr,
				oic->out.msg.size,
				result,
				response_type,
				data);
			qtee_shmbridge_inv_shm_buf(&oic->in_shm);
			qtee_shmbridge_inv_shm_buf(&oic->out_shm);
		}

		if (ret != -EBUSY)
			break;

		msleep(SCM_EBUSY_WAIT_MS);

	} while (++i < SCM_EBUSY_MAX_RETRY);

	if (ret)
		pr_err("QTEE returned with %d!\n", ret);

	return ret;
}
