// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#include <linux/delay.h>
#include <linux/qcom_scm.h>

#include "si_core.h"

#include "trace_si_core.h"

/* 6 Sec. retry seems reasonable!? */
#define SCM_EBUSY_WAIT_MS 30
#define SCM_EBUSY_MAX_RETRY 200

static int invoke_direct_smc(struct si_object_invoke_ctx *oic,
			     int *result, u64 *response_type,
			     unsigned int *data)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_QCOM_SI_CORE_MEM_FFA)
	ret = qcom_scm_invoke_smc_ffa(oic->shm.ffa_handle,
				      oic->shm.offset,
				      oic->shm.in_size,
				      oic->shm.out_size, result,
				      response_type, data);
#else
	ret = qcom_scm_invoke_smc(oic->in.paddr, oic->in.msg.size,
				  oic->out.paddr, oic->out.msg.size, result,
				  response_type, data);
#endif

	return ret;
}

static int invoke_callback_smc(struct si_object_invoke_ctx *oic,
			       int *result, u64 *response_type,
			       unsigned int *data)
{
	int ret = 0;
#if IS_ENABLED(CONFIG_QCOM_SI_CORE_MEM_FFA)
	ret = qcom_scm_invoke_callback_response_ffa(oic->shm.ffa_handle,
						    oic->shm.offset +
						    oic->shm.in_size,
						    oic->shm.out_size,
						    result,
						    response_type, data);
#else
	ret = qcom_scm_invoke_callback_response(oic->out.paddr,
						oic->out.msg.size, result,
						response_type, data);
#endif

	return ret;
}

int si_object_invoke_ctx_invoke(struct si_object_invoke_ctx *oic,
	int *result, u64 *response_type, unsigned int *data)
{
	int ret, i = 0;

	/* TODO. buffers always coherent!? */

	do {
		/* Direct invocation of callback!? */
		if (!(oic->flags & OIC_FLAG_BUSY)) {
			ret = invoke_direct_smc(oic, result, response_type,
						data);
		} else {
			ret = invoke_callback_smc(oic, result, response_type,
						  data);
		}

		if (ret != -EBUSY)
			break;

		msleep(SCM_EBUSY_WAIT_MS);

	} while (++i < SCM_EBUSY_MAX_RETRY);

	if (ret)
		pr_err("QTEE returned with %d!\n", ret);

	trace_si_objcet_do_ctx_invoke_ret(oic->context_id, oic->flags, i, *response_type, ret);
	return ret;
}
