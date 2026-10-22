// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#define pr_fmt(fmt) "si-ffa: %s: " fmt, __func__

#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/slab.h>
#include <linux/genalloc.h>

#include "si_core.h"

#define DEFAULT_FFA_SHM_SIZE    SZ_4M   /*4M*/

/* A list of currently shared memory regions with QTEE.
 * Since clients can attempt to share the same memory with
 * QTEE multiple times (due to legacy implementation) via
 * the MEM_SHARE ABI, we must enable a layer of ref-counting
 * ontop.
 */
struct ffa_mem_share_list {
	struct list_head head;
	struct mutex lock;
};

struct ffa_mem_share_list_entry {
	struct kref refcount;
	struct list_head list;
	bool is_dma_mem;
	union {
		phys_addr_t paddr;
		dma_addr_t dma_addr;
	};
	uint64_t ffa_handle;
};

struct ffa_shm_pool {
	phys_addr_t paddr;
	void *vaddr;
	size_t size;
	uint64_t ffa_handle;
	struct gen_pool *genpool;
};

static struct ffa_mem_share_list ffa_mem_share_lst;
static struct ffa_shm_pool ffa_pool;
static struct ffa_device *qtee_ffa_dev;

static uint64_t ffa_mem_share_dma_addr_query(dma_addr_t dma_addr)
{
	struct ffa_mem_share_list_entry *entry;
	uint64_t handle = 0;

	list_for_each_entry(entry, &ffa_mem_share_lst.head, list)
		if (entry->is_dma_mem && entry->dma_addr == dma_addr) {
			kref_get(&entry->refcount);
			pr_info("DMA memory %llx already shared over FFA handle %llx.\n",
				 entry->dma_addr, entry->ffa_handle);
			handle = entry->ffa_handle;
			break;
		}

	return handle;
}

static uint64_t ffa_mem_share_phys_addr_query(phys_addr_t paddr)
{
	struct ffa_mem_share_list_entry *entry;
	uint64_t handle = 0;

	list_for_each_entry(entry, &ffa_mem_share_lst.head, list)
		if (!entry->is_dma_mem && entry->paddr == paddr) {
			kref_get(&entry->refcount);
			pr_info("PHY memory %llx already shared over FFA handle %llx.\n",
				 entry->paddr, entry->ffa_handle);
			handle = entry->ffa_handle;
			break;
		}

	return handle;
}

static uint64_t ffa_mem_share_list_query(struct scatterlist *sgl)
{
	dma_addr_t dma_addr;
	phys_addr_t paddr;

	dma_addr = sg_dma_address(sgl);
	if (dma_addr)
		return ffa_mem_share_dma_addr_query(dma_addr);

	/* Phys address stored in the first scatter item */
	paddr = page_to_phys(sg_page(sgl));
	return ffa_mem_share_phys_addr_query(paddr);
}

static void ffa_mem_share_list_free(struct kref *ref)
{
	struct ffa_mem_share_list_entry *entry = container_of(ref,
							      struct ffa_mem_share_list_entry,
							      refcount);

	list_del(&entry->list);
	kfree(entry);
}

static int ffa_mem_share_list_add(uint64_t ffa_handle, struct scatterlist *sgl)
{
	struct ffa_mem_share_list_entry *entry;

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return -ENOMEM;

	entry->ffa_handle = ffa_handle;
	if (sg_dma_address(sgl)) {
		entry->is_dma_mem = true;
		entry->dma_addr = sg_dma_address(sgl);
	} else {
		entry->is_dma_mem = false;
		entry->paddr = page_to_phys(sg_page(sgl));
	}

	kref_init(&entry->refcount);
	list_add_tail(&entry->list, &ffa_mem_share_lst.head);
	return 0;
}

