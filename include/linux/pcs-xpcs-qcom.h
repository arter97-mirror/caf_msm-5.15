/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2020 Synopsys, Inc. and/or its affiliates.
 * Synopsys DesignWare XPCS helpers
 */
/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. */

#ifndef __LINUX_PCS_XPCS_QCOM_H
#define __LINUX_PCS_XPCS_QCOM_H

#include <linux/phy.h>
#include <linux/phylink.h>

/* AN mode */
#define DW_AN_C73			1
#define DW_AN_C37_SGMII			2
#define DW_2500BASEX			3
#define DW_AN_C37_USXGMII		4

struct xpcs_id;

struct dw_xpcs_qcom {
	const struct xpcs_id *id;
	struct phylink_pcs pcs;
	void __iomem *addr;
	u32 pcs_intr;
	bool intr_en;
	bool sgmii_2p5g_en;
	bool fixed_phy_mode;
	bool c37_an_en;
};

enum dw_xpcs_err_type {
	DW_XPCS_FLT_ERR = 1,
	DW_XPCS_ALIGN_SYNC_ERR,
	DW_XPCS_HIBER_ERR,
	DW_XPCS_ERR_CNT
};

extern char dw_xpcs_err_names[4][21];

#if IS_ENABLED(CONFIG_PCS_QCOM)
int qcom_xpcs_get_an_mode(struct dw_xpcs_qcom *xpcs,
			  phy_interface_t interface);
void qcom_xpcs_link_up(struct phylink_pcs *pcs, unsigned int mode,
		       phy_interface_t interface, int speed,
		       int duplex);
void qcom_xpcs_configure_intr(struct phylink_pcs *pcs, phy_interface_t interface,
			    int speed, int duplex);
void qcom_xpcs_validate(struct dw_xpcs_qcom *xpcs, unsigned long *supported,
		   struct phylink_link_state *state);
int qcom_xpcs_config_eee(struct dw_xpcs_qcom *xpcs, int mult_fact_100ns,
		    int enable);
struct dw_xpcs_qcom *qcom_xpcs_create(void __iomem *addr,
				      phy_interface_t interface);
void qcom_xpcs_destroy(struct dw_xpcs_qcom *xpcs);
const struct xpcs_compat *xpcs_find_compat(const struct xpcs_id *id,
					   phy_interface_t interface);
int qcom_xpcs_serdes_loopback(struct dw_xpcs_qcom *xpcs, bool on);
int qcom_xpcs_pcs_loopback(struct dw_xpcs_qcom *xpcs, bool on);
int qcom_xpcs_check_aneg_ioc(struct dw_xpcs_qcom *xpcs, phy_interface_t interface);
int qcom_xpcs_handle_an_intr(struct dw_xpcs_qcom *xpcs, phy_interface_t interface);
int qcom_xpcs_intr_enable(struct dw_xpcs_qcom *xpcs);
irqreturn_t ethqos_xpcs_isr(int irq, void *dev_data);
int ethqos_xpcs_intr_config(struct net_device *ndev);
int ethqos_xpcs_intr_enable(struct net_device *ndev);
void ethqos_xpcs_irq_enable(struct net_device *ndev);
void ethqos_xpcs_irq_disable(struct net_device *ndev);
int ethqos_xpcs_init(struct net_device *ndev);
int qcom_xpcs_verify_lnk_status_usxgmii(struct dw_xpcs_qcom *xpcs);
int qcom_xpcs_verify_an(struct dw_xpcs_qcom *xpcs);
int qcom_xpcs_lpm(struct dw_xpcs_qcom *xpcs, bool lpm);
int qcom_xpcs_usxgmii_link_error_detect(struct dw_xpcs_qcom *xpcs, int speed);
int qcom_xpcs_soft_reset_usxgmii(struct dw_xpcs_qcom *xpcs);
void qcom_xpcs_link_up_usxgmii(struct dw_xpcs_qcom *xpcs, int speed,
			       phy_interface_t interface);

#else /* IS_ENABLED(CONFIG_PCS_QCOM) */
static inline int qcom_xpcs_get_an_mode(struct dw_xpcs_qcom *xpcs,
					phy_interface_t interface)
{
	return -EINVAL;
}

static inline void qcom_xpcs_link_up(struct phylink_pcs *pcs, unsigned int mode,
				     phy_interface_t interface, int speed,
				     int duplex)
{
}

static inline void qcom_xpcs_configure_intr(struct phylink_pcs *pcs, phy_interface_t interface,
			    int speed, int duplex)
{
}

static inline void qcom_xpcs_validate(struct dw_xpcs_qcom *xpcs,
				      unsigned long *supported,
				      struct phylink_link_state *state)
{
}

static inline int qcom_xpcs_config_eee(struct dw_xpcs_qcom *xpcs, int mult_fact_100ns,
				       int enable)
{
	return -EINVAL;
}

static inline struct dw_xpcs_qcom *qcom_xpcs_create(void __iomem *addr,
						    phy_interface_t interface)
{
	return NULL;
}

static inline void qcom_xpcs_destroy(struct dw_xpcs_qcom *xpcs)
{
}

static inline const struct xpcs_compat *xpcs_find_compat(const struct xpcs_id *id,
							 phy_interface_t interface)
{
	return NULL;
}

static inline int qcom_xpcs_serdes_loopback(struct dw_xpcs_qcom *xpcs, bool on)
{
	return -EINVAL;
}

static inline int qcom_xpcs_pcs_loopback(struct dw_xpcs_qcom *xpcs, bool on)
{
	return -EINVAL;
}

static inline int qcom_xpcs_check_aneg_ioc(struct dw_xpcs_qcom *xpcs,
					   phy_interface_t interface)
{
	return -EINVAL;
}

static inline int qcom_xpcs_handle_an_intr(struct dw_xpcs_qcom *xpcs, phy_interface_t interface)
{
	return -EINVAL;
}

static inline int qcom_xpcs_intr_enable(struct dw_xpcs_qcom *xpcs)
{
	return -EINVAL;
}

static inline irqreturn_t ethqos_xpcs_isr(int irq, void *dev_data)
{
	return IRQ_NONE;
}

static inline int ethqos_xpcs_intr_config(struct net_device *ndev)
{
	return -EINVAL;
}

static inline int ethqos_xpcs_intr_enable(struct net_device *ndev)
{
	return -EINVAL;
}
static inline void ethqos_xpcs_irq_enable(struct net_device *ndev)
{
}
static inline void ethqos_xpcs_irq_disable(struct net_device *ndev)
{
}
static inline int ethqos_xpcs_init(struct net_device *ndev)
{
	return 0;
}
static inline int qcom_xpcs_verify_lnk_status_usxgmii(struct dw_xpcs_qcom *xpcs)
{
	return 0;
}
static inline int qcom_xpcs_verify_an(struct dw_xpcs_qcom *xpcs)
{
	return 0;
}
static inline int qcom_xpcs_lpm(struct dw_xpcs_qcom *xpcs, bool lpm)
{
	return 0;
}
static inline int qcom_xpcs_usxgmii_link_error_detect(struct dw_xpcs_qcom *xpcs, int speed)
{
	return 0;
}
static inline int qcom_xpcs_soft_reset_usxgmii(struct dw_xpcs_qcom *xpcs)
{
	return 0;
}
static inline void qcom_xpcs_link_up_usxgmii(struct dw_xpcs_qcom *xpcs,
					     int speed,
					     phy_interface_t interface)
{
}
#endif /* IS_ENABLED(CONFIG_PCS_QCOM) */
#endif /* __LINUX_PCS_XPCS_QCOM_H */
