/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef _LINUX_SI_CORE_XTS_H__
#define _LINUX_SI_CORE_XTS_H__

#include <linux/dma-buf.h>
#include <linux/firmware/qcom/si_object.h>

/*
 * Flags layout for init_si_mem_object_sg() / init_si_mem_object_pages()
 * -----------------------------------------------------------------------
 * 31            12 11     8 7   5 4 3   1    0
 * +---------------+--------+-----+-+-----+---+
 * |   RESERVED    |FFA PERM| RSV |U| RSV |L/S|
 * +---------------+--------+-----+-+-----+---+
 *
 *  Bit(s)  Define                    Description
 *  ------  ------------------------  ----------------------------------------
 *  0       SI_CORE_MEM_OBJ_LEND      FFA operation: 0 = MEM_SHARE (default)
 *                                                   1 = MEM_LEND
 *  1-3     (reserved)
 *  4       SI_CORE_MEM_OBJ_UNCACHED  Cache: caller must flush + invalidate
 *  5-7     (reserved)
 *  8-11    SI_CORE_MEM_OBJ_FFA_PERM  FFA_MEM_* permission bits (shifted by 8)
 *  12+     (reserved)
 */

/* FFA operation type (bit 0): 0 = MEM_SHARE (default), 1 = MEM_LEND. */
#define SI_CORE_MEM_OBJ_SHARE     0
#define SI_CORE_MEM_OBJ_LEND      BIT(0)

/* Cache attribute (bit 4).
 * When set, the caller must perform explicit cache maintenance (flush +
 * invalidate) on the memory shared with QTEE.
 */
#define SI_CORE_MEM_OBJ_UNCACHED  BIT(4)

/*
 * FFA permission bits encoded in flags (bits 8-11).
 *
 * Use SI_CORE_MEM_OBJ_FFA_PERM() to encode FFA_MEM_* permission values into
 * the flags field. If not specified, FFA_MEM_RW is used as the default.
 *
 * Valid FFA permission combinations (from <linux/arm_ffa.h>):
 *   FFA_MEM_RW | FFA_MEM_NO_EXEC  - Read-write, non-executable (default)
 *   FFA_MEM_RO | FFA_MEM_NO_EXEC  - Read-only, non-executable
 *   FFA_MEM_RO | FFA_MEM_EXEC     - Read-only, executable
 *
 * Example:
 *   flags = SI_CORE_MEM_OBJ_SHARE |
 *           SI_CORE_MEM_OBJ_FFA_PERM(FFA_MEM_RO | FFA_MEM_EXEC)
 */
#define SI_CORE_MEM_OBJ_FFA_PERM_SHIFT  8
#define SI_CORE_MEM_OBJ_FFA_PERM_MASK   (0xFU << SI_CORE_MEM_OBJ_FFA_PERM_SHIFT)
#define SI_CORE_MEM_OBJ_FFA_PERM(p)     (((uint32_t)(p) & 0xFU) << SI_CORE_MEM_OBJ_FFA_PERM_SHIFT)

struct si_object *init_si_mem_object_user(struct dma_buf *dma_buf,
	void (*release)(void *), void *private);
#if IS_ENABLED(CONFIG_QCOM_SI_CORE_MEM_FFA)
struct si_object *init_si_mem_object_pages(size_t *size, uint64_t tag,
					   uint64_t flags, void (*release)(void *),
					   void *private);
#else
static inline struct si_object *init_si_mem_object_pages(size_t *size, uint64_t tag,
							 uint64_t flags,
							 void (*release)(void *),
							 void *private)
{
	return ERR_PTR(-EOPNOTSUPP);
}
#endif
struct si_object *init_si_mem_object_sg(struct sg_table *sgt, uint64_t tag,
					uint32_t flags, void (*release)(void *),
					void *private);

int dma_map_mem_object(struct si_object *object, unsigned int nents);
void dma_unmap_mem_object(struct si_object *object, unsigned int nents);
/* For 'mem_object_to_dma_buf' and 'is_mem_object' caller should own the 'object',
 * (i.e. someone should have already called '__get_si_object').
 */

int is_mem_object(struct si_object *object);
int early_map_memory_obj(struct si_object *object);
struct dma_buf *mem_object_to_dma_buf(struct si_object *object);
struct sg_table *mem_object_to_sgt(struct si_object *object);

#endif /* _LINUX_SI_CORE_XTS_H__ */