static int ffa_mem_share_list_del(uint64_t ffa_handle)
{
	struct ffa_mem_share_list_entry *entry;
	int rc = -1;

	list_for_each_entry(entry, &ffa_mem_share_lst.head, list)
		if (entry->ffa_handle == ffa_handle) {
			rc = kref_put(&entry->refcount, ffa_mem_share_list_free);
			break;
		}

	return rc;
}

int qtee_ffa_mem_share(struct sg_table *sgt, uint64_t tag, u8 attrs, uint64_t *ffa_handle)
{
	int rc = 0;

	if (!qtee_ffa_dev)
		return -ENODEV;

	struct ffa_mem_region_attributes mem_attr = {
		.receiver = qtee_ffa_dev->vm_id,
		.attrs = attrs,
		.flag = 0,
	};

	struct ffa_mem_ops_args mem_args = {
		.attrs = &mem_attr,
		.use_txbuf = true,
		.nattrs = 1,
		.flags = 0,
		.tag = tag,
		.sg = sgt->sgl,
	};

	mutex_lock(&ffa_mem_share_lst.lock);
	*ffa_handle = ffa_mem_share_list_query(sgt->sgl);
	/* Early return if this memory is already shared over FFA. */
	if (*ffa_handle)
		goto exit;

	rc = qtee_ffa_dev->ops->mem_ops->memory_share(&mem_args);
	if (rc) {
		pr_err("memory_share failed: %d, attrs: %x, tag: %llx\n", rc, attrs, tag);
		goto exit;
	}

	rc = ffa_mem_share_list_add(mem_args.g_handle, sgt->sgl);
	if (rc) {
		pr_err("ffa_mem_share_list_add failed: %d\n", rc);
		goto exit;
	}

	*ffa_handle = mem_args.g_handle;
	pr_debug("mem_share success, ffa_handle: 0x%llx\n", *ffa_handle);

exit:
	mutex_unlock(&ffa_mem_share_lst.lock);
	return rc;
}

int qtee_ffa_mem_lend(struct sg_table *sgt, uint64_t tag, u8 attrs, uint64_t *ffa_handle)
{
	int rc = 0;

	if (!qtee_ffa_dev)
		return -ENODEV;

	struct ffa_mem_region_attributes mem_attr = {
		.receiver = qtee_ffa_dev->vm_id,
		.attrs = attrs,
		.flag = 0,
	};

	struct ffa_mem_ops_args mem_args = {
		.attrs = &mem_attr,
		.use_txbuf = true,
		.nattrs = 1,
		.flags = 0,
		.tag = tag,
		.sg = sgt->sgl,
	};

	rc = qtee_ffa_dev->ops->mem_ops->memory_lend(&mem_args);
	if (rc) {
		pr_err("memory_lend failed: %d, attrs: %x, tag: %llx\n", rc, attrs, tag);
		return rc;
	}

	*ffa_handle = mem_args.g_handle;
	pr_debug("mem_lend success, ffa_handle: 0x%llx\n", *ffa_handle);

	return 0;
}

int qtee_ffa_mem_reclaim(uint64_t ffa_handle)
{
	int rc = 0;

	if (!qtee_ffa_dev)
		return -ENODEV;

	mutex_lock(&ffa_mem_share_lst.lock);
	rc = ffa_mem_share_list_del(ffa_handle);
	/* When reclaiming LENT memory, rc = -1.
	 * When reclaiming SHARED memory with ref-count = 0, rc = 1.
	 * When reclaiming SHARED memory with ref-count > 0, rc = 0
	 */
	if (rc == 0)
		goto exit;

	/* We assume that QTEE has already called MEM_RELINQUISH.
	 * And so, we do not need to send a DIRECT_REQ message first.
	 */
	rc = qtee_ffa_dev->ops->mem_ops->memory_reclaim(ffa_handle, 0);
	if (rc) {
		pr_err("mem_reclaim failed: 0x%llx %d\n", ffa_handle, rc);
		goto exit;
	}
	pr_debug("mem_reclaim success, ffa_handle: 0x%llx\n", ffa_handle);

exit:
	mutex_unlock(&ffa_mem_share_lst.lock);
	return rc;
}

