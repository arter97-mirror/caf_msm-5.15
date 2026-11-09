/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM scm

#if !defined(_TRACE_SCM_SMC_INTERFACE_H) || defined(TRACE_HEADER_MULTI_READ)

#define _TRACE_SCM_SMC_INTERFACE_H

#include <linux/tracepoint.h>

u8 n, i;
size_t max, pos;
char *out;

TRACE_EVENT(scm_smc_request,

	TP_PROTO(const struct arm_smccc_args *smc),

	TP_ARGS(smc),

	TP_STRUCT__entry(
		__field(u64, smc_id)
		__field(u8, svc_id)
		__field(u8, cmd_id)
		__field(u8, args_cnt)
		__dynamic_array(char, out,
				(min_t(u8, (smc->args[1] & 0xF), (u8)6) * 64) + 1)
	),

	TP_fast_assign(
		__entry->smc_id = smc->args[0];
		__entry->svc_id = (smc->args[0] >> 8) & 0xFF;
		__entry->cmd_id = smc->args[0] & 0xFF;

		n = min_t(u8, (smc->args[1] & 0xF), (u8)6);

		__entry->args_cnt = n;

		out = __get_dynamic_array(out);
		max = n * 64;       /* total buffer size minus NUL */
		pos = 0;

		for (i = 0; i < n; ++i) {
			unsigned long long v = (unsigned long long)smc->args[2 + i];

			pos += scnprintf(out + pos, max - pos,
					" x%u:(hex:0x%08llx decimal:%llu)", i + 2, v, v);

			if (pos >= max)
				break;
		}
		out[pos] = '\0';
	),

	TP_printk("smc_id:0x%08llx svc_id:0x%02x, cmd_id:0x%02x, args_cnt:%u%s",
		__entry->smc_id, __entry->svc_id, __entry->cmd_id, __entry->args_cnt,
		(char *)__get_dynamic_array(out))
);


DECLARE_EVENT_CLASS(scm_waitq_response,

	TP_PROTO(u32 wq_ctx, u32 smc_ctx),

	TP_ARGS(wq_ctx, smc_ctx),

	TP_STRUCT__entry(
		__field(u32, wq_ctx)
		__field(u32, smc_call_ctx)
	),

	TP_fast_assign(
		__entry->wq_ctx        = wq_ctx;
		__entry->smc_call_ctx  = smc_ctx;
	),

	TP_printk("wq_ctx:%u, smc_call_ctx:%u", __entry->wq_ctx, __entry->smc_call_ctx)
);

DEFINE_EVENT(scm_waitq_response, scm_waitq_sleep,
	TP_PROTO(u32 wq_ctx, u32 smc_ctx),
	TP_ARGS(wq_ctx, smc_ctx));

DEFINE_EVENT(scm_waitq_response, scm_waitq_wake_ack,
	TP_PROTO(u32 wq_ctx, u32 smc_ctx),
	TP_ARGS(wq_ctx, smc_ctx));

TRACE_EVENT(scm_waitq_resume,

	TP_PROTO(u32 smc_ctx),

	TP_ARGS(smc_ctx),

	TP_STRUCT__entry(
		__field(u32, smc_call_ctx)
	),

	TP_fast_assign(
		__entry->smc_call_ctx  = smc_ctx;
	),

	TP_printk("smc_call_ctx:%u", __entry->smc_call_ctx)
);

TRACE_EVENT(scm_waitq_get_wq_ctx,

	TP_PROTO(u32 wq_ctx, u32 flags, u32 pending),

	TP_ARGS(wq_ctx, flags, pending),

	TP_STRUCT__entry(
		__field(u32, wq_ctx)
		__field(u32, flags)
		__field(u32, more_pending)
	),

	TP_fast_assign(
		__entry->wq_ctx        = wq_ctx;
		__entry->flags         = flags;
		__entry->more_pending  = pending;
	),

	TP_printk("wq_ctx:%u, flags:%u, more_pending:%u",
				__entry->wq_ctx, __entry->flags, __entry->more_pending)
);

TRACE_EVENT(scm_smc_done,

	TP_PROTO(int ret, u64 smc_id, struct arm_smccc_res *smc_res),

	TP_ARGS(ret, smc_id, smc_res),

	TP_STRUCT__entry(
		__field(int, ret)
		__field(u64, smc_id)
		__field(unsigned long, res0)
		__field(unsigned long, res1)
		__field(unsigned long, res2)
	),

	TP_fast_assign(
		__entry->ret  = ret;
		__entry->smc_id  = smc_id;
		__entry->res0 = smc_res->a1;
		__entry->res1 = smc_res->a2;
		__entry->res2 = smc_res->a3;
	),

	TP_printk("smc_id:0x%08llx, ret:%d res0:%lu res1:%lu res2:%lu", __entry->smc_id,
				__entry->ret, __entry->res0, __entry->res1, __entry->res2)
);

#endif /* _TRACE_SCM_SMC_INTERFACE_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#define TRACE_INCLUDE_FILE qcom_scm_trace

#include <trace/define_trace.h>
