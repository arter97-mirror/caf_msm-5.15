/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __SI_CORE_MEM_OBJ_H__
#define __SI_CORE_MEM_OBJ_H__

#include <linux/platform_device.h>

/* Defined in mem-object.c. */
ssize_t mem_objects_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);

int mem_object_init(struct platform_device *pdev);

#endif /* __SI_CORE_MEM_OBJ_H__ */