int qtee_ffa_shm_alloc(size_t in_size, size_t out_size,
		       struct ffa_shm *shm)
{
	unsigned long va;
	phys_addr_t pa;
	size_t total_size = 0;

	if (!ffa_pool.genpool) {
		pr_err("ffa_pool not available!\n");
		return -ENOMEM;
	}

	if (in_size != ALIGN(in_size, PAGE_SIZE)) {
		pr_err("in_size = %zu is not page aligned\n", in_size);
		return -EINVAL;
	}

	if (out_size != ALIGN(out_size, PAGE_SIZE)) {
		pr_err("out_size = %zu is not page aligned\n", out_size);
		return -EINVAL;
	}

	if (out_size > SIZE_MAX - in_size) {
		pr_err("overflow detected for allocation size\n");
		return -EINVAL;
	}

	total_size = in_size + out_size;
	if (total_size > ffa_pool.size) {
		pr_err("requested size %zu is larger than pool size %zu\n",
			total_size, ffa_pool.size);
		return -EINVAL;
	}

	va = gen_pool_alloc(ffa_pool.genpool, total_size);
	if (!va) {
		pr_err("failed to sub-allocate %zu bytes from pool\n",
			total_size);
		return -ENOMEM;
	}

	memset((void *)va, 0, total_size);
	shm->ffa_handle = ffa_pool.ffa_handle;
	shm->in_vaddr = (void *)va;
	shm->in_size = in_size;
	shm->out_vaddr = (void *)(va + in_size);
	shm->out_size = out_size;

	pa = gen_pool_virt_to_phys(ffa_pool.genpool, va);
	shm->paddr = pa;
	shm->offset = (size_t)(pa - ffa_pool.paddr);

	pr_debug("in_vaddr: %p, in_size: %zu, out_vaddr: %p, out_size: %zu,\n",
		 shm->in_vaddr, shm->in_size, shm->out_vaddr, shm->out_size);
	pr_debug("offset: %zu\n", shm->offset);

	return 0;
}

void qtee_ffa_shm_free(struct ffa_shm shm)
{
	size_t total_size = 0;

	if (!shm.in_vaddr)
		return;

	total_size = shm.in_size + shm.out_size;
	gen_pool_free(ffa_pool.genpool, (unsigned long)shm.in_vaddr,
		      total_size);
}

