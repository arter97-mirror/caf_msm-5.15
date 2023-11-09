/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */

#ifndef __SI_CORE_DOORBELL_H__
#define __SI_CORE_DOORBELL_H__

#ifdef CONFIG_QCOM_SI_CORE_DOORBELL
int si_core_doorbell_init(struct platform_device *pdev);
void si_core_doorbell_deinit(struct platform_device *pdev);
#else
static inline int si_core_doorbell_init(struct platform_device *pdev)
{
	return 0;
}

static inline void si_core_doorbell_deinit(struct platform_device *pdev)
{
}
#endif /* CONFIG_QCOM_SI_CORE_DOORBELL */
#endif /* __SI_CORE_DOORBELL_H__ */
