/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM si_core

#if !defined(_TRACE_SI_CORE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_SI_CORE_H

#include <linux/tracepoint.h>

TRACE_EVENT(si_objcet_do_invoke_wait,
	TP_PROTO(const char *ob_name, uint32_t ob_type, uint32_t op),
	TP_ARGS(ob_name, ob_type, op),
	TP_STRUCT__entry(
		__string(str, ob_name)
		__field(uint32_t, ob_type)
		__field(uint32_t, op)
	),
	TP_fast_assign(
		__assign_str(str, ob_name);
		__entry->ob_type = ob_type;
		__entry->op = op;
	),
	TP_printk("ob_name=%s ob_type=0x%04x op=0x%08x",
		__get_str(str), __entry->ob_type, __entry->op)
);

TRACE_EVENT(si_objcet_do_invoke_ret,
	TP_PROTO(const char *ob_name, uint32_t ob_type, uint32_t op, int ret),
	TP_ARGS(ob_name, ob_type, op, ret),
	TP_STRUCT__entry(
		__string(str, ob_name)
		__field(uint32_t, ob_type)
		__field(uint32_t, op)
		__field(int, ret)
	),
	TP_fast_assign(
		__assign_str(str, ob_name);
		__entry->ob_type = ob_type;
		__entry->op = op;
		__entry->ret = ret;
	),
	TP_printk("ob_name=%s ob_type=0x%04x op=0x%08x ret=%d",
		__get_str(str), __entry->ob_type, __entry->op, __entry->ret)
);

TRACE_EVENT(si_objcet_do_ctx_invoke_ret,
	TP_PROTO(unsigned int oic_context_id, unsigned int oic_flags, int i, uint64_t response_type,
		int ret),
	TP_ARGS(oic_context_id, oic_flags, i, response_type, ret),
	TP_STRUCT__entry(
		__field(unsigned int, oic_context_id)
		__field(unsigned int, oic_flags)
		__field(int, i)
		__field(uint64_t, response_type)
		__field(int, ret)
	),
	TP_fast_assign(
		__entry->oic_context_id = oic_context_id;
		__entry->oic_flags = oic_flags;
		__entry->i = i;
		__entry->response_type = response_type;
		__entry->ret = ret;
	),
	TP_printk("oic_context_id=0x%08x oic_flags=0x%02x i=%d response_type=0x%08llx ret=%d",
		__entry->oic_context_id, __entry->oic_flags, __entry->i, __entry->response_type,
		__entry->ret)
);

TRACE_EVENT(si_objcet_invoke_ret,
	TP_PROTO(const char *ob_name, uint32_t ob_type, unsigned int ob_id, unsigned long op,
		int errno),
	TP_ARGS(ob_name, ob_type, ob_id, op, errno),
	TP_STRUCT__entry(
		__string(str, ob_name)
		__field(uint32_t, ob_type)
		__field(unsigned int, ob_id)
		__field(unsigned long, op)
		__field(int, errno)
	),
	TP_fast_assign(
		__assign_str(str, ob_name);
		__entry->ob_type = ob_type;
		__entry->ob_id = ob_id;
		__entry->op = op;
		__entry->errno = errno;
	),
	TP_printk("ob_name=%s ob_type=0x%04x ob_id=0x%08x op=0x%08lx errno=%d",
		__get_str(str), __entry->ob_type, __entry->ob_id, __entry->op, __entry->errno)
);

TRACE_EVENT(qseecom_process_listener_from_smcinvoke_ret,
	TP_PROTO(uint64_t response_type, int result, int ret),
	TP_ARGS(response_type, result, ret),
	TP_STRUCT__entry(
		__field(uint64_t, response_type)
		__field(int, result)
		__field(int, ret)
	),
	TP_fast_assign(
		__entry->response_type = response_type;
		__entry->result = result;
		__entry->ret = ret;
	),
	TP_printk("process_listener_from_smcinvoke_ret response_type=0x%08llx result=%d ret=%d",
		__entry->response_type, __entry->result, __entry->ret)
);

#endif /* _TRACE_SI_CORE_H */
/*
 * Path must be relative to location of 'define_trace.h' header in kernel
 * Define path if not defined in bazel file
 */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE trace_si_core

/* This part must be outside protection */
#include <trace/define_trace.h>