int qtee_ffa_shm_init(struct platform_device *pdev)
{
	int rc;
	uint32_t custom_ffa_pool_size;
	unsigned int order;
	size_t nr_pages;
	unsigned int i;
	struct page **pages;
	struct sg_table sgt;

	if (ffa_pool.vaddr) {
		pr_err("ffa_pool is already initialized\n");
		return 0;
	}


	mutex_init(&ffa_mem_share_lst.lock);
	INIT_LIST_HEAD(&ffa_mem_share_lst.head);

	rc = of_property_read_u32((&pdev->dev)->of_node,
				  "qcom,ffa-pool-size", &custom_ffa_pool_size);
	if (rc)
		ffa_pool.size = DEFAULT_FFA_SHM_SIZE;
	else
		ffa_pool.size = custom_ffa_pool_size * PAGE_SIZE;

	pr_info("Using FFA pool size = %zu\n", ffa_pool.size);

	order = get_order(ffa_pool.size);
	ffa_pool.vaddr = (void *)__get_free_pages(GFP_KERNEL|__GFP_COMP,
						  order);
	if (!ffa_pool.vaddr)
		return -ENOMEM;

	ffa_pool.paddr = dma_map_single(&pdev->dev, ffa_pool.vaddr,
					ffa_pool.size, DMA_TO_DEVICE);
	if (dma_mapping_error(&pdev->dev, ffa_pool.paddr)) {
		pr_err("dma_map_single() failed\n");
		rc = -ENOMEM;
		goto err_dma_mapping;
	}

	/* Create a general mem pool */
	ffa_pool.genpool = gen_pool_create(PAGE_SHIFT, -1);
	if (!ffa_pool.genpool) {
		pr_err("gen_pool_add_virt() failed\n");
		rc = -ENOMEM;
		goto err_gen_pool_create;
	}

	gen_pool_set_algo(ffa_pool.genpool, gen_pool_best_fit, NULL);
	rc = gen_pool_add_virt(ffa_pool.genpool, (uintptr_t)ffa_pool.vaddr,
				ffa_pool.paddr, ffa_pool.size, -1);
	if (rc) {
		pr_err("gen_pool_add_virt() failed, rc = %d\n", rc);
		goto err_gen_pool_add_virt;
	}

	nr_pages = 1 << order;
	pages = kcalloc(nr_pages, sizeof(*pages), GFP_KERNEL);
	if (!pages) {
		rc = -ENOMEM;
		goto err_gen_pool_add_virt;
	}

	for (i = 0; i < nr_pages; i++)
		pages[i] = virt_to_page((uint8_t *)ffa_pool.vaddr + i * PAGE_SIZE);

	rc = sg_alloc_table_from_pages(&sgt, pages, nr_pages, 0,
				       nr_pages * PAGE_SIZE, GFP_KERNEL);
	kfree(pages);
	if (rc)
		goto err_gen_pool_add_virt;

	rc = qtee_ffa_mem_share(&sgt, 0, FFA_MEM_RW, &ffa_pool.ffa_handle);
	sg_free_table(&sgt);
	if (rc) {
		pr_err("qtee_ffa_mem_share() failed, rc = %d\n", rc);
		goto err_gen_pool_add_virt;
	}

	return 0;

err_gen_pool_add_virt:
	gen_pool_destroy(ffa_pool.genpool);
	ffa_pool.genpool = NULL;
err_gen_pool_create:
	dma_unmap_single(&pdev->dev, ffa_pool.paddr, ffa_pool.size,
			 DMA_TO_DEVICE);
err_dma_mapping:
	free_pages((long)ffa_pool.vaddr, order);
	ffa_pool.vaddr = NULL;

	return rc;
}

void qtee_ffa_shm_deinit(struct platform_device *pdev)
{
	qtee_ffa_mem_reclaim(ffa_pool.ffa_handle);
	gen_pool_destroy(ffa_pool.genpool);
	ffa_pool.genpool = NULL;
	dma_unmap_single(&pdev->dev, ffa_pool.paddr, ffa_pool.size,
			 DMA_TO_DEVICE);
	free_pages((long)ffa_pool.vaddr, get_order(ffa_pool.size));
	ffa_pool.vaddr = NULL;
}

static int si_core_ffa_probe(struct ffa_device *ffa_dev)
{
	/* There is no possibility of QTEE SP going down and
	 * coming back online which could require re-binding
	 * to the driver. If probe is called multiple times,
	 * this is an error.
	 */
	if (qtee_ffa_dev)
		return -EEXIST;

	qtee_ffa_dev = ffa_dev;
	return 0;
}

static void si_core_ffa_remove(struct ffa_device *ffa_dev)
{
	qtee_ffa_dev = NULL;
}

static const struct ffa_device_id qtee_ffa_device_id[] = {
	{ QTEE_SP_FFA_UUID },
	{}
};

static struct ffa_driver si_core_ffa_driver = {
	.name = "si_core",
	.probe = si_core_ffa_probe,
	.remove = si_core_ffa_remove,
	.id_table = qtee_ffa_device_id,
};

int si_core_ffa_driver_register(void)
{
	return ffa_register(&si_core_ffa_driver);
}

void si_core_ffa_driver_unregister(void)
{
	ffa_unregister(&si_core_ffa_driver);
}
