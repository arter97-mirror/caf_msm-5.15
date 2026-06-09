// SPDX-License-Identifier: GPL-2.0

// Copyright (c) 2018-19, Linaro Limited
// Copyright (c) 2021, The Linux Foundation. All rights reserved.
// Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.

#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/phy.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#include <linux/io.h>
#include <linux/iopoll.h>
#include <linux/mii.h>
#include <linux/of_mdio.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/debugfs.h>
#include <linux/dma-iommu.h>
#include <linux/qcom_scm.h>
#include <linux/iommu.h>
#include <linux/micrel_phy.h>
#include <linux/tcp.h>
#include <linux/ip.h>
#include <linux/ipv6.h>
#include <linux/inetdevice.h>
#include <net/addrconf.h>
#include <linux/rtnetlink.h>
#include <linux/route.h>
#include <linux/if_arp.h>
#include <linux/inet.h>
#include <linux/panic_notifier.h>
#include <net/inet_common.h>
#include <linux/io-64-nonatomic-lo-hi.h>
#include <linux/gunyah/gh_vm.h>
#include <linux/gunyah/gh_rm_drv.h>
#include "stmmac.h"
#include "stmmac_platform.h"
#include "dwmac-qcom-ethqos.h"
#include "stmmac_ptp.h"
#include "dwmac-qcom-serdes.h"

#include <linux/kernel.h>
#include <linux/netlink.h>
#include <net/netlink.h>
#include <net/net_namespace.h>

#define MYGRP 1
#define PHY_RGMII_LOOPBACK_1000 0x4140
#define PHY_RGMII_LOOPBACK_100 0x6100
#define PHY_RGMII_LOOPBACK_10 0x4100
#define MDIO_PHYXS_VEND_PROVISION_5	0xc444
#define PHY_USXGMII_LOOPBACK_10000	0x0803
#define PHY_USXGMII_LOOPBACK_5000	0x0805
#define PHY_USXGMII_LOOPBACK_2500	0x0804
#define PHY_USXGMII_LOOPBACK_1000	0x0802
#define PHY_USXGMII_LOOPBACK_100	0x0801
#define PHY_USXGMII_LOOPBACK_10	0x0800
#define TN_SYSFS_DEV_ATTR_PERMS 0644
#define ETH_RTK_PHY_ID_RTL8261N 0x001CCAF3
#define EFUSE_MAC_ADDR_MASK 16
#define EMAC_SPEED_PEAK_IDX 7
static void ethqos_rgmii_io_macro_loopback(struct qcom_ethqos *ethqos,
					   int mode);
static int phy_digital_loopback_config(struct qcom_ethqos *ethqos, int speed, int config);
static void __iomem *tlmm_central_base_addr;
static char buf[2000];

static char err_names[10][14] = {"PHY_RW_ERR",
	"PHY_DET_ERR",
	"CRC_ERR",
	"RECEIVE_ERR",
	"OVERFLOW_ERR",
	"FBE_ERR",
	"RBU_ERR",
	"TDU_ERR",
	"DRIBBLE_ERR",
	"WDT_ERR",
};

#define NETLINK_TESTFAMILY 24
#define MYGRP 1
//Timer Variable
#define TIMEOUT_NSEC   (1000000000L)      //1 second in nano seconds
#define TIMEOUT_SEC    (180)                //180 seconds

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
static const u32 mac_reg_offsets[] = {0x0, 0x50, 0x60, 0x6c, 0xa0, 0xd0, 0x110,
					0x11c, 0x140, 0x200, 0x230, 0x240, 0x250,
					0x260, 0x278, 0x290, 0x300, 0x700, 0x730,
					0x800, 0x8cc, 0x900, 0x9d0, 0x9f0, 0xa00,
					0xa20, 0xa5c, 0xc00, 0xc10, 0xc80, 0xc88,
					0x1000, 0x1008, 0x1020, 0x1030, 0x1040, 0x1060,
					0x1070, 0x1080, 0x1090, 0x10a0, 0x10b0, 0x10e8,
					0x3000, 0x3010, 0x3018, 0x302c, 0x3040, 0x3050,
					0x3080, 0x7000, 0x7030, 0x7040, 0x7048, 0x7070,
					0x7080, 0x8000, 0x8010, 0x8040, 0x8070, 0x9000,
					0x9010, 0x9040, 0x9070, 0xa000, 0xa010, 0xa040,
					0xa070, 0xb000, 0xb010, 0xb040, 0xb070, 0xc000,
					0xc010, 0xc040, 0xc070, 0xd000, 0xd010, 0xd070,
					0xe000, 0xe024, 0xe02c, 0xe044, 0xe04c, 0xf000,
					0xf024, 0xf02c, 0xf044, 0xf04c, 0x10000, 0x10024,
					0x1002c, 0x10044, 0x1004c, 0x11000, 0x11024,
					0x1102c, 0x11044, 0x1104c, 0x12000, 0x12024,
					0x1202c, 0x12044, 0x1204c, 0x13000, 0x1300c,
					0x13024, 0x13030, 0x13038, 0x13044, 0x13050,
					0x13060, 0x13070};
static const u32 mac_reg_sizes[ARRAY_SIZE(mac_reg_offsets)] = {
					0x18, 0xc, 0x8, 0x30, 0x1c, 0x10, 0x8,
					0x14, 0x8, 0x28, 0xc, 0xc, 0xc,
					0xc, 0xc, 0x4, 0x100, 0x10, 0x8,
					0xac, 0x1c, 0xc0, 0x1c, 0x8, 0x10,
					0x18, 0xe8, 0x8, 0x4, 0x4, 0x8,
					0x4, 0xc, 0x4, 0x8, 0x1c, 0xc,
					0x4, 0x8, 0x8, 0x8, 0x8, 0x10,
					0xc, 0x4, 0x10, 0x4, 0x8, 0x14,
					0x8, 0x24, 0xc, 0x4, 0x20, 0x4,
					0x54, 0xc, 0xc, 0x14, 0x8, 0xc,
					0x18, 0x14, 0x8, 0xc, 0x18, 0x14,
					0x8, 0xc, 0x18, 0x14, 0x8, 0xc,
					0x18, 0x14, 0x8, 0xc, 0x18, 0x8,
					0x20, 0x4, 0x14, 0x4, 0x34, 0x20,
					0x4, 0x14, 0x4, 0x34, 0x20, 0x4,
					0x14, 0x4, 0x34, 0x20, 0x4, 0x14,
					0x4, 0x34, 0x20, 0x4, 0x14, 0x4,
					0x34, 0x8, 0xc, 0x4, 0x4, 0x4,
					0x4, 0x8, 0xc, 0x10};

#else
static const u32 mac_reg_offsets[] = {0};
static const u32 mac_reg_sizes[ARRAY_SIZE(mac_reg_offsets)] = {0};
#endif

#define RGMII_IO_MACRO_DEBUG1		0x20
#define EMAC_SYSTEM_LOW_POWER_DEBUG	0x28
#define RGMII_BLOCK_SIZE		0x200 //0x1D4 is max content in RGMII block

/* RGMII_IO_MACRO_CONFIG fields */
#define RGMII_CONFIG_FUNC_CLK_EN		BIT(30)
#define RGMII_CONFIG_POS_NEG_DATA_SEL		BIT(23)
#define RGMII_CONFIG_GPIO_CFG_RX_INT		GENMASK(21, 20)
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	#define RGMII_CONFIG_GPIO_CFG_TX_INT		GENMASK(21, 19)
	#define RGMII_CONFIG_MAX_SPD_PRG_9		GENMASK(18, 10)
	#define RGMII_CONFIG_MAX_SPD_PRG_2		GENMASK(9, 6)
#else
	#define RGMII_CONFIG_GPIO_CFG_TX_INT		GENMASK(19, 17)
	#define RGMII_CONFIG_MAX_SPD_PRG_9		GENMASK(16, 8)
	#define RGMII_CONFIG_MAX_SPD_PRG_2		GENMASK(7, 6)
#endif
#define RGMII_CONFIG_INTF_SEL			GENMASK(5, 4)
#define RGMII_CONFIG_BYPASS_TX_ID_EN		BIT(3)
#define RGMII_CONFIG_LOOPBACK_EN		BIT(2)
#define RGMII_CONFIG_PROG_SWAP			BIT(1)
#define RGMII_CONFIG_DDR_MODE			BIT(0)

/*RGMII DLL CONFIG*/
#define HSR_DLL_CONFIG					0x000B642C
#define HSR_DLL_CONFIG_2					0xA001
#define HSR_MACRO_CONFIG_2					0x01
#define HSR_DLL_TEST_CTRL					0x1400000
#define HSR_DDR_CONFIG					0x80040868
#define HSR_SDCC_USR_CTRL					0x2C010800
#define MACRO_CONFIG_2_MASK				GENMASK(24, 17)
#define	DLL_CONFIG_2_MASK				GENMASK(22, 0)
#define HSR_SDCC_DLL_TEST_CTRL				0x1800000
#define DDR_CONFIG_PRG_RCLK_DLY			        115
#define DLL_BYPASS					BIT(30)

/* SDCC_HC_REG_DLL_CONFIG fields */
#define SDCC_DLL_CONFIG_DLL_RST			BIT(30)
#define SDCC_DLL_CONFIG_PDN			BIT(29)
#define SDCC_DLL_CONFIG_MCLK_FREQ		GENMASK(26, 24)
#define SDCC_DLL_CONFIG_CDR_SELEXT		GENMASK(23, 20)
#define SDCC_DLL_CONFIG_CDR_EXT_EN		BIT(19)
#define SDCC_DLL_CONFIG_CK_OUT_EN		BIT(18)
#define SDCC_DLL_CONFIG_CDR_EN			BIT(17)
#define SDCC_DLL_CONFIG_DLL_EN			BIT(16)
#define SDCC_DLL_CONFIG_CDR_UPD_RATE	GENMASK(15, 14)
#define SDCC_DLL_CONFIG_DLL_PHASE_DET	GENMASK(11, 10)

#define SDCC_DLL_MCLK_GATING_EN			BIT(5)
#define SDCC_DLL_CDR_FINE_PHASE			GENMASK(3, 2)

/*SDCC_HC_REG_DLL_TEST_CTL fields*/
#define SDC4_DLL_TEST_CTL GENMASK(31, 0)

/* SDCC_HC_REG_DDR_CONFIG fields */
#define SDCC_DDR_CONFIG_PRG_DLY_EN		BIT(31)
#define SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY	GENMASK(26, 21)
#define SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_CODE	GENMASK(29, 27)
#define SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_EN	BIT(30)
#define SDCC_DDR_CONFIG_PRG_RCLK_DLY		GENMASK(8, 0)

/* SDCC_HC_REG_DLL_CONFIG2 fields */
#define SDCC_DLL_CONFIG2_DLL_CLOCK_DIS		BIT(21)
#define SDCC_DLL_CONFIG2_MCLK_FREQ_CALC		GENMASK(17, 10)
#define SDCC_DLL_CONFIG2_DDR_TRAFFIC_INIT_SEL	GENMASK(3, 2)
#define SDCC_DLL_CONFIG2_DDR_TRAFFIC_INIT_SW	BIT(1)
#define SDCC_DLL_CONFIG2_DDR_CAL_EN		BIT(0)

/* SDC4_STATUS bits */
#define SDC4_STATUS_DLL_LOCK			BIT(7)

/* DDR_STATUS bits */
#define DDR_STATUS_DLL_LOCK			BIT(0)

/* RGMII_IO_MACRO_CONFIG2 fields */
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	#define RGMII_CONFIG2_RSVD_CONFIG15		GENMASK(31, 24)
#else
	#define RGMII_CONFIG2_RSVD_CONFIG15		GENMASK(31, 17)
#endif
#define RGMII_CONFIG2_MODE_EN_VIA_GMII        BIT(21)
#define RGMII_CONFIG2_MAX_SPD_PRG_3		GENMASK(20, 17)
#define RGMII_CONFIG2_RGMII_CLK_SEL_CFG		BIT(16)
#define RGMII_CONFIG2_TX_TO_RX_LOOPBACK_EN	BIT(13)
#define RGMII_CONFIG2_CLK_DIVIDE_SEL		BIT(12)
#define RGMII_CONFIG2_RX_PROG_SWAP		BIT(7)
#define RGMII_CONFIG2_DATA_DIVIDE_CLK_SEL	BIT(6)
#define RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN	BIT(5)

/* EMAC_WRAPPER_SGMII_PHY_CNTRL0 fields */
#define SGMII_PHY_CNTRL0_2P5G_1G_CLK_SEL		GENMASK(6, 5)

/* EMAC_WRAPPER_SGMII_PHY_CNTRL1 fields */
#define SGMII_PHY_CNTRL1_RGMII_SGMII_CLK_MUX_SEL		BIT(0)
#define SGMII_PHY_CNTRL1_USXGMII_GMII_MASTER_CLK_MUX_SEL		BIT(4)
#define SGMII_PHY_CNTRL1_SGMII_TX_TO_RX_LOOPBACK_EN		BIT(3)

/* EMAC_WRAPPER_USXGMII_MUX_SEL fields */
#define USXGMII_CLK_BLK_GMII_CLK_BLK_SEL		BIT(1)
#define USXGMII_CLK_BLK_CLK_EN		BIT(0)

/* RGMII_IO_MACRO_SCRATCH_2 fields */
#define RGMII_SCRATCH2_MAX_SPD_PRG_4		GENMASK(5, 2)
#define RGMII_SCRATCH2_MAX_SPD_PRG_5		GENMASK(9, 6)
#define RGMII_SCRATCH2_MAX_SPD_PRG_6		GENMASK(13, 10)

/*RGMIII_IO_MACRO_BYPASS fields */
#define RGMII_BYPASS_EN		BIT(0)

#define EMAC_I0_EMAC_CORE_HW_VERSION_RGOFFADDR 0x00000070
#define EMAC_HW_v2_3_2_RG 0x20030002
#define USXGMII_DIV_CONFIG_RPCS_COUNT_DIV2		GENMASK(11, 8)

#define MII_BUSY 0x00000001
#define MII_WRITE 0x00000002

/* GMAC4 defines */
#define MII_GMAC4_GOC_SHIFT		2
#define MII_GMAC4_WRITE			BIT(MII_GMAC4_GOC_SHIFT)
#define MII_GMAC4_READ			(3 << MII_GMAC4_GOC_SHIFT)

#define MII_BUSY 0x00000001
#define MII_WRITE 0x00000002

#define DWC_ETH_QOS_PHY_INTR_STATUS     0x0013

#define LINK_UP 1
#define LINK_DOWN 0

#define LINK_DOWN_STATE 0x800
#define LINK_UP_STATE 0x400

#define MICREL_PHY_ID PHY_ID_KSZ9031
#define DWC_ETH_QOS_MICREL_PHY_INTCS 0x1b
#define DWC_ETH_QOS_MICREL_PHY_CTL 0x1f
#define DWC_ETH_QOS_MICREL_INTR_LEVEL 0x4000
#define DWC_ETH_QOS_BASIC_STATUS     0x0001
#define LINK_STATE_MASK 0x4
#define AUTONEG_STATE_MASK 0x20
#define MICREL_LINK_UP_INTR_STATUS BIT(0)

#define EMAC_PHY_REG_OFFSET 0x73000
#define RGMII_REG_OFFSET 0x74000
#define EGPIO_ENABLE 0x1000
#define ETHQOS_SYSFS_DEV_ATTR_PERMS 0644
#define BUFF_SZ 256

#define GMAC_CONFIG_PS			BIT(15)
#define GMAC_CONFIG_FES			BIT(14)
#define GMAC_AN_CTRL_RAN	BIT(9)
#define GMAC_AN_CTRL_ANE	BIT(12)

#define DWMAC4_PCS_BASE	0x000000e0
#define RGMII_CONFIG_10M_CLK_DVD GENMASK(18, 10)

#if IS_ENABLED(CONFIG_DWXGMAC_QCOM_4K)
	#define XGMAC_MTLQ_BASE_ADDR			0x00008000
	#define XGMAC_CH_OFFSET					0x00001000
#else
	#define XGMAC_MTLQ_BASE_ADDR			0x00001100
	#define XGMAC_CH_OFFSET					0x00000080
#endif
#define XGMAC_MTL_RXQ_DMA_MAP0 0x00001030
#define XGMAC_MTL_RXQ_DMA_MAP1 0x00001034
#define XGMAC_MTLQX_BASE_ADDR(x)		(XGMAC_MTLQ_BASE_ADDR + \
						((x) * XGMAC_CH_OFFSET))
#define XGMAC_MTL_TCx_QUANTUM_WEIGHT(x)		(XGMAC_MTLQX_BASE_ADDR(x) + 0x18)
#define XGMAC_MTL_TCx_SENDSLOPE(x)			(XGMAC_MTLQX_BASE_ADDR(x) + 0x1c)
#define XGMAC_MTL_TCx_HICREDIT(x)			(XGMAC_MTLQX_BASE_ADDR(x) + 0x20)
#define XGMAC_MTL_TCx_LOCREDIT(x)			(XGMAC_MTLQX_BASE_ADDR(x) + 0x24)
#define XGMAC_Qx_TX_FLOW_CTRL(x)	(0x00000070 + (x) * 4)
#define XGMAC_RXQ_CTRL2			0x000000a8
#define XGMAC_RXQ_CTRL3			0x000000ac
#define XGMAC_RX_FLOW_CTRL		0x00000090
#define PCP_UPPER_LIMIT		7

static DECLARE_WAIT_QUEUE_HEAD(mac_rec_wq);
static bool mac_rec_wq_flag;

static struct ethqos_emac_por emac_por[] = {
	{ .offset = RGMII_IO_MACRO_CONFIG,	.value = 0x0 },
	{ .offset = SDCC_HC_REG_DLL_CONFIG,	.value = 0x0 },
	{ .offset = SDCC_TEST_CTL,	        .value = 0x0 },
	{ .offset = SDCC_HC_REG_DDR_CONFIG,	.value = 0x0 },
	{ .offset = SDCC_HC_REG_DLL_CONFIG2,	.value = 0x0 },
	{ .offset = SDCC_USR_CTL,		.value = 0x0 },
	{ .offset = RGMII_IO_MACRO_CONFIG2,	.value = 0x0},
};

static struct ethqos_emac_driver_data emac_por_data = {
	.por = emac_por,
	.num_por = ARRAY_SIZE(emac_por),
};

struct emac_emb_smmu_cb_ctx emac_emb_smmu_ctx = {0};
struct plat_stmmacenet_data *plat_dat;
struct qcom_ethqos *pethqos[ETH_MAX_NICS];
void *ipc_emac_log_ctxt;

#ifdef MODULE
static char *eipv4;
module_param(eipv4, charp, 0660);
MODULE_PARM_DESC(eipv4, "ipv4 value from ethernet partition");

static char *eipv6;
module_param(eipv6, charp, 0660);
MODULE_PARM_DESC(eipv6, "ipv6 value from ethernet partition");

static char *ermac;
module_param(ermac, charp, 0660);
MODULE_PARM_DESC(ermac, "mac address from ethernet partition");
static char *eiface;
module_param(eiface, charp, 0660);
MODULE_PARM_DESC(eiface, "Interface type from ethernet partition");
static char *eqos;
module_param(eqos, charp, 0600);
MODULE_PARM_DESC(eqos, "QOS Config support from ethernet partition");
static char *ewait_switch_rdy;
module_param(ewait_switch_rdy, charp, 0600);
MODULE_PARAM_DESC(ewait_switch_rdy, "Link up delay option from ethernet partition");
static char *eautoneg;
module_param(eautoneg, charp, 0600);
MODULE_PARAM_DESC(eautoneg, "Autonegotiation enablement option from ethernet partition");
#endif

static bool qos_use_skprio;
module_param(qos_use_skprio, bool, 0644);
MODULE_PARM_DESC(qos_use_skprio, "Use SKPRIO for Tx Routing");

inline void *qcom_ethqos_get_priv(struct qcom_ethqos *ethqos)
{
	struct platform_device *pdev = ethqos->pdev;
	struct net_device *dev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(dev);

	return priv;
}

static unsigned char dev_addr[ETH_ALEN] = {
	0, 0x55, 0x7b, 0xb5, 0x7d, 0xf7};

void *ipc_stmmac_log_ctxt;
void *ipc_stmmac_log_ctxt_low;
int stmmac_enable_ipc_low;
static struct ip_params pparams;
static struct mac_params mparams = {
	.eth_intf = PHY_INTERFACE_MODE_NA,
	.is_valid_eth_intf = false,
	.link_speed = 0,
	.qoscfg_name = "",
	.wait_switch_rdy = false,
	.c37_an_en = true
};
long phyaddr_pt_param = -1;

#define RX_CLK_SYSFS_DEV_ATTR_PERMS 0644

unsigned int dwmac_qcom_get_eth_type(unsigned char *buf)
{
	return
		((((u16)buf[QTAG_ETH_TYPE_OFFSET] << 8) |
		  buf[QTAG_ETH_TYPE_OFFSET + 1]) == ETH_P_8021Q) ?
		(((u16)buf[QTAG_VLAN_ETH_TYPE_OFFSET] << 8) |
		 buf[QTAG_VLAN_ETH_TYPE_OFFSET + 1]) :
		 (((u16)buf[QTAG_ETH_TYPE_OFFSET] << 8) |
		  buf[QTAG_ETH_TYPE_OFFSET + 1]);
}

static inline unsigned int dwmac_qcom_get_vlan_ucp(unsigned char  *buf)
{
	return
		(((u16)buf[QTAG_UCP_FIELD_OFFSET] << 8)
		 | buf[QTAG_UCP_FIELD_OFFSET + 1]);
}

static bool is_skprio_routing_set(void *_priv)
{
	struct stmmac_priv *priv = _priv;
	return (qos_use_skprio || priv->plat->qos_use_skprio);
}

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
void dwmac_qcom_set_skb_prio(void *priv_n, struct sk_buff *skb, u32 queue)
{
	u32 priority = 0;
	u32 c2t_map = 0;
	struct qcom_ethqos *ethqos = priv_n;
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	if (!is_skprio_routing_set(priv))
		return;

	if (priv->plat->qos_active && priv->plat->qos_ch_map.tc_rx_info[queue]) {
		c2t_map = priv->plat->qos_ch_map.ch_to_tc_map_rx[queue];
		while (c2t_map) {
			c2t_map = c2t_map >> 1;
			priority++;
		}
		skb->priority = priority;
	}
}
#endif

u16 dwmac_qcom_select_queue(struct net_device *dev,
			    struct sk_buff *skb,
			    struct net_device *sb_dev)
{
	u16 txqueue_select = ALL_OTHER_TRAFFIC_TX_CHANNEL;
	unsigned int eth_type, priority;
	struct stmmac_priv *priv = netdev_priv(dev);
	struct qcom_ethqos *ethqos = (struct qcom_ethqos *)priv->plat->bsp_priv;
	s8 eprio;
	int i = 0, tc_prio = 0;

	if (priv->plat->tc_select_queue) {
		if (skb && skb->priority) {
			txqueue_select =  netdev_pick_tx(dev, skb, NULL) %
					  dev->real_num_tx_queues;
		}

		if (ethqos->ipa_enabled && txqueue_select == 0)
			txqueue_select = 1;

		return txqueue_select;
	}
	/* Retrieve ETH type */
	eth_type = dwmac_qcom_get_eth_type(skb->data);

	if (priv->plat->qos_active && !is_skprio_routing_set(priv) && skb_vlan_tag_present(skb)) {
		priority =  skb_vlan_tag_get_prio(skb);
		tc_prio = 1 << priority;
		for (i = priv->plat->tx_qos_queues_to_use - 1; i > 1; i--) {
			if (priv->tx_queue_pcp_map[i] & tc_prio) {
				txqueue_select = i;
				break;
			}
		}
	} else if (priv->plat->qos_active && is_skprio_routing_set(priv) && skb->priority) {
		/* TODO: IF qos ie enabled and IPA offload is disabled, we need to handle*/
		tc_prio = 1 << (skb->priority - 1);
		for (i = 2; i < priv->plat->tx_qos_queues_to_use; i++) {
			if (priv->plat->qos_ch_map.tc_tx_info[i]) {
				if (priv->plat->qos_ch_map.ch_to_tc_map_tx[i] & tc_prio) {
					txqueue_select = i;
					break;
				}
			}
		}
	} else if (eth_type == ETH_P_TSN) {
		/* Read VLAN priority field from skb->data */
		priority = dwmac_qcom_get_vlan_ucp(skb->data);

		priority >>= VLAN_TAG_UCP_SHIFT;
		if (priority == CLASS_A_TRAFFIC_UCP) {
			txqueue_select = CLASS_A_TRAFFIC_TX_CHANNEL;
		} else if (priority == CLASS_B_TRAFFIC_UCP) {
			txqueue_select = CLASS_B_TRAFFIC_TX_CHANNEL;
		} else {
			if (ethqos->ipa_enabled)
				txqueue_select = ALL_OTHER_TRAFFIC_TX_CHANNEL;
			else
				txqueue_select = ALL_OTHER_TX_TRAFFIC_IPA_DISABLED;
		}
	} else if (eth_type == ETH_P_1588) {
		/*gPTP seelct tx queue 1*/
		txqueue_select = NON_TAGGED_IP_TRAFFIC_TX_CHANNEL;
	} else if (ethqos->cv2x_pvm_only_enabled && skb_vlan_tag_present(skb)) {
		/* VLAN tagged IP packet or any other non vlan packets (PTP)*/
		/* Getting VLAN priority field from skb */
		priority =  skb_vlan_tag_get_prio(skb);
		#if IS_ENABLED(CONFIG_ETHQOS_QCOM_HOSTVM)
			eprio = -1;
		#else
			eprio = ethqos->cv2x_priority;
		#endif

		if (priority == eprio)
			txqueue_select = ethqos->cv2x_queue;
		else if (ethqos->ipa_enabled)
			txqueue_select = ALL_OTHER_TRAFFIC_TX_CHANNEL;
		else
			txqueue_select = ALL_OTHER_TX_TRAFFIC_IPA_DISABLED;
	} else if (ethqos->ipa_enabled)
		txqueue_select = ALL_OTHER_TRAFFIC_TX_CHANNEL;
	else
		txqueue_select = ALL_OTHER_TX_TRAFFIC_IPA_DISABLED;

	return txqueue_select;
}

void dwmac_qcom_program_avb_algorithm(struct stmmac_priv *priv,
				      struct ifr_data_struct *req)
{
	struct dwmac_qcom_avb_algorithm l_avb_struct, *u_avb_struct =
		(struct dwmac_qcom_avb_algorithm *)req->ptr;
	struct dwmac_qcom_avb_algorithm_params *avb_params;

	ETHQOSDBG("\n");

	if (copy_from_user(&l_avb_struct, (void __user *)u_avb_struct,
			   sizeof(struct dwmac_qcom_avb_algorithm)))
		ETHQOSERR("Failed to fetch AVB Struct\n");

	if (priv->speed == SPEED_1000)
		avb_params = &l_avb_struct.speed1000params;
	else
		avb_params = &l_avb_struct.speed100params;

	/* Application uses 1 for CLASS A traffic and
	 * 2 for CLASS B traffic
	 * Configure right channel accordingly
	 */
	if (l_avb_struct.qinx == 1)
		l_avb_struct.qinx = CLASS_A_TRAFFIC_TX_CHANNEL;
	else if (l_avb_struct.qinx == 2)
		l_avb_struct.qinx = CLASS_B_TRAFFIC_TX_CHANNEL;

	priv->plat->tx_queues_cfg[l_avb_struct.qinx].mode_to_use =
		MTL_QUEUE_AVB;
	priv->plat->tx_queues_cfg[l_avb_struct.qinx].send_slope =
		avb_params->send_slope,
	priv->plat->tx_queues_cfg[l_avb_struct.qinx].idle_slope =
		avb_params->idle_slope,
	priv->plat->tx_queues_cfg[l_avb_struct.qinx].high_credit =
		avb_params->hi_credit,
	priv->plat->tx_queues_cfg[l_avb_struct.qinx].low_credit =
		avb_params->low_credit,

	priv->hw->mac->config_cbs(priv->hw,
	priv->plat->tx_queues_cfg[l_avb_struct.qinx].send_slope,
	   priv->plat->tx_queues_cfg[l_avb_struct.qinx].idle_slope,
	   priv->plat->tx_queues_cfg[l_avb_struct.qinx].high_credit,
	   priv->plat->tx_queues_cfg[l_avb_struct.qinx].low_credit,
	   l_avb_struct.qinx);

	ETHQOSDBG("\n");
}

unsigned int dwmac_qcom_get_plat_tx_coal_frames(struct sk_buff *skb)
{
#ifdef CONFIG_PTPSUPPORT_OBJ
	bool is_udp;
#endif
	unsigned int eth_type;

	eth_type = dwmac_qcom_get_eth_type(skb->data);

#ifdef CONFIG_PTPSUPPORT_OBJ
	if (eth_type == ETH_P_1588)
		return PTP_INT_MOD;
#endif

	if (eth_type == ETH_P_TSN)
		return AVB_INT_MOD;
	if (eth_type == ETH_P_IP || eth_type == ETH_P_IPV6) {
#ifdef CONFIG_PTPSUPPORT_OBJ
		is_udp = (((eth_type == ETH_P_IP) &&
				   (ip_hdr(skb)->protocol ==
					IPPROTO_UDP)) ||
				  ((eth_type == ETH_P_IPV6) &&
				   (ipv6_hdr(skb)->nexthdr ==
					IPPROTO_UDP)));

		if (is_udp && ((udp_hdr(skb)->dest ==
			htons(PTP_UDP_EV_PORT)) ||
			(udp_hdr(skb)->dest ==
			  htons(PTP_UDP_GEN_PORT))))
			return PTP_INT_MOD;
#endif
		return IP_PKT_INT_MOD;
	}
	return DEFAULT_INT_MOD;
}

int ethqos_send_netlink_socket(int nl_data, int nl_type, int nl_flag)
{
	struct sk_buff *skb_out;
	struct qcom_ethqos *ethqos = pethqos[0];
	int res = 0;

	if (!ethqos->socket_nl) {
		ETHQOSINFO("Netlink-kernel : No Socket_nl created\n");
		return -1;
	}

	if (!netlink_has_listeners(ethqos->socket_nl, MYGRP)) {
		ETHQOSINFO("Netlink-kernel : No listeners\n");
		return -1;
	}

	skb_out = nlmsg_new(sizeof(nl_data), GFP_KERNEL);
	if (!skb_out) {
		ETHQOSINFO("Failed to allocate a new skb\n");
		return -1;
	}

	ethqos->nlh = nlmsg_put(skb_out, 0, 0, nl_type, sizeof(nl_data), nl_flag);
	NETLINK_CB(skb_out).portid = 0; //Multicast
	NETLINK_CB(skb_out).dst_group = MYGRP;// 0 if unicast

	memcpy(nlmsg_data(ethqos->nlh), &nl_data, sizeof(nl_data));

	res = netlink_broadcast(ethqos->socket_nl, skb_out, 0, MYGRP, GFP_KERNEL);
	if (res < 0) {
		ETHQOSINFO("Error while sending bak to user, err id: %d\n", res);
		return -1;
	}
	return 0;
}

static void ethqos_phy_interface_reset(struct work_struct *work)
{
	int ret;
	struct qcom_ethqos *ethqos =
		container_of(work, struct qcom_ethqos, emac_phy_work);

	ret = qcom_ethqos_bring_down_phy_if(&ethqos->pdev->dev);
	if (ret < 0) {
		ETHQOSERR("%s failed %d\n", __func__, __LINE__);
		return;
	}
	ret = qcom_ethqos_bring_up_phy_if(&ethqos->pdev->dev, 0);
	if (ret < 0) {
		ETHQOSERR("%s failed %d\n", __func__, __LINE__);
		return;
	}
}

int ethqos_handle_prv_ioctl(struct net_device *dev, struct ifreq *ifr, int cmd)
{
	struct stmmac_priv *pdata = netdev_priv(dev);
	struct ifr_data_struct req;
	struct pps_cfg eth_pps_cfg;
	int ret = 0;
	enum eth_mode data;
	unsigned int user_data;
	unsigned int new_mode;
	struct qcom_ethqos *ethqos;
	struct device *device;

	if (!pdata) {
		ETHQOSINFO("%s:: pdata is NULL\n", __func__);
		return -EFAULT;
	}
	ethqos  = pdata->plat->bsp_priv;
	if (!ethqos) {
		ETHQOSINFO("%s:: ethqos is NULL\n", __func__);
		return -EFAULT;
	}

	device = pdata->device;
	if (!device) {
		ETHQOSINFO("%s:: device is NULL\n", __func__);
		return -EFAULT;
	}

	if (copy_from_user(&req, ifr->ifr_ifru.ifru_data,
			   sizeof(struct ifr_data_struct)))
		return -EFAULT;

	switch (req.cmd) {
	case ETHQOS_CONFIG_PPSOUT_CMD:
		if (copy_from_user(&eth_pps_cfg, (void __user *)req.ptr,
				   sizeof(struct pps_cfg)))
			return -EFAULT;

		if (eth_pps_cfg.ppsout_ch < 0 ||
		    eth_pps_cfg.ppsout_ch >= pdata->dma_cap.pps_out_num)
			ret = -EOPNOTSUPP;
		else if ((eth_pps_cfg.ppsout_align == 1) &&
			 ((eth_pps_cfg.ppsout_ch != DWC_ETH_QOS_PPS_CH_0) &&
			 (eth_pps_cfg.ppsout_ch != DWC_ETH_QOS_PPS_CH_3)))
			ret = -EOPNOTSUPP;
		else
			ret = ppsout_config(pdata, &eth_pps_cfg);
		break;
	case ETHQOS_AVB_ALGORITHM:
		dwmac_qcom_program_avb_algorithm(pdata, &req);
		break;
	case ETHQOS_THERM_GET_REM_RESP:
		if (copy_from_user(&user_data, (void __user *)req.ptr,
				   sizeof(unsigned int))) {
			ETHQOSERR("Failed to copy data from peer\n");
			return -EFAULT;
		}
		ETHQOSINFO("Got ACK or NACK from peer : %d\n", user_data);
		ethqos->peer_resp = user_data;
		if (ethqos->init_complete)
			complete(&ethqos->thermal_timer);
		break;
	case ETHQOS_THERM_SET_LOCAL_ETH_CAP:
		ETHQOSINFO("Send local Capabilities to peer\n");
		data = ((USXGMII_10G | USXGMII_5G) | SGMII_1G);
		ret = ethqos_send_netlink_socket(data, THERM_SET_LOCAL_ETH_CAP, SUCCESS);
		if (ret < 0)
			return -1;
		break;
	case ETHQOS_THERM_GET_REM_CAP:
		if (copy_from_user(&user_data, (void __user *)req.ptr,
				   sizeof(unsigned int))) {
			ETHQOSERR("Failed to copy\n");
			return -EFAULT;
		}
		ETHQOSINFO("Got remote capabilities : %d\n", user_data);
		ethqos->peer_cap = user_data;
		break;
	case ETHQOS_THERM_SET_LOCAL_ETH_MODE:
		if (copy_from_user(&new_mode, (void __user *)req.ptr,
				   sizeof(unsigned int))) {
			ETHQOSERR("Failed to copy\n");
			return -EFAULT;
		}
		ETHQOSINFO("Got mode from local eth : %d\n", new_mode);
		ethqos->current_mode = new_mode;

		if (ethqos->current_mode == USXGMII_10G ||
		    ethqos->current_mode == SGMII_1G ||
		    ethqos->current_mode ==  USXGMII_5G)
			queue_work(system_wq, &ethqos->emac_phy_state_work);

		if (ethqos->current_mode == USXGMII_10G ||
		    ethqos->current_mode == SGMII_1G ||
		    ethqos->current_mode ==  USXGMII_5G) {
			ret = ethqos_send_netlink_socket(ethqos->current_mode,
							 THERM_NEW_PEER_ETH_MODE, SUCCESS);
			if (ret < 0)
				return -1;
		} else {
			ret = ethqos_send_netlink_socket(ethqos->current_mode,
							 THERM_NEW_PEER_ETH_MODE, FAIL);
			if (ret < 0)
				return -1;
		}
	default:
		break;
	}
	return ret;
}

static int set_early_ethernet_ipv4(char *ipv4_addr_in)
{
	int ret = 1;

	pparams.is_valid_ipv4_addr = false;

	if (!ipv4_addr_in)
		return ret;

	strscpy(pparams.ipv4_addr_str,
		ipv4_addr_in, sizeof(pparams.ipv4_addr_str));
	ETHQOSDBG("Early ethernet IPv4 addr: %s\n", pparams.ipv4_addr_str);

	ret = in4_pton(pparams.ipv4_addr_str, -1,
		       (u8 *)&pparams.ipv4_addr.s_addr, -1, NULL);
	if (ret != 1 || pparams.ipv4_addr.s_addr == 0) {
		ETHQOSERR("Invalid ipv4 address programmed: %s\n",
			  ipv4_addr_in);
		return ret;
	}

	pparams.is_valid_ipv4_addr = true;
	return ret;
}

static int set_early_ethernet_ipv6(char *ipv6_addr_in)
{
	int ret = 1;

	pparams.is_valid_ipv6_addr = false;

	if (!ipv6_addr_in)
		return ret;

	strscpy(pparams.ipv6_addr_str,
		ipv6_addr_in, sizeof(pparams.ipv6_addr_str));
	ETHQOSDBG("Early ethernet IPv6 addr: %s\n", pparams.ipv6_addr_str);

	ret = in6_pton(pparams.ipv6_addr_str, -1,
		       (u8 *)&pparams.ipv6_addr.ifr6_addr.s6_addr32, -1, NULL);
	if (ret != 1 || !pparams.ipv6_addr.ifr6_addr.s6_addr32)  {
		ETHQOSERR("Invalid ipv6 address programmed: %s\n",
			  ipv6_addr_in);
		return ret;
	}

	pparams.is_valid_ipv6_addr = true;
	return ret;
}

static int set_early_ethernet_mac(char *mac_addr)
{
	bool valid_mac = false;

	pparams.is_valid_mac_addr = false;
	if (!mac_addr)
		return 1;

	valid_mac = mac_pton(mac_addr, pparams.mac_addr);
	if (!valid_mac)
		goto fail;

	valid_mac = is_valid_ether_addr(pparams.mac_addr);
	if (!valid_mac)
		goto fail;

	pparams.is_valid_mac_addr = true;
	return 0;

fail:
	ETHQOSERR("Invalid Mac address programmed: %s\n", mac_addr);
	return 1;
}

static int set_ethernet_phyaddr(char *phy_addr)
{
	if (!phy_addr)
		return 1;

	if (kstrtol(phy_addr, 0, &phyaddr_pt_param))
		goto fail;

	return 0;

fail:
	ETHQOSERR("Invalid phy addr programmed: %s\n", phy_addr);
	return 1;
}

static int set_ethernet_interface(char *eth_intf)
{
	mparams.is_valid_eth_intf = false;
	if (!eth_intf)
		return 1;

	if (strlen(eth_intf) == 0)
		return 1;

	ETHQOSDBG("Command Line eth interface: %s\n", eth_intf);
	if (!strcmp("rgmii", eth_intf)) {
		mparams.eth_intf = PHY_INTERFACE_MODE_RGMII;
		mparams.is_valid_eth_intf = true;
	} else if (!strcmp("sgmii", eth_intf)) {
		mparams.eth_intf = PHY_INTERFACE_MODE_SGMII;
		mparams.is_valid_eth_intf = true;
	} else if (!strcmp("usxgmii", eth_intf)) {
		mparams.eth_intf =  PHY_INTERFACE_MODE_USXGMII;
		mparams.is_valid_eth_intf = true;
	} else if (!strcmp("2500base", eth_intf)) {
		mparams.eth_intf =  PHY_INTERFACE_MODE_2500BASEX;
		mparams.is_valid_eth_intf = true;
	} else if (!strcmp("5gbase-r", eth_intf)) {
		mparams.eth_intf =  PHY_INTERFACE_MODE_5GBASER;
		mparams.is_valid_eth_intf = true;
	} else {
		ETHQOSERR("Invalid Eth interface programmed: %s\n", eth_intf);
		return 1;
	}
	return 0;
}

static int set_ethernet_speed(char *eth_speed)
{
	if (!eth_speed)
		return 1;

	if (kstrtou32(eth_speed, 0, &mparams.link_speed))
		goto fail;

	switch (mparams.link_speed) {
	case SPEED_10000:
	case SPEED_5000:
	case SPEED_2500:
	case SPEED_1000:
	case SPEED_100:
	case SPEED_10:
		break;

	default:
		/* Reset link speed on invalid value */
		mparams.link_speed = 0;
		return 1;
	}
	return 0;

fail:
	ETHQOSERR("Invalid link speed programmed: %d\n", mparams.link_speed);
	return 1;
}

static int set_ethernet_qos_cfg(char *qoscfg)
{
	if (!qoscfg)
		return 1;

	if ((strlen(qoscfg) == 0) || (strlen(qoscfg) > 4))
		return 1;

	strscpy(mparams.qoscfg_name, qoscfg, sizeof(mparams.qoscfg_name));

	return 0;
}

static int set_ethernet_wait_switch_rdy(char *eth_wait_switch_rdy)
{
	if (!eth_wait_switch_rdy || strlen(eth_wait_switch_rdy) == 0)
		return -EINVAL;

	if (kstrtobool(eth_wait_switch_rdy, &mparams.wait_switch_rdy))
		return -EINVAL;

	return 0;
}

static int set_ethernet_autoneg(char *eth_autoneg)
{
	if (!eth_autoneg || strlen(eth_autoneg) == 0)
		return -EINVAL;

	if (kstrtobool(eth_autoneg, &mparams.c37_an_en))
		return -EINVAL;
	return 0;
}

#ifndef MODULE
static int __init set_early_ethernet_ipv4_static(char *ipv4_addr_in)
{
	int ret = 1;

	ret = set_early_ethernet_ipv4(ipv4_addr_in);
	return ret;
}

__setup("eipv4=", set_early_ethernet_ipv4_static);

static int __init set_early_ethernet_ipv6_static(char *ipv6_addr_in)
{
	int ret = 1;

	ret = set_early_ethernet_ipv6(ipv6_addr_in);
	return ret;
}

__setup("eipv6=", set_early_ethernet_ipv6_static);

static int __init set_early_ethernet_mac_static(char *mac_addr)
{
	int ret = 1;

	ret = set_early_ethernet_mac(mac_addr);
	return ret;
}

__setup("ermac=", set_early_ethernet_mac_static);

static int __init set_ethernet_phyaddr_static(char *phy_addr)
{
	int ret = 1;

	ret = set_ethernet_phyaddr(phy_addr);
	return ret;
}

__setup("ephyaddr=", set_ethernet_phyaddr_static);

static int __init set_ethernet_interface_static(char *eth_intf)
{
	int ret = 1;

	ret = set_ethernet_interface(eth_intf);
	return ret;
}

__setup("eiface=", set_ethernet_interface_static);

static int __init set_ethernet_speed_static(char *eth_speed)
{
	int ret = 1;

	ret = set_ethernet_speed(eth_speed);
	return ret;
}

__setup("espeed=", set_ethernet_speed_static);

static int __init set_ethernet_qoscfg_static(char *eth_qos)
{
	int ret = 1;

	ret = set_ethernet_qos_cfg(eth_qos);
	if (ret)
		mparams.qoscfg_name[0] = '\0';

	return 0;
}

__setup("eqos=", set_ethernet_qoscfg_static);

static int __init set_ethernet_wait_switch_rdy_static(char *eth_wait_switch_rdy)
{
	int ret = -EINVAL;

	ret = set_ethernet_wait_switch_rdy(eth_wait_switch_rdy);
	if (ret)
		mparams.wait_switch_rdy = false;
	return 0;
}

__setup("ewait_switch_rdy=", set_ethernet_wait_switch_rdy_static);

static int __init set_ethernet_autoneg_static(char *eth_autoneg)
{
	int ret = -EINVAL;

	ret = set_ethernet_autoneg(eth_autoneg);
	if (ret)
		mparams.c37_an_en = true;	// C37 AN is enabled by default
	return 0;
}
__setup("eautoneg=", set_ethernet_autoneg_static);
#endif

static int qcom_ethqos_add_ipaddr(struct ip_params *ip_info,
				  struct net_device *dev)
{
	int res = 0;
	struct ifreq ir;
	struct sockaddr_in *sin = (void *)&ir.ifr_ifru.ifru_addr;
	struct net *net = dev_net(dev);

	if (!net || !net->genl_sock || !net->genl_sock->sk_socket) {
		ETHQOSINFO("Sock is null, unable to assign ipv4 address\n");
		return res;
	}
	/*For valid Ipv4 address*/
	memset(&ir, 0, sizeof(ir));
	memcpy(&sin->sin_addr.s_addr, &ip_info->ipv4_addr,
	       sizeof(sin->sin_addr.s_addr));

	strscpy(ir.ifr_ifrn.ifrn_name,
		dev->name, sizeof(ir.ifr_ifrn.ifrn_name));
	sin->sin_family = AF_INET;
	sin->sin_port = 0;

	res = devinet_ioctl(net, SIOCSIFADDR, &ir);
		if (res) {
			ETHQOSERR("can't setup IPv4 address!: %d\r\n", res);
		} else {
			ETHQOSINFO("Assigned IPv4 address: %s\r\n",
				   ip_info->ipv4_addr_str);
#ifdef CONFIG_MSM_BOOT_TIME_MARKER
	update_marker("M - Etherent Assigned IPv4 address");
#endif
		}
	return res;
}

static int qcom_ethqos_add_ipv6addr(struct ip_params *ip_info,
				    struct net_device *dev)
{
	int ret = 0;
	struct in6_ifreq ir6;
	char *prefix;
	struct net *net = dev_net(dev);
	struct stmmac_priv *priv = netdev_priv(dev);
	struct qcom_ethqos *ethqos = priv->plat->bsp_priv;
	struct inet6_dev *idev;

	idev = __in6_dev_get_safely(dev);

	/*For valid IPv6 address*/

	if (!net || !net->genl_sock || !net->genl_sock->sk_socket) {
		ETHQOSERR("Sock is null, unable to assign ipv6 address\n");
		return -EFAULT;
	}

	if (!net->ipv6.devconf_dflt) {
		ETHQOSERR("ipv6.devconf_dflt is null, schedule wq\n");
		schedule_delayed_work(&ethqos->ipv6_addr_assign_wq,
				      msecs_to_jiffies(100));
		return ret;
	}
	memset(&ir6, 0, sizeof(ir6));
	memcpy(&ir6, &ip_info->ipv6_addr, sizeof(struct in6_ifreq));
	ir6.ifr6_ifindex = dev->ifindex;

	prefix = strnchr(ip_info->ipv6_addr_str,
			 strlen(ip_info->ipv6_addr_str), '/');

	if (!prefix) {
		ir6.ifr6_prefixlen = 0;
	} else {
		ret = kstrtoul(prefix + 1, 0, (unsigned long *)&ir6.ifr6_prefixlen);
		if (ir6.ifr6_prefixlen > 128)
			ir6.ifr6_prefixlen = 0;
	}

	addrconf_add_linklocal(idev, &ir6.ifr6_addr, 0);
	ETHQOSDBG("Assigned IPv6 address: %s\r\n",
		  ip_info->ipv6_addr_str);

#ifdef CONFIG_MSM_BOOT_TIME_MARKER
	update_marker("M - Ethernet Assigned IPv6 address");
#endif

	return ret;
}

/* Local SMC buffer is valid only for HW where IO macro space is moved to TZ.
 * for other configurations it should always be passed as NULL
 */
static int rgmii_readl(struct qcom_ethqos *ethqos, unsigned int offset)
{
	if (ethqos->shm_rgmii_local.vaddr)
		return readl(ethqos->shm_rgmii_local.vaddr + offset);
	else
		return readl(ethqos->rgmii_base + offset);
}

static void rgmii_writel(struct qcom_ethqos *ethqos,
			 int value, unsigned int offset)
{
	writel(value, ethqos->rgmii_base + offset);
}

static void rgmii_updatel(struct qcom_ethqos *ethqos,
			  int mask, int val, unsigned int offset)
{
	unsigned int temp;

	temp =  rgmii_readl(ethqos, offset);
	temp = (temp & ~(mask)) | val;
	rgmii_writel(ethqos, temp, offset);
}

static void rgmii_dump(void *priv)
{
	struct qcom_ethqos *ethqos = priv;
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
		/* Reg_dump should have the whole IO macro reg space so below copy to buffer
		 * has to be done based on offset in the reg_dump unlike invoking register
		 * read for every config as below
		 */
		qcom_scm_call_iomacro_dump(ethqos->rgmii_phy_base,
					   ethqos->shm_rgmii_local.paddr, RGMII_BLOCK_SIZE);
		qtee_shmbridge_inv_shm_buf(&ethqos->shm_rgmii_local);
	}
#endif

	dev_dbg(&ethqos->pdev->dev, "Rgmii register dump\n");
	dev_dbg(&ethqos->pdev->dev, "RGMII_IO_MACRO_CONFIG: %x\n",
		rgmii_readl(ethqos, RGMII_IO_MACRO_CONFIG));
	dev_dbg(&ethqos->pdev->dev, "SDCC_HC_REG_DLL_CONFIG: %x\n",
		rgmii_readl(ethqos, SDCC_HC_REG_DLL_CONFIG));
	dev_dbg(&ethqos->pdev->dev, "SDCC_HC_REG_DDR_CONFIG: %x\n",
		rgmii_readl(ethqos, SDCC_HC_REG_DDR_CONFIG));
	dev_dbg(&ethqos->pdev->dev, "SDCC_HC_REG_DLL_CONFIG2: %x\n",
		rgmii_readl(ethqos, SDCC_HC_REG_DLL_CONFIG2));
	dev_dbg(&ethqos->pdev->dev, "SDC4_STATUS: %x\n",
		rgmii_readl(ethqos, SDC4_STATUS));
	dev_dbg(&ethqos->pdev->dev, "SDCC_USR_CTL: %x\n",
		rgmii_readl(ethqos, SDCC_USR_CTL));
	dev_dbg(&ethqos->pdev->dev, "RGMII_IO_MACRO_CONFIG2: %x\n",
		rgmii_readl(ethqos, RGMII_IO_MACRO_CONFIG2));
	dev_dbg(&ethqos->pdev->dev, "RGMII_IO_MACRO_DEBUG1: %x\n",
		rgmii_readl(ethqos, RGMII_IO_MACRO_DEBUG1));
	dev_dbg(&ethqos->pdev->dev, "EMAC_SYSTEM_LOW_POWER_DEBUG: %x\n",
		rgmii_readl(ethqos, EMAC_SYSTEM_LOW_POWER_DEBUG));
}

/* Clock rates */
#define RGMII_1000_NOM_CLK_FREQ			(250 * 1000 * 1000UL)
#define RGMII_ID_MODE_100_LOW_SVS_CLK_FREQ	 (50 * 1000 * 1000UL)
#define RGMII_ID_MODE_10_LOW_SVS_CLK_FREQ	  (5 * 1000 * 1000UL)
#define GMII_EEE_CLK_FREQ			(100000 * 1000UL)

static void
ethqos_update_rgmii_clk(struct qcom_ethqos *ethqos, unsigned int speed, int interface)
{
	if (interface == PHY_INTERFACE_MODE_RGMII ||
	    interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    interface == PHY_INTERFACE_MODE_RGMII_RXID ||
	    interface == PHY_INTERFACE_MODE_RGMII_TXID) {
		switch (speed) {
		case SPEED_1000:
			ethqos->rgmii_clk_rate =  RGMII_1000_NOM_CLK_FREQ;
			break;

		case SPEED_100:
			ethqos->rgmii_clk_rate =  RGMII_ID_MODE_100_LOW_SVS_CLK_FREQ;
			break;

		case SPEED_10:
			ethqos->rgmii_clk_rate =  RGMII_ID_MODE_10_LOW_SVS_CLK_FREQ;
			break;

		case 0:
			ethqos->rgmii_clk_rate = 0;
			break;

		default:
			dev_err(&ethqos->pdev->dev, "Invalid speed %d\n", speed);
			return;
		}
		clk_set_rate(ethqos->rgmii_clk, ethqos->rgmii_clk_rate);
	}
}

static void
ethqos_update_bus_cfg(struct qcom_ethqos *ethqos, unsigned int speed)
{
	int ret = 0;

	switch (speed) {
	case SPEED_10000:
		ethqos->vote_idx = VOTE_IDX_10000MBPS;
		break;

	case SPEED_5000:
		ethqos->vote_idx = VOTE_IDX_5000MBPS;
		break;

	case SPEED_2500:
		ethqos->vote_idx = VOTE_IDX_2500MBPS;
		break;

	case SPEED_1000:
		ethqos->vote_idx = VOTE_IDX_1000MBPS;
		break;

	case SPEED_100:
		ethqos->vote_idx = VOTE_IDX_100MBPS;
		break;

	case SPEED_10:
		ethqos->vote_idx = VOTE_IDX_10MBPS;
		break;

	case 0:
		ethqos->vote_idx = VOTE_IDX_0MBPS;
		break;

	default:
		dev_err(&ethqos->pdev->dev, "Invalid speed %d\n", speed);
		return;
	}

	if (ethqos->axi_icc_path && ethqos->emac_axi_icc) {
		ret = icc_set_bw(ethqos->axi_icc_path,
				 ethqos->emac_axi_icc[ethqos->vote_idx].average_bandwidth,
				 ethqos->emac_axi_icc[ethqos->vote_idx].peak_bandwidth);
		if (ret)
			ETHQOSERR("Interconnect set BW failed for Emac->Axi path\n");
	}

	if (ethqos->apb_icc_path && ethqos->emac_apb_icc) {
		ret = icc_set_bw(ethqos->apb_icc_path,
				 ethqos->emac_apb_icc[ethqos->vote_idx].average_bandwidth,
				 ethqos->emac_apb_icc[ethqos->vote_idx].peak_bandwidth);
		if (ret)
			ETHQOSERR("Interconnect set BW failed for Emac->Apb path\n");
	}
}

static void
ethqos_update_clk_and_bus_cfg(struct qcom_ethqos *ethqos,
			      unsigned int speed, int interface)
{
	ethqos_update_rgmii_clk(ethqos, speed, interface);

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	if (ethqos->clk_eee)
		clk_set_rate(ethqos->clk_eee, GMII_EEE_CLK_FREQ);
#endif

	ethqos_update_bus_cfg(ethqos, speed);
}

static void ethqos_read_iomacro_por_values(struct qcom_ethqos *ethqos)
{
	int i;
	ethqos->por = emac_por_data.por;
	ethqos->num_por = emac_por_data.num_por;

	/* Read to POR values and enable clk */
	for (i = 0; i < ethqos->num_por; i++)
		ethqos->por[i].value =
			rgmii_readl(ethqos, ethqos->por[i].offset);
}

static void ethqos_read_io_macro_from_dtsi(struct device_node *np_hw,
					   struct qcom_ethqos *ethqos)
{
	int ret = 0;
	int i = 0;
	/*The order of storing the data should be as follows from dtsi
	 * ethqos->por[i++].value
	 * RGMII_IO_MACRO_CONFIG -> index 0,
	 * SDCC_HC_REG_DLL_CONFIG, -> index 1
	 * SDCC_TEST_CTL, -> index 2
	 * SDCC_HC_REG_DDR_CONFIG, -> index 3
	 * SDCC_HC_REG_DLL_CONFIG2 -> index 4
	 * SDCC_USR_CTL,-> index 5
	 * RGMII_IO_MACRO_CONFIG2, -> index 6
	 */
	ret = of_property_read_u32(np_hw,
				   "rgmii-io-macro-config",
				   &ethqos->por[i++].value);
	if (ret)
		ETHQOSDBG("default por rgmii_io_macro_config\n");

	ret = of_property_read_u32(np_hw,
				   "sdcc-hc-reg-dll-config",
				   &ethqos->por[i++].value);
	if (ret)
		ETHQOSDBG("default por sdcc_hc_reg_dll_config\n");

	ret = of_property_read_u32(np_hw,
				   "sdcc-test-ctl",
				   &ethqos->por[i++].value);
	if (ret)
		ETHQOSDBG("default por sdcc_test_ctl\n");

	ret = of_property_read_u32(np_hw,
				   "sdcc-hc-reg-ddr-config",
				   &ethqos->por[i++].value);
	if (ret)
		ETHQOSDBG("default por sdcc_hc_reg_ddr_config\n");

	ret = of_property_read_u32(np_hw,
				   "sdcc-hc-reg-dll-config2",
				   &ethqos->por[i++].value);
	if (ret)
		ETHQOSDBG("default por sdcc_hc_reg_dll_config2\n");

	ret = of_property_read_u32(np_hw,
				   "sdcc-usr-ctl",
				   &ethqos->por[i++].value);
	if (ret)
		ETHQOSDBG("default por sdcc_usr_ctl\n");

	ret = of_property_read_u32(np_hw,
				   "rgmii-io-macro-config2",
				   &ethqos->por[i++].value);
	if (ret)
		ETHQOSDBG("default por rgmii-io-macro-config2\n");
}

static void ethqos_iomacro_rgmii_init_v4(struct qcom_ethqos *ethqos)
{
	rgmii_updatel(ethqos, RGMII_BYPASS_EN, 0, RGMII_IO_MACRO_BYPASS);

	switch (ethqos->speed) {
	case SPEED_1000:
		rgmii_updatel(ethqos, RGMII_CONFIG2_MODE_EN_VIA_GMII,
			      RGMII_CONFIG2_MODE_EN_VIA_GMII, RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_RGMII_SGMII_CLK_MUX_SEL,
			      SGMII_PHY_CNTRL1_RGMII_SGMII_CLK_MUX_SEL,
			      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
		rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_USXGMII_GMII_MASTER_CLK_MUX_SEL,
			      SGMII_PHY_CNTRL1_USXGMII_GMII_MASTER_CLK_MUX_SEL,
			      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
		rgmii_updatel(ethqos, USXGMII_CLK_BLK_GMII_CLK_BLK_SEL,
			      0, EMAC_WRAPPER_USXGMII_MUX_SEL);
		rgmii_updatel(ethqos, USXGMII_CLK_BLK_CLK_EN,
			      0, EMAC_WRAPPER_USXGMII_MUX_SEL);
		rgmii_updatel(ethqos, SGMII_PHY_CNTRL0_2P5G_1G_CLK_SEL,
			      0, EMAC_WRAPPER_SGMII_PHY_CNTRL0);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RGMII_CLK_SEL_CFG,
			      0, RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG2_MAX_SPD_PRG_3,
			      BIT(17), RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_2,
			      BIT(8), RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_9,
			      (BIT(10) | BIT(11) | BIT(14)), RGMII_IO_MACRO_CONFIG);
		break;
	case SPEED_100:
		rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_RGMII_SGMII_CLK_MUX_SEL,
			      SGMII_PHY_CNTRL1_RGMII_SGMII_CLK_MUX_SEL,
			      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
		rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_USXGMII_GMII_MASTER_CLK_MUX_SEL,
			      SGMII_PHY_CNTRL1_USXGMII_GMII_MASTER_CLK_MUX_SEL,
			      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
		rgmii_updatel(ethqos, USXGMII_CLK_BLK_GMII_CLK_BLK_SEL,
			      0, EMAC_WRAPPER_USXGMII_MUX_SEL);
		rgmii_updatel(ethqos, USXGMII_CLK_BLK_CLK_EN,
			      0, EMAC_WRAPPER_USXGMII_MUX_SEL);
		rgmii_updatel(ethqos, SGMII_PHY_CNTRL0_2P5G_1G_CLK_SEL,
			      0, EMAC_WRAPPER_SGMII_PHY_CNTRL0);
		rgmii_updatel(ethqos, RGMII_CONFIG2_MAX_SPD_PRG_3,
			      0, RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_2,
			      (BIT(6) | BIT(7)), RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_DDR_MODE,
			      RGMII_CONFIG_DDR_MODE, RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_BYPASS_TX_ID_EN,
			      RGMII_CONFIG_BYPASS_TX_ID_EN,
			      RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_POS_NEG_DATA_SEL,
			      0, RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_PROG_SWAP,
			      0, RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG2_DATA_DIVIDE_CLK_SEL,
			      0, RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
			      RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
			      RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RSVD_CONFIG15,
			      0, RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RX_PROG_SWAP,
			      RGMII_CONFIG2_RX_PROG_SWAP,
			      RGMII_IO_MACRO_CONFIG2);

		/* Write 0x5 to PRG_RCLK_DLY_CODE */
		rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_CODE,
			      (BIT(29) | BIT(27)), SDCC_HC_REG_DDR_CONFIG);
		rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY,
			      SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY,
			      SDCC_HC_REG_DDR_CONFIG);
		rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_EN,
			      SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_EN,
			      SDCC_HC_REG_DDR_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
			      0, RGMII_IO_MACRO_CONFIG);
		break;
	case SPEED_10:
		rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_RGMII_SGMII_CLK_MUX_SEL,
			      SGMII_PHY_CNTRL1_RGMII_SGMII_CLK_MUX_SEL,
			      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
		rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_USXGMII_GMII_MASTER_CLK_MUX_SEL,
			      SGMII_PHY_CNTRL1_USXGMII_GMII_MASTER_CLK_MUX_SEL,
			      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
		rgmii_updatel(ethqos, USXGMII_CLK_BLK_GMII_CLK_BLK_SEL,
			      0, EMAC_WRAPPER_USXGMII_MUX_SEL);
		rgmii_updatel(ethqos, USXGMII_CLK_BLK_CLK_EN,
			      0, EMAC_WRAPPER_USXGMII_MUX_SEL);
		rgmii_updatel(ethqos, SGMII_PHY_CNTRL0_2P5G_1G_CLK_SEL,
			      0, EMAC_WRAPPER_SGMII_PHY_CNTRL0);
		rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_9,
			      BIT(10), RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_SCRATCH2_MAX_SPD_PRG_4,
			      0, RGMII_IO_MACRO_SCRATCH_2);
		rgmii_updatel(ethqos, RGMII_SCRATCH2_MAX_SPD_PRG_5,
			      0, RGMII_IO_MACRO_SCRATCH_2);
		rgmii_updatel(ethqos, RGMII_SCRATCH2_MAX_SPD_PRG_6,
			      0, RGMII_IO_MACRO_SCRATCH_2);
		rgmii_updatel(ethqos, RGMII_CONFIG_DDR_MODE,
			      RGMII_CONFIG_DDR_MODE, RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_BYPASS_TX_ID_EN,
			      RGMII_CONFIG_BYPASS_TX_ID_EN,
			      RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_POS_NEG_DATA_SEL,
			      0, RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_PROG_SWAP,
			      0, RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG2_DATA_DIVIDE_CLK_SEL,
			      0, RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos,
			      RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
			      RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
			      RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RSVD_CONFIG15,
			      0, RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RX_PROG_SWAP,
			      RGMII_CONFIG2_RX_PROG_SWAP,
			      RGMII_IO_MACRO_CONFIG2);

		/* Write 0x5 to PRG_RCLK_DLY_CODE */
		rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_CODE,
			      (BIT(29) | BIT(27)), SDCC_HC_REG_DDR_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
			      0, RGMII_IO_MACRO_CONFIG);
		break;
	}

	/* DLL bypass mode for 10Mbps and 100Mbps
	 * 1.   Write 1 to PDN bit of SDCC_HC_REG_DLL_CONFIG register.
	 * 2.   Write 1 to bypass bit of SDCC_USR_CTL register
	 * 3.   Default value of this register is 0x00010800
	 */
	if (ethqos->speed == SPEED_10 || ethqos->speed == SPEED_100) {
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_PDN,
			      SDCC_DLL_CONFIG_PDN, SDCC_HC_REG_DLL_CONFIG);
		rgmii_updatel(ethqos, DLL_BYPASS,
			      DLL_BYPASS, SDCC_USR_CTL);
	}
}

static void ethqos_set_func_clk_en(struct qcom_ethqos *ethqos)
{
	rgmii_updatel(ethqos, RGMII_CONFIG_FUNC_CLK_EN,
		      RGMII_CONFIG_FUNC_CLK_EN, RGMII_IO_MACRO_CONFIG);
}
static int ethqos_dll_configure(struct qcom_ethqos *ethqos)
{
	unsigned int val;
	int retry = 1000;

	/* Set CDR_EN */
	if (ethqos->emac_ver == EMAC_HW_v2_3_2_RG ||
	    ethqos->emac_ver == EMAC_HW_v2_1_2)
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_CDR_EN,
			      0, SDCC_HC_REG_DLL_CONFIG);
	else
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_CDR_EN,
			      SDCC_DLL_CONFIG_CDR_EN, SDCC_HC_REG_DLL_CONFIG);
	/* Set CDR_EXT_EN */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_CDR_EXT_EN,
		      SDCC_DLL_CONFIG_CDR_EXT_EN, SDCC_HC_REG_DLL_CONFIG);

	/* Clear CK_OUT_EN */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_CK_OUT_EN,
		      0, SDCC_HC_REG_DLL_CONFIG);

	/* Set DLL_EN */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_EN,
		      SDCC_DLL_CONFIG_DLL_EN, SDCC_HC_REG_DLL_CONFIG);

	if (ethqos->emac_ver != EMAC_HW_v2_3_2_RG &&
	    ethqos->emac_ver != EMAC_HW_v2_1_2 &&
	    ethqos->emac_ver != EMAC_HW_v2_3_0) {
		rgmii_updatel(ethqos, SDCC_DLL_MCLK_GATING_EN,
			      0, SDCC_HC_REG_DLL_CONFIG);

		rgmii_updatel(ethqos, SDCC_DLL_CDR_FINE_PHASE,
			      0, SDCC_HC_REG_DLL_CONFIG);
	}
	/* Wait for CK_OUT_EN clear */
	do {
		val = rgmii_readl(ethqos, SDCC_HC_REG_DLL_CONFIG);
		val &= SDCC_DLL_CONFIG_CK_OUT_EN;
		if (!val)
			break;
		mdelay(1);
		retry--;
	} while (retry > 0);
	if (!retry)
		dev_err(&ethqos->pdev->dev, "Clear CK_OUT_EN timedout\n");

	/* Set CK_OUT_EN */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_CK_OUT_EN,
		      SDCC_DLL_CONFIG_CK_OUT_EN, SDCC_HC_REG_DLL_CONFIG);

	/* Wait for CK_OUT_EN set */
	retry = 1000;
	do {
		val = rgmii_readl(ethqos, SDCC_HC_REG_DLL_CONFIG);
		val &= SDCC_DLL_CONFIG_CK_OUT_EN;
		if (val)
			break;
		mdelay(1);
		retry--;
	} while (retry > 0);
	if (!retry)
		dev_err(&ethqos->pdev->dev, "Set CK_OUT_EN timedout\n");

	/* Set DDR_CAL_EN */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG2_DDR_CAL_EN,
		      SDCC_DLL_CONFIG2_DDR_CAL_EN, SDCC_HC_REG_DLL_CONFIG2);

	if (ethqos->emac_ver != EMAC_HW_v2_3_2_RG &&
	    ethqos->emac_ver != EMAC_HW_v2_1_2 &&
	    ethqos->emac_ver != EMAC_HW_v2_3_0) {
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG2_DLL_CLOCK_DIS,
			      0, SDCC_HC_REG_DLL_CONFIG2);

		rgmii_updatel(ethqos, SDCC_DLL_CONFIG2_MCLK_FREQ_CALC,
			      0x1A << 10, SDCC_HC_REG_DLL_CONFIG2);

		rgmii_updatel(ethqos, SDCC_DLL_CONFIG2_DDR_TRAFFIC_INIT_SEL,
			      BIT(2), SDCC_HC_REG_DLL_CONFIG2);

		rgmii_updatel(ethqos, SDCC_DLL_CONFIG2_DDR_TRAFFIC_INIT_SW,
			      SDCC_DLL_CONFIG2_DDR_TRAFFIC_INIT_SW,
			      SDCC_HC_REG_DLL_CONFIG2);
	}

	return 0;
}

void emac_rgmii_io_macro_config_1G(struct qcom_ethqos *ethqos)
{
	rgmii_updatel(ethqos, RGMII_CONFIG_DDR_MODE,
		      RGMII_CONFIG_DDR_MODE, RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG_BYPASS_TX_ID_EN,
		      0, RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG_POS_NEG_DATA_SEL,
		      RGMII_CONFIG_POS_NEG_DATA_SEL,
		      RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG_PROG_SWAP,
		      RGMII_CONFIG_PROG_SWAP, RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG2_DATA_DIVIDE_CLK_SEL,
		      0, RGMII_IO_MACRO_CONFIG2);
	rgmii_updatel(ethqos, RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
		      RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
		      RGMII_IO_MACRO_CONFIG2);
	rgmii_updatel(ethqos, RGMII_CONFIG2_RSVD_CONFIG15,
		      0, RGMII_IO_MACRO_CONFIG2);
	rgmii_updatel(ethqos, RGMII_CONFIG2_RX_PROG_SWAP,
		      RGMII_CONFIG2_RX_PROG_SWAP,
		      RGMII_IO_MACRO_CONFIG2);

	/* Set PRG_RCLK_DLY to 115 */
	rgmii_updatel(ethqos, SDCC_DDR_CONFIG_PRG_RCLK_DLY,
		      115, SDCC_HC_REG_DDR_CONFIG);
	rgmii_updatel(ethqos, SDCC_DDR_CONFIG_PRG_DLY_EN,
		      SDCC_DDR_CONFIG_PRG_DLY_EN,
		      SDCC_HC_REG_DDR_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
		      0, RGMII_IO_MACRO_CONFIG);
}

void emac_rgmii_io_macro_config_100M(struct qcom_ethqos *ethqos)
{
	rgmii_updatel(ethqos, RGMII_CONFIG_DDR_MODE,
		      RGMII_CONFIG_DDR_MODE, RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG_BYPASS_TX_ID_EN,
		      RGMII_CONFIG_BYPASS_TX_ID_EN,
		      RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG_POS_NEG_DATA_SEL,
		      0, RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG_PROG_SWAP,
		      0, RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG2_DATA_DIVIDE_CLK_SEL,
		      0, RGMII_IO_MACRO_CONFIG2);
	rgmii_updatel(ethqos, RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
		      RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
		      RGMII_IO_MACRO_CONFIG2);
	rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_2,
		      BIT(6), RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG2_RSVD_CONFIG15,
		      0, RGMII_IO_MACRO_CONFIG2);
	rgmii_updatel(ethqos, RGMII_CONFIG2_RX_PROG_SWAP,
		      RGMII_CONFIG2_RX_PROG_SWAP,
		      RGMII_IO_MACRO_CONFIG2);

	/* Write 0x5 to PRG_RCLK_DLY_CODE */
	rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_CODE,
		      (BIT(29) | BIT(27)), SDCC_HC_REG_DDR_CONFIG);
	rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY,
		      SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY,
		      SDCC_HC_REG_DDR_CONFIG);
	rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_EN,
		      SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_EN,
		      SDCC_HC_REG_DDR_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
		      0, RGMII_IO_MACRO_CONFIG);
}

void emac_rgmii_io_macro_config_10M(struct qcom_ethqos *ethqos)
{
	rgmii_updatel(ethqos, RGMII_CONFIG_DDR_MODE,
		      RGMII_CONFIG_DDR_MODE, RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG_BYPASS_TX_ID_EN,
		      RGMII_CONFIG_BYPASS_TX_ID_EN,
		      RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG_POS_NEG_DATA_SEL,
		      0, RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG_PROG_SWAP,
		      0, RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG2_DATA_DIVIDE_CLK_SEL,
		      0, RGMII_IO_MACRO_CONFIG2);
	rgmii_updatel(ethqos,
		      RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
		      RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
		      RGMII_IO_MACRO_CONFIG2);
	rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_9,
		      BIT(12) | GENMASK(9, 8),
		      RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG2_RSVD_CONFIG15,
		      0, RGMII_IO_MACRO_CONFIG2);
	rgmii_updatel(ethqos, RGMII_CONFIG2_RX_PROG_SWAP,
		      RGMII_CONFIG2_RX_PROG_SWAP,
		      RGMII_IO_MACRO_CONFIG2);

	/* Write 0x5 to PRG_RCLK_DLY_CODE */
	rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_CODE,
		      (BIT(29) | BIT(27)), SDCC_HC_REG_DDR_CONFIG);
}

static int ethqos_rgmii_macro_init(struct qcom_ethqos *ethqos)
{
	/* Disable loopback mode */
	rgmii_updatel(ethqos, RGMII_CONFIG2_TX_TO_RX_LOOPBACK_EN,
		      0, RGMII_IO_MACRO_CONFIG2);

	/* Select RGMII, write 0 to interface select */
	rgmii_updatel(ethqos, RGMII_CONFIG_INTF_SEL,
		      0, RGMII_IO_MACRO_CONFIG);

	switch (ethqos->speed) {
	case SPEED_1000:
		rgmii_updatel(ethqos, RGMII_CONFIG_DDR_MODE,
			      RGMII_CONFIG_DDR_MODE, RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_BYPASS_TX_ID_EN,
			      0, RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_POS_NEG_DATA_SEL,
			      RGMII_CONFIG_POS_NEG_DATA_SEL,
			      RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_PROG_SWAP,
			      RGMII_CONFIG_PROG_SWAP, RGMII_IO_MACRO_CONFIG);
		if (ethqos->emac_ver != EMAC_HW_v2_1_2)
			rgmii_updatel(ethqos, RGMII_CONFIG2_DATA_DIVIDE_CLK_SEL,
				      0, RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
			      RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
			      RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RSVD_CONFIG15,
			      0, RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RX_PROG_SWAP,
			      RGMII_CONFIG2_RX_PROG_SWAP,
			      RGMII_IO_MACRO_CONFIG2);

		/* Set PRG_RCLK_DLY to 57 for 1.8 ns delay */
		if (ethqos->emac_ver == EMAC_HW_v2_3_2_RG)
			rgmii_updatel(ethqos, SDCC_DDR_CONFIG_PRG_RCLK_DLY,
				      69, SDCC_HC_REG_DDR_CONFIG);
		else if (ethqos->emac_ver == EMAC_HW_v2_1_2)
			rgmii_updatel(ethqos, SDCC_DDR_CONFIG_PRG_RCLK_DLY,
				      52, SDCC_HC_REG_DDR_CONFIG);
		else if (ethqos->emac_ver == EMAC_HW_v2_1_1)
			rgmii_updatel(ethqos, SDCC_DDR_CONFIG_PRG_RCLK_DLY,
				      130, SDCC_HC_REG_DDR_CONFIG);
		else if (ethqos->emac_ver == EMAC_HW_v2_3_1 || ethqos->emac_ver == EMAC_HW_v4_0_0)
			rgmii_updatel(ethqos, SDCC_DDR_CONFIG_PRG_RCLK_DLY,
				      104, SDCC_HC_REG_DDR_CONFIG);
		else
			rgmii_updatel(ethqos, SDCC_DDR_CONFIG_PRG_RCLK_DLY,
				      57, SDCC_HC_REG_DDR_CONFIG);
		rgmii_updatel(ethqos, SDCC_DDR_CONFIG_PRG_DLY_EN,
			      SDCC_DDR_CONFIG_PRG_DLY_EN,
			      SDCC_HC_REG_DDR_CONFIG);
		if (ethqos->emac_ver == EMAC_HW_v2_3_2_RG ||
		    ethqos->emac_ver == EMAC_HW_v2_1_2 || ethqos->emac_ver == EMAC_HW_v4_0_0)
			rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
				      0, RGMII_IO_MACRO_CONFIG);
		else
			rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
				      RGMII_CONFIG_LOOPBACK_EN,
				      RGMII_IO_MACRO_CONFIG);
		break;

	case SPEED_100:
		rgmii_updatel(ethqos, RGMII_CONFIG_DDR_MODE,
			      RGMII_CONFIG_DDR_MODE, RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_BYPASS_TX_ID_EN,
			      RGMII_CONFIG_BYPASS_TX_ID_EN,
			      RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_POS_NEG_DATA_SEL,
			      0, RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_PROG_SWAP,
			      0, RGMII_IO_MACRO_CONFIG);
		if (ethqos->emac_ver != EMAC_HW_v2_1_2 && ethqos->emac_ver != EMAC_HW_v4_0_0)
			rgmii_updatel(ethqos, RGMII_CONFIG2_DATA_DIVIDE_CLK_SEL,
				      0, RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
			      RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
			      RGMII_IO_MACRO_CONFIG2);
		if (ethqos->emac_ver == EMAC_HW_v4_0_0)
			rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_2,
				      BIT(6) | BIT(7), RGMII_IO_MACRO_CONFIG);
		else
			rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_2,
				      BIT(6), RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RSVD_CONFIG15,
			      0, RGMII_IO_MACRO_CONFIG2);
		if (ethqos->emac_ver == EMAC_HW_v2_3_2_RG ||
		    ethqos->emac_ver == EMAC_HW_v2_1_2 ||
			ethqos->emac_ver == EMAC_HW_v2_1_1 ||
			ethqos->emac_ver == EMAC_HW_v2_3_1 || ethqos->emac_ver == EMAC_HW_v4_0_0)
			rgmii_updatel(ethqos, RGMII_CONFIG2_RX_PROG_SWAP,
				      RGMII_CONFIG2_RX_PROG_SWAP,
				      RGMII_IO_MACRO_CONFIG2);
		else
			rgmii_updatel(ethqos, RGMII_CONFIG2_RX_PROG_SWAP,
				      0, RGMII_IO_MACRO_CONFIG2);
		/* Write 0x5 to PRG_RCLK_DLY_CODE */
		rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_CODE,
			      (BIT(29) | BIT(27)), SDCC_HC_REG_DDR_CONFIG);
		rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY,
			      SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY,
			      SDCC_HC_REG_DDR_CONFIG);
		rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_EN,
			      SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_EN,
			      SDCC_HC_REG_DDR_CONFIG);
		if (ethqos->emac_ver == EMAC_HW_v2_3_2_RG ||
		    ethqos->emac_ver == EMAC_HW_v2_1_2 || ethqos->emac_ver == EMAC_HW_v4_0_0)
			rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
				      0, RGMII_IO_MACRO_CONFIG);
		else
			rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
				      RGMII_CONFIG_LOOPBACK_EN,
			      RGMII_IO_MACRO_CONFIG);
		break;

	case SPEED_10:
		rgmii_updatel(ethqos, RGMII_CONFIG_DDR_MODE,
			      RGMII_CONFIG_DDR_MODE, RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_BYPASS_TX_ID_EN,
			      RGMII_CONFIG_BYPASS_TX_ID_EN,
			      RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_POS_NEG_DATA_SEL,
			      0, RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_PROG_SWAP,
			      0, RGMII_IO_MACRO_CONFIG);
		if (ethqos->emac_ver != EMAC_HW_v2_1_2)
			rgmii_updatel(ethqos, RGMII_CONFIG2_DATA_DIVIDE_CLK_SEL,
				      0, RGMII_IO_MACRO_CONFIG2);
		if (ethqos->emac_ver == EMAC_HW_v2_3_2_RG ||
		    ethqos->emac_ver == EMAC_HW_v2_1_2 ||
			ethqos->emac_ver == EMAC_HW_v2_1_1)
			rgmii_updatel(ethqos,
				      RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
				      RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
				      RGMII_IO_MACRO_CONFIG2);
		else
			rgmii_updatel(ethqos,
				      RGMII_CONFIG2_TX_CLK_PHASE_SHIFT_EN,
				      0, RGMII_IO_MACRO_CONFIG2);
		if (ethqos->emac_ver != EMAC_HW_v4_0_0)
			rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_9,
				      BIT(12) | GENMASK(9, 8),
				      RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RSVD_CONFIG15,
			      0, RGMII_IO_MACRO_CONFIG2);
		if (ethqos->emac_ver == EMAC_HW_v2_3_2_RG ||
		    ethqos->emac_ver == EMAC_HW_v2_1_2 ||
			ethqos->emac_ver == EMAC_HW_v2_1_1)
			rgmii_updatel(ethqos, RGMII_CONFIG2_RX_PROG_SWAP,
				      RGMII_CONFIG2_RX_PROG_SWAP,
				      RGMII_IO_MACRO_CONFIG2);
		else
			rgmii_updatel(ethqos, RGMII_CONFIG2_RX_PROG_SWAP,
				      0, RGMII_IO_MACRO_CONFIG2);
		/* Write 0x5 to PRG_RCLK_DLY_CODE */
		rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_CODE,
			      (BIT(29) | BIT(27)), SDCC_HC_REG_DDR_CONFIG);
		rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY,
			      SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY,
			      SDCC_HC_REG_DDR_CONFIG);
		rgmii_updatel(ethqos, SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_EN,
			      SDCC_DDR_CONFIG_EXT_PRG_RCLK_DLY_EN,
			      SDCC_HC_REG_DDR_CONFIG);
		if (ethqos->emac_ver == EMAC_HW_v2_3_2_RG ||
		    ethqos->emac_ver == EMAC_HW_v2_1_2)
			rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
				      0, RGMII_IO_MACRO_CONFIG);
		else
			rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
				      RGMII_CONFIG_LOOPBACK_EN,
				      RGMII_IO_MACRO_CONFIG);
		break;
	default:
		dev_err(&ethqos->pdev->dev,
			"Invalid speed %d\n", ethqos->speed);
		return -EINVAL;
	}

	return 0;
}

static int ethqos_rgmii_macro_init_v3(struct qcom_ethqos *ethqos)
{
	/* Disable loopback mode */
	rgmii_updatel(ethqos, RGMII_CONFIG2_TX_TO_RX_LOOPBACK_EN,
		      0, RGMII_IO_MACRO_CONFIG2);

	/* Select RGMII, write 0 to interface select */
	rgmii_updatel(ethqos, RGMII_CONFIG_INTF_SEL,
		      0, RGMII_IO_MACRO_CONFIG);

	switch (ethqos->speed) {
	case SPEED_1000:
		emac_rgmii_io_macro_config_1G(ethqos);
		break;

	case SPEED_100:
		emac_rgmii_io_macro_config_100M(ethqos);
		break;

	case SPEED_10:
		emac_rgmii_io_macro_config_10M(ethqos);
		break;
	default:
		dev_err(&ethqos->pdev->dev,
			"Invalid speed %d\n", ethqos->speed);
		return -EINVAL;
	}

	return 0;
}

int ethqos_configure_sgmii_v3_1(struct qcom_ethqos *ethqos)
{
	u32 value = 0;
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	ethqos_set_func_clk_en(ethqos);

	value = readl(ethqos->ioaddr + MAC_CTRL_REG);
	switch (ethqos->speed) {
	case SPEED_2500:
		value &= ~GMAC_CONFIG_PS;
		writel(value, ethqos->ioaddr + MAC_CTRL_REG);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RGMII_CLK_SEL_CFG,
			      RGMII_CONFIG2_RGMII_CLK_SEL_CFG, RGMII_IO_MACRO_CONFIG2);
		value = readl(priv->ioaddr + DWMAC4_PCS_BASE);
		value &= ~GMAC_AN_CTRL_ANE;
		writel(value, priv->ioaddr + DWMAC4_PCS_BASE);
	break;
	case SPEED_1000:
		value &= ~GMAC_CONFIG_PS;
		writel(value, ethqos->ioaddr + MAC_CTRL_REG);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RGMII_CLK_SEL_CFG,
			      RGMII_CONFIG2_RGMII_CLK_SEL_CFG, RGMII_IO_MACRO_CONFIG2);
		value = readl(priv->ioaddr + DWMAC4_PCS_BASE);
		value |= GMAC_AN_CTRL_RAN | GMAC_AN_CTRL_ANE;
		writel(value, priv->ioaddr + DWMAC4_PCS_BASE);
	break;

	case SPEED_100:
		value |= GMAC_CONFIG_PS | GMAC_CONFIG_FES;
		writel(value, ethqos->ioaddr + MAC_CTRL_REG);
		value = readl(priv->ioaddr + DWMAC4_PCS_BASE);
		value |= GMAC_AN_CTRL_RAN | GMAC_AN_CTRL_ANE;
		writel(value, priv->ioaddr + DWMAC4_PCS_BASE);
	break;
	case SPEED_10:
		value |= GMAC_CONFIG_PS;
		value &= ~GMAC_CONFIG_FES;
		writel(value, ethqos->ioaddr + MAC_CTRL_REG);
		rgmii_updatel(ethqos, RGMII_CONFIG_10M_CLK_DVD, BIT(10) |
			      GENMASK(15, 14), RGMII_IO_MACRO_CONFIG);
		value = readl(priv->ioaddr + DWMAC4_PCS_BASE);
		value |= GMAC_AN_CTRL_RAN | GMAC_AN_CTRL_ANE;
		writel(value, priv->ioaddr + DWMAC4_PCS_BASE);
	break;

	default:
		dev_err(&ethqos->pdev->dev,
			"Invalid speed %d\n", ethqos->speed);
		return -EINVAL;
	}

	return 0;
}

static int ethqos_configure_mac_v3_1(struct qcom_ethqos *ethqos)
{
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);
	int ret = 0;

	switch (priv->plat->interface) {
	case PHY_INTERFACE_MODE_SGMII:
		ret = ethqos_configure_sgmii_v3_1(ethqos);
		qcom_ethqos_serdes_update(ethqos, ethqos->speed, priv->plat->interface);
		break;
	}
	return ret;
}

static int ethqos_configure(struct qcom_ethqos *ethqos)
{
	volatile unsigned int dll_lock;
	unsigned int i, retry = 1000;
	int ret;

	if (ethqos->emac_ver == EMAC_HW_v3_1_0)
		return ethqos_configure_mac_v3_1(ethqos);

	/* Reset to POR values and enable clk */
	for (i = 0; i < ethqos->num_por; i++)
		rgmii_writel(ethqos, ethqos->por[i].value,
			     ethqos->por[i].offset);
	ethqos_set_func_clk_en(ethqos);

	/* Initialize the DLL first */

	/* Set DLL_RST */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_RST,
		      SDCC_DLL_CONFIG_DLL_RST, SDCC_HC_REG_DLL_CONFIG);

	/* Set PDN */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_PDN,
		      SDCC_DLL_CONFIG_PDN, SDCC_HC_REG_DLL_CONFIG);

	/* Clear DLL_RST */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_RST, 0,
		      SDCC_HC_REG_DLL_CONFIG);

	/* Clear PDN */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_PDN, 0,
		      SDCC_HC_REG_DLL_CONFIG);

	if (ethqos->speed != SPEED_100 && ethqos->speed != SPEED_10) {
		/* Set DLL_EN */
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_EN,
			      SDCC_DLL_CONFIG_DLL_EN, SDCC_HC_REG_DLL_CONFIG);

		/* Set CK_OUT_EN */
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_CK_OUT_EN,
			      SDCC_DLL_CONFIG_CK_OUT_EN,
			      SDCC_HC_REG_DLL_CONFIG);

		/* Set USR_CTL bit 26 with mask of 3 bits */
		rgmii_updatel(ethqos, GENMASK(26, 24), BIT(26), SDCC_USR_CTL);

		/* wait for DLL LOCK */
		do {
			mdelay(1);
			dll_lock = rgmii_readl(ethqos, SDC4_STATUS);
			if (dll_lock & SDC4_STATUS_DLL_LOCK)
				break;
			retry--;
		} while (retry > 0);
		if (!retry)
			dev_err(&ethqos->pdev->dev,
				"Timeout while waiting for DLL lock\n");
	}

	if (ethqos->speed == SPEED_1000)
		ethqos_dll_configure(ethqos);

	ret = ethqos_rgmii_macro_init(ethqos);
	if (ret == 0)
		dev_info(&ethqos->pdev->dev, "Successfully configured RGMII MACRO\n");

	return 0;
}

/* for EMAC_HW_VER >= 3 */
static int ethqos_configure_mac_v3(struct qcom_ethqos *ethqos)
{
	unsigned int dll_lock;
	unsigned int i, retry = 1000;
	int ret = 0;
	/* Reset to POR values and enable clk */
	for (i = 0; i < ethqos->num_por; i++)
		rgmii_writel(ethqos, ethqos->por[i].value,
			     ethqos->por[i].offset);
	ethqos_set_func_clk_en(ethqos);

	/* Put DLL into Reset and Powerdown */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_RST,
		      SDCC_DLL_CONFIG_DLL_RST, SDCC_HC_REG_DLL_CONFIG);
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_PDN,
		      SDCC_DLL_CONFIG_PDN, SDCC_HC_REG_DLL_CONFIG)
		;
	/*Power on and set DLL, Set->RST & PDN to '0' */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_RST,
		      0, SDCC_HC_REG_DLL_CONFIG);
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_PDN,
		      0, SDCC_HC_REG_DLL_CONFIG);

	/* for 10 or 100Mbps further configuration not required */
	if (ethqos->speed == SPEED_1000) {
		/* Disable DLL output clock */
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_CK_OUT_EN,
			      0, SDCC_HC_REG_DLL_CONFIG);

		/* Configure SDCC_DLL_TEST_CTRL */
		rgmii_writel(ethqos, HSR_SDCC_DLL_TEST_CTRL, SDCC_TEST_CTL);

		/* Configure SDCC_USR_CTRL */
		rgmii_writel(ethqos, HSR_SDCC_USR_CTRL, SDCC_USR_CTL);

		/* Configure DDR_CONFIG */
		rgmii_writel(ethqos, HSR_DDR_CONFIG, SDCC_HC_REG_DDR_CONFIG);

		/* Configure PRG_RCLK_DLY */
		rgmii_updatel(ethqos, SDCC_DDR_CONFIG_PRG_RCLK_DLY,
			      DDR_CONFIG_PRG_RCLK_DLY, SDCC_HC_REG_DDR_CONFIG);
		/*Enable PRG_RCLK_CLY */
		rgmii_updatel(ethqos, SDCC_DDR_CONFIG_PRG_DLY_EN,
			      SDCC_DDR_CONFIG_PRG_DLY_EN, SDCC_HC_REG_DDR_CONFIG);

		/* Configure DLL_CONFIG */
		rgmii_writel(ethqos, HSR_DLL_CONFIG, SDCC_HC_REG_DLL_CONFIG);

		/*Set -> DLL_CONFIG_2 MCLK_FREQ_CALC*/
		rgmii_writel(ethqos, HSR_DLL_CONFIG_2, SDCC_HC_REG_DLL_CONFIG2);

		/*Power Down and Reset DLL*/
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_RST,
			      SDCC_DLL_CONFIG_DLL_RST, SDCC_HC_REG_DLL_CONFIG);
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_PDN,
			      SDCC_DLL_CONFIG_PDN, SDCC_HC_REG_DLL_CONFIG);

		/*wait for 52us*/
		usleep_range(52, 55);

		/*Power on and set DLL, Set->RST & PDN to '0' */
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_RST,
			      0, SDCC_HC_REG_DLL_CONFIG);
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_PDN,
			      0, SDCC_HC_REG_DLL_CONFIG);

		/*Wait for 8000 input clock cycles, 8000 cycles of 100 MHz = 80us*/
		usleep_range(80, 85);

		/* Enable DLL output clock */
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_CK_OUT_EN,
			      SDCC_DLL_CONFIG_CK_OUT_EN, SDCC_HC_REG_DLL_CONFIG);

		/* Check for DLL lock */
		do {
			udelay(1);
			dll_lock = rgmii_readl(ethqos, SDC4_STATUS);
			if (dll_lock & SDC4_STATUS_DLL_LOCK)
				break;
			retry--;
		} while (retry > 0);
		if (!retry)
			dev_err(&ethqos->pdev->dev,
				"Timeout while waiting for DLL lock\n");
	}

	/* DLL bypass mode for 10Mbps and 100Mbps
	 * 1.   Write 1 to PDN bit of SDCC_HC_REG_DLL_CONFIG register.
	 * 2.   Write 1 to bypass bit of SDCC_USR_CTL register
	 * 3.   Default value of this register is 0x00010800
	 */
	if (ethqos->speed == SPEED_10 || ethqos->speed == SPEED_100) {
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_PDN,
			      SDCC_DLL_CONFIG_PDN, SDCC_HC_REG_DLL_CONFIG);
		rgmii_updatel(ethqos, DLL_BYPASS,
			      DLL_BYPASS, SDCC_USR_CTL);
	}

	ret = ethqos_rgmii_macro_init_v3(ethqos);

	return ret;
}

static int ethqos_serdes_power_up(struct net_device *ndev, void *priv)
{
	struct qcom_ethqos *ethqos = priv;
	struct net_device *dev = ndev;
	struct stmmac_priv *s_priv = netdev_priv(dev);

	ETHQOSINFO("%s : speed = %d interface = %d",
		   __func__,
		   ethqos->speed,
		   s_priv->plat->interface);

	return qcom_ethqos_serdes_update(ethqos, ethqos->speed,
					 s_priv->plat->interface);
}

static int ethqos_dll_configure_v4(struct qcom_ethqos *ethqos)
{
	unsigned int dll_lock;
	int retry = 1000;
	/* Set Disable DLL Output CLK */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_CK_OUT_EN,
		      0,
		      SDCC_HC_REG_DLL_CONFIG);

	/* Set Disable DLL Input CLK */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG2_DLL_CLOCK_DIS,
		      SDCC_DLL_CONFIG2_DLL_CLOCK_DIS, SDCC_HC_REG_DLL_CONFIG2);

	/*Put DLL into RESET and Power Down*/
	/* Set DLL_RST */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_RST,
		      SDCC_DLL_CONFIG_DLL_RST, SDCC_HC_REG_DLL_CONFIG);

	/* Set PDN */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_PDN,
		      SDCC_DLL_CONFIG_PDN, SDCC_HC_REG_DLL_CONFIG);

	/* Configure SDCC_USR_CTRL */
	rgmii_updatel(ethqos, GENMASK(31, 0),
		      BIT(6) | BIT(7) | BIT(9) | BIT(10) | BIT(16) | BIT(24) | BIT(27),
		      SDCC_USR_CTL);

	/* Configure DLL_TEST_CTL */
	rgmii_updatel(ethqos, SDC4_DLL_TEST_CTL,
		      BIT(24) | BIT(22), SDCC_TEST_CTL);

	rgmii_updatel(ethqos, SDCC_DLL_CONFIG2_DDR_CAL_EN,
		      SDCC_DLL_CONFIG2_DDR_CAL_EN, SDCC_HC_REG_DLL_CONFIG2);

	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_CDR_EXT_EN,
		      SDCC_DLL_CONFIG_CDR_EXT_EN, SDCC_HC_REG_DLL_CONFIG);

	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_CDR_UPD_RATE,
		      BIT(14), SDCC_HC_REG_DLL_CONFIG);

	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_PHASE_DET,
		      BIT(10), SDCC_HC_REG_DLL_CONFIG);

	rgmii_updatel(ethqos, SDCC_DLL_MCLK_GATING_EN,
		      SDCC_DLL_MCLK_GATING_EN, SDCC_HC_REG_DLL_CONFIG);

	rgmii_updatel(ethqos, SDCC_DLL_CDR_FINE_PHASE,
		      BIT(2) | BIT(3), SDCC_HC_REG_DLL_CONFIG);

	rgmii_updatel(ethqos, SDCC_DLL_CONFIG2_DLL_CLOCK_DIS,
		      0, SDCC_HC_REG_DLL_CONFIG2);

	rgmii_updatel(ethqos, SDCC_DLL_CONFIG2_MCLK_FREQ_CALC,
		      0x28 << 10, SDCC_HC_REG_DLL_CONFIG2);

	rgmii_updatel(ethqos, RGMII_CONFIG2_RSVD_CONFIG15,
		      BIT(24), RGMII_IO_MACRO_CONFIG2);

	/*wait for 52us*/
	usleep_range(52, 55);

	/*Put DLL into RESET*/
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_RST,
		      SDCC_DLL_CONFIG_DLL_RST, SDCC_HC_REG_DLL_CONFIG);

	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_PDN,
		      SDCC_DLL_CONFIG_PDN, SDCC_HC_REG_DLL_CONFIG);

	/*Wait for 8000 input clock cycles, 8000 cycles of 100 MHz = 80us*/
	usleep_range(80, 85);

	/*Take DLL out of RESET*/
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_RST,
		      0, SDCC_HC_REG_DLL_CONFIG);

	/* reset PDN */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_PDN,
		      0, SDCC_HC_REG_DLL_CONFIG);

	/* Set Disable DLL Output CLK */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_CK_OUT_EN,
		      SDCC_DLL_CONFIG_CK_OUT_EN,
		      SDCC_HC_REG_DLL_CONFIG);

	/* wait for DLL LOCK */
	retry = 1000;
	do {
		mdelay(1);
		dll_lock = rgmii_readl(ethqos, SDC4_STATUS);
		if (dll_lock & DDR_STATUS_DLL_LOCK)
			break;
		retry--;
	} while (retry > 0);

	if (!retry)
		dev_err(&ethqos->pdev->dev,
			"Timeout while waiting for DLL lock\n");

	return 0;
}

static int ethqos_configure_rgmii_v4(struct qcom_ethqos *ethqos)
{
	unsigned int i;
	struct device_node *rgmii_io_macro_node = NULL;
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	struct net_device *ndev = dev_get_drvdata(&ethqos->pdev->dev);
	struct stmmac_priv *priv = netdev_priv(ndev);

	if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
		/* Reg_dump should have the whole IO macro reg space so below copy
		 * to buffer has to be done based on offset in the reg_dump unlike invoking
		 * register read for every config as below
		 */
		qcom_scm_call_iomacro_dump(ethqos->rgmii_phy_base,
					   ethqos->shm_rgmii_local.paddr, RGMII_BLOCK_SIZE);
		qtee_shmbridge_inv_shm_buf(&ethqos->shm_rgmii_local);
	}
#endif

	if (ethqos->probed) {
		ethqos_read_iomacro_por_values(ethqos);

		rgmii_io_macro_node = of_find_node_by_name(ethqos->pdev->dev.of_node,
							   "rgmii-io-macro-info-hsr");
		if (rgmii_io_macro_node)
			ethqos_read_io_macro_from_dtsi(rgmii_io_macro_node, ethqos);

		ethqos->probed = false;
		of_node_put(rgmii_io_macro_node);
	}

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	/*Invoke SCM call */
	if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
		/* Init values */
		memcpy(ethqos->shm_rgmii_hsr.vaddr, ethqos->por,
		       (ethqos->num_por * sizeof(struct ethqos_emac_por)));
		qtee_shmbridge_flush_shm_buf(&ethqos->shm_rgmii_hsr);

		qcom_scm_call_ethqos_configure(ethqos->rgmii_phy_base, ethqos->speed,
					       priv->plat->interface, ethqos->shm_rgmii_hsr.paddr,
					       ethqos->num_por * sizeof(struct ethqos_emac_por));
		return 0;
	}
#endif

	/* Reset to POR values and enable clk */
	for (i = 0; i < ethqos->num_por; i++)
		rgmii_writel(ethqos, ethqos->por[i].value,
			     ethqos->por[i].offset);

	ethqos_set_func_clk_en(ethqos);

	/* Initialize the DLL first */
	/* Set DLL_RST */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_RST,
		      SDCC_DLL_CONFIG_DLL_RST, SDCC_HC_REG_DLL_CONFIG);

	/* Set PDN */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_PDN,
		      SDCC_DLL_CONFIG_PDN, SDCC_HC_REG_DLL_CONFIG);

	/* Clear DLL_RST */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_RST, 0,
		      SDCC_HC_REG_DLL_CONFIG);

	/* Clear PDN */
	rgmii_updatel(ethqos, SDCC_DLL_CONFIG_PDN, 0,
		      SDCC_HC_REG_DLL_CONFIG);

	if (ethqos->speed != SPEED_100 && ethqos->speed != SPEED_10) {
		/* Set DLL_EN */
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_DLL_EN,
			      SDCC_DLL_CONFIG_DLL_EN, SDCC_HC_REG_DLL_CONFIG);

		/* Set CK_OUT_EN */
		rgmii_updatel(ethqos, SDCC_DLL_CONFIG_CK_OUT_EN,
			      SDCC_DLL_CONFIG_CK_OUT_EN,
			      SDCC_HC_REG_DLL_CONFIG);

		/* Configure SDCC_USR_CTRL */
		rgmii_updatel(ethqos, GENMASK(31, 0),
			      BIT(6) | BIT(7) | BIT(9) | BIT(10) | BIT(16) | BIT(24) | BIT(27),
			      SDCC_USR_CTL);
	}

	if (ethqos->speed == SPEED_1000)
		ethqos_dll_configure_v4(ethqos);

	/*Initialize Mux selection for RGMII */
	ethqos_iomacro_rgmii_init_v4(ethqos);

	return 0;
}

static int ethqos_configure_sgmii_v4(struct qcom_ethqos *ethqos)
{
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	struct net_device *ndev = dev_get_drvdata(&ethqos->pdev->dev);
	struct stmmac_priv *priv = netdev_priv(ndev);

	/*Invoke SMC call */
	if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
		qcom_scm_call_ethqos_configure(ethqos->rgmii_phy_base, ethqos->speed,
					       priv->plat->interface, ethqos->shm_rgmii_hsr.paddr,
					       RGMII_BLOCK_SIZE);
		return 0;
	}
#endif

	ethqos_set_func_clk_en(ethqos);

	rgmii_updatel(ethqos, RGMII_BYPASS_EN, RGMII_BYPASS_EN, RGMII_IO_MACRO_BYPASS);
	rgmii_updatel(ethqos, RGMII_CONFIG2_MODE_EN_VIA_GMII, 0, RGMII_IO_MACRO_CONFIG2);
	rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_RGMII_SGMII_CLK_MUX_SEL, 0,
		      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
	rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_USXGMII_GMII_MASTER_CLK_MUX_SEL,
		      SGMII_PHY_CNTRL1_USXGMII_GMII_MASTER_CLK_MUX_SEL,
		      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
	rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_SGMII_TX_TO_RX_LOOPBACK_EN, 0,
		      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
	rgmii_updatel(ethqos, USXGMII_CLK_BLK_GMII_CLK_BLK_SEL, 0, EMAC_WRAPPER_USXGMII_MUX_SEL);
	rgmii_updatel(ethqos, USXGMII_CLK_BLK_CLK_EN, 0, EMAC_WRAPPER_USXGMII_MUX_SEL);

	rgmii_updatel(ethqos, SGMII_PHY_CNTRL0_2P5G_1G_CLK_SEL, 0, EMAC_WRAPPER_SGMII_PHY_CNTRL0);
	rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_2, (BIT(6) | BIT(9)), RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_9, (BIT(10) | BIT(14) | BIT(15)),
		      RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, RGMII_CONFIG2_MAX_SPD_PRG_3, (BIT(17) | BIT(20)),
		      RGMII_IO_MACRO_CONFIG2);
	rgmii_updatel(ethqos, RGMII_SCRATCH2_MAX_SPD_PRG_4, BIT(2), RGMII_IO_MACRO_SCRATCH_2);
	rgmii_updatel(ethqos, RGMII_SCRATCH2_MAX_SPD_PRG_5, BIT(6) | BIT(7),
		      RGMII_IO_MACRO_SCRATCH_2);
	rgmii_updatel(ethqos, RGMII_SCRATCH2_MAX_SPD_PRG_6, 0, RGMII_IO_MACRO_SCRATCH_2);
	rgmii_updatel(ethqos, RGMII_CONFIG2_RGMII_CLK_SEL_CFG, RGMII_CONFIG2_RGMII_CLK_SEL_CFG,
		      RGMII_IO_MACRO_CONFIG2);

	return 0;
}

static int ethqos_configure_usx5g_v4(struct qcom_ethqos *ethqos)
{
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	struct net_device *ndev = dev_get_drvdata(&ethqos->pdev->dev);
	struct stmmac_priv *priv = netdev_priv(ndev);

	/*Invoke SMC call */
	if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
		qcom_scm_call_ethqos_configure(ethqos->rgmii_phy_base, ethqos->speed,
					       priv->plat->interface,
					       ethqos->shm_rgmii_hsr.paddr,
					       RGMII_BLOCK_SIZE);
		return 0;
	}
#endif

	ethqos_set_func_clk_en(ethqos);

	rgmii_updatel(ethqos, RGMII_BYPASS_EN, RGMII_BYPASS_EN, RGMII_IO_MACRO_BYPASS);
	rgmii_updatel(ethqos, RGMII_CONFIG_FUNC_CLK_EN, BIT(30), RGMII_IO_MACRO_CONFIG);
	rgmii_updatel(ethqos, SGMII_PHY_CNTRL0_2P5G_1G_CLK_SEL, BIT(5),
		      EMAC_WRAPPER_SGMII_PHY_CNTRL0);
	rgmii_updatel(ethqos, USXGMII_DIV_CONFIG_RPCS_COUNT_DIV2, BIT(8),
		      EMAC_WRAPPER_USXGMII_DIV_CONFIG);
	rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_RGMII_SGMII_CLK_MUX_SEL, 0,
		      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
	rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_USXGMII_GMII_MASTER_CLK_MUX_SEL,
		      SGMII_PHY_CNTRL1_USXGMII_GMII_MASTER_CLK_MUX_SEL,
		      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
	rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_SGMII_TX_TO_RX_LOOPBACK_EN, 0,
		      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
	rgmii_updatel(ethqos, RGMII_CONFIG2_MODE_EN_VIA_GMII, 0, RGMII_IO_MACRO_CONFIG2);
	rgmii_updatel(ethqos, USXGMII_CLK_BLK_GMII_CLK_BLK_SEL, 0, EMAC_WRAPPER_USXGMII_MUX_SEL);
	rgmii_updatel(ethqos, USXGMII_CLK_BLK_CLK_EN, 0, EMAC_WRAPPER_USXGMII_MUX_SEL);

	if (ethqos->speed == SPEED_5000) {
		rgmii_updatel(ethqos, USXGMII_CLK_BLK_GMII_CLK_BLK_SEL,
			      USXGMII_CLK_BLK_GMII_CLK_BLK_SEL, EMAC_WRAPPER_USXGMII_MUX_SEL);
		rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_2, (BIT(6) | BIT(9)),
			      RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_9, (BIT(10) | BIT(14) | BIT(15)),
			      RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG2_MAX_SPD_PRG_3, (BIT(17) | BIT(20)),
			      RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_SCRATCH2_MAX_SPD_PRG_4, BIT(2),
			      RGMII_IO_MACRO_SCRATCH_2);
		rgmii_updatel(ethqos, RGMII_SCRATCH2_MAX_SPD_PRG_5, (BIT(6) | BIT(7)),
			      RGMII_IO_MACRO_SCRATCH_2);
		rgmii_updatel(ethqos, RGMII_SCRATCH2_MAX_SPD_PRG_6, 0, RGMII_IO_MACRO_SCRATCH_2);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RGMII_CLK_SEL_CFG, BIT(16),
			      RGMII_IO_MACRO_CONFIG2);
	}

	return 0;
}

static int ethqos_configure_usxgmii_v4(struct qcom_ethqos *ethqos)
{
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	struct net_device *ndev = dev_get_drvdata(&ethqos->pdev->dev);
	struct stmmac_priv *priv = netdev_priv(ndev);

	/*Invoke SMC call */
	if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
		qcom_scm_call_ethqos_configure(ethqos->rgmii_phy_base, ethqos->speed,
					       priv->plat->interface,
					       ethqos->shm_rgmii_hsr.paddr,
					       RGMII_BLOCK_SIZE);
		return 0;
	}
#endif
	ethqos_set_func_clk_en(ethqos);

	rgmii_updatel(ethqos, RGMII_BYPASS_EN, RGMII_BYPASS_EN, RGMII_IO_MACRO_BYPASS);
	rgmii_updatel(ethqos, RGMII_CONFIG2_MODE_EN_VIA_GMII, 0, RGMII_IO_MACRO_CONFIG2);
	rgmii_updatel(ethqos, SGMII_PHY_CNTRL0_2P5G_1G_CLK_SEL, BIT(5),
		      EMAC_WRAPPER_SGMII_PHY_CNTRL0);
	rgmii_updatel(ethqos, USXGMII_DIV_CONFIG_RPCS_COUNT_DIV2, BIT(8),
		      EMAC_WRAPPER_USXGMII_DIV_CONFIG);
	rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_RGMII_SGMII_CLK_MUX_SEL, 0,
		      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
	rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_USXGMII_GMII_MASTER_CLK_MUX_SEL,
		      SGMII_PHY_CNTRL1_USXGMII_GMII_MASTER_CLK_MUX_SEL,
		      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
	rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_SGMII_TX_TO_RX_LOOPBACK_EN, 0,
		      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
	rgmii_updatel(ethqos, USXGMII_CLK_BLK_GMII_CLK_BLK_SEL, 0, EMAC_WRAPPER_USXGMII_MUX_SEL);
	rgmii_updatel(ethqos, USXGMII_CLK_BLK_CLK_EN, 0, EMAC_WRAPPER_USXGMII_MUX_SEL);

	switch (ethqos->speed) {
	case SPEED_10000:
		rgmii_updatel(ethqos, USXGMII_CLK_BLK_GMII_CLK_BLK_SEL,
			      USXGMII_CLK_BLK_GMII_CLK_BLK_SEL,
			      EMAC_WRAPPER_USXGMII_MUX_SEL);
		break;

	case SPEED_5000:
		rgmii_updatel(ethqos, SGMII_PHY_CNTRL0_2P5G_1G_CLK_SEL, 0,
			      EMAC_WRAPPER_SGMII_PHY_CNTRL0);
		rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_2, (BIT(6) | BIT(7)),
			      RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG2_MAX_SPD_PRG_3, (BIT(17) | BIT(18)),
			      RGMII_IO_MACRO_CONFIG2);
		break;

	case SPEED_2500:
		rgmii_updatel(ethqos, SGMII_PHY_CNTRL0_2P5G_1G_CLK_SEL, 0,
			      EMAC_WRAPPER_SGMII_PHY_CNTRL0);
		rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_9, (BIT(10) | BIT(11)),
			      RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_SCRATCH2_MAX_SPD_PRG_4, (BIT(2) | BIT(3)),
			      RGMII_IO_MACRO_SCRATCH_2);
		rgmii_updatel(ethqos, RGMII_SCRATCH2_MAX_SPD_PRG_5, 0,
			      RGMII_IO_MACRO_SCRATCH_2);
		break;

	case SPEED_1000:
		rgmii_updatel(ethqos, RGMII_CONFIG2_RGMII_CLK_SEL_CFG,
			      RGMII_CONFIG2_RGMII_CLK_SEL_CFG,
			      RGMII_IO_MACRO_CONFIG2);
		break;

	case SPEED_100:
		rgmii_updatel(ethqos, RGMII_CONFIG2_RGMII_CLK_SEL_CFG,
			      RGMII_CONFIG2_RGMII_CLK_SEL_CFG,
			      RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG_MAX_SPD_PRG_2, BIT(9),
			      RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG2_MAX_SPD_PRG_3, BIT(20),
			      RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_SCRATCH2_MAX_SPD_PRG_6, BIT(1),
			      RGMII_IO_MACRO_SCRATCH_2);
		break;

	case SPEED_10:
		rgmii_updatel(ethqos, RGMII_CONFIG2_RGMII_CLK_SEL_CFG,
			      RGMII_CONFIG2_RGMII_CLK_SEL_CFG,
			      RGMII_IO_MACRO_CONFIG2);
		break;

	default:
		dev_err(&ethqos->pdev->dev,
			"Invalid speed %d\n", ethqos->speed);
		return -EINVAL;
	}
	return 0;
}

/**
 * Separate out serdes programming from mac configuration for mac2mac
 * to ensure mac is configured after serdes programming done.
 */
static int ethqos_configure_mac_v4(struct qcom_ethqos *ethqos)
{
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);
	int ret = 0;

	switch (priv->plat->interface) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		ret  = ethqos_configure_rgmii_v4(ethqos);
		break;

	case PHY_INTERFACE_MODE_SGMII:
		ret = ethqos_configure_sgmii_v4(ethqos);
		if (!priv->plat->fixed_phy_mode)
			qcom_ethqos_serdes_update(ethqos, ethqos->speed, priv->plat->interface);
		break;

	case PHY_INTERFACE_MODE_2500BASEX:
		ret = ethqos_configure_sgmii_v4(ethqos);
		if (!priv->plat->fixed_phy_mode)
			qcom_ethqos_serdes_update(ethqos, ethqos->speed, priv->plat->interface);
		break;
	case PHY_INTERFACE_MODE_5GBASER:
		ret = ethqos_configure_usx5g_v4(ethqos);
		if (!priv->plat->fixed_phy_mode)
			qcom_ethqos_serdes_update(ethqos, ethqos->speed, priv->plat->interface);
		break;
	case PHY_INTERFACE_MODE_USXGMII:
		ret = ethqos_configure_usxgmii_v4(ethqos);
		if (!priv->plat->fixed_phy_mode)
			qcom_ethqos_serdes_update(ethqos, ethqos->speed, priv->plat->interface);
		break;
	}

	return ret;
}

static int ethqos_serdes_update(void *priv_n, unsigned int speed)
{
	struct qcom_ethqos *ethqos = priv_n;
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);
	int ret = 0;

	if (ethqos->emac_ver != EMAC_HW_v4_0_0)
		return 0;

	switch (priv->plat->interface) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		break;
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_2500BASEX:
	case PHY_INTERFACE_MODE_5GBASER:
	case PHY_INTERFACE_MODE_USXGMII:
		ret = qcom_ethqos_serdes_update(ethqos, speed, priv->plat->interface);
		break;
	}

	return ret;
}

static void ethqos_set_icc_peak_vote(struct qcom_ethqos *ethqos, bool is_peak)
{
	int idx, ret;

	if (is_peak)
		idx = EMAC_SPEED_PEAK_IDX;
	else
		idx = ethqos->vote_idx;

	/* Apply AXI bandwidth vote */
	if (ethqos->axi_icc_path) {
		ret = icc_set_bw(ethqos->axi_icc_path,
				 emac_axi_icc_data[idx].average_bandwidth,
				 emac_axi_icc_data[idx].peak_bandwidth);
		if (ret)
			ETHQOSERR("AXI ICC vote failed (idx=%d): %d\n", idx, ret);
	}

	/* Apply APB bandwidth vote */
	if (ethqos->apb_icc_path) {
		ret = icc_set_bw(ethqos->apb_icc_path,
				 emac_apb_icc_data[idx].average_bandwidth,
				 emac_apb_icc_data[idx].peak_bandwidth);
		if (ret)
			ETHQOSERR("APB ICC vote failed (idx=%d): %d\n", idx, ret);
	}
}

#define MDIO_ICC_IDLE_MS 5

static void ethqos_mdio_icc_idle_handler(struct work_struct *work)
{
	struct qcom_ethqos *ethqos =
		container_of(work, struct qcom_ethqos, mdio_icc_idle_work.work);
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	mutex_lock(&ethqos->mdio_icc_lock);
	if (priv->plat->mdio_op_busy) {
		mutex_unlock(&ethqos->mdio_icc_lock);
		schedule_delayed_work(&ethqos->mdio_icc_idle_work,
				      msecs_to_jiffies(MDIO_ICC_IDLE_MS));
		return;
	}

	ethqos_set_icc_peak_vote(ethqos, false);
	ethqos->mdio_icc_peak_active = false;
	mutex_unlock(&ethqos->mdio_icc_lock);
}

static void ethqos_mdio_icc_acquire(void *priv)
{
	struct qcom_ethqos *ethqos = priv;

	mutex_lock(&ethqos->mdio_icc_lock);
	cancel_delayed_work(&ethqos->mdio_icc_idle_work);
	if (!ethqos->mdio_icc_peak_active) {
		ethqos_set_icc_peak_vote(ethqos, true);
		ethqos->mdio_icc_peak_active = true;
	}
	mutex_unlock(&ethqos->mdio_icc_lock);
}

static void ethqos_mdio_icc_release(void *priv)
{
	struct qcom_ethqos *ethqos = priv;

	schedule_delayed_work(&ethqos->mdio_icc_idle_work,
			      msecs_to_jiffies(MDIO_ICC_IDLE_MS));
}

static void ethqos_mdio_icc_cancel(void *priv)
{
	struct qcom_ethqos *ethqos = priv;

	cancel_delayed_work_sync(&ethqos->mdio_icc_idle_work);
	mutex_lock(&ethqos->mdio_icc_lock);
	if (ethqos->mdio_icc_peak_active) {
		ethqos_set_icc_peak_vote(ethqos, false);
		ethqos->mdio_icc_peak_active = false;
	}
	mutex_unlock(&ethqos->mdio_icc_lock);
}

static void ethqos_fix_mac_speed(void *priv_n, unsigned int speed)
{
	struct qcom_ethqos *ethqos = priv_n;
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);
	int ret = 0;

	ethqos->speed = speed;
	ethqos_update_clk_and_bus_cfg(ethqos, speed, priv->plat->interface);

	if (ethqos->emac_ver == EMAC_HW_v3_0_0_RG)
		ret = ethqos_configure_mac_v3(ethqos);
	else if (ethqos->emac_ver == EMAC_HW_v4_0_0)
		ret = ethqos_configure_mac_v4(ethqos);
	else
		ret = ethqos_configure(ethqos);

	if (ret != 0)
		ETHQOSERR("HSR configuration has failed\n");
}

static int ethqos_phy_intr_config(struct qcom_ethqos *ethqos)
{
	int ret = 0;

	ethqos->phy_intr = platform_get_irq_byname(ethqos->pdev, "phy-intr");

	if (ethqos->phy_intr < 0) {
		if (ethqos->phy_intr != -EPROBE_DEFER) {
			dev_err(&ethqos->pdev->dev,
				"PHY IRQ configuration information not found\n");
		}
		ret = 1;
	}

	return ret;
}

static int ethqos_wol_intr_config(struct qcom_ethqos *ethqos)
{
	int ret = 0;

	ethqos->wol_intr = platform_get_irq_byname(ethqos->pdev, "eth_wol_irq");

	if (ethqos->wol_intr < 0) {
		if (ethqos->wol_intr != -EPROBE_DEFER) {
			dev_err(&ethqos->pdev->dev,
				"WOL IRQ configuration information not found\n");
		}
		ret = 1;
	}

	return ret;
}

static void ethqos_handle_phy_interrupt(struct qcom_ethqos *ethqos)
{
	int phy_intr_status = 0;
	struct platform_device *pdev = ethqos->pdev;

	struct net_device *dev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(dev);
	int micrel_intr_status = 0;

	/*Interrupt routine shouldn't be called for mac2mac*/
	if (priv->plat->mac2mac_en || priv->plat->fixed_phy_mode) {
		WARN_ON(1);
		return;
	}

	/*If phy driver support interrupt handling use it*/
	if (priv->phydev && priv->phydev->drv && priv->phydev->drv->handle_interrupt) {
		priv->phydev->drv->handle_interrupt(priv->phydev);
		return;
	}

	/*Use legacy way of handling the interrupt*/
	if ((priv->phydev && priv->phydev->drv && (priv->phydev->phy_id &
	     priv->phydev->drv->phy_id_mask)
	     == MICREL_PHY_ID) ||
	    (priv->phydev && priv->phydev->drv && (priv->phydev->phy_id &
	     priv->phydev->drv->phy_id_mask)
	     == PHY_ID_KSZ9131)) {
		if (priv->mii) {
			phy_intr_status = priv->mii->read(priv->mii,
							  priv->plat->phy_addr,
							  DWC_ETH_QOS_BASIC_STATUS);
		}

		ETHQOSDBG("Basic Status Reg (%#x) = %#x\n",
			  DWC_ETH_QOS_BASIC_STATUS, phy_intr_status);

		if (priv->mii) {
			micrel_intr_status = priv->mii->read(priv->mii,
							     priv->plat->phy_addr,
							     DWC_ETH_QOS_MICREL_PHY_INTCS);
		}

		ETHQOSDBG("MICREL PHY Intr EN Reg (%#x) = %#x\n",
			  DWC_ETH_QOS_MICREL_PHY_INTCS, micrel_intr_status);

		/**
		 * Call ack interrupt to clear the WOL
		 * interrupt status fields
		 */
		if (priv->phydev->drv->config_intr)
			priv->phydev->drv->config_intr(priv->phydev);

		/* Interrupt received for link state change */
		if (phy_intr_status & LINK_STATE_MASK) {
			if (micrel_intr_status & MICREL_LINK_UP_INTR_STATUS)
				ETHQOSDBG("Intr for link UP state\n");
			phy_mac_interrupt(priv->phydev);
		} else if (!(phy_intr_status & LINK_STATE_MASK)) {
			ETHQOSDBG("Intr for link DOWN state\n");
			phy_mac_interrupt(priv->phydev);
		} else if (!(phy_intr_status & AUTONEG_STATE_MASK)) {
			ETHQOSDBG("Intr for link down with auto-neg err\n");
		}
	} else {
		if (priv->mii) {
			phy_intr_status =
			priv->mii->read(priv->mii, priv->plat->phy_addr,
					DWC_ETH_QOS_PHY_INTR_STATUS);
		}

		if (!priv->plat->mac2mac_en) {
			if (phy_intr_status & LINK_UP_STATE)
				phy_mac_interrupt(priv->phydev);
			else if (phy_intr_status & LINK_DOWN_STATE)
				phy_mac_interrupt(priv->phydev);
		}
	}
}

static void ethqos_defer_phy_isr_work(struct work_struct *work)
{
	struct qcom_ethqos *ethqos =
		container_of(work, struct qcom_ethqos, emac_phy_work);

	if (ethqos->clks_suspended)
		wait_for_completion(&ethqos->clk_enable_done);

	ethqos_handle_phy_interrupt(ethqos);
}

static irqreturn_t ETHQOS_PHY_ISR(int irq, void *dev_data)
{
	struct qcom_ethqos *ethqos = (struct qcom_ethqos *)dev_data;

	pm_wakeup_event(&ethqos->pdev->dev, 5000);

	queue_work(system_wq, &ethqos->emac_phy_work);

	return IRQ_HANDLED;
}

static void ethqos_defer_wol_isr_work(struct work_struct *work)
{
	struct qcom_ethqos *ethqos =
		container_of(work, struct qcom_ethqos, emac_wol_work);
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	if (ethqos->clks_suspended)
		wait_for_completion(&ethqos->clk_enable_done);

	/* WoL interrupt handler function*/
	priv->phydev->drv->handle_interrupt(priv->phydev);
}

static irqreturn_t ETHQOS_WOL_ISR(int irq, void *dev_data)
{
	struct qcom_ethqos *ethqos = (struct qcom_ethqos *)dev_data;

	pm_wakeup_event(&ethqos->pdev->dev, 5000);

	 /* queue the WoL interrupt handler */
	queue_work(system_wq, &ethqos->emac_wol_work);

	return IRQ_HANDLED;
}

static void ethqos_phy_irq_enable(void *priv_n)
{
	struct stmmac_priv *priv = priv_n;
	struct qcom_ethqos *ethqos = priv->plat->bsp_priv;

	if (ethqos->phy_intr) {
		ETHQOSINFO("enabling irq = %d\n", priv->phy_irq_enabled);
		enable_irq(ethqos->phy_intr);
		priv->phy_irq_enabled = true;
	}
}

static void ethqos_wol_irq_enable(void *priv_n)
{
	struct stmmac_priv *priv = priv_n;
	struct qcom_ethqos *ethqos = priv->plat->bsp_priv;

	if (ethqos->wol_intr) {
		ETHQOSINFO("enabling  wol irq = %d\n", priv->wol_irq_enabled);
		enable_irq(ethqos->wol_intr);
		priv->wol_irq_enabled = true;
	}
}

static void ethqos_wol_irq_disable(void *priv_n)
{
	struct stmmac_priv *priv = priv_n;
	struct qcom_ethqos *ethqos = priv->plat->bsp_priv;

	if (ethqos->wol_intr) {
		ETHQOSINFO("disabling irq = %d\n", priv->wol_irq_enabled);
		disable_irq(ethqos->wol_intr);
		priv->wol_irq_enabled = false;
	}
}

static void ethqos_phy_irq_disable(void *priv_n)
{
	struct stmmac_priv *priv = priv_n;
	struct qcom_ethqos *ethqos = priv->plat->bsp_priv;

	if (ethqos->phy_intr) {
		ETHQOSINFO("disabling irq = %d\n", priv->phy_irq_enabled);
		disable_irq(ethqos->phy_intr);
		priv->phy_irq_enabled = false;
	}
}

static int ethqos_phy_intr_enable(struct qcom_ethqos *ethqos)
{
	int ret = 0;
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	INIT_WORK(&ethqos->emac_phy_work, ethqos_defer_phy_isr_work);
	init_completion(&ethqos->clk_enable_done);

	ret = request_irq(ethqos->phy_intr, ETHQOS_PHY_ISR,
			  IRQF_SHARED, "emac_phy_intr", ethqos);
	if (ret) {
		ETHQOSERR("Unable to register PHY IRQ %d\n",
			  ethqos->phy_intr);
		return ret;
	}
	priv->plat->phy_intr_en_extn_stm = true;
	priv->phy_irq_enabled = true;
	return ret;
}

static int ethqos_wol_intr_enable(struct qcom_ethqos *ethqos)
{
	int ret = 0;
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	INIT_WORK(&ethqos->emac_wol_work, ethqos_defer_wol_isr_work);
	init_completion(&ethqos->clk_enable_done);

	ret = request_irq(ethqos->wol_intr, ETHQOS_WOL_ISR,
			  IRQF_SHARED, "emac_pin_wol_intr", ethqos);
	if (ret) {
		ETHQOSERR("Unable to register PHY IRQ %d\n",
			  ethqos->wol_intr);
		return ret;
	}

	priv->wol_irq_enabled = true;
	return ret;
}


static const struct of_device_id qcom_ethqos_match[] = {
	{ .compatible = "qcom,stmmac-ethqos", },
	{ .compatible = "qcom,emac-smmu-embedded", },
	{ }
};

static void emac_emb_smmu_exit(void)
{
	emac_emb_smmu_ctx.valid = false;
	emac_emb_smmu_ctx.pdev_master = NULL;
	emac_emb_smmu_ctx.smmu_pdev = NULL;
	emac_emb_smmu_ctx.iommu_domain = NULL;
}

static int emac_emb_smmu_cb_probe(struct platform_device *pdev,
				  struct plat_stmmacenet_data *plat_dat)
{
	int result = 0;
	u32 iova_ap_mapping[2];
	struct device *dev = &pdev->dev;

	ETHQOSDBG("EMAC EMB SMMU CB probe: smmu pdev=%p\n", pdev);

	result = of_property_read_u32_array(dev->of_node,
					    "qcom,iommu-dma-addr-pool",
					    iova_ap_mapping,
					    ARRAY_SIZE(iova_ap_mapping));
	if (result) {
		ETHQOSERR("Failed to read EMB start/size iova addresses\n");
		return result;
	}

	emac_emb_smmu_ctx.smmu_pdev = pdev;

	if (dma_set_mask(dev, DMA_BIT_MASK(32)) ||
	    dma_set_coherent_mask(dev, DMA_BIT_MASK(32))) {
		ETHQOSERR("DMA set 32bit mask failed\n");
		return -EOPNOTSUPP;
	}

	emac_emb_smmu_ctx.valid = true;

	emac_emb_smmu_ctx.iommu_domain =
		iommu_get_domain_for_dev(&emac_emb_smmu_ctx.smmu_pdev->dev);

	ETHQOSINFO("Successfully attached to IOMMU\n");
	plat_dat->stmmac_emb_smmu_ctx = emac_emb_smmu_ctx;
	if (emac_emb_smmu_ctx.pdev_master)
		goto smmu_probe_done;

smmu_probe_done:
	emac_emb_smmu_ctx.ret = result;
	return result;
}

static void ethqos_pps_irq_config(struct qcom_ethqos *ethqos)
{
	ethqos->pps_class_a_irq =
	platform_get_irq_byname(ethqos->pdev, "ptp_pps_irq_0");
	if (ethqos->pps_class_a_irq < 0) {
		if (ethqos->pps_class_a_irq != -EPROBE_DEFER)
			ETHQOSERR("class_a_irq config info not found\n");
	}
	ethqos->pps_class_b_irq =
	platform_get_irq_byname(ethqos->pdev, "ptp_pps_irq_1");
	if (ethqos->pps_class_b_irq < 0) {
		if (ethqos->pps_class_b_irq != -EPROBE_DEFER)
			ETHQOSERR("class_b_irq config info not found\n");
	}
}

static void qcom_ethqos_phy_suspend_clks(struct qcom_ethqos *ethqos)
{
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	ETHQOSINFO("Enter\n");

	if (priv->plat->phy_intr_en_extn_stm)
		reinit_completion(&ethqos->clk_enable_done);

	if (priv->plat->mdio_op_busy)
		wait_for_completion(&priv->plat->mdio_op);

	ethqos->clks_suspended = 1;
	atomic_set(&priv->plat->phy_clks_suspended, 1);

	ethqos_update_clk_and_bus_cfg(ethqos, 0, priv->plat->interface);

	if (priv->plat->stmmac_clk)
		clk_disable_unprepare(priv->plat->stmmac_clk);

	if (priv->plat->pclk)
		clk_disable_unprepare(priv->plat->pclk);

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	if (priv->ptp_init && priv->plat->clk_ptp_ref)
		clk_disable_unprepare(priv->plat->clk_ptp_ref);
#else
	if (priv->plat->clk_ptp_ref)
		clk_disable_unprepare(priv->plat->clk_ptp_ref);
#endif

	if (ethqos->rgmii_clk)
		clk_disable_unprepare(ethqos->rgmii_clk);

	ETHQOSINFO("Exit\n");
}

static int ethqos_writeback_desc_rec(struct stmmac_priv *priv, int chan)
{
	return -EOPNOTSUPP;
}

static int ethqos_reset_phy_rec(struct stmmac_priv *priv, int int_en)
{
	struct qcom_ethqos *ethqos =   priv->plat->bsp_priv;

	if (int_en && (priv->phydev &&
		       priv->phydev->autoneg == AUTONEG_DISABLE) && priv->mii) {
		ethqos->backup_autoneg = priv->phydev->autoneg;
		ethqos->backup_bmcr = priv->mii->read(priv->mii,
						      priv->plat->phy_addr,
						      MII_BMCR);
	} else {
		ethqos->backup_autoneg = AUTONEG_ENABLE;
	}

	ethqos_phy_power_off(ethqos);

	ethqos_phy_power_on(ethqos);

	if (int_en) {
		if (priv->mii) {
			ETHQOSERR("do mdio reset\n");
			priv->mii->reset(priv->mii);
		}

		/*Enable phy interrupt*/
		if (priv->plat->phy_intr_en_extn_stm && priv->phydev) {
			ETHQOSDBG("PHY interrupt Mode enabled\n");
			priv->phydev->irq = PHY_MAC_INTERRUPT;
			priv->phydev->interrupts =  PHY_INTERRUPT_ENABLED;

			if (priv->phydev->drv->config_intr &&
			    !priv->phydev->drv->config_intr(priv->phydev)) {
				ETHQOSERR("config_phy_intr successful after phy on\n");
			}
		} else if (!priv->plat->phy_intr_en_extn_stm && priv->phydev) {
			priv->phydev->irq = PHY_POLL;
			ETHQOSDBG("PHY Polling Mode enabled\n");
		} else {
			ETHQOSERR("phydev is null , intr value=%d\n",
				  priv->plat->phy_intr_en_extn_stm);
		}
		if (ethqos->backup_autoneg == AUTONEG_DISABLE && priv->phydev) {
			priv->phydev->autoneg = ethqos->backup_autoneg;
			phy_write(priv->phydev, MII_BMCR, ethqos->backup_bmcr);
		}
	}

	return 1;
}

static int ethqos_tx_clean_rec(struct stmmac_priv *priv, int chan)
{
	if (!priv->plat->tx_queues_cfg[chan].skip_sw)
		stmmac_tx_clean(priv, priv->dma_tx_size, chan);
	return 1;
}

static int ethqos_reset_tx_dma_rec(struct stmmac_priv *priv, int chan)
{
	stmmac_tx_err(priv, chan);
	return 1;
}

static int ethqos_schedule_poll(struct stmmac_priv *priv, int chan)
{
	struct stmmac_channel *ch;

	ch = &priv->channel[chan];

	if (!priv->plat->rx_queues_cfg[chan].skip_sw) {
		if (likely(napi_schedule_prep(&ch->rx_napi))) {
			priv->hw->dma->disable_dma_irq(priv->ioaddr, chan, 1, 0);
			__napi_schedule(&ch->rx_napi);
		}
	}
	return 1;
}

static void ethqos_tdu_rec_wq(struct work_struct *work)
{
	struct delayed_work *dwork;
	struct stmmac_priv *priv;
	struct qcom_ethqos *ethqos;
	int ret;

	ETHQOSDBG("Enter\n");
	dwork = container_of(work, struct delayed_work, work);
	ethqos = container_of(dwork, struct qcom_ethqos, tdu_rec);

	priv = qcom_ethqos_get_priv(ethqos);
	if (!priv)
		return;

	ret = ethqos_tx_clean_rec(priv, ethqos->tdu_chan);
	if (!ret)
		return;

	ethqos->tdu_scheduled = false;
}

static void ethqos_mac_rec_init(struct qcom_ethqos *ethqos)
{
	int threshold[] = {10, 10, 10, 10, 10, 10, 10, 10, 10, 10};
	int en[] = {0, 1, 0, 0, 0, 0, 0, 0, 0, 0};
	int i;

	for (i = 0; i < MAC_ERR_CNT; i++) {
		ethqos->mac_rec_threshold[i] = threshold[i];
		ethqos->mac_rec_en[i] = en[i];
		ethqos->mac_err_cnt[i] = 0;
		ethqos->mac_rec_cnt[i] = 0;
	}
	INIT_DELAYED_WORK(&ethqos->tdu_rec,
			  ethqos_tdu_rec_wq);
}

static int dwmac_qcom_handle_mac_err(void *pdata, int type, int chan)
{
	int ret = 1;
	struct qcom_ethqos *ethqos;
	struct stmmac_priv *priv;

	if (!pdata)
		return -EINVAL;
	priv = pdata;
	ethqos = priv->plat->bsp_priv;

	if (!ethqos)
		return -EINVAL;

	ethqos->mac_err_cnt[type]++;

	if (ethqos->mac_rec_en[type]) {
		if (ethqos->mac_rec_cnt[type] >=
		    ethqos->mac_rec_threshold[type]) {
			ETHQOSERR("exceeded recovery threshold for %s",
				  err_names[type]);
			ethqos->mac_rec_en[type] = false;
			ret = 0;
		} else {
			ethqos->mac_rec_cnt[type]++;
			switch (type) {
			case PHY_RW_ERR:
			{
				ret = ethqos_reset_phy_rec(priv, true);
				if (!ret) {
					ETHQOSERR("recovery failed for %s",
						  err_names[type]);
					ethqos->mac_rec_fail[type] = true;
				}
			}
			break;
			case PHY_DET_ERR:
			{
				ret = ethqos_reset_phy_rec(priv, false);
				if (!ret) {
					ETHQOSERR("recovery failed for %s",
						  err_names[type]);
					ethqos->mac_rec_fail[type] = true;
				}
			}
			break;
			case FBE_ERR:
			{
				ret = ethqos_reset_tx_dma_rec(priv, chan);
				if (!ret) {
					ETHQOSERR("recovery failed for %s",
						  err_names[type]);
					ethqos->mac_rec_fail[type] = true;
				}
			}
			break;
			case RBU_ERR:
			{
				ret = ethqos_schedule_poll(priv, chan);
				if (!ret) {
					ETHQOSERR("recovery failed for %s",
						  err_names[type]);
					ethqos->mac_rec_fail[type] = true;
				}
			}
			break;
			case TDU_ERR:
			{
				if (!ethqos->tdu_scheduled) {
					ethqos->tdu_chan = chan;
					schedule_delayed_work
					(&ethqos->tdu_rec,
					 msecs_to_jiffies(3000));
					ethqos->tdu_scheduled = true;
				}
			}
			break;
			case CRC_ERR:
			case RECEIVE_ERR:
			case OVERFLOW_ERR:
			case DRIBBLE_ERR:
			case WDT_ERR:
			{
				ret = ethqos_writeback_desc_rec(priv, chan);
				if (!ret) {
					ETHQOSERR("recovery failed for %s",
						  err_names[type]);
					ethqos->mac_rec_fail[type] = true;
				}
			}
			break;
			default:
			break;
			}
		}
	}
	mac_rec_wq_flag = true;
	wake_up_interruptible(&mac_rec_wq);

	return ret;
}

inline bool qcom_ethqos_is_phy_link_up(struct qcom_ethqos *ethqos)
{
	/* PHY driver initializes phydev->link=1.
	 * So, phydev->link is 1 even on bootup with no PHY connected.
	 * phydev->link is valid only after adjust_link is called once.
	 */
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	if (priv->plat->mac2mac_en || priv->plat->fixed_phy_mode) {
		return priv->plat->mac2mac_link;
	} else {
		return (priv->dev->phydev &&
			priv->dev->phydev->link);
	}
}

static void qcom_ethqos_phy_resume_clks(struct qcom_ethqos *ethqos)
{
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	ETHQOSINFO("Enter\n");

	if (priv->plat->stmmac_clk)
		clk_prepare_enable(priv->plat->stmmac_clk);

	if (priv->plat->pclk)
		clk_prepare_enable(priv->plat->pclk);

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	if (priv->ptp_init && priv->plat->clk_ptp_ref)
		clk_prepare_enable(priv->plat->clk_ptp_ref);
#else
	if (priv->plat->clk_ptp_ref)
		clk_prepare_enable(priv->plat->clk_ptp_ref);
#endif

	if (ethqos->rgmii_clk)
		clk_prepare_enable(ethqos->rgmii_clk);

	if (qcom_ethqos_is_phy_link_up(ethqos))
		ethqos_update_clk_and_bus_cfg(ethqos, ethqos->speed, priv->plat->interface);
	else
		ethqos_update_clk_and_bus_cfg(ethqos, SPEED_10, priv->plat->interface);

	atomic_set(&priv->plat->phy_clks_suspended, 0);
	ethqos->clks_suspended = 0;

	if (priv->plat->phy_intr_en_extn_stm)
		complete_all(&ethqos->clk_enable_done);

	ETHQOSINFO("Exit\n");
}

void qcom_ethqos_request_phy_wol(void *plat_n)
{
	struct plat_stmmacenet_data *plat = plat_n;
	struct qcom_ethqos *ethqos;
	struct stmmac_priv *priv;
	int ret = 0;

	if (!plat)
		return;

	ethqos = plat->bsp_priv;
	priv = qcom_ethqos_get_priv(ethqos);

	if (!priv || !priv->en_wol)
		return;

	/* Check if phydev is valid*/
	/* Check and enable Wake-on-LAN functionality in PHY*/
	if (priv->phydev) {
		struct ethtool_wolinfo wol = {.cmd = ETHTOOL_GWOL};
		phy_ethtool_get_wol(priv->phydev, &wol);

		wol.cmd = ETHTOOL_SWOL;
		wol.wolopts = wol.supported;
		ret = phy_ethtool_set_wol(priv->phydev, &wol);

		if (ret) {
			ETHQOSERR("set wol in PHY failed\n");
			return;
		}

		if (ret == EOPNOTSUPP) {
			ETHQOSERR("WOL not supported\n");
			return;
		}

		device_set_wakeup_capable(priv->device, 1);

		if (priv->plat->separate_wol_pin)
			enable_irq_wake(ethqos->wol_intr);
		else
			enable_irq_wake(ethqos->phy_intr);
		device_set_wakeup_enable(&ethqos->pdev->dev, 1);
	}
}

static void qcom_ethqos_bringup_iface(struct work_struct *work)
{
	struct platform_device *pdev = NULL;
	struct net_device *ndev = NULL;
	struct qcom_ethqos *ethqos =
		container_of(work, struct qcom_ethqos, early_eth);

	ETHQOSINFO("entry\n");
	if (!ethqos)
		return;
	pdev = ethqos->pdev;
	if (!pdev)
		return;
	ndev = platform_get_drvdata(pdev);
	if (!ndev || netif_running(ndev))
		return;
	rtnl_lock();
	if (dev_change_flags(ndev, ndev->flags | IFF_UP, NULL) < 0)
		ETHQOSINFO("ERROR\n");
	rtnl_unlock();
	ETHQOSINFO("exit\n");
}

static void ethqos_is_ipv4_NW_stack_ready(struct work_struct *work)
{
	struct delayed_work *dwork;
	struct qcom_ethqos *ethqos;
	struct platform_device *pdev = NULL;
	struct net_device *ndev = NULL;
	int ret;

	ETHQOSINFO("\n");
	dwork = container_of(work, struct delayed_work, work);
	ethqos = container_of(dwork, struct qcom_ethqos, ipv4_addr_assign_wq);

	if (!ethqos)
		return;

	pdev = ethqos->pdev;

	if (!pdev)
		return;

	ndev = platform_get_drvdata(pdev);

	ret = qcom_ethqos_add_ipaddr(&pparams, ndev);
	if (ret)
		return;

	cancel_delayed_work_sync(&ethqos->ipv4_addr_assign_wq);
	flush_delayed_work(&ethqos->ipv4_addr_assign_wq);
}

static void ethqos_is_ipv6_NW_stack_ready(struct work_struct *work)
{
	struct delayed_work *dwork;
	struct qcom_ethqos *ethqos;
	struct platform_device *pdev = NULL;
	struct net_device *ndev = NULL;
	int ret;

	ETHQOSINFO("\n");
	dwork = container_of(work, struct delayed_work, work);
	ethqos = container_of(dwork, struct qcom_ethqos, ipv6_addr_assign_wq);

	if (!ethqos)
		return;

	pdev = ethqos->pdev;

	if (!pdev)
		return;

	ndev = platform_get_drvdata(pdev);

	ret = qcom_ethqos_add_ipv6addr(&pparams, ndev);
	if (ret)
		return;
}

static void ethqos_set_early_eth_param(struct stmmac_priv *priv,
				       struct qcom_ethqos *ethqos)
{
	int ret = 0;

	qcom_ethqos_add_ipaddr(&pparams, priv->dev);

	if (pparams.is_valid_ipv4_addr) {
		INIT_DELAYED_WORK(&ethqos->ipv4_addr_assign_wq,
				  ethqos_is_ipv4_NW_stack_ready);
		ret = qcom_ethqos_add_ipaddr(&pparams, priv->dev);
		if (ret)
			schedule_delayed_work(&ethqos->ipv4_addr_assign_wq,
					      msecs_to_jiffies(1000));
	}

	if (pparams.is_valid_ipv6_addr) {
		INIT_DELAYED_WORK(&ethqos->ipv6_addr_assign_wq,
				  ethqos_is_ipv6_NW_stack_ready);
		ret = qcom_ethqos_add_ipv6addr(&pparams, priv->dev);
		if (ret)
			schedule_delayed_work(&ethqos->ipv6_addr_assign_wq,
					      msecs_to_jiffies(100));
	}

	return;
}

/**
 * Utility functions for loopback
 */
static void print_loopback_detail(enum loopback_mode loopback)
{
	switch (loopback) {
	case DISABLE_LOOPBACK:
		ETHQOSINFO("Loopback is disabled\n");
		break;
	case ENABLE_IO_MACRO_LOOPBACK:
		ETHQOSINFO("Loopback is Enabled as IO MACRO LOOPBACK\n");
		break;
	case ENABLE_MAC_LOOPBACK:
		ETHQOSINFO("Loopback is Enabled as MAC LOOPBACK\n");
		break;
	case ENABLE_PHY_LOOPBACK:
		ETHQOSINFO("Loopback is Enabled as PHY LOOPBACK\n");
		break;
	case ENABLE_SERDES_LOOPBACK:
		ETHQOSINFO("Loopback is Enabled as SERDES LOOPBACK\n");
		break;
	default:
		ETHQOSINFO("Invalid Loopback=%d\n", loopback);
		break;
	}
}

static ssize_t loopback_arg_parse(struct qcom_ethqos *ethqos, const char *buf,
				  int *config, int *speed)
{
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);
	unsigned long ret;

	ret = sscanf(buf, "%d %d", config,  speed);
	if (*config == DISABLE_LOOPBACK && ret != 1) {
		ETHQOSERR("Speed is not needed while disabling loopback\n");
		return -EINVAL;
	}

	if (*config > DISABLE_LOOPBACK && ret != 2) {
		ETHQOSERR("Speed is also needed while enabling loopback\n");
		return -EINVAL;
	}

	if (priv->loopback_direction == NETWORK_LOOPBACK_MODE) {
		ETHQOSERR("Not enabling host loopback since network loopback mode is enabled\n");
		return -EINVAL;
	}

	if (*config < DISABLE_LOOPBACK || *config > ENABLE_SERDES_LOOPBACK) {
		ETHQOSERR("Invalid config =%d\n", *config);
		return -EINVAL;
	}

	if (*config == ENABLE_PHY_LOOPBACK &&
	    (priv->plat->mac2mac_en || priv->plat->fixed_phy_mode)) {
		ETHQOSINFO("Not supported with Mac2Mac enabled\n");
		return -EOPNOTSUPP;
	}

	if ((*config == ENABLE_PHY_LOOPBACK  || priv->current_loopback ==
			ENABLE_PHY_LOOPBACK) &&
			ethqos->current_phy_mode == DISABLE_PHY_IMMEDIATELY) {
		ETHQOSERR("Can't enabled/disable ");
		ETHQOSERR("phy loopback when phy is off\n");
		return -EPERM;
	}

	/*Argument validation*/
	if (*config == ENABLE_IO_MACRO_LOOPBACK || *config == ENABLE_MAC_LOOPBACK ||
	    *config == ENABLE_PHY_LOOPBACK || *config == ENABLE_SERDES_LOOPBACK) {
		switch (priv->plat->interface) {
		case PHY_INTERFACE_MODE_RGMII:
		case PHY_INTERFACE_MODE_RGMII_ID:
		case PHY_INTERFACE_MODE_RGMII_RXID:
		case PHY_INTERFACE_MODE_RGMII_TXID:
			if ((*config == ENABLE_SERDES_LOOPBACK) ||
			    (*speed != SPEED_1000 && *speed != SPEED_100 &&
			     *speed != SPEED_10))
				return -EINVAL;
			break;
		case PHY_INTERFACE_MODE_SGMII:
			if ((*config == ENABLE_IO_MACRO_LOOPBACK) ||
			    (*speed != SPEED_2500 && *speed != SPEED_1000 &&
			     *speed != SPEED_100 && *speed != SPEED_10))
				return -EINVAL;
			break;
		case PHY_INTERFACE_MODE_USXGMII:
			if ((*config == ENABLE_IO_MACRO_LOOPBACK) ||
			    (*speed != SPEED_10000 && *speed != SPEED_5000 &&
			     *speed != SPEED_2500 && *speed != SPEED_1000 &&
			     *speed != SPEED_100 && *speed != SPEED_10))
				return -EINVAL;
			break;
		case PHY_INTERFACE_MODE_2500BASEX:
			if ((*config == ENABLE_IO_MACRO_LOOPBACK) ||
			    (*speed != SPEED_2500))
				return -EINVAL;
			break;
		case PHY_INTERFACE_MODE_5GBASER:
			if ((*config == ENABLE_IO_MACRO_LOOPBACK) ||
			    (*speed != SPEED_5000))
				return -EINVAL;
			break;
		default:
			ETHQOSERR("Unsupported PHY interface\n");
			return -EINVAL;
		}
	}

	if (*config == priv->current_loopback) {
		switch (*config) {
		case DISABLE_LOOPBACK:
			ETHQOSINFO("Loopback is already disabled\n");
			break;
		case ENABLE_IO_MACRO_LOOPBACK:
			ETHQOSINFO("Loopback is already Enabled as ");
			ETHQOSINFO("IO MACRO LOOPBACK\n");
			break;
		case ENABLE_MAC_LOOPBACK:
			ETHQOSINFO("Loopback is already Enabled as ");
			ETHQOSINFO("MAC LOOPBACK\n");
			break;
		case ENABLE_PHY_LOOPBACK:
			ETHQOSINFO("Loopback is already Enabled as ");
			ETHQOSINFO("PHY LOOPBACK\n");
			break;
		case ENABLE_SERDES_LOOPBACK:
			ETHQOSINFO("Loopback is already Enabled as ");
			ETHQOSINFO("SERDES LOOPBACK\n");
			break;
		}
		return -EINVAL;
	}
	/*If request to enable loopback & some other loopback already enabled*/
	if (*config != DISABLE_LOOPBACK &&
	    priv->current_loopback > DISABLE_LOOPBACK) {
		ETHQOSINFO("Loopback is already enabled\n");
		print_loopback_detail(priv->current_loopback);
		return -EINVAL;
	}
	return 0;
}

static void setup_config_registers(struct qcom_ethqos *ethqos,
				   int speed, int duplex, int mode)
{
	struct platform_device *pdev = ethqos->pdev;
	struct net_device *dev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(dev);
	u32 ctrl = 0;

	ETHQOSDBG("Speed=%d,dupex=%d,mode=%d\n", speed, duplex, mode);

	if (mode > DISABLE_LOOPBACK && !qcom_ethqos_is_phy_link_up(ethqos)) {
		/*If Link is Down & need to enable Loopback*/
		ETHQOSDBG("Enable Lower Up Flag & disable phy dev\n");
		netif_carrier_on(dev);
	} else if (mode == DISABLE_LOOPBACK &&
		!qcom_ethqos_is_phy_link_up(ethqos)) {
		ETHQOSDBG("Disable Lower Up as Link is down\n");
		netif_carrier_off(dev);
	}

	/*Disable phy interrupt by Link/Down by cable plug in/out*/
	if (mode > DISABLE_LOOPBACK) {
		disable_irq(ethqos->phy_intr);
		if (priv->plat->separate_wol_pin)
			disable_irq(ethqos->wol_intr);
	} else if (mode == DISABLE_LOOPBACK) {
		enable_irq(ethqos->phy_intr);
		if (priv->plat->separate_wol_pin)
			enable_irq(ethqos->wol_intr);
	} else {
		ETHQOSERR("Mode is invalid\n");
	}

	ctrl = readl_relaxed(priv->ioaddr + MAC_CTRL_REG);
	ETHQOSDBG("Old ctrl=0x%x with mask with flow control\n", ctrl);

	ctrl |= priv->hw->link.duplex;
	ctrl &= ~priv->hw->link.speed_mask;
	switch (speed) {
	case SPEED_10000:
		ctrl |= priv->hw->link.xgmii.speed10000;
		break;
	case SPEED_5000:
		ctrl |= priv->hw->link.xgmii.speed5000;
		break;
	case SPEED_2500:
		if (priv->plat->interface == PHY_INTERFACE_MODE_USXGMII ||
		    priv->plat->interface == PHY_INTERFACE_MODE_2500BASEX)
			ctrl |= priv->hw->link.xgmii.speed2500;
		else if (priv->plat->interface == PHY_INTERFACE_MODE_SGMII)
			ctrl |= priv->hw->link.speed2500;
		break;
	case SPEED_1000:
		ctrl |= priv->hw->link.speed1000;
		break;
	case SPEED_100:
		ctrl |= priv->hw->link.speed100;
		break;
	case SPEED_10:
		ctrl |= priv->hw->link.speed10;
		break;
	default:
		speed = SPEED_UNKNOWN;
		ETHQOSDBG("unknown speed\n");
		break;
	}
	writel_relaxed(ctrl, priv->ioaddr + MAC_CTRL_REG);
	ETHQOSDBG("New ctrl=%x priv hw speeed =%d\n", ctrl,
		  priv->hw->link.speed1000);

	if (priv->dev->phydev) {
		priv->dev->phydev->duplex = duplex;
		priv->dev->phydev->speed = speed;
	}
	priv->speed  = speed;

	if (speed != SPEED_UNKNOWN) {
		if (priv->plat->fixed_phy_mode)
			ethqos_serdes_update(ethqos, speed);
		ethqos_fix_mac_speed(ethqos, speed);
	}

	/*We need to reset the clks when speed change occurs on remote
	 *this is because we need to align rgmii clocks with data else
	 *the data would stall on speed change.
	 */
	if (priv->plat->rgmii_rst) {
		reset_control_assert(priv->plat->rgmii_rst);
		msleep(100);
		reset_control_deassert(priv->plat->rgmii_rst);
	}

	ETHQOSERR("End\n");
}

void qcom_serdes_loopback_v3_1(struct plat_stmmacenet_data *plat, bool on)
{
	struct qcom_ethqos *ethqos = plat->bsp_priv;

	if (on)
		rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_SGMII_TX_TO_RX_LOOPBACK_EN,
			      SGMII_PHY_CNTRL1_SGMII_TX_TO_RX_LOOPBACK_EN,
			      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
	else
		rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_SGMII_TX_TO_RX_LOOPBACK_EN, 0,
			      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
}

static void ethqos_rgmii_io_macro_loopback(struct qcom_ethqos *ethqos, int mode)
{
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	   /*Invoke SCM call */
	if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
		if (mode == ENABLE_IO_MACRO_LOOPBACK)
			qcom_scm_call_loopback_configure(ethqos->rgmii_phy_base,
							 ENABLE_IO_MACRO_LOOPBACK, 0);
		else
			qcom_scm_call_loopback_configure(ethqos->rgmii_phy_base,
							 DISABLE_LOOPBACK, 0);
		return;
	}
#endif

	/* Set loopback mode */
	if (mode == 1) {
		rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
			      RGMII_CONFIG_LOOPBACK_EN,
			      RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG2_TX_TO_RX_LOOPBACK_EN,
			      RGMII_CONFIG2_TX_TO_RX_LOOPBACK_EN,
			      RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RX_PROG_SWAP,
			      0, RGMII_IO_MACRO_CONFIG2);
	} else {
		rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
			      0, RGMII_IO_MACRO_CONFIG);
		rgmii_updatel(ethqos, RGMII_CONFIG2_TX_TO_RX_LOOPBACK_EN,
			      0, RGMII_IO_MACRO_CONFIG2);
		rgmii_updatel(ethqos, RGMII_CONFIG2_RX_PROG_SWAP,
			      RGMII_CONFIG2_RX_PROG_SWAP,
			      RGMII_IO_MACRO_CONFIG2);
	}
}

static void ethqos_mac_loopback(struct qcom_ethqos *ethqos, int mode)
{
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	u32 read_value = (u32)readl_relaxed(ethqos->ioaddr + XGMAC_RX_CONFIG);
	/* Set loopback mode */
	if (mode == 1)
		read_value |= XGMAC_CONFIG_LM;
	else
		read_value &= ~XGMAC_CONFIG_LM;
	writel_relaxed(read_value, ethqos->ioaddr + XGMAC_RX_CONFIG);

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	/*Invoke SCM call */
	if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
		if (mode == 1)
			qcom_scm_call_loopback_configure(ethqos->rgmii_phy_base,
							 ENABLE_MAC_LOOPBACK,
							 priv->plat->interface);
		else
			qcom_scm_call_loopback_configure(ethqos->rgmii_phy_base,
							 DISABLE_LOOPBACK,
							 priv->plat->interface);

		return;
	}
#endif

	if (mode == 1) {
		switch (priv->plat->interface) {
		case PHY_INTERFACE_MODE_RGMII:
		case PHY_INTERFACE_MODE_RGMII_ID:
		case PHY_INTERFACE_MODE_RGMII_RXID:
		case PHY_INTERFACE_MODE_RGMII_TXID:
			rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
				      RGMII_CONFIG_LOOPBACK_EN,
				      RGMII_IO_MACRO_CONFIG);
			break;
		case PHY_INTERFACE_MODE_SGMII:
		case PHY_INTERFACE_MODE_2500BASEX:
		case PHY_INTERFACE_MODE_5GBASER:
		case PHY_INTERFACE_MODE_USXGMII:
			rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_SGMII_TX_TO_RX_LOOPBACK_EN,
				      SGMII_PHY_CNTRL1_SGMII_TX_TO_RX_LOOPBACK_EN,
				      EMAC_WRAPPER_SGMII_PHY_CNTRL1);
			break;
		}
	} else {
		switch (priv->plat->interface) {
		case PHY_INTERFACE_MODE_RGMII:
		case PHY_INTERFACE_MODE_RGMII_ID:
		case PHY_INTERFACE_MODE_RGMII_RXID:
		case PHY_INTERFACE_MODE_RGMII_TXID:
			rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
				      0, RGMII_IO_MACRO_CONFIG);
			break;
		case PHY_INTERFACE_MODE_SGMII:
		case PHY_INTERFACE_MODE_2500BASEX:
		case PHY_INTERFACE_MODE_5GBASER:
		case PHY_INTERFACE_MODE_USXGMII:
			rgmii_updatel(ethqos, SGMII_PHY_CNTRL1_SGMII_TX_TO_RX_LOOPBACK_EN,
				      0, EMAC_WRAPPER_SGMII_PHY_CNTRL1);
			break;
		}
	}
}

static int phy_rgmii_digital_loopback(struct qcom_ethqos *ethqos, int speed, int config)
{
	struct platform_device *pdev = ethqos->pdev;
	struct net_device *dev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(dev);
	unsigned int phydata = 0;

	if (priv->phydev && priv->phydev->drv && priv->phydev->drv->set_loopback &&
	    ((priv->phydev->phy_id &
	      priv->phydev->drv->phy_id_mask) == PHY_ID_KSZ9131)) {
		priv->phydev->drv->set_loopback(priv->phydev, config);
	}
	if (config == 1) {
		/*Backup BMCR before Enabling Phy Loopback */
		ethqos->bmcr_backup = priv->mii->read(priv->mii,
						      priv->plat->phy_addr,
						      MII_BMCR);
		ETHQOSINFO("Request for phy digital loopback enable\n");
		switch (speed) {
		case SPEED_1000:
			phydata = PHY_RGMII_LOOPBACK_1000;
			break;
		case SPEED_100:
			phydata = PHY_RGMII_LOOPBACK_100;
			break;
		case SPEED_10:
			/*KSZ9131RNX_LBR = 0x11*/
			if (!priv->plat->fixed_phy_mode && priv->phydev && priv->phydev->drv &&
			    ((priv->phydev->phy_id &
			    priv->phydev->drv->phy_id_mask) == PHY_ID_KSZ9131)) {
				phydata = priv->mii->read(priv->mii,
							  priv->plat->phy_addr, 0x11);
				phydata = phydata | (1 << 2);
				priv->mii->write(priv->mii,
						 priv->plat->phy_addr, 0x11, phydata);
			}
			phydata = PHY_RGMII_LOOPBACK_10;
			break;
		default:
			ETHQOSERR("Invalid link speed\n");
			break;
		}
	} else if (config == 0) {
		ETHQOSINFO("Request for phy digital loopback disable\n");
		if (ethqos->bmcr_backup)
			phydata = ethqos->bmcr_backup;
		else
			phydata = 0x1140;
	} else {
		ETHQOSERR("Invalid option\n");
		return -EINVAL;
	}
	if (phydata != 0 && priv->mii) {
		priv->mii->write(priv->mii, priv->plat->phy_addr, MII_BMCR, phydata);
		ETHQOSINFO("write done for phy loopback\n");
	}
	return 0;
}

static int phy_sgmii_usxgmii_digital_loopback(struct qcom_ethqos *ethqos, int speed, int config)
{
	struct platform_device *pdev = ethqos->pdev;
	struct net_device *dev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);
	unsigned int phydata = 0;

	if (!priv->dev->phydev) {
		ETHQOSERR("Invalid operation when link is down");
		return -EINVAL;
	}
	qcom_xpcs_link_up(&priv->hw->qxpcs->pcs, 1, priv->plat->interface,
			  speed, priv->dev->phydev->duplex);

	if (config == 1) {
		/*Backup BMCR before Enabling Phy Loopback */
		if (dev->phydev) {
			ETHQOSINFO("%s: Backup BMCR", __func__);
			ethqos->bmcr_backup = phy_read_mmd(dev->phydev, MDIO_MMD_PHYXS,
							   MDIO_PHYXS_VEND_PROVISION_5);
		}

		ETHQOSINFO("Request for phy digital loopback enable\n");
		switch (speed) {
		case SPEED_10000:
			phydata = PHY_USXGMII_LOOPBACK_10000;
			break;
		case SPEED_5000:
			phydata = PHY_USXGMII_LOOPBACK_5000;
			break;
		case SPEED_2500:
			phydata = PHY_USXGMII_LOOPBACK_2500;
			break;
		case SPEED_1000:
			phydata = PHY_USXGMII_LOOPBACK_1000;
			break;
		case SPEED_100:
			phydata = PHY_USXGMII_LOOPBACK_100;
			break;
		case SPEED_10:
			phydata = PHY_USXGMII_LOOPBACK_10;
			break;
		default:
			ETHQOSERR("Invalid link speed\n");
			break;
		}
	} else if (config == 0) {
		ETHQOSINFO("Request for phy digital loopback disable\n");
		phydata = ethqos->bmcr_backup;
	} else {
		ETHQOSERR("Invalid option\n");
		return -EINVAL;
	}
	if (dev->phydev) {
		phy_write_mmd(dev->phydev, MDIO_MMD_PHYXS, MDIO_PHYXS_VEND_PROVISION_5, phydata);
		ETHQOSINFO("write done for phy loopback\n");
	}
	return 0;
}

static int phy_digital_loopback_config(struct qcom_ethqos *ethqos, int speed, int config)
{
	struct platform_device *pdev = ethqos->pdev;
	struct net_device *dev = platform_get_drvdata(pdev);
	struct stmmac_priv *priv = netdev_priv(dev);

	switch (priv->plat->interface) {
	case PHY_INTERFACE_MODE_RGMII:
	case PHY_INTERFACE_MODE_RGMII_ID:
	case PHY_INTERFACE_MODE_RGMII_RXID:
	case PHY_INTERFACE_MODE_RGMII_TXID:
		return phy_rgmii_digital_loopback(ethqos, speed, config);
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_USXGMII:
	case PHY_INTERFACE_MODE_5GBASER:
	case PHY_INTERFACE_MODE_2500BASEX:
		return phy_sgmii_usxgmii_digital_loopback(ethqos, speed, config);
	default:
		ETHQOSERR("Invalid interface with PHY loopback\n");
		return -EINVAL;
	}
}

static void ethqos_pcs_loopback(struct qcom_ethqos *ethqos, int config)
{
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	if (config == 1)
		qcom_xpcs_pcs_loopback(priv->hw->qxpcs, 1);
	else if (config == 0)
		qcom_xpcs_pcs_loopback(priv->hw->qxpcs, 0);
}

static void ethqos_serdes_loopback(struct qcom_ethqos *ethqos, int speed, int config)
{
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);
	int duplex = DUPLEX_FULL;

	if (IS_ERR_OR_NULL(priv)) {
		ETHQOSERR("priv is NULL or Error\n");
		return;
	}

	/* Ensure PCS context exists */
	if (IS_ERR_OR_NULL(priv->hw) || IS_ERR_OR_NULL(priv->hw->qxpcs)) {
		ETHQOSERR("QXPCS is NULL or Error\n");
		return;
	}

	/* In mac2mac/fixed-link mode, PHY may be absent (phydev == NULL).
	 * Default to full duplex if PHY is not present and still configure
	 * PCS link parameters for SERDES loopback.
	 */
	if (!IS_ERR_OR_NULL(priv->dev) && !IS_ERR_OR_NULL(priv->dev->phydev))
		duplex = priv->dev->phydev->duplex;

	qcom_xpcs_link_up(&priv->hw->qxpcs->pcs, 1, priv->plat->interface, speed, duplex);

	if (config == 1)
		qcom_xpcs_serdes_loopback(priv->hw->qxpcs, 1);
	else if (config == 0)
		qcom_xpcs_serdes_loopback(priv->hw->qxpcs, 0);
}

#if IS_ENABLED(CONFIG_DEBUG_FS)
static ssize_t phy_reg_read(struct file *file, const char __user *user_buf,
			    size_t count, loff_t *position)
{
	struct qcom_ethqos *ethqos = file->private_data;
	struct stmmac_priv *priv;
	int ret;
	u32 mmd = 0, offset = 0, reg_value = 0;
	char in_buf[400];

	if (!ethqos) {
		ETHQOSERR("NULL Pointer\n");
		return -EINVAL;
	}

	priv = qcom_ethqos_get_priv(ethqos);

	ret = copy_from_user(in_buf, user_buf, count);
	if (ret) {
		ETHQOSERR("unable to copy from user\n");
		return -EFAULT;
	}

	ret = sscanf(in_buf, "%x %x\n", &mmd, &offset);
	reg_value = phy_read_mmd(priv->phydev, mmd, offset);

	pr_info("PHY register device = 0x%x offset = 0x%x value:0x%x\n", mmd, offset, reg_value);

	return count;
}

static ssize_t phy_reg_write(struct file *file,
			     const char __user *user_buf,
			     size_t count,
			     loff_t *ppos)
{
	struct qcom_ethqos *ethqos = file->private_data;
	struct stmmac_priv *priv;
	int ret;
	u32 mmd = 0, offset = 0, reg_value = 0, value = 0;
	char in_buf[400];

	if (!ethqos) {
		ETHQOSERR("NULL Pointer\n");
		return -EINVAL;
	}

	priv = qcom_ethqos_get_priv(ethqos);

	ret = copy_from_user(in_buf, user_buf, count);
	if (ret) {
		ETHQOSERR("unable to copy from user\n");
		return -EFAULT;
	}

	ret = sscanf(in_buf, "%x %x %x\n", &mmd, &offset, &value);
	if (ret != 3) {
		ETHQOSERR("No of arguments are invalid. Requires device, offset, value");
		return -EINVAL;
	}

	ret = phy_write_mmd(priv->phydev, mmd, offset, value);
	if (ret < 0) {
		ETHQOSERR("Error writing mmd register");
		return -EFAULT;
	}

	reg_value = phy_read_mmd(priv->phydev, mmd, offset);
	pr_info("PHY Write cmd value In :0x%x value out :0x%x\n", value, reg_value);

	return count;
}

static ssize_t read_speed_config(struct file *file,
				 char __user *user_buf,
				 size_t count, loff_t *ppos)
{
	struct qcom_ethqos *ethqos = file->private_data;
	unsigned int len = 0, buf_len = 1000;

	len += scnprintf(buf, buf_len,
			 "Speed enforced: %d\n", ethqos->max_speed_enforce);

	return simple_read_from_buffer(user_buf, count, ppos, buf, len);
}

static ssize_t read_qos_regs(struct file *file,
			     char __user *user_buf,
			     size_t count, loff_t *ppos)
{
	struct qcom_ethqos *ethqos = file->private_data;
	struct stmmac_priv *priv;
	char *buf;
	u32 vlan = 0;
	u32 quant_weight, send_slope, high_cred, low_cred;
	u32 l3l4_ctrl = 0, l4_addr = 0, l3_addr0 = 0;
	u32 l3_addr1 = 0, l3_addr2 = 0, l3_addr3 = 0;
	int len = 0;
	int ret;
	int i;

	priv = qcom_ethqos_get_priv(ethqos);

	if (!priv->plat->has_xgmac) {
		ETHQOSERR("Operation dump_qos_regs only supported for xgmac\n");
		return -EINVAL;
	}

	buf = kzalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len += scnprintf(buf + len, count - len, "EMAC: %d\n", priv->plat->port_num);
	len += scnprintf(buf + len, count - len, "DMA MAP:\n");
	len += scnprintf(buf + len, count - len, " 0-3: %08X\n",
					 readl(priv->ioaddr + XGMAC_MTL_RXQ_DMA_MAP0));
	len += scnprintf(buf + len, count - len, "   4: %08X\n",
					 readl(priv->ioaddr + XGMAC_MTL_RXQ_DMA_MAP1));

	len += scnprintf(buf + len, count - len, "PCP MAP:\n");
	len += scnprintf(buf + len, count - len, " 0-3: %08X\n",
		readl(priv->ioaddr + XGMAC_RXQ_CTRL2));
	len += scnprintf(buf + len, count - len, "   4: %08X\n",
		readl(priv->ioaddr + XGMAC_RXQ_CTRL3));

	len += scnprintf(buf + len, count - len, "Tx Flow Control:\n");
	for (i = 0; i <= PCP_UPPER_LIMIT; i++) {
		len += scnprintf(buf + len, count - len, "%4d: %08X\n",
				 i, readl(priv->ioaddr + XGMAC_Qx_TX_FLOW_CTRL(i)));
	}

	len += scnprintf(buf + len, count - len, "Rx Flow Control\n");
	len += scnprintf(buf + len, count - len, " val: %08X\n",
			 readl(priv->ioaddr + XGMAC_RX_FLOW_CTRL));

	len += scnprintf(buf + len, count - len, "VLAN Filters:\n");
	for (i = 0; i < priv->hw->num_vlan; i++) {
		stmmac_read_vlan_filter(priv, priv->dev, priv->hw, i, &vlan);
		len += scnprintf(buf + len, count - len, "%4d: %08X\n",
						 i, vlan);
	}

	len += scnprintf(buf + len, count - len, "L3L4 Filters:\n");
	for (i = 0; i < priv->dma_cap.l3l4fnum; i++) {
		stmmac_read_l3l4_filter(priv, priv->hw, i, &l3l4_ctrl, &l4_addr, &l3_addr0,
					&l3_addr1, &l3_addr2, &l3_addr3);
		len += scnprintf(buf + len, count - len, "%4d: %08X %08X %08X %08X %08X %08X\n",
				 i, l3l4_ctrl, l4_addr, l3_addr0, l3_addr1, l3_addr2, l3_addr3);
	}

	len += scnprintf(buf + len, count - len, "CBS:\n");
	for (i = 0; i < priv->plat->tx_qos_queues_to_use; i++) {
		quant_weight = readl(priv->ioaddr + XGMAC_MTL_TCx_QUANTUM_WEIGHT(i));
		send_slope = readl(priv->ioaddr + XGMAC_MTL_TCx_SENDSLOPE(i));
		high_cred = readl(priv->ioaddr + XGMAC_MTL_TCx_HICREDIT(i));
		low_cred = readl(priv->ioaddr + XGMAC_MTL_TCx_LOCREDIT(i));

		len += scnprintf(buf + len, count - len, "%4d: %08X %08X %08X %08X\n",
						 i, quant_weight, send_slope, high_cred, low_cred);
	}

	if (len > count)
		len = count;

	ret = simple_read_from_buffer(user_buf, count, ppos, buf, len);
	kfree(buf);

	return ret;
}

static ssize_t speed_chg_handling_config(struct file *file, const char __user *user_buffer,
					 size_t count, loff_t *position)
{
	char *in_buf;
	int buf_len = 200;
	unsigned long ret;
	struct qcom_ethqos *ethqos = file->private_data;
	int speed = 0;

	in_buf = kzalloc(buf_len, GFP_KERNEL);
	if (!in_buf)
		return -ENOMEM;

	if (buf_len < count) {
		ETHQOSERR("Required buffer exceeds available limit\n");
		return -ENOMEM;
	}
	ret = copy_from_user(in_buf, user_buffer, count);
	if (ret) {
		ETHQOSERR("unable to copy from user\n");
		goto fail;
	}

	ret = sscanf(in_buf, "%d", &speed);
	if (ret != 1) {
		ETHQOSERR("Max speed param is needed\n");
		goto fail;
	}

	switch (speed) {
	case SPEED_5000:
	case SPEED_2500:
	case 0:
		ethqos->max_speed_enforce = speed;
		break;

	default:
		ETHQOSINFO("Invalid speed\n");
		goto fail;
	}

	ETHQOSINFO("Set debugfs speed to %dMbps\n", speed);

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	/*Invoke SMC call */
	if (ethqos->emac_ver == EMAC_HW_v4_0_0)
		qcom_scm_call_ethqos_configure(ethqos->rgmii_phy_base, speed, 0xFFFFFF,
					       ethqos->shm_rgmii_hsr.paddr, 0x200);
#endif

	kfree(in_buf);
	return count;

fail:
	kfree(in_buf);
	return -EIO;
}

static ssize_t read_mac_recovery_enable(struct file *filp, char __user *usr_buf,
					size_t count, loff_t *f_pos)
{
	char *buf;
	unsigned int len = 0, buf_len = 6000;
	ssize_t ret_cnt;

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "PHY_RW_rec", pethqos[0]->mac_rec_en[PHY_RW_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "PHY_DET_rec", pethqos[0]->mac_rec_en[PHY_DET_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "CRC_rec", pethqos[0]->mac_rec_en[CRC_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "RECEIVE_rec", pethqos[0]->mac_rec_en[RECEIVE_ERR]);
		len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "OVERFLOW_rec", pethqos[0]->mac_rec_en[OVERFLOW_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "FBE_rec", pethqos[0]->mac_rec_en[FBE_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "RBU_rec", pethqos[0]->mac_rec_en[RBU_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "TDU_rec", pethqos[0]->mac_rec_en[TDU_ERR]);
		len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "DRIBBLE_rec", pethqos[0]->mac_rec_en[DRIBBLE_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "WDT_rec", pethqos[0]->mac_rec_en[WDT_ERR]);

	if (len > buf_len)
		len = buf_len;

	ETHQOSDBG("%s", buf);
	ret_cnt = simple_read_from_buffer(usr_buf, count, f_pos, buf, len);
	kfree(buf);
	return ret_cnt;
}

static ssize_t ethqos_test_mac_recovery(struct file *file,
					const char __user *user_buf,
					size_t count, loff_t *ppos)
{
	unsigned char in_buf[4] = {0};
	int ret, err, chan;
	struct qcom_ethqos *ethqos = pethqos[0];

	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	if (sizeof(in_buf) < count) {
		ETHQOSERR("emac string is too long - count=%u\n", count);
		return -EFAULT;
	}

	memset(in_buf, 0,  sizeof(in_buf));
	ret = copy_from_user(in_buf, user_buf, count);

	err = in_buf[0] - '0';

	chan = in_buf[2] - '0';

	if (err < 0 || err > 9) {
		ETHQOSERR("Invalid error\n");
		return -EFAULT;
	}
	if (err != PHY_DET_ERR && err != PHY_RW_ERR) {
		if (chan < 0 || chan >= priv->plat->tx_queues_to_use) {
			ETHQOSERR("Invalid channel\n");
			return -EFAULT;
		}
	}
	if (priv->plat->handle_mac_err)
		priv->plat->handle_mac_err(priv, err, chan);

	return count;
}

static ssize_t mac_reg_read(struct file *file,
			    const char __user *buf, size_t count, loff_t *ppos)
{
	char in_buf[400];
	u32 offset = 0;
	unsigned long ret;
	struct qcom_ethqos *ethqos = file->private_data;
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	ret = copy_from_user(in_buf, buf, count);
	if (ret) {
		pr_err("%s: unable tocopyfromuser\n", __func__);
		return -EFAULT;
	}

	ret = sscanf(in_buf, "%x", &offset);

	if (offset % 4 != 0) {
		pr_err("offset is invalid\n");
		return -EINVAL;
	}

	pr_info("0X%x\n", readl(priv->ioaddr + offset));
	return count;
}

static ssize_t pcs_reg_read(struct file *file,
			    const char __user *buf, size_t count, loff_t *ppos)
{
	char in_buf[400];
	u32 offset = 0;
	unsigned long ret;
	struct qcom_ethqos *ethqos = file->private_data;
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	ret = copy_from_user(in_buf, buf, count);
	if (ret) {
		pr_err("%s: unable tocopyfromuser\n", __func__);
		return -EFAULT;
	}

	ret = sscanf(in_buf, "%x", &offset);

	if (offset % 4 != 0) {
		pr_err("offset is invalid\n");
		return -EINVAL;
	}

	pr_info("0X%x\n", readl(priv->hw->qxpcs->addr + offset));
	return count;
}

static ssize_t iomacro_reg_read(struct file *file,
				const char __user *buf, size_t count, loff_t *ppos)
{
	char in_buf[400];
	u32 offset = 0;
	unsigned long ret;
	struct qcom_ethqos *ethqos = file->private_data;

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
		qcom_scm_call_iomacro_dump(ethqos->rgmii_phy_base,
					   ethqos->shm_rgmii_local.paddr,
					   RGMII_BLOCK_SIZE);
		qtee_shmbridge_inv_shm_buf(&ethqos->shm_rgmii_local);
	}
#endif

	ret = copy_from_user(in_buf, buf, count);
	if (ret) {
		pr_err("%s: unable tocopyfromuser\n", __func__);
		return -EFAULT;
	}

	ret = sscanf(in_buf, "%x", &offset);

	if (offset % 4 != 0) {
		pr_err("offset is invalid\n");
		return -EINVAL;
	}

	pr_info("0X%x\n", rgmii_readl(ethqos, offset));
	return count;
}

static ssize_t serdes_reg_read(struct file *file,
			       const char __user *buf, size_t count, loff_t *ppos)
{
	char in_buf[400];
	u32 offset = 0;
	unsigned long ret;
	struct qcom_ethqos *ethqos = file->private_data;

	ret = copy_from_user(in_buf, buf, count);
	if (ret) {
		pr_err("%s: unable tocopyfromuser\n", __func__);
		return -EFAULT;
	}

	ret = sscanf(in_buf, "%x", &offset);

	if (offset % 4 != 0) {
		pr_err("offset is invalid\n");
		return -EINVAL;
	}

	pr_info("0X%x\n", readl(ethqos->sgmii_base + offset));
	return count;
}

static ssize_t mac_reg_write(struct file *file,
			     const char __user *buf, size_t count, loff_t *ppos)
{
	char in_buf[400];
	u32 offset = 0;
	u32 value = 0;
	unsigned long ret;
	struct qcom_ethqos *ethqos = file->private_data;
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	ret = copy_from_user(in_buf, buf, count);
	if (ret) {
		pr_err("%s: unable to copyfromuser\n", __func__);
		return -EFAULT;
	}

	ret = sscanf(in_buf, "%x %x", &offset, &value);
	if (ret != 2) {
		pr_err("number of arguments are invalid, operation requires both offset and value\n");
		return -EINVAL;
	}

	if (offset % 4 != 0) {
		pr_err("offset is invalid\n");
		return -EINVAL;
	}

	pr_info("Old value: 0X%x\n", readl(priv->ioaddr + offset));
	writel(value, priv->ioaddr + offset);
	pr_info("New Value: 0X%x\n", readl(priv->ioaddr + offset));

	return count;
}

static ssize_t pcs_reg_write(struct file *file,
			     const char __user *buf, size_t count, loff_t *ppos)
{
	char in_buf[400];
	u32 offset = 0;
	u32 value = 0;
	unsigned long ret;
	struct qcom_ethqos *ethqos = file->private_data;
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	ret = copy_from_user(in_buf, buf, count);
	if (ret) {
		pr_err("%s: unable to copyfromuser\n", __func__);
		return -EFAULT;
	}

	ret = sscanf(in_buf, "%x %x", &offset, &value);
	if (ret != 2) {
		pr_err("number of arguments are invalid, operation requires both offset and value\n");
		return -EINVAL;
	}

	if (offset % 4 != 0) {
		pr_err("offset is invalid\n");
		return -EINVAL;
	}

	pr_info("Old Value: 0X%x\n", readl(priv->hw->qxpcs->addr + offset));
	writel(value, priv->hw->qxpcs->addr + offset);
	pr_info("New Value: 0X%x\n", readl(priv->hw->qxpcs->addr + offset));

	return count;
}

static ssize_t serdes_reg_write(struct file *file,
				const char __user *buf, size_t count, loff_t *ppos)
{
	char in_buf[400];
	u32 offset = 0;
	u32 value = 0;
	unsigned long ret;
	struct qcom_ethqos *ethqos = file->private_data;

	ret = copy_from_user(in_buf, buf, count);
	if (ret) {
		pr_err("%s: unable to copyfromuser\n", __func__);
		return -EFAULT;
	}

	ret = sscanf(in_buf, "%x %x", &offset, &value);
	if (ret != 2) {
		pr_err("number of arguments are invalid, operation requires both offset and value\n");
		return -EINVAL;
	}

	if (offset % 4 != 0) {
		pr_err("offset is invalid\n");
		return -EINVAL;
	}

	pr_info("Old Value: 0X%x\n", readl_relaxed(ethqos->sgmii_base + offset));
	writel_relaxed(value, ethqos->sgmii_base + offset);
	pr_info("New Value: 0X%x\n", readl_relaxed(ethqos->sgmii_base + offset));

	return count;
}

static const struct file_operations fops_phy_reg_read = {
	.write = phy_reg_read,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_phy_reg_write = {
	.write = phy_reg_write,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_enforce_speed = {
	.read = read_speed_config,
	.write = speed_chg_handling_config,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_dump_qos_regs = {
	.read = read_qos_regs,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_mac_rec = {
	.read = read_mac_recovery_enable,
	.write = ethqos_test_mac_recovery,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_mac_read = {
	.write = mac_reg_read,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_mac_pcs_read = {
	.write = pcs_reg_read,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_mac_iomacro_read = {
	.write = iomacro_reg_read,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_mac_serdes_dump = {
	.write = serdes_reg_read,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_mac_pcs_write = {
	.write = pcs_reg_write,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_mac_serdes_write = {
	.write = serdes_reg_write,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static const struct file_operations fops_mac_write = {
	.write = mac_reg_write,
	.open = simple_open,
	.owner = THIS_MODULE,
	.llseek = default_llseek,
};

static int create_debufs_for_sgmii_usxgmii(struct qcom_ethqos *ethqos)
{
	struct stmmac_priv *priv;
	static struct dentry *mac_serdes_dump;
	static struct dentry *mac_pcs_write;
	static struct dentry *mac_serdes_write;
	static struct dentry *mac_pcs_dump;

	priv = qcom_ethqos_get_priv(ethqos);

	mac_pcs_dump = debugfs_create_file("pcs_reg_read", (0400),
					   ethqos->debugfs_dir, ethqos,
					   &fops_mac_pcs_read);
	if (!mac_pcs_dump || IS_ERR(mac_pcs_dump)) {
		ETHQOSERR("Cannot create debugfs mac_pcs_dump %x\n",
			  mac_pcs_dump);
		goto fail;
	}

	mac_pcs_write = debugfs_create_file("pcs_reg_write", (0220),
					    ethqos->debugfs_dir, ethqos,
					    &fops_mac_pcs_write);
	if (!mac_pcs_write || IS_ERR(mac_pcs_write)) {
		ETHQOSERR("Cannot create debugfs mac_pcs_write %x\n",
			  mac_pcs_write);
		goto fail;
	}

	mac_serdes_write = debugfs_create_file("serdes_reg_write", (0220),
					       ethqos->debugfs_dir, ethqos,
					       &fops_mac_serdes_write);
	if (!mac_serdes_write || IS_ERR(mac_serdes_write)) {
		ETHQOSERR("Cannot create debugfs mac_pcs_write %x\n",
			  mac_serdes_write);
		goto fail;
	}

	mac_serdes_dump = debugfs_create_file("serdes_reg_read", (0400),
					      ethqos->debugfs_dir, ethqos,
					      &fops_mac_serdes_dump);
	if (!mac_serdes_dump || IS_ERR(mac_serdes_dump)) {
		ETHQOSERR("Cannot create debugfs mac_serdes_dump %x\n",
			  mac_serdes_dump);
		goto fail;
	}

	return 0;

fail:
	return -ENOMEM;
}

static int ethqos_create_debugfs(struct qcom_ethqos        *ethqos)
{
	static struct dentry *enforce_max_speed;
	static struct dentry *mac_dump;
	static struct dentry *mac_iomacro_dump;
	static struct dentry *mac_write;
	static struct dentry *phy_reg_read;
	static struct dentry *phyreg_write;
	static struct dentry *dump_qos_regs;
	struct stmmac_priv *priv;
	char dir_name[32];
	struct net_device *netdev;
	static struct dentry *mac_rec;

	if (!ethqos) {
		ETHQOSERR("Null Param %s\n", __func__);
		return -ENOMEM;
	}
	netdev = platform_get_drvdata(ethqos->pdev);

	priv = qcom_ethqos_get_priv(ethqos);

	snprintf(dir_name, sizeof(dir_name), "%s%d", "eth", priv->plat->port_num);

	ethqos->debugfs_dir = debugfs_create_dir(dir_name, NULL);

	if (!ethqos->debugfs_dir || IS_ERR(ethqos->debugfs_dir)) {
		ETHQOSERR("Can't create debugfs dir\n");
		return -ENOMEM;
	}

	mac_dump = debugfs_create_file("mac_reg_read", (0400),
				       ethqos->debugfs_dir, ethqos, &fops_mac_read);
	if (!mac_dump || IS_ERR(mac_dump)) {
		ETHQOSERR("Cannot create debugfs mac_dump %x\n",
			  mac_dump);
		goto fail;
	}

	mac_iomacro_dump = debugfs_create_file("iomacro_reg_read", (0400),
					       ethqos->debugfs_dir, ethqos, &fops_mac_iomacro_read);
	if (!mac_iomacro_dump || IS_ERR(mac_iomacro_dump)) {
		ETHQOSERR("Cannot create debugfs mac_iomacro_dump %x\n",
			  mac_iomacro_dump);
		goto fail;
	}

	mac_write = debugfs_create_file("mac_reg_write", (0220),
					ethqos->debugfs_dir, ethqos, &fops_mac_write);
	if (!mac_write || IS_ERR(mac_write)) {
		ETHQOSERR("Cannot create debugfs mac_write %x\n",
			  mac_write);
		goto fail;
	}
	phy_reg_read = debugfs_create_file("phy_reg_read", (0660),
					   ethqos->debugfs_dir, ethqos, &fops_phy_reg_read);
	if (!phy_reg_read || IS_ERR(phy_reg_read)) {
		ETHQOSERR("Cannot create debugfs phy_reg_read %x\n",
			  mac_write);
		goto fail;
	}

	phyreg_write = debugfs_create_file("phy_reg_write", (0220),
					   ethqos->debugfs_dir, ethqos, &fops_phy_reg_write);
	if (!phyreg_write || IS_ERR(phyreg_write)) {
		ETHQOSERR("Cannot create debugfs phy_reg_write %x\n",
			  phyreg_write);
		goto fail;
	}

	if (priv->plat->interface == PHY_INTERFACE_MODE_USXGMII ||
	    priv->plat->interface == PHY_INTERFACE_MODE_SGMII ||
	    priv->plat->interface == PHY_INTERFACE_MODE_2500BASEX ||
	    priv->plat->interface == PHY_INTERFACE_MODE_5GBASER) {
		if (create_debufs_for_sgmii_usxgmii(ethqos))
			goto fail;
	}

	if (priv->plat->mac_err_rec) {
		mac_rec = debugfs_create_file("test_mac_recovery", 0400,
					      ethqos->debugfs_dir, ethqos,
					      &fops_mac_rec);
		if (!mac_rec || IS_ERR(mac_rec)) {
			ETHQOSERR("Can't create mac_rec directory");
			goto fail;
		}
	}

	enforce_max_speed = debugfs_create_file("enforce_max_speed", 0400,
						ethqos->debugfs_dir, ethqos,
						&fops_enforce_speed);
	if (!enforce_max_speed || IS_ERR(enforce_max_speed)) {
		ETHQOSERR("Can't create enforce_max_speed %d\n",
			  (long)enforce_max_speed);
		goto fail;
	}

	dump_qos_regs = debugfs_create_file("dump_qos_regs", 0400,
					    ethqos->debugfs_dir, ethqos,
					    &fops_dump_qos_regs);
	if (!dump_qos_regs || IS_ERR(dump_qos_regs)) {
		ETHQOSERR("Can't create dump_qos_regs %d\n",
			  (long)dump_qos_regs);
		goto fail;
	}

	return 0;

fail:
	debugfs_remove_recursive(ethqos->debugfs_dir);
	return -ENOMEM;
}
#endif /* CONFIG_DEBUG_FS */

static ssize_t read_phy_reg_dump_sysfs(struct device *dev,
				       struct device_attribute *attr,  char *user_buf)
{
	struct device *parent = NULL;
	struct net_device *netdev;
	unsigned int len = 0, buf_len = 2000;
	char *buf;
	int phydata = 0;
	int i = 0;
	struct stmmac_priv *priv;

	parent = kobj_to_dev(dev->kobj.parent);

	netdev = to_net_dev(parent);
	if (!netdev) {
		pr_err("netdev is NULL\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		pr_err("priv is NULL\n");
		return -EINVAL;
	}

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len += scnprintf(buf + len, buf_len - len,
					 "\n************* PHY Reg dump *************\n");

	for (i = 0; i < 32; i++) {
		phydata = priv->mii->read(priv->mii, priv->plat->phy_addr, i);
		len += scnprintf(buf + len, buf_len - len,
					 "MII Register (%#x) = %#x\n",
					 i, phydata);
	}

	if (len > buf_len) {
		ETHQOSERR("(len > buf_len) buffer not sufficient\n");
		len = buf_len;
	}

	memcpy(user_buf, buf, len);
	kfree(buf);
	return len;
}

static DEVICE_ATTR(phy_reg_dump, ETHQOS_SYSFS_DEV_ATTR_PERMS,
		   read_phy_reg_dump_sysfs, NULL);

static ssize_t read_rgmii_reg_dump_sysfs(struct device *dev,
					 struct device_attribute *attr, char *user_buf)
{
	struct device *parent = NULL;
	struct qcom_ethqos *ethqos;
	struct net_device *netdev;
	struct stmmac_priv *priv;
	unsigned int len = 0, buf_len = 2000;
	char *buf;
	int rgmii_data = 0;

	parent = kobj_to_dev(dev->kobj.parent);
	netdev = to_net_dev(parent);
	if (!netdev) {
		pr_err("netdev is NULL\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		pr_err("priv is NULL\n");
		return -EINVAL;
	}

	ethqos = (struct qcom_ethqos *)priv->plat->bsp_priv;
	if (!ethqos) {
		pr_err("ethqos is NULL\n");
		return -EINVAL;
	}

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	/* Reg_dump should have the whole IO macro reg space so below copy to buffer has to
	 * be done based on offset in the reg_dump unlike invoking register read for every
	 * config as below
	 */
	if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
		qcom_scm_call_iomacro_dump(ethqos->rgmii_phy_base, ethqos->shm_rgmii_local.paddr,
					   RGMII_BLOCK_SIZE);
		qtee_shmbridge_inv_shm_buf(&ethqos->shm_rgmii_local);
	}
#endif

	len += scnprintf(buf + len, buf_len - len,
					 "\n************* RGMII Reg dump *************\n");
	rgmii_data = rgmii_readl(ethqos, RGMII_IO_MACRO_CONFIG);
	len += scnprintf(buf + len, buf_len - len,
					 "RGMII_IO_MACRO_CONFIG Register = %#x\n",
					 rgmii_data);
	rgmii_data = rgmii_readl(ethqos, SDCC_HC_REG_DLL_CONFIG);
	len += scnprintf(buf + len, buf_len - len,
					 "SDCC_HC_REG_DLL_CONFIG Register = %#x\n",
					 rgmii_data);
	rgmii_data = rgmii_readl(ethqos, SDCC_HC_REG_DDR_CONFIG);
	len += scnprintf(buf + len, buf_len - len,
					 "SDCC_HC_REG_DDR_CONFIG Register = %#x\n",
					 rgmii_data);
	rgmii_data = rgmii_readl(ethqos, SDCC_HC_REG_DLL_CONFIG2);
	len += scnprintf(buf + len, buf_len - len,
					 "SDCC_HC_REG_DLL_CONFIG2 Register = %#x\n",
					 rgmii_data);
	rgmii_data = rgmii_readl(ethqos, SDC4_STATUS);
	len += scnprintf(buf + len, buf_len - len,
					 "SDC4_STATUS Register = %#x\n",
					 rgmii_data);
	rgmii_data = rgmii_readl(ethqos, SDCC_USR_CTL);
	len += scnprintf(buf + len, buf_len - len,
					 "SDCC_USR_CTL Register = %#x\n",
					 rgmii_data);
	rgmii_data = rgmii_readl(ethqos, RGMII_IO_MACRO_CONFIG2);
	len += scnprintf(buf + len, buf_len - len,
					 "RGMII_IO_MACRO_CONFIG2 Register = %#x\n",
					 rgmii_data);
	rgmii_data = rgmii_readl(ethqos, RGMII_IO_MACRO_DEBUG1);
	len += scnprintf(buf + len, buf_len - len,
					 "RGMII_IO_MACRO_DEBUG1 Register = %#x\n",
					 rgmii_data);
	rgmii_data = rgmii_readl(ethqos, EMAC_SYSTEM_LOW_POWER_DEBUG);
	len += scnprintf(buf + len, buf_len - len,
					 "EMAC_SYSTEM_LOW_POWER_DEBUG Register = %#x\n",
					 rgmii_data);

	if (len > buf_len) {
		ETHQOSERR("(len > buf_len) buffer not sufficient\n");
		len = buf_len;
	}

	memcpy(user_buf, buf, len);
	kfree(buf);
	return len;
}

static DEVICE_ATTR(rgmii_reg_dump, ETHQOS_SYSFS_DEV_ATTR_PERMS,
		   read_rgmii_reg_dump_sysfs, NULL);

static ssize_t nw_loopback_handling_config_sysfs(struct device *dev,
						 struct device_attribute *attr,
						 const char *user_buffer, size_t count)
{
	char *in_buf = kstrdup(user_buffer, GFP_KERNEL);
	unsigned long ret;
	int config = 0;
	struct device *parent = NULL;
	struct qcom_ethqos *ethqos;
	struct net_device *netdev;
	struct stmmac_priv *priv;

	parent = kobj_to_dev(dev->kobj.parent);

	netdev = to_net_dev(parent);
	if (!netdev) {
		pr_err("netdev is NULL\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		pr_err("priv is NULL\n");
		return -EINVAL;
	}

	ethqos = (struct qcom_ethqos *)priv->plat->bsp_priv;
	if (!ethqos) {
		pr_err("ethqos is NULL\n");
		return -EINVAL;
	}

	if (!in_buf) {
		ETHQOSERR("Error in allocating memory for in_buf\n");
		return -EINVAL;
	}

	ret = sscanf(in_buf, "%d", &config);
	if (ret != 1) {
		ETHQOSERR("Error in reading option from user");
		return -EINVAL;
	}

	if (priv->loopback_direction == HOST_LOOPBACK_MODE) {
		ETHQOSERR("Not enabling network loopback since host loopback mode is enabled\n");
		return -EINVAL;
	}

	if (config < DISABLE_NW_LOOPBACK || config > ENABLE_PCS_NW_LOOPBACK) {
		ETHQOSERR("Invalid config =%d\n", config);
		return -EINVAL;
	}

	if (priv->plat->interface != PHY_INTERFACE_MODE_SGMII &&
	    priv->plat->interface != PHY_INTERFACE_MODE_USXGMII &&
	    priv->plat->interface != PHY_INTERFACE_MODE_5GBASER) {
		ETHQOSERR("PCS loopback is not supported for the interface type enabled\n");
		return -EINVAL;
	}

	if (config == priv->current_loopback) {
		switch (config) {
		case DISABLE_NW_LOOPBACK:
			ETHQOSINFO("Network loopback is already disabled\n");
			break;
		case ENABLE_PCS_NW_LOOPBACK:
			ETHQOSINFO("Loopback is already Enabled as ");
			ETHQOSINFO("PCS network loopback\n");
			break;
		}
		return -EINVAL;
	}

	ethqos_pcs_loopback(ethqos, config);

	priv->current_loopback = config;

	if (priv->current_loopback > DISABLE_NW_LOOPBACK)
		priv->loopback_direction = NETWORK_LOOPBACK_MODE;
	else
		priv->loopback_direction = DISABLE_NW_LOOPBACK;

	kfree(in_buf);

	return count;
}

static ssize_t loopback_handling_config_sysfs(struct device *dev,
					      struct device_attribute *attr,
					      const char *user_buffer, size_t count)
{
	unsigned long ret;
	int config = 0;
	int speed = 0;
	struct device *parent = NULL;
	struct qcom_ethqos *ethqos;
	struct net_device *netdev;
	struct stmmac_priv *priv;

	parent = kobj_to_dev(dev->kobj.parent);
	netdev = to_net_dev(parent);
	if (!netdev) {
		pr_err("netdev is NULL\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		pr_err("priv is NULL\n");
		return -EINVAL;
	}

	ethqos = (struct qcom_ethqos *)priv->plat->bsp_priv;
	if (!ethqos) {
		pr_err("ethqos is NULL\n");
		return -EINVAL;
	}

	ret = loopback_arg_parse(ethqos, user_buffer, &config, &speed);
	if (ret) {
		ETHQOSERR("Bad arguments\n");
		return -EINVAL;
	}

	switch (config) {
	case DISABLE_LOOPBACK:
		ETHQOSINFO("Disabled loopback, config = %d\n", config);
		break;
	default:
		ETHQOSINFO("enable loopback = %d with link speed = %d backup now\n",
			   config, speed);
		break;
	}

	/*Backup speed & duplex and disable power saving before Enabling Loopback */
	if (priv->current_loopback == DISABLE_LOOPBACK &&
	    config > DISABLE_LOOPBACK) {
		/*Backup old speed & duplex*/
		if (priv->dev->phydev) {
			ethqos->backup_speed = priv->speed;
			ethqos->backup_duplex = priv->dev->phydev->duplex;
		} else {
			ethqos->backup_speed = SPEED_UNKNOWN;
			ethqos->backup_duplex = DUPLEX_UNKNOWN;
		}
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
		if (priv->plat->enable_power_saving)
			priv->plat->enable_power_saving(netdev, false);
#endif
	}

	if (config == DISABLE_LOOPBACK)
		setup_config_registers(ethqos, ethqos->backup_speed,
				       ethqos->backup_duplex, 0);
	else
		setup_config_registers(ethqos, speed, DUPLEX_FULL, config);

	switch (config) {
	case DISABLE_LOOPBACK:
		ETHQOSINFO("Request to Disable Loopback\n");
		if (priv->current_loopback == ENABLE_IO_MACRO_LOOPBACK)
			ethqos_rgmii_io_macro_loopback(ethqos, 0);
		else if (priv->current_loopback == ENABLE_MAC_LOOPBACK)
			ethqos_mac_loopback(ethqos, 0);
		else if (priv->current_loopback == ENABLE_PHY_LOOPBACK)
			phy_digital_loopback_config(ethqos,
						    ethqos->backup_speed, 0);
		else if (priv->current_loopback == ENABLE_SERDES_LOOPBACK)
			ethqos_serdes_loopback(ethqos, ethqos->backup_speed, 0);
		break;
	case ENABLE_IO_MACRO_LOOPBACK:
		ETHQOSINFO("Request to Enable IO MACRO LOOPBACK\n");
		ethqos_rgmii_io_macro_loopback(ethqos, 1);
		break;
	case ENABLE_MAC_LOOPBACK:
		ETHQOSINFO("Request to Enable MAC LOOPBACK\n");
		ethqos_mac_loopback(ethqos, 1);
		break;
	case ENABLE_PHY_LOOPBACK:
		ETHQOSINFO("Request to Enable PHY LOOPBACK\n");
		ethqos->loopback_speed = speed;
		phy_digital_loopback_config(ethqos, speed, 1);
		break;
	case ENABLE_SERDES_LOOPBACK:
		ETHQOSINFO("Request to Enable SERDES LOOPBACK\n");
		ethqos_serdes_loopback(ethqos, speed, 1);
		break;
	default:
		ETHQOSINFO("Invalid Loopback=%d\n", config);
		break;
	}

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	if (priv->plat->enable_power_saving && config == DISABLE_LOOPBACK)
		priv->plat->enable_power_saving(netdev, true);
#endif

	priv->current_loopback = config;

	if (priv->current_loopback > DISABLE_LOOPBACK)
		priv->loopback_direction = HOST_LOOPBACK_MODE;
	else
		priv->loopback_direction = DISABLE_NW_LOOPBACK;

	return count;
}

static ssize_t read_nw_loopback_config_sysfs(struct device *dev,
					     struct device_attribute *attr,
					     char *user_buf)
{
	unsigned int len = 0, buf_len = 2000;
	struct device *parent = NULL;
	struct net_device *netdev;
	struct stmmac_priv *priv;

	parent = kobj_to_dev(dev->kobj.parent);
	netdev = to_net_dev(parent);
	if (!netdev) {
		pr_err("netdev is NULL\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		pr_err("priv is NULL\n");
		return -EINVAL;
	}

	if (priv->loopback_direction == HOST_LOOPBACK_MODE) {
		len += scnprintf(buf + len, buf_len - len,
		 "Host loopback mode is enabled, NW loopback mode is disabled\n");
		goto out;
	}

	if (priv->current_loopback == DISABLE_NW_LOOPBACK)
		len += scnprintf(buf + len, buf_len - len,
				 "Network loopback is Disabled\n");
	else if (priv->current_loopback == ENABLE_PCS_NW_LOOPBACK)
		len += scnprintf(buf + len, buf_len - len,
				 "Current Loopback is PCS network LOOPBACK\n");
	else
		len += scnprintf(buf + len, buf_len - len,
				 "Invalid LOOPBACK Config\n");

out:
	if (len > buf_len)
		len = buf_len;

	memcpy(user_buf, buf, len);

	return len;
}

static DEVICE_ATTR(nw_loopback_enable, ETHQOS_SYSFS_DEV_ATTR_PERMS,
		   read_nw_loopback_config_sysfs, nw_loopback_handling_config_sysfs);

static ssize_t read_loopback_config_sysfs(struct device *dev,
					  struct device_attribute *attr,
					  char *user_buf)
{
	unsigned int len = 0, buf_len = 2000;
	struct device *parent = NULL;
	struct net_device *netdev;
	struct stmmac_priv *priv;

	parent = kobj_to_dev(dev->kobj.parent);
	netdev = to_net_dev(parent);
	if (!netdev) {
		pr_err("netdev is NULL\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		pr_err("priv is NULL\n");
		return -EINVAL;
	}

	if (priv->loopback_direction == NETWORK_LOOPBACK_MODE) {
		len += scnprintf(buf + len, buf_len - len,
		 "NW loopback mode is enabled, Host loopback mode is disabled\n");
		goto out;
	}

	if (priv->current_loopback == DISABLE_LOOPBACK)
		len += scnprintf(buf + len, buf_len - len,
				 "Loopback is Disabled\n");
	else if (priv->current_loopback == ENABLE_IO_MACRO_LOOPBACK)
		len += scnprintf(buf + len, buf_len - len,
				 "Current Loopback is IO MACRO LOOPBACK\n");
	else if (priv->current_loopback == ENABLE_MAC_LOOPBACK)
		len += scnprintf(buf + len, buf_len - len,
				 "Current Loopback is MAC LOOPBACK\n");
	else if (priv->current_loopback == ENABLE_PHY_LOOPBACK)
		len += scnprintf(buf + len, buf_len - len,
				 "Current Loopback is PHY LOOPBACK\n");
	else if (priv->current_loopback == ENABLE_SERDES_LOOPBACK)
		len += scnprintf(buf + len, buf_len - len,
				 "Current Loopback is SERDES LOOPBACK\n");
	else
		len += scnprintf(buf + len, buf_len - len,
				 "Invalid LOOPBACK Config\n");

out:
	if (len > buf_len)
		len = buf_len;

	memcpy(user_buf, buf, len);

	return len;
}

static DEVICE_ATTR(loopback_enable_mode, ETHQOS_SYSFS_DEV_ATTR_PERMS,
		   read_loopback_config_sysfs, loopback_handling_config_sysfs);

static ssize_t show_icb_read(struct device *dev, struct device_attribute *attr, char *user_buf)
{
	unsigned int len = 0, buf_len = 2000;
	char *buf;
	struct device *parent = NULL;
	struct qcom_ethqos *ethqos;
	struct net_device *netdev;
	struct stmmac_priv *priv;

	parent = kobj_to_dev(dev->kobj.parent);
	netdev = to_net_dev(parent);
	if (!netdev) {
		pr_err("netdev is NULL\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		pr_err("priv is NULL\n");
		return -EINVAL;
	}

	ethqos = (struct qcom_ethqos *)priv->plat->bsp_priv;
	if (!ethqos) {
		pr_err("ethqos is NULL\n");
		return -EINVAL;
	}

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (ethqos->emac_axi_icc && ethqos->emac_apb_icc) {
		len += scnprintf(buf + len, buf_len - len, "Current Speed in Mbps: %d\n",
				 ethqos->speed);
		len += scnprintf(buf + len, buf_len - len, "ICB vote_idx : %d\n",
				 ethqos->vote_idx);
		len += scnprintf(buf + len, buf_len - len,
				 "Master-Emac -> AXI peak BW vote in KBps: %d\n",
				 ethqos->emac_axi_icc[ethqos->vote_idx].peak_bandwidth);
		len += scnprintf(buf + len, buf_len - len,
				 "Master-Emac -> AXI average BW vote in KBps: %d\n",
				 ethqos->emac_axi_icc[ethqos->vote_idx].average_bandwidth);
		len += scnprintf(buf + len, buf_len - len,
				 "Slave-Emac -> APB peak BW vote in KBps: %d\n",
				 ethqos->emac_apb_icc[ethqos->vote_idx].peak_bandwidth);
	}

	memcpy(user_buf, buf, len);
	kfree(buf);
	return len;
}

static DEVICE_ATTR(icb_dump, ETHQOS_SYSFS_DEV_ATTR_PERMS, show_icb_read, NULL);

static ssize_t show_ipc_stmmac_log_ctxt_low(struct device *dev,
					    struct device_attribute *attr, char *user_buf)
{
	return 0;
}

static ssize_t store_ipc_stmmac_log_ctxt_low(struct device *dev, struct device_attribute *attr,
					     const char *user_buf, size_t count)
{
	int tmp = 0;

	if (sscanf(user_buf, "%du", &tmp) < 0) {
		ETHQOSERR("sscanf failed\n");
		return -EINVAL;
	} else {
		if (tmp) {
			if (!ipc_stmmac_log_ctxt_low) {
				ipc_stmmac_log_ctxt_low =
				ipc_log_context_create(IPCLOG_STATE_PAGES,
						       "stmmac_low", 0);
			}
			if (!ipc_stmmac_log_ctxt_low) {
				ETHQOSERR("failed to create ipc stmmac low context\n");
				return -EFAULT;
			}
		} else {
			if (ipc_stmmac_log_ctxt_low)
				ipc_log_context_destroy(ipc_stmmac_log_ctxt_low);
			ipc_stmmac_log_ctxt_low = NULL;
		}
	}

	stmmac_enable_ipc_low = tmp;

	return count;
}

static DEVICE_ATTR(ipc_stmmac_log_low, ETHQOS_SYSFS_DEV_ATTR_PERMS,
		   show_ipc_stmmac_log_ctxt_low,
		   store_ipc_stmmac_log_ctxt_low);

static ssize_t show_tc_queue_select_sysfs(struct device *dev,
					  struct device_attribute *attr, char *user_buf)
{
	return 0;
}

static ssize_t store_tc_queue_select_sysfs(struct device *dev, struct device_attribute *attr,
					   const char *user_buf, size_t count)
{
	struct device *parent = NULL;
	struct net_device *netdev;
	struct stmmac_priv *priv;
	int tmp;

	parent = kobj_to_dev(dev->kobj.parent);
	netdev = to_net_dev(parent);
	if (!netdev) {
		pr_err("netdev is NULL\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		pr_err("priv is NULL\n");
		return -EINVAL;
	}

	if (sscanf(user_buf, "%du", &tmp) < 0) {
		ETHQOSERR("sscanf failed\n");
		return -EINVAL;
	}

	if (tmp) {
		ETHQOSINFO(" enabling TC based queue selection");
		priv->plat->tc_select_queue = true;
	} else {
		ETHQOSINFO(" disabling TC based queue selection");
		priv->plat->tc_select_queue = false;
	}
	return count;
}

static DEVICE_ATTR(tc_queue_select, ETHQOS_SYSFS_DEV_ATTR_PERMS,
		   show_tc_queue_select_sysfs, store_tc_queue_select_sysfs);

static int ethqos_create_sysfs_nodes(struct qcom_ethqos *ethqos)
{
	struct stmmac_priv *priv;
	struct net_device *netdev;
	int ret;

	if (!ethqos) {
		ETHQOSERR("Null Param %s\n", __func__);
		return -ENOMEM;
	}
	netdev = platform_get_drvdata(ethqos->pdev);
	if (!netdev) {
		ETHQOSERR("%s: netdev is NULL\n", __func__);
		return -EINVAL;
	}
	priv = qcom_ethqos_get_priv(ethqos);
	if (!priv) {
		ETHQOSERR("%s: priv is NULL\n", __func__);
		return -EINVAL;
	}

	ethqos->sysfs_kobj = kobject_create_and_add("ethqos", &netdev->dev.kobj);
	if (!ethqos->sysfs_kobj) {
		ETHQOSERR("Unable to create ethqos kobject\n");
		return -EFAULT;
	}

	ret = sysfs_create_file(ethqos->sysfs_kobj, &dev_attr_phy_reg_dump.attr);
	if (ret) {
		ETHQOSERR("unable to create sysfs phy_reg_dump node\n");
		goto fail;
	}

	ret = sysfs_create_file(ethqos->sysfs_kobj, &dev_attr_rgmii_reg_dump.attr);
	if (ret) {
		ETHQOSERR("unable to create sysfs rgmii_reg_dump node\n");
		goto fail;
	}

	ret = sysfs_create_file(ethqos->sysfs_kobj, &dev_attr_icb_dump.attr);
	if (ret) {
		ETHQOSERR("unable to create sysfs icb_dump node\n");
		goto fail;
	}

	ret = sysfs_create_file(ethqos->sysfs_kobj, &dev_attr_ipc_stmmac_log_low.attr);
	if (ret) {
		ETHQOSERR("unable to create sysfs ipc_stmmac_log_low node\n");
		goto fail;
	}

	ret = sysfs_create_file(ethqos->sysfs_kobj, &dev_attr_loopback_enable_mode.attr);
	if (ret) {
		ETHQOSERR("unable to create sysfs loopback_enable_mode node\n");
		goto fail;
	}

	ret = sysfs_create_file(ethqos->sysfs_kobj, &dev_attr_nw_loopback_enable.attr);
	if (ret) {
		ETHQOSERR("unable to create sysfs nw_loopback_enable node\n");
		goto fail;
	}

	ret = sysfs_create_file(ethqos->sysfs_kobj, &dev_attr_tc_queue_select.attr);
	if (ret) {
		ETHQOSERR("unable to create sysfs tc_queue_select node\n");
		goto fail;
	}

	return 0;

fail:
	kobject_put(ethqos->sysfs_kobj);
	return -ENOMEM;
}

static ssize_t read_ethqos_rx_clock(struct device *dev,
				    struct device_attribute *attr,
				    char *user_buf)
{
	struct net_device *netdev = to_net_dev(dev);
	struct stmmac_priv *priv;
	unsigned int BUFF_SIZE = 2000;
	struct plat_stmmacenet_data *plat;

	if (!netdev) {
		ETHQOSERR("NULL Pointer\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	plat = priv->plat;

	if (!plat->rx_clk_rdy)
		return scnprintf(user_buf, BUFF_SIZE,
				"Rx clock is not ready");
	else
		return scnprintf(user_buf, BUFF_SIZE,
				"Rx clock is ready");
}

static ssize_t write_ethqos_rx_clock(struct device *dev,
				     struct device_attribute *attr,
				     const char *user_buffer,
				     size_t count)
{
	s8 input = 0;
	struct net_device *netdev = to_net_dev(dev);
	struct stmmac_priv *priv;
	struct qcom_ethqos *ethqos;
	struct plat_stmmacenet_data *plat;

	if (!netdev) {
		ETHQOSERR("NULL Pointer\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	ethqos = priv->plat->bsp_priv;

	plat = priv->plat;

	if (kstrtos8(user_buffer, 0, &input))
		return -EFAULT;

	if (input != 0 && input != 1) {
		ETHQOSERR("invalid input\n");
		return -EINVAL;
	}

	if (input && !plat->rx_clk_rdy) {
		plat->rx_clk_rdy = true;
		rtnl_lock();
		phylink_resume(priv->phylink);
		rtnl_unlock();
	} else if (!input && plat->rx_clk_rdy) {
		plat->rx_clk_rdy = false;
		rtnl_lock();
		phylink_stop(priv->phylink);
		rtnl_unlock();
	}

	return count;
}

static DEVICE_ATTR(rx_clock_rdy, RX_CLK_SYSFS_DEV_ATTR_PERMS,
		   read_ethqos_rx_clock,
		   write_ethqos_rx_clock);

static ssize_t read_phy_off(struct device *dev, struct device_attribute *attr, char *user_buf)
{
	struct net_device *netdev = to_net_dev(dev);
	struct stmmac_priv *priv;
	struct qcom_ethqos *ethqos;
	unsigned int len = 0, buf_len = 2000;

	if (!netdev) {
		ETHQOSERR("%s: netdev is NULL\n", __func__);
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		ETHQOSERR("%s: priv is NULL\n", __func__);
		return -EINVAL;
	}

	ethqos = priv->plat->bsp_priv;
	if (!ethqos) {
		ETHQOSERR("%s: ethqos is NULL\n", __func__);
		return -EINVAL;
	}
	if (ethqos->current_phy_mode == DISABLE_PHY_IMMEDIATELY)
		len += scnprintf(buf + len, buf_len - len,
				"Disable phy immediately enabled\n");
	else if (ethqos->current_phy_mode == ENABLE_PHY_IMMEDIATELY)
		len += scnprintf(buf + len, buf_len - len,
				 "Enable phy immediately enabled\n");
	else if (ethqos->current_phy_mode == DISABLE_PHY_AT_SUSPEND_ONLY) {
		len += scnprintf(buf + len, buf_len - len,
				 "Disable Phy at suspend\n");
		len += scnprintf(buf + len, buf_len - len,
				 " & do not enable at resume enabled\n");
	} else if (ethqos->current_phy_mode ==
		 DISABLE_PHY_SUSPEND_ENABLE_RESUME) {
		len += scnprintf(buf + len, buf_len - len,
				 "Disable Phy at suspend\n");
		len += scnprintf(buf + len, buf_len - len,
				 " & enable at resume enabled\n");
	} else if (ethqos->current_phy_mode == DISABLE_PHY_ON_OFF)
		len += scnprintf(buf + len, buf_len - len,
				 "Disable phy on/off disabled\n");
	else
		len += scnprintf(buf + len, buf_len - len,
					"Invalid Phy State\n");

	if (len > buf_len)
		len = buf_len;

	return scnprintf(user_buf, buf_len, "%s", buf);
}

static ssize_t phy_off_config(struct device *dev, struct device_attribute *attr,
			      const char *user_buf, size_t size)
{
	struct net_device *netdev = to_net_dev(dev);
	struct stmmac_priv *priv;
	struct qcom_ethqos *ethqos;

	s8 config = 0;
	unsigned long ret;

	if (!netdev) {
		ETHQOSERR("%s: netdev is NULL\n", __func__);
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		ETHQOSERR("%s: priv is NULL\n", __func__);
		return -EINVAL;
	}

	ethqos = priv->plat->bsp_priv;
	if (!ethqos) {
		ETHQOSERR("%s: ethqos is NULL\n", __func__);
		return -EINVAL;
	}

	ret = kstrtos8(user_buf, 0, &config);
	if (ret) {
		ETHQOSERR("unable to copy from user\n");
		return -EFAULT;
	}
	if (config > DISABLE_PHY_ON_OFF || config < DISABLE_PHY_IMMEDIATELY) {
		ETHQOSERR("Invalid option =%d", config);
		return -EINVAL;
	}
	if (config == ethqos->current_phy_mode) {
		ETHQOSERR("No effect as duplicate config");
		return -EPERM;
	}
	if (config == DISABLE_PHY_IMMEDIATELY) {
		ethqos->current_phy_mode = DISABLE_PHY_IMMEDIATELY;
	//make phy off
		if (priv->current_loopback == ENABLE_PHY_LOOPBACK) {
			/* If Phy loopback is enabled
			 *  Disabled It before phy off
			 */
			phy_digital_loopback_config(ethqos,
						    ethqos->loopback_speed, 0);
			ETHQOSDBG("Disable phy Loopback");
			priv->current_loopback = ENABLE_PHY_LOOPBACK;
		}
		ethqos_trigger_phylink(ethqos, false);
		ethqos_phy_power_off(ethqos);
	} else if (config == ENABLE_PHY_IMMEDIATELY) {
		ethqos->current_phy_mode = ENABLE_PHY_IMMEDIATELY;
		//make phy on
		ethqos_phy_power_on(ethqos);
		ethqos_trigger_phylink(ethqos, true);
		if (priv->current_loopback == ENABLE_PHY_LOOPBACK) {
			/*If Phy loopback is enabled , enabled It again*/
			phy_digital_loopback_config(ethqos,
						    ethqos->loopback_speed, 1);
			ETHQOSDBG("Enabling Phy loopback again");
		}
	} else if (config == DISABLE_PHY_AT_SUSPEND_ONLY) {
		ethqos->current_phy_mode = DISABLE_PHY_AT_SUSPEND_ONLY;
	} else if (config == DISABLE_PHY_SUSPEND_ENABLE_RESUME) {
		ethqos->current_phy_mode = DISABLE_PHY_SUSPEND_ENABLE_RESUME;
	} else if (config == DISABLE_PHY_ON_OFF) {
		ethqos->current_phy_mode = DISABLE_PHY_ON_OFF;
	} else {
		ETHQOSERR("Invalid option\n");
		return -EINVAL;
	}
	return size;
}

static DEVICE_ATTR(phy_off, ETHQOS_SYSFS_DEV_ATTR_PERMS,
			read_phy_off, phy_off_config);

static int ethqos_update_mdio_drv_strength(struct qcom_ethqos *ethqos,
					   struct device_node *np)
{
	u32 mdio_drv_str[2];
	struct resource *resource = NULL;
	unsigned long tlmm_central_base = 0;
	unsigned long tlmm_central_size = 0;
	int ret = 0;
	unsigned long v;

	resource = platform_get_resource_byname(ethqos->pdev,
						IORESOURCE_MEM, "tlmm-central-base");

	if (!resource) {
		ETHQOSERR("Resource tlmm-central-base not found\n");
		goto err_out;
	}

	tlmm_central_base = resource->start;
	tlmm_central_size = resource_size(resource);
	ETHQOSDBG("tlmm_central_base = 0x%x, size = 0x%x\n",
		  tlmm_central_base, tlmm_central_size);

	tlmm_central_base_addr = ioremap(tlmm_central_base,
					 tlmm_central_size);

	if (!tlmm_central_base_addr) {
		ETHQOSERR("cannot map dwc_tlmm_central reg memory, aborting\n");
		ret = -EIO;
		goto err_out;
	}

	if (np && !of_property_read_u32(np, "mdio-drv-str",
					&mdio_drv_str[0])) {
		switch (mdio_drv_str[0]) {
		case 2:
			mdio_drv_str[0] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_2MA;
			break;
		case 4:
			mdio_drv_str[0] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_4MA;
			break;
		case 6:
			mdio_drv_str[0] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_6MA;
			break;
		case 8:
			mdio_drv_str[0] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_8MA;
			break;
		case 10:
			mdio_drv_str[0] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_10MA;
			break;
		case 12:
			mdio_drv_str[0] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_12MA;
			break;
		case 14:
			mdio_drv_str[0] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_14MA;
			break;
		case 16:
			mdio_drv_str[0] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_16MA;
			break;
		default:
			mdio_drv_str[0] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_16MA;
			break;
		}

		TLMM_MDIO_HDRV_PULL_CTL_RGRD(v);
		v = (v & (unsigned long)(0xFFFFFE3F))
		 | (((mdio_drv_str[0]) & ((unsigned long)(0x7))) << 6);
		TLMM_MDIO_HDRV_PULL_CTL_RGWR(v);
	}

	if (np && !of_property_read_u32(np, "mdc-drv-str",
					&mdio_drv_str[1])) {
		switch (mdio_drv_str[1]) {
		case 2:
			mdio_drv_str[1] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_2MA;
			break;
		case 4:
			mdio_drv_str[1] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_4MA;
			break;
		case 6:
			mdio_drv_str[1] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_6MA;
			break;
		case 8:
			mdio_drv_str[1] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_8MA;
			break;
		case 10:
			mdio_drv_str[1] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_10MA;
			break;
		case 12:
			mdio_drv_str[1] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_12MA;
			break;
		case 14:
			mdio_drv_str[1] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_14MA;
			break;
		case 16:
			mdio_drv_str[1] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_16MA;
			break;
		default:
			mdio_drv_str[1] = TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_16MA;
			break;
		}

		TLMM_MDC_HDRV_PULL_CTL_RGRD(v);
		v = (v & (unsigned long)(0xFFFFFE3F))
		 | (((mdio_drv_str[1]) & (unsigned long)(0x7)) << 6);
		TLMM_MDC_HDRV_PULL_CTL_RGWR(v);
	}

err_out:
	if (tlmm_central_base_addr)
		iounmap(tlmm_central_base_addr);

	return ret;
}


#if IS_ENABLED(CONFIG_ETHQOS_QCOM_HOSTVM)
void qcom_ethstate_update(struct plat_stmmacenet_data *plat, enum eth_state event)
{
	struct qcom_ethqos *ethqos = plat->bsp_priv;

	if (event == EMAC_LINK_UP) {
		ethqos->last_event_linkup = true;
		if (ethqos->passthrough_en)
			qcom_notify_ethstate_tosvm(NOTIFICATION, event);
	} else {
		ethqos->last_event_linkup = false;
		qcom_notify_ethstate_tosvm(NOTIFICATION, event);
	}
}

static ssize_t show_passthrough_en(struct device *dev,
				   struct device_attribute *attr, char *user_buf)
{
	struct net_device *netdev = to_net_dev(dev);
	struct stmmac_priv *priv;
	struct qcom_ethqos *ethqos;

	if (!netdev) {
		ETHQOSERR("netdev is NULL\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		ETHQOSERR("priv is NULL\n");
		return -EINVAL;
	}

	ethqos = priv->plat->bsp_priv;
	if (!ethqos) {
		ETHQOSERR("ethqos is NULL\n");
		return -EINVAL;
	}

	return scnprintf(user_buf, BUFF_SZ, "%d\n", ethqos->passthrough_en);
}

static ssize_t store_passthrough_en(struct device *dev,
				    struct device_attribute *attr, const char *user_buf,
				    size_t size)
{
	struct net_device *netdev = to_net_dev(dev);
	struct stmmac_priv *priv;
	struct qcom_ethqos *ethqos;
	s8 input = 0;

	if (!netdev) {
		ETHQOSERR("netdev is NULL\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		ETHQOSERR("priv is NULL\n");
		return -EINVAL;
	}

	ethqos = priv->plat->bsp_priv;
	if (!ethqos) {
		ETHQOSERR("ethqos is NULL\n");
		return -EINVAL;
	}

	if (kstrtos8(user_buf, 0, &input)) {
		ETHQOSERR("Error in reading option from user\n");
		return -EINVAL;
	}

	if (input != 0 && input != 1) {
		ETHQOSERR("Invalid option set by user\n");
		return -EINVAL;
	}

	if (input == ethqos->passthrough_en) {
		ETHQOSINFO("Ethqos: No effect as duplicate passthrough_en input: %d\n", input);
	} else {
		ETHQOSINFO("Ethqos:  passthrough_en input: %d\n", input);
		ethqos->passthrough_en = input;
		if (input) {
			if (ethqos->last_event_linkup)
				qcom_notify_ethstate_tosvm(NOTIFICATION, EMAC_LINK_UP);
		} else {
			qcom_notify_ethstate_tosvm(NOTIFICATION, EMAC_LINK_DOWN);
		}
	}

	return size;
}
#else
static ssize_t show_cv2x_priority(struct device *dev,
				  struct device_attribute *attr, char *user_buf)
{
	struct net_device *netdev = to_net_dev(dev);
	struct stmmac_priv *priv;
	struct qcom_ethqos *ethqos;

	if (!netdev) {
		ETHQOSERR("netdev is NULL\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		ETHQOSERR("priv is NULL\n");
		return -EINVAL;
	}

	ethqos = priv->plat->bsp_priv;
	if (!ethqos) {
		ETHQOSERR("ethqos is NULL\n");
		return -EINVAL;
	}

	return scnprintf(user_buf, BUFF_SZ, "%d\n", ethqos->cv2x_priority);
}

static ssize_t store_cv2x_priority(struct device *dev,
				   struct device_attribute *attr, const char *user_buf, size_t size)
{
	struct net_device *netdev = to_net_dev(dev);
	struct stmmac_priv *priv;
	struct qcom_ethqos *ethqos;
	u32 prio;
	s8 input = 0;

	if (!netdev) {
		ETHQOSERR("netdev is NULL\n");
		return -EINVAL;
	}

	priv = netdev_priv(netdev);
	if (!priv) {
		ETHQOSERR("priv is NULL\n");
		return -EINVAL;
	}

	ethqos = priv->plat->bsp_priv;
	if (!ethqos) {
		ETHQOSERR("ethqos is NULL\n");
		return -EINVAL;
	}

	if (kstrtos8(user_buf, 0, &input)) {
		ETHQOSERR("Error in reading option from user\n");
		return -EINVAL;
	}

	if (input < 1 || input > 7) {
		ETHQOSERR("Invalid option set by user\n");
		return -EINVAL;
	}

	if (input == ethqos->cv2x_priority)
		ETHQOSERR("No effect as duplicate input\n");

	ethqos->cv2x_priority = input;
	prio  = 1 << input;
	ethqos->cv2x_pvm_only_enabled = true;
	// Configuring the priority for q4
	priv->plat->rx_queues_cfg[ethqos->cv2x_queue].prio = prio;
	stmmac_rx_queue_prio(priv, priv->hw, prio, ethqos->cv2x_queue);

	return size;
}
#endif

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_HOSTVM)
static DEVICE_ATTR(passthrough_en, ETHQOS_SYSFS_DEV_ATTR_PERMS, show_passthrough_en,
		   store_passthrough_en);
#else
static DEVICE_ATTR(cv2x_priority, ETHQOS_SYSFS_DEV_ATTR_PERMS, show_cv2x_priority,
		   store_cv2x_priority);
#endif

static int ethqos_remove_sysfs(struct qcom_ethqos *ethqos)
{
	struct net_device *net_dev;

	if (!ethqos) {
		ETHQOSERR("ethqos is NULL\n");
		return -EINVAL;
	}

	net_dev = platform_get_drvdata(ethqos->pdev);
	if (!net_dev) {
		ETHQOSERR("netdev is NULL\n");
		return -EINVAL;
	}

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_HOSTVM)
	sysfs_remove_file(&net_dev->dev.kobj,
			  &dev_attr_passthrough_en.attr);
#else
	sysfs_remove_file(&net_dev->dev.kobj,
			  &dev_attr_cv2x_priority.attr);
#endif

	sysfs_remove_file(&net_dev->dev.kobj,
			  &dev_attr_phy_off.attr);

	return 0;
}

static int ethqos_create_sysfs(struct qcom_ethqos *ethqos)
{
	int ret;
	struct net_device *net_dev;
	struct stmmac_priv *priv;

	if (!ethqos) {
		ETHQOSERR("ethqos is NULL\n");
		return -EINVAL;
	}

	net_dev = platform_get_drvdata(ethqos->pdev);
	if (!net_dev) {
		ETHQOSERR("netdev is NULL\n");
		return -EINVAL;
	}

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_HOSTVM)
	ret = sysfs_create_file(&net_dev->dev.kobj,
				&dev_attr_passthrough_en.attr);
	if (ret) {
		ETHQOSERR("unable to create passthrough_en sysfs node\n");
		goto fail;
	}
#else
	ret = sysfs_create_file(&net_dev->dev.kobj,
				&dev_attr_cv2x_priority.attr);
	if (ret) {
		ETHQOSERR("unable to create cv2x_priority sysfs node\n");
		goto fail;
	}
#endif

	priv = qcom_ethqos_get_priv(ethqos);

	if (!priv->plat->mac2mac_en && !priv->plat->fixed_phy_mode) {
		ret = sysfs_create_file(&net_dev->dev.kobj,
					&dev_attr_phy_off.attr);
		if (ret) {
			ETHQOSERR("Unable to create phy_off sysfs node\n");
			goto fail;
		}
	}

	if (priv->plat->plat_wait_for_emac_rx_clk_en) {
		ret = sysfs_create_file(&net_dev->dev.kobj,
					&dev_attr_rx_clock_rdy.attr);
		if (ret)
			ETHQOSERR(" Can't create rx_clock_rdy sysfs node");
	}

	return ret;

fail:
	return ethqos_remove_sysfs(ethqos);
}

static void read_mac_addr_from_fuse_reg(struct device_node *np)
{
	int ret, i, count, x;
	u32 mac_efuse_prop, efuse_size = 8;
	unsigned long mac_addr;
	unsigned char temp_mac_addr[ETH_ALEN];

	/* If the property doesn't exist or empty return */
	count = of_property_count_u32_elems(np, "mac-efuse-addr");
	if (!count || count < 0)
		return;

	/* Loop over all addresses given until we get valid address */
	for (x = 0; x < count; x++) {
		void __iomem *mac_efuse_addr;

		ret = of_property_read_u32_index(np, "mac-efuse-addr",
						 x, &mac_efuse_prop);
		if (!ret) {
			mac_efuse_addr = ioremap(mac_efuse_prop, efuse_size);
			if (!mac_efuse_addr)
				continue;

			mac_addr = readq(mac_efuse_addr) >> EFUSE_MAC_ADDR_MASK;
			ETHQOSINFO("Mac address read: %llx\n", mac_addr);

			/* create byte array out of value read from efuse */
			for (i = 0; i < ETH_ALEN ; i++) {
				temp_mac_addr[ETH_ALEN - 1 - i] =
					mac_addr & 0xff;
				mac_addr = mac_addr >> 8;
			}
			if (is_valid_ether_addr(temp_mac_addr)) {
				strscpy(pparams.mac_addr, temp_mac_addr, sizeof(pparams.mac_addr));
				pparams.is_valid_mac_addr = true;
			} else {
				ETHQOSERR("Fuse Mac address is invalid\n");
			}

			iounmap(mac_efuse_addr);

			/* if valid address is found return */
			if (pparams.is_valid_mac_addr)
				return;
		}
	}
}

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
static void ethqos_disable_sgmii_usxgmii_clks(struct qcom_ethqos *ethqos)
{
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);
	struct plat_stmmacenet_data *plat = priv->plat;

	clk_disable_unprepare(ethqos->clk_eee);
	clk_disable_unprepare(ethqos->sgmii_rx_clk);
	clk_disable_unprepare(ethqos->sgmii_tx_clk);

	if (plat->interface == PHY_INTERFACE_MODE_SGMII ||
	    plat->interface == PHY_INTERFACE_MODE_2500BASEX) {
		clk_disable_unprepare(ethqos->xgxs_rx_clk);
		clk_disable_unprepare(ethqos->xgxs_tx_clk);
	} else if (plat->interface == PHY_INTERFACE_MODE_USXGMII ||
		   plat->interface == PHY_INTERFACE_MODE_5GBASER) {
		clk_disable_unprepare(ethqos->pcs_rx_clk);
		clk_disable_unprepare(ethqos->pcs_tx_clk);
	}
}

static int ethqos_enable_sgmii_usxgmii_clks(struct qcom_ethqos *ethqos, int interface)
{
	int ret = 0;
	struct platform_device *pdev = ethqos->pdev;

	/* Clocks needed for both SGMII and USXGMII interfaces*/
	ethqos->clk_eee = devm_clk_get_optional(&pdev->dev, "eee");
	if (IS_ERR(ethqos->clk_eee)) {
		dev_warn(&pdev->dev, "Cannot get eee clock\n");
		ret = PTR_ERR(ethqos->clk_eee);
		ethqos->clk_eee = NULL;
		goto err_clk;
	} else {
		ret = clk_prepare_enable(ethqos->clk_eee);
		if (ret)
			goto err_clk;
	}

	ethqos->sgmii_rx_clk = devm_clk_get_optional(&pdev->dev, "sgmii_rx");
	if (IS_ERR(ethqos->sgmii_rx_clk)) {
		dev_warn(&pdev->dev, "Cannot get sgmii_rx clock\n");
		ret = PTR_ERR(ethqos->sgmii_rx_clk);
		ethqos->sgmii_rx_clk = NULL;
		goto err_clk;
	} else {
		ret = clk_prepare_enable(ethqos->sgmii_rx_clk);
		if (ret)
			goto err_clk;
	}

	ethqos->sgmii_tx_clk = devm_clk_get_optional(&pdev->dev, "sgmii_tx");
	if (IS_ERR(ethqos->sgmii_tx_clk)) {
		dev_warn(&pdev->dev, "Cannot get sgmii_tx clock\n");
		ret = PTR_ERR(ethqos->sgmii_tx_clk);
		ethqos->sgmii_tx_clk = NULL;
		goto err_clk;
	} else {
		ret = clk_prepare_enable(ethqos->sgmii_tx_clk);
		if (ret)
			goto err_clk;
	}

	/* Update mux logic to change the parent clock */
	ethqos->sgmii_rx_clk_src = devm_clk_get_optional(&pdev->dev, "sgmii_rclk_src");
	if (IS_ERR(ethqos->sgmii_rx_clk_src)) {
		dev_warn(&pdev->dev, "Cannot get sgmii_rx source clock\n");
		ret = PTR_ERR(ethqos->sgmii_rx_clk_src);
		ethqos->sgmii_rx_clk_src = NULL;
		goto err_clk;
	} else {
		ethqos->sgmii_rclk = devm_clk_get_optional(&pdev->dev, "sgmii_rclk");
		if (IS_ERR(ethqos->sgmii_rclk)) {
			dev_warn(&pdev->dev, "Cannot get sgmii_rclk\n");
			ret = PTR_ERR(ethqos->sgmii_rclk);
			ethqos->sgmii_rclk = NULL;
			goto err_clk;
		} else {
			ret = clk_set_parent(ethqos->sgmii_rx_clk_src, ethqos->sgmii_rclk);
			if (ret)
				goto err_clk;
		}
	}

	ethqos->sgmii_tx_clk_src = devm_clk_get_optional(&pdev->dev, "sgmii_tclk_src");
	if (IS_ERR(ethqos->sgmii_tx_clk_src)) {
		dev_warn(&pdev->dev, "Cannot get sgmii_tx source clock\n");
		ret = PTR_ERR(ethqos->sgmii_tx_clk_src);
		ethqos->sgmii_tx_clk_src = NULL;
		goto err_clk;
	} else {
		ethqos->sgmii_tclk = devm_clk_get_optional(&pdev->dev, "sgmii_tclk");
		if (IS_ERR(ethqos->sgmii_tclk)) {
			dev_warn(&pdev->dev, "Cannot get sgmii_tclk\n");
			ret = PTR_ERR(ethqos->sgmii_tclk);
			ethqos->sgmii_tclk = NULL;
			goto err_clk;
		} else {
			ret = clk_set_parent(ethqos->sgmii_tx_clk_src, ethqos->sgmii_tclk);
			if (ret)
				goto err_clk;
		}
	}

	ethqos->sgmii_mac_rx_clk_src = devm_clk_get_optional(&pdev->dev, "sgmii_mac_rclk_src");
	if (IS_ERR(ethqos->sgmii_mac_rx_clk_src)) {
		dev_warn(&pdev->dev, "Cannot get sgmii_mac_rx source clock\n");
		ret = PTR_ERR(ethqos->sgmii_mac_rx_clk_src);
		ethqos->sgmii_mac_rx_clk_src = NULL;
		goto err_clk;
	} else {
		ethqos->sgmii_mac_rclk = devm_clk_get_optional(&pdev->dev, "sgmii_mac_rclk");
		if (IS_ERR(ethqos->sgmii_mac_rclk)) {
			dev_warn(&pdev->dev, "Cannot get sgmii_mac_rclk\n");
			ret = PTR_ERR(ethqos->sgmii_mac_rclk);
			ethqos->sgmii_mac_rclk = NULL;
			goto err_clk;
		} else {
			ret = clk_set_parent(ethqos->sgmii_mac_rx_clk_src, ethqos->sgmii_mac_rclk);
			if (ret)
				goto err_clk;
		}
	}

	ethqos->sgmii_mac_tx_clk_src = devm_clk_get_optional(&pdev->dev, "sgmii_mac_tclk_src");
	if (IS_ERR(ethqos->sgmii_mac_tx_clk_src)) {
		dev_warn(&pdev->dev, "Cannot get sgmii_mac_tx source clock\n");
		ret = PTR_ERR(ethqos->sgmii_mac_tx_clk_src);
		ethqos->sgmii_mac_tx_clk_src = NULL;
		goto err_clk;
	} else {
		ethqos->sgmii_mac_tclk = devm_clk_get_optional(&pdev->dev, "sgmii_mac_tclk");
		if (IS_ERR(ethqos->sgmii_mac_tclk)) {
			dev_warn(&pdev->dev, "Cannot get sgmii_mac_tclk\n");
			ret = PTR_ERR(ethqos->sgmii_mac_tclk);
			ethqos->sgmii_mac_tclk = NULL;
			goto err_clk;
		} else {
			ret = clk_set_parent(ethqos->sgmii_mac_tx_clk_src, ethqos->sgmii_mac_tclk);
			if (ret)
				goto err_clk;
		}
	}

	if (interface == PHY_INTERFACE_MODE_SGMII ||
	    interface == PHY_INTERFACE_MODE_2500BASEX) {
		/*Clocks specific to SGMII interface */
		ethqos->xgxs_rx_clk = devm_clk_get_optional(&pdev->dev, "xgxs_rx");
		if (IS_ERR(ethqos->xgxs_rx_clk)) {
			dev_warn(&pdev->dev, "Cannot get xgxs_rx clock\n");
			ret = PTR_ERR(ethqos->xgxs_rx_clk);
			ethqos->xgxs_rx_clk = NULL;
			goto err_clk;
		} else {
			ret = clk_prepare_enable(ethqos->xgxs_rx_clk);
			if (ret)
				goto err_clk;
		}

		ethqos->xgxs_tx_clk = devm_clk_get_optional(&pdev->dev, "xgxs_tx");
		if (IS_ERR(ethqos->xgxs_tx_clk)) {
			dev_warn(&pdev->dev, "Cannot get xgxs_tx clock\n");
			ret = PTR_ERR(ethqos->xgxs_tx_clk);
			ethqos->xgxs_tx_clk = NULL;
			goto err_clk;
		} else {
			ret = clk_prepare_enable(ethqos->xgxs_tx_clk);
			if (ret)
				goto err_clk;
		}
	} else if (interface == PHY_INTERFACE_MODE_USXGMII ||
		   interface == PHY_INTERFACE_MODE_5GBASER) {
		/*Clocks specific to USXGMII interface */
		ethqos->pcs_rx_clk = devm_clk_get_optional(&pdev->dev, "pcs_rx");
		if (IS_ERR(ethqos->pcs_rx_clk)) {
			dev_warn(&pdev->dev, "Cannot get pcs_rx clock\n");
			ret = PTR_ERR(ethqos->pcs_rx_clk);
			ethqos->pcs_rx_clk = NULL;
			goto err_clk;
		} else {
			ret = clk_prepare_enable(ethqos->pcs_rx_clk);
			if (ret)
				goto err_clk;
		}

		ethqos->pcs_tx_clk = devm_clk_get_optional(&pdev->dev, "pcs_tx");
		if (IS_ERR(ethqos->pcs_tx_clk)) {
			dev_warn(&pdev->dev, "Cannot get pcs_tx clock\n");
			ret = PTR_ERR(ethqos->pcs_tx_clk);
			ethqos->pcs_tx_clk = NULL;
			goto err_clk;
		} else {
			ret = clk_prepare_enable(ethqos->pcs_tx_clk);
			if (ret)
				goto err_clk;
		}
	}

err_clk:
	return ret;
}

static int ethqos_resume_sgmii_usxgmii_clks(struct qcom_ethqos *ethqos)
{
	int ret = 0;
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);
	struct plat_stmmacenet_data *plat = priv->plat;

	ret = clk_prepare_enable(ethqos->clk_eee);
	if (ret)
		goto err;

	ret = clk_prepare_enable(ethqos->sgmii_rx_clk);
	if (ret)
		goto err;

	ret = clk_prepare_enable(ethqos->sgmii_tx_clk);
	if (ret)
		goto err;

	if (plat->interface == PHY_INTERFACE_MODE_SGMII ||
	    plat->interface == PHY_INTERFACE_MODE_2500BASEX) {
		ret = clk_prepare_enable(ethqos->xgxs_rx_clk);
		if (ret)
			goto err;
		ret = clk_prepare_enable(ethqos->xgxs_tx_clk);
		if (ret)
			goto err;
	} else if (plat->interface == PHY_INTERFACE_MODE_USXGMII ||
		   plat->interface == PHY_INTERFACE_MODE_5GBASER) {
		ret = clk_prepare_enable(ethqos->pcs_rx_clk);
		if (ret)
			goto err;
		ret = clk_prepare_enable(ethqos->pcs_tx_clk);
		if (ret)
			goto err;
	}

	return 0;
err:
	ETHQOSERR("Failed to resume SGMII/USXGMII clocks\n");
	return ret;
}

/* Skip stmmac_ethtool_set_wol and do something similar to qcom_ethqos_request_phy_wol.
 */
static int ethqos_enable_wol(struct net_device *ndev, struct ethtool_wolinfo *wol)
{
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct qcom_ethqos *ethqos = get_stmmac_bsp_priv(priv->device);
	int ret = 0;

	if (priv->phydev) {
		wol->cmd = ETHTOOL_SWOL;
		ret = phy_ethtool_set_wol(priv->phydev, wol);
		if (ret)
			return ret;

		if (wol->wolopts) {
			if (!priv->dev->wol_enabled) {
				if (priv->plat->separate_wol_pin)
					ret = enable_irq_wake(ethqos->wol_intr);
				else
					ret = enable_irq_wake(ethqos->phy_intr);
			}

			priv->dev->wol_enabled = true;
			ETHQOSINFO("Enabled WoL\n");
		} else {
			if (priv->dev->wol_enabled) {
				if (priv->plat->separate_wol_pin)
					ret = disable_irq_wake(ethqos->wol_intr);
				else
					ret = disable_irq_wake(ethqos->phy_intr);
			}

			priv->dev->wol_enabled = false;
			ETHQOSINFO("Disabled WoL\n");
		}

		if (ret) {
			ETHQOSERR("Failed to configure WoL\n");
			return ret;
		}
	} else {
		ret = -ENODEV;
	}

	return ret;
}

#else
static inline void ethqos_disable_sgmii_usxgmii_clks(struct qcom_ethqos *ethqos)
{
}

static inline int ethqos_enable_sgmii_usxgmii_clks(struct qcom_ethqos *ethqos, int interface)
{
	return 0;
}

static int ethqos_resume_sgmii_usxgmii_clks(struct qcom_ethqos *ethqos)
{
	return 0;
}
#endif

static unsigned int ethqos_poll_rec_dev_emac(struct file *file,
					     poll_table *wait)
{
	int mask = 0;

	ETHQOSDBG("\n");

	poll_wait(file, &mac_rec_wq, wait);

	if (mac_rec_wq_flag) {
		mask = POLLIN | POLLRDNORM;
		mac_rec_wq_flag = false;
	}

	ETHQOSDBG("mask %d\n", mask);

	return mask;
}

static ssize_t ethqos_mac_recovery_enable(struct file *file,
					  const char __user *user_buf,
					  size_t count, loff_t *ppos)
{
	static unsigned char in_buf[15] = {0};
	int i, ret;
	struct qcom_ethqos *ethqos = pethqos[0];

	if (sizeof(in_buf) < count) {
		ETHQOSERR("emac string is too long - count=%u\n", count);
		return -EFAULT;
	}

	memset(in_buf, 0,  sizeof(in_buf));
	ret = copy_from_user(in_buf, user_buf, count);

	for (i = 0; i < MAC_ERR_CNT; i++) {
		if (in_buf[i] == '1')
			ethqos->mac_rec_en[i] = true;
		else
			ethqos->mac_rec_en[i] = false;
	}

	return count;
}

static ssize_t ethqos_read_rec_dev_emac(struct file *filp, char __user *usr_buf,
					size_t count, loff_t *f_pos)
{
	char *buf;
	unsigned int len = 0, buf_len = 6000;
	ssize_t ret_cnt;

	buf = kzalloc(buf_len, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "PHY_RW_ERR", pethqos[0]->mac_err_cnt[PHY_RW_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "PHY_DET_ERR", pethqos[0]->mac_err_cnt[PHY_DET_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "CRC_ERR", pethqos[0]->mac_err_cnt[CRC_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "RECEIVE_ERR", pethqos[0]->mac_err_cnt[RECEIVE_ERR]);
		len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "OVERFLOW_ERR", pethqos[0]->mac_err_cnt[OVERFLOW_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "FBE_ERR", pethqos[0]->mac_err_cnt[FBE_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "RBU_ERR", pethqos[0]->mac_err_cnt[RBU_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "TDU_ERR", pethqos[0]->mac_err_cnt[TDU_ERR]);
		len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "DRIBBLE_ERR", pethqos[0]->mac_err_cnt[DRIBBLE_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "WDT_ERR", pethqos[0]->mac_err_cnt[WDT_ERR]);

	len += scnprintf(buf + len, buf_len - len, "\n\n%s  =  %d\n",
		 "PHY_RW_EN", pethqos[0]->mac_rec_en[PHY_RW_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "PHY_DET_EN", pethqos[0]->mac_rec_en[PHY_DET_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "CRC_EN", pethqos[0]->mac_rec_en[CRC_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "RECEIVE_EN", pethqos[0]->mac_rec_en[RECEIVE_ERR]);
		len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "OVERFLOW_EN", pethqos[0]->mac_rec_en[OVERFLOW_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "FBE_EN", pethqos[0]->mac_rec_en[FBE_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "RBU_EN", pethqos[0]->mac_rec_en[RBU_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "TDU_EN", pethqos[0]->mac_rec_en[TDU_ERR]);
		len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "DRIBBLE_EN", pethqos[0]->mac_rec_en[DRIBBLE_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "WDT_EN", pethqos[0]->mac_rec_en[WDT_ERR]);

	len += scnprintf(buf + len, buf_len - len, "\n\n%s  =  %d\n",
		 "PHY_RW_REC", pethqos[0]->mac_rec_cnt[PHY_RW_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "PHY_DET_REC", pethqos[0]->mac_rec_cnt[PHY_DET_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "CRC_REC", pethqos[0]->mac_rec_cnt[CRC_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "RECEIVE_REC", pethqos[0]->mac_rec_cnt[RECEIVE_ERR]);
		len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "OVERFLOW_REC", pethqos[0]->mac_rec_cnt[OVERFLOW_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "FBE_REC", pethqos[0]->mac_rec_cnt[FBE_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "RBU_REC", pethqos[0]->mac_rec_cnt[RBU_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "TDU_REC", pethqos[0]->mac_rec_cnt[TDU_ERR]);
		len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "DRIBBLE_REC", pethqos[0]->mac_rec_cnt[DRIBBLE_ERR]);
	len += scnprintf(buf + len, buf_len - len, "%s  =  %d\n",
		 "WDT_REC", pethqos[0]->mac_rec_cnt[WDT_ERR]);

	if (len > buf_len)
		len = buf_len;

	ETHQOSDBG("%s", buf);
	ret_cnt = simple_read_from_buffer(usr_buf, count, f_pos, buf, len);
	kfree(buf);
	return ret_cnt;
}

static const struct file_operations emac_rec_fops = {
	.owner = THIS_MODULE,
	.open = simple_open,
	.read = ethqos_read_rec_dev_emac,
	.write = ethqos_mac_recovery_enable,
	.poll = ethqos_poll_rec_dev_emac,
};

static void ethqos_delete_emac_rec_device_node(dev_t *emac_dev_t,
					       struct cdev **emac_cdev,
					       struct class **emac_class)
{
	if (!*emac_class) {
		ETHQOSERR("failed to destroy device and class\n");
		goto fail_to_del_node;
	}

	if (!emac_dev_t) {
		ETHQOSERR("failed to unregister chrdev region\n");
		goto fail_to_del_node;
	}

	if (!*emac_cdev) {
		ETHQOSERR("failed to delete cdev\n");
		goto fail_to_del_node;
	}

	device_destroy(*emac_class, *emac_dev_t);
	class_destroy(*emac_class);
	cdev_del(*emac_cdev);
	unregister_chrdev_region(*emac_dev_t, 1);

	return;

fail_to_del_node:
	ETHQOSERR("failed to delete chrdev node\n");
	return;
}

static int ethqos_create_emac_rec_device_node(dev_t *emac_dev_t,
					      struct cdev **emac_cdev,
					      struct class **emac_class,
					      char *emac_dev_node_name)
{
	int ret;

	ret = alloc_chrdev_region(emac_dev_t, 0, 1,
				  emac_dev_node_name);
	if (ret) {
		ETHQOSERR("alloc_chrdev_region error for node %s\n",
			  emac_dev_node_name);
		goto alloc_chrdev1_region_fail;
	}

	*emac_cdev = cdev_alloc();
	if (!*emac_cdev) {
		ret = -ENOMEM;
		ETHQOSERR("failed to alloc cdev\n");
		goto fail_alloc_cdev;
	}
	cdev_init(*emac_cdev, &emac_rec_fops);

	ret = cdev_add(*emac_cdev, *emac_dev_t, 1);
	if (ret < 0) {
		ETHQOSERR(":cdev_add err=%d\n", -ret);
		goto cdev1_add_fail;
	}

	*emac_class = class_create(THIS_MODULE, emac_dev_node_name);
	if (!*emac_class) {
		ret = -ENODEV;
		ETHQOSERR("failed to create class\n");
		goto fail_create_class;
	}

	if (!device_create(*emac_class, NULL,
			   *emac_dev_t, NULL, emac_dev_node_name)) {
		ret = -EINVAL;
		ETHQOSERR("failed to create device_create\n");
		goto fail_create_device;
	}

	ETHQOSERR(" mac recovery node opened");
	return 0;

fail_create_device:
	class_destroy(*emac_class);
fail_create_class:
	cdev_del(*emac_cdev);
cdev1_add_fail:
fail_alloc_cdev:
	unregister_chrdev_region(*emac_dev_t, 1);
alloc_chrdev1_region_fail:
		return ret;
}

static int qcom_ethqos_panic_notifier(struct notifier_block *nb,
				      unsigned long event, void *ptr)
{
	struct qcom_ethqos *ethqos = container_of(nb, struct qcom_ethqos, panic_nb);
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);
	size_t i, j, k = 0;

	if (atomic_read(&priv->plat->phy_clks_suspended) || !ethqos->mac_reg_list) {
		ETHQOSERR("EMAC register dump failed\n");
		return NOTIFY_DONE;
	}

	pr_info("Dumping EMAC registers\n");

	for (i = 0; i < ARRAY_SIZE(mac_reg_offsets); i++) {
		for (j = 0; j < mac_reg_sizes[i]; j += MAC_REG_SIZE) {
			ethqos->mac_reg_list[k].offset = mac_reg_offsets[i] + j;
			ethqos->mac_reg_list[k].value = readl(ethqos->ioaddr +
							mac_reg_offsets[i] + j);
			k++;
		}
	}

	pr_info("EMAC register dump complete: Dumped %u registers\n", k);

	pr_info("qcom-ethqos: ethqos 0x%p\n", ethqos);

	pr_info("qcom-ethqos: stmmac_priv 0x%p\n", ethqos->priv);

	return NOTIFY_DONE;
}

static ssize_t ethqos_read_dev_emac(struct file *filp, char __user *buf,
				    size_t count, loff_t *f_pos)
{
	unsigned int len = 0;
	char *temp_buf = NULL;
	ssize_t ret_cnt = 0;

	ret_cnt = simple_read_from_buffer(buf, count, f_pos, temp_buf, len);
	return ret_cnt;
}

static ssize_t ethqos_write_dev_emac(struct file *file,
				     const char __user *user_buf,
				     size_t count, loff_t *ppos)
{
	unsigned char in_buf[300] = {0};
	unsigned long ret;
	struct qcom_ethqos *ethqos;
	struct stmmac_priv *priv;
	struct vlan_filter_info vlan_filter_info;
	char vlan_str[30] = {0};
	char *prefix = NULL;
	u32 err;
	unsigned int number;

	/*emac_cdev charecter device is applicable only for auto which supports single port */
	ethqos = pethqos[0];
	priv = qcom_ethqos_get_priv(ethqos);

	if (sizeof(in_buf) < count) {
		ETHQOSERR("emac string is too long - count=%u\n", count);
		return -EFAULT;
	}

	memset(in_buf, 0,  sizeof(in_buf));
	ret = copy_from_user(in_buf, user_buf, count);

	if (ret)
		return -EFAULT;

	strscpy(vlan_str, in_buf, sizeof(vlan_str));

	ETHQOSINFO("emac string is %s\n", vlan_str);

	if (strnstr(vlan_str, "QOE", sizeof(vlan_str))) {
		ethqos->qoe_vlan.available = true;
		vlan_filter_info.vlan_id = ethqos->qoe_vlan.vlan_id;
		vlan_filter_info.rx_queue = ethqos->qoe_vlan.rx_queue;
		vlan_filter_info.vlan_offset = ethqos->qoe_vlan.vlan_offset;
		priv->hw->mac->qcom_set_vlan(&vlan_filter_info, priv->ioaddr);
	}

	if (strnstr(vlan_str, "qvlanid=", sizeof(vlan_str))) {
		prefix = strnchr(vlan_str,
				 strlen(vlan_str), '=');
		ETHQOSINFO("vlanid data written is %s\n", prefix + 1);
		if (prefix) {
			err = kstrtouint(prefix + 1, 0, &number);
			if (!err)
				ethqos->qoe_vlan.vlan_id = number;
		}
	}

	return count;
}

void qcom_ethsvm_command_req(enum msg_type command, void *buf, int len, void *user_data)
{
	struct stmmac_priv *priv = (struct stmmac_priv *)user_data;
	u8 *buffer = (u8 *)buf;

	pr_info("%s GVM command = %d priv = %p buf = %pM\n", __func__, command, priv, buffer);

	switch (command) {
	case VLAN_ADD:
		/* Last 12 bits are the vlan id */
		priv->dev->netdev_ops->ndo_vlan_rx_add_vid(priv->dev,
							   htons(ETH_P_8021Q),
							   (*((u16 *)buffer) & 0xFFF));
		break;

	case VLAN_DEL:
		if (priv->dev->netdev_ops->ndo_vlan_rx_kill_vid)
			priv->dev->netdev_ops->ndo_vlan_rx_kill_vid(priv->dev,
							      htons(ETH_P_8021Q),
							      (*((u16 *)buffer) & 0xFFF));
		break;

	case PRIO_ADD:
		/* Disable the MAC Rx/Tx */
		stmmac_mac_set(priv, priv->ioaddr, false);

		/* Enable queue 4 for SVM and set priority based routing */
		writel(0x4, priv->ioaddr + 0x1034);
		stmmac_rx_queue_enable(priv, priv->hw, MTL_QUEUE_DCB, 4);
		stmmac_rx_queue_prio(priv, priv->hw, (1 << *buffer), 0x4);

		/* Enable the MAC Rx/Tx */
		stmmac_mac_set(priv, priv->ioaddr, true);
		break;

	case PRIO_DEL:
		stmmac_rx_queue_prio(priv, priv->hw, 0, 0x4);
		break;

	case UNICAST_DEL:
	case MULTICAST_DEL:
	case UNICAST_ADD:
	case MULTICAST_ADD:
	default:  /* Implement handling for SVM requests */
		pr_err("%s Command 0x%x not implemented\n", __func__, command);
		break;
	}
}

static void ethqos_get_qoe_dt(struct qcom_ethqos *ethqos,
			      struct device_node *np)
{
	int res;

	res = of_property_read_u32(np, "qcom,qoe_mode", &ethqos->qoe_mode);
	if (res) {
		ETHQOSDBG("qoe_mode not in dtsi\n");
		ethqos->qoe_mode = 0;
	}

	if (ethqos->qoe_mode) {
		res = of_property_read_u32(np, "qcom,qoe-queue",
					   &ethqos->qoe_vlan.rx_queue);
		if (res) {
			ETHQOSERR("qoe-queue not in dtsi for qoe_mode %u\n",
				  ethqos->qoe_mode);
			ethqos->qoe_vlan.rx_queue = QMI_TAG_TX_CHANNEL;
		}

		res = of_property_read_u32(np, "qcom,qoe-vlan-offset",
					   &ethqos->qoe_vlan.vlan_offset);
		if (res) {
			ETHQOSERR("qoe-vlan-offset not in dtsi\n");
			ethqos->qoe_vlan.vlan_offset = 0;
		}
	}
}

static const struct file_operations emac_fops = {
	.owner = THIS_MODULE,
	.read = ethqos_read_dev_emac,
	.write = ethqos_write_dev_emac,
};

static int ethqos_create_emac_device_node(dev_t *emac_dev_t,
					  struct cdev **emac_cdev,
					  struct class **emac_class,
					  char *emac_dev_node_name)
{
	int ret;

	ret = alloc_chrdev_region(emac_dev_t, 0, 1,
				  emac_dev_node_name);
	if (ret) {
		ETHQOSERR("alloc_chrdev_region error for node %s\n",
			  emac_dev_node_name);
		goto alloc_chrdev1_region_fail;
	}

	*emac_cdev = cdev_alloc();
	if (!*emac_cdev) {
		ret = -ENOMEM;
		ETHQOSERR("failed to alloc cdev\n");
		goto fail_alloc_cdev;
	}
	cdev_init(*emac_cdev, &emac_fops);

	ret = cdev_add(*emac_cdev, *emac_dev_t, 1);
	if (ret < 0) {
		ETHQOSERR(":cdev_add err=%d\n", -ret);
		goto cdev1_add_fail;
	}

	*emac_class = class_create(THIS_MODULE, emac_dev_node_name);
	if (!*emac_class) {
		ret = -ENODEV;
		ETHQOSERR("failed to create class\n");
		goto fail_create_class;
	}

	if (!device_create(*emac_class, NULL,
			   *emac_dev_t, NULL, emac_dev_node_name)) {
		ret = -EINVAL;
		ETHQOSERR("failed to create device_create\n");
		goto fail_create_device;
	}

	return 0;

fail_create_device:
	class_destroy(*emac_class);
fail_create_class:
	cdev_del(*emac_cdev);
cdev1_add_fail:
fail_alloc_cdev:
	unregister_chrdev_region(*emac_dev_t, 1);
alloc_chrdev1_region_fail:
		return ret;
}

static int ethqos_fixed_link_check(struct platform_device *pdev)
{
	struct device_node *fixed_phy_node;
	struct property *status_prop;
	struct property *speed_prop;
	int mac2mac_speed;
	static u32 speed;
	struct device_node *np = pdev->dev.of_node;

	fixed_phy_node = of_get_child_by_name(np, "fixed-link");
	if (of_device_is_available(fixed_phy_node)) {
		of_property_read_u32(fixed_phy_node, "speed", &mac2mac_speed);
		plat_dat->fixed_phy_mode = true;
		plat_dat->phy_addr = -1;
		plat_dat->fixed_phy_speed = mac2mac_speed;
		ETHQOSINFO("mac2mac mode: Fixed-link enabled from dt, Speed = %d\n",
			   mac2mac_speed);
		goto out;
	} else if (!fixed_phy_node) {
		goto out;
	}

	/* Check partition if mac2mac configuration enabled using the same */
	if (phyaddr_pt_param == 0xFF) {
		if (fixed_phy_node) {
			status_prop = kzalloc(sizeof(*status_prop), GFP_KERNEL);

			if (!status_prop) {
				ETHQOSERR("kzalloc failed\n");
				return -ENOMEM;
			}

			status_prop->name = kstrdup("status", GFP_KERNEL);
			status_prop->value = kstrdup("okay", GFP_KERNEL);
			if (!(status_prop->value)) {
				ETHQOSERR("kstrdup failed to allocate space\n");
				kfree(status_prop);
				return -ENOMEM;
			}
			status_prop->length = strlen(status_prop->value) + 1;

			if (!(of_update_property(fixed_phy_node, status_prop) == 0)) {
				kfree(status_prop);
				ETHQOSERR("Fixed-link status update failed\n");
				return -ENOENT;
			}

			plat_dat->fixed_phy_mode = true;

			ETHQOSINFO("mac2mac mode: Fixed-link enabled from Partition\n");

			speed_prop = kzalloc(sizeof(*speed_prop), GFP_KERNEL);

			if (!speed_prop) {
				ETHQOSERR("kzalloc failed\n");
				return -ENOMEM;
			}

			speed = cpu_to_be32(mparams.link_speed);

			speed_prop->name = kstrdup("speed", GFP_KERNEL);
			speed_prop->value = &speed;
			speed_prop->length = sizeof(u32);

			if (!(of_update_property(fixed_phy_node, speed_prop) == 0)) {
				kfree(speed_prop);
				ETHQOSERR("Fixed-link speed update failed\n");
				return -ENOENT;
			}

			plat_dat->fixed_phy_speed = mparams.link_speed;

			ETHQOSINFO("mac2mac mode: Fixed-link speed updated from partition: %u\n",
				   mparams.link_speed);
		}
	}

out:
	if (plat_dat->fixed_phy_mode) {
		// Set plat_wait_for_emac_rx_clk_en,
		// dtsi will overwrite configuration from emac partition
		plat_dat->plat_wait_for_emac_rx_clk_en = mparams.wait_switch_rdy;
		of_property_read_u32(np, "wait_for_rx_clk_rdy",
				     &plat_dat->plat_wait_for_emac_rx_clk_en);
		plat_dat->fixed_phy_mode_needs_mdio = of_property_read_bool(np,
									    "fixed-link-needs-mdio-bus");
	}

	of_node_put(fixed_phy_node);
	return 0;
}

static int ethqos_update_phyid(struct device_node *np)
{
	struct property *phyid_prop;
	static unsigned long address;

	phyid_prop = kzalloc(sizeof(*phyid_prop), GFP_KERNEL);

	if (!phyid_prop) {
		ETHQOSERR("kzalloc failed\n");
		return -ENOMEM;
	}

	address = cpu_to_be32(phyaddr_pt_param);

	phyid_prop->name = kstrdup("snps,phy-addr", GFP_KERNEL);
	phyid_prop->value = &address;
	phyid_prop->length = sizeof(u32);

	if (!(of_update_property(np, phyid_prop) == 0)) {
		kfree(phyid_prop);
		ETHQOSERR("Phy address failed to update\n");
		goto out;
	}

	ETHQOSINFO("Phy address changed to %ld from the partition",
		   phyaddr_pt_param);

out:
	return 0;
}

static int qcom_ethqos_register_panic_notifier(struct qcom_ethqos *ethqos)
{
	int ret;
	size_t i;
	unsigned long num_registers = 0;

	if (ethqos->panic_notifier_registered)
		return 0;

	for (i = 0; i < ARRAY_SIZE(mac_reg_sizes); i++) {
		if (mac_reg_sizes[i] % MAC_REG_SIZE) {
			ETHQOSERR("Invalid register size in mac_reg_sizes found at index %u: %u\n",
				  i, mac_reg_sizes[i]);
			return -EINVAL;
		}
		num_registers += (mac_reg_sizes[i] / MAC_REG_SIZE);
	}

	if (num_registers == 0) {
		ETHQOSDBG("Panic notifier not registered: no registers to capture\n");
		return 0;
	}

	ETHQOSDBG("Allocating memory for %lu registers", num_registers);
	ethqos->mac_reg_list = kcalloc(num_registers, sizeof(struct mac_csr_data), GFP_KERNEL);
	if (!ethqos->mac_reg_list) {
		ETHQOSERR("Failed to allocate memory for panic notifier register dump\n");
		return -ENOMEM;
	}

	ethqos->panic_nb.notifier_call	= qcom_ethqos_panic_notifier;
	ethqos->panic_nb.priority = INT_MAX;

	ret = atomic_notifier_chain_register(&panic_notifier_list,
					     &ethqos->panic_nb);
	if (ret)
		ETHQOSERR("Failed to register panic notifier\n");
	else
		ethqos->panic_notifier_registered = true;

	return ret;
}

static void qcom_ethqos_unregister_panic_notifier(struct qcom_ethqos *ethqos)
{
	kfree(ethqos->mac_reg_list);
	ethqos->mac_reg_list = NULL;

	if (ethqos->panic_notifier_registered) {
		atomic_notifier_chain_unregister(&panic_notifier_list,
						 &ethqos->panic_nb);
		ethqos->panic_notifier_registered = false;
	}
}

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
static void ethqos_xpcs_power_saving(struct net_device *ndev, bool enable)
{
	struct stmmac_priv *priv = netdev_priv(ndev);
	int ret = 0;

	if (priv->hw->qxpcs) {
		ret = qcom_xpcs_lpm(priv->hw->qxpcs, enable);
		if (ret)
			ETHQOSERR("XPCS power saving Failed enable %d\n", enable);
	}

	ETHQOSINFO("XPCS power saving enable %d\n", enable);
}
#endif

static int ethqos_serdes_power_saving(struct net_device *ndev, void *priv,
				      bool power_state, bool needs_serdes_reset)
{
	struct qcom_ethqos *ethqos = priv;
	int ret = 0;

	if (ethqos->emac_ver != EMAC_HW_v4_0_0)
		return -EINVAL;

	if (ethqos->power_state == power_state)
		return -EINVAL;

	if (power_state) {
		if (ethqos->vreg_a_sgmii_1p2 && ethqos->vreg_a_sgmii_0p9) {
			ret = ethqos_enable_serdes_consumers(ethqos);
			if (ret < 0)
				return ret;

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
			ret = qcom_ethqos_enable_serdes_clocks(ethqos);
			if (ret)
				return -EINVAL;
#endif

			if (needs_serdes_reset)
				qcom_ethqos_serdes_soft_reset(ethqos);

			ETHQOSINFO("power saving turned off\n");
		}
	} else {
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
		qcom_ethqos_serdes_power_down(ethqos);

		qcom_ethqos_disable_serdes_clocks(ethqos);
#endif
		if (ethqos->vreg_a_sgmii_1p2 && ethqos->vreg_a_sgmii_0p9) {
			ret = ethqos_disable_serdes_consumers(ethqos);
			if (ret < 0)
				return ret;

			ETHQOSINFO("power saving turned on\n");
		}
	}
	ethqos->power_state = power_state;
	return ret;
}

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
static int __ethqos_emac_power_down(struct stmmac_priv *priv)
{
	int ret = 0;
	struct qcom_ethqos *ethqos = priv->plat->bsp_priv;

	ethqos_update_bus_cfg(ethqos, SPEED_10);

	if (ethqos->vreg_a_sgmii_1p2 && ethqos->vreg_a_sgmii_0p9) {
		ethqos_disable_sgmii_usxgmii_clks(ethqos);

		if (priv->plat->serdes_powersaving)
			ret = priv->plat->serdes_powersaving(priv->dev,
							     priv->plat->bsp_priv,
							     false,
							     false);
	}

	ethqos->clk_active = false;

	return ret;
}

static int __ethqos_emac_power_up(struct stmmac_priv *priv)
{
	int ret = 0;
	struct qcom_ethqos *ethqos = priv->plat->bsp_priv;

	if (ethqos->clk_gated)
		return 0;

	if (ethqos->vreg_a_sgmii_1p2 && ethqos->vreg_a_sgmii_0p9) {
		if (priv->plat->serdes_powersaving) {
			ret = priv->plat->serdes_powersaving(priv->dev,
							     priv->plat->bsp_priv,
							     true,
							     false);
			if (ret < 0) {
				ETHQOSERR("Failed to enable serdes clocks/regulators\n");
				goto err;
			}
		}

		ret = ethqos_resume_sgmii_usxgmii_clks(ethqos);
		if (ret < 0) {
			ETHQOSERR("Failed to enable sgmii/usxgmii clocks\n");
			goto err;
		}
	}

	ethqos_update_bus_cfg(ethqos, ethqos->speed);

	ethqos->clk_active = true;

err:
	return ret;
}

static void ethqos_enable_clock_gating(struct net_device *ndev)
{
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct qcom_ethqos *ethqos = priv->plat->bsp_priv;

	mutex_lock(&ethqos->ps_lock);

	if (!ethqos->clk_gated) {
		ethqos->clk_gated = true;

		if (ethqos->clk_active)
			__ethqos_emac_power_down(priv);
	}

	ETHQOSINFO("Enable Clock Gating EMAC = %u ps_refcount = %u clk_active = %u\n",
		   priv->plat->port_num, refcount_read(&ethqos->ps_refcount), ethqos->clk_active);

	mutex_unlock(&ethqos->ps_lock);
}

static void ethqos_disable_clock_gating(struct net_device *ndev)
{
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct qcom_ethqos *ethqos = priv->plat->bsp_priv;

	mutex_lock(&ethqos->ps_lock);

	if (ethqos->clk_gated) {
		ethqos->clk_gated = false;

		if (refcount_read(&ethqos->ps_refcount) != 0)
			__ethqos_emac_power_up(priv);
	}

	ETHQOSINFO("Disable Clock Gating EMAC = %u ps_refcount = %u clk_active = %u\n",
		   priv->plat->port_num, refcount_read(&ethqos->ps_refcount), ethqos->clk_active);

	mutex_unlock(&ethqos->ps_lock);
}

int ethqos_enable_power_saving(struct net_device *ndev, bool enable)
{
	int ret = 0;
	struct stmmac_priv *priv = netdev_priv(ndev);
	struct qcom_ethqos *ethqos = priv->plat->bsp_priv;
	unsigned int old_refcount, new_refcount;

	mutex_lock(&ethqos->ps_lock);

	old_refcount = refcount_read(&ethqos->ps_refcount);

	if (enable) {
		if (refcount_dec_and_test(&ethqos->ps_refcount))
			ret = __ethqos_emac_power_down(priv);
	} else {
		if (!refcount_inc_not_zero(&ethqos->ps_refcount)) {
			refcount_set(&ethqos->ps_refcount, 1);
			ret = __ethqos_emac_power_up(priv);
		}
	}

	new_refcount = refcount_read(&ethqos->ps_refcount);

	ETHQOSINFO("EMAC = %d Save Power = %u ps_refcount = %u->%u\n",
		   priv->plat->port_num, enable, old_refcount, new_refcount);

	mutex_unlock(&ethqos->ps_lock);

	return ret;
}
#endif

/**
 * Currently c37 an is not supported on 2500Basex and SGMII 2.5Gbps
 */
bool is_c37_an_supported(struct stmmac_priv *priv)
{
	if (priv->plat->interface == PHY_INTERFACE_MODE_2500BASEX)
		return false;
	if (priv->speed == SPEED_2500 && priv->plat->interface == PHY_INTERFACE_MODE_SGMII)
		return false;
	return true;
}

void ethqos_set_mac_speed_mode_duplex(struct stmmac_priv *priv, int speed,
				      phy_interface_t interface, int duplex)
{
	u32 ctrl;

	ctrl = readl(priv->ioaddr + MAC_CTRL_REG);
	ctrl &= ~priv->hw->link.speed_mask;

	if (interface == PHY_INTERFACE_MODE_USXGMII || interface == PHY_INTERFACE_MODE_5GBASER) {
		switch (speed) {
		case SPEED_10000:
			ctrl |= priv->hw->link.xgmii.speed10000;
			break;
		case SPEED_5000:
			ctrl |= priv->hw->link.xgmii.speed5000;
			break;
		case SPEED_2500:
			ctrl |= priv->hw->link.xgmii.speed2500;
			break;
		case SPEED_1000:
			ctrl |= priv->hw->link.speed1000;
			break;
		case SPEED_100:
			ctrl |= priv->hw->link.speed100;
			break;
		case SPEED_10:
			ctrl |= priv->hw->link.speed10;
			break;
		default:
			return;
		}
	} else if (interface == PHY_INTERFACE_MODE_XLGMII) {
		switch (speed) {
		case SPEED_100000:
			ctrl |= priv->hw->link.xlgmii.speed100000;
			break;
		case SPEED_50000:
			ctrl |= priv->hw->link.xlgmii.speed50000;
			break;
		case SPEED_40000:
			ctrl |= priv->hw->link.xlgmii.speed40000;
			break;
		case SPEED_25000:
			ctrl |= priv->hw->link.xlgmii.speed25000;
			break;
		case SPEED_10000:
			ctrl |= priv->hw->link.xgmii.speed10000;
			break;
		case SPEED_2500:
			ctrl |= priv->hw->link.speed2500;
			break;
		case SPEED_1000:
			ctrl |= priv->hw->link.speed1000;
			break;
		default:
			return;
		}
	} else {
		switch (speed) {
		case SPEED_2500:
			if (interface == PHY_INTERFACE_MODE_2500BASEX)
				ctrl |= priv->hw->link.xgmii.speed2500;
			else
				ctrl |= priv->hw->link.speed2500;
			break;
		case SPEED_1000:
			ctrl |= priv->hw->link.speed1000;
			break;
		case SPEED_100:
			ctrl |= priv->hw->link.speed100;
			break;
		case SPEED_10:
			ctrl |= priv->hw->link.speed10;
			break;
		default:
			return;
		}
	}

	if (!duplex)
		ctrl &= ~priv->hw->link.duplex;
	else
		ctrl |= priv->hw->link.duplex;

	writel(ctrl, priv->ioaddr + MAC_CTRL_REG);
}

static void ethqos_xpcs_link_up(void *priv_n, unsigned int speed)
{
	struct qcom_ethqos *ethqos = priv_n;
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);
	u32 retry = 10;

	if (!priv || !priv->hw->qxpcs) {
		ETHQOSERR("QXPCS doesn't exist");
		return;
	}

	// SGMII 2.5Gbps and 2500BaseX doesn't support PCS AN interrupt
	if (priv->plat->fixed_phy_mode && is_c37_an_supported(priv) && priv->hw->qxpcs->c37_an_en)
		qcom_xpcs_configure_intr(&priv->hw->qxpcs->pcs, priv->plat->interface,
					 speed, DUPLEX_FULL);
	else if (priv->dev->phydev)
		qcom_xpcs_link_up(&priv->hw->qxpcs->pcs, 1, priv->plat->interface,
				  speed, priv->dev->phydev->duplex);

	/* Check PCS link up and do serdes reset when client reconnects */
	do {
		if (!qcom_xpcs_verify_lnk_status_usxgmii(priv->hw->qxpcs))
			break;
		if (priv->plat->serdes_phy_soft_reset)
			priv->plat->serdes_phy_soft_reset(priv->plat->bsp_priv);
		usleep_range(15000, 20000);
	} while (--retry);
}

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_HOSTVM)
static int qcom_ethqos_vm_notifier(struct notifier_block *nb,
				   unsigned long event, void *ptr)
{
	struct qcom_ethqos *ethqos = container_of(nb, struct qcom_ethqos, vm_nb);
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);
	gh_vmid_t cb_vm_id = *(gh_vmid_t *)ptr;
	gh_vmid_t v2x_vm_id = 0;
	int result;

	if (event == GH_VM_BEFORE_POWERUP) {
		result = gh_rm_get_vmid(GH_TELE_VM, &v2x_vm_id);
		if (result) {
			ETHQOSINFO("gh_rm_get_vmid() failed %d", result);
			return NOTIFY_DONE;
		}
		priv->v2x_vm_id = v2x_vm_id;
	}
	if (cb_vm_id != priv->v2x_vm_id) {
		ETHQOSINFO("vm id mismatch, ignoring cb_vm %u v2x_vm %u event %u",
			   cb_vm_id, v2x_vm_id, event);
		return NOTIFY_DONE;
	}

	ETHQOSINFO("vm_ssr 0x%p stmmac_priv 0x%p event = %d\n", ethqos, ethqos->priv, event);

	if (event == GH_VM_EARLY_POWEROFF) {
		stmmac_stop_rx(priv, priv->ioaddr, 4);
		stmmac_stop_tx(priv, priv->ioaddr, 4);
	}

	return NOTIFY_DONE;
}

static int qcom_ethqos_register_vm_notifier(struct qcom_ethqos *ethqos)
{
	ethqos->vm_nb.notifier_call = qcom_ethqos_vm_notifier;
	return gh_register_vm_notifier(&ethqos->vm_nb);
}

static int qcom_ethqos_unregister_vm_notifier(struct qcom_ethqos *ethqos)
{
	return gh_unregister_vm_notifier(&ethqos->vm_nb);
}
#else
static int qcom_ethqos_register_vm_notifier(struct qcom_ethqos *ethqos)
{
	return 0;
}

static int qcom_ethqos_unregister_vm_notifier(struct qcom_ethqos *ethqos)
{
	return 0;
}
#endif /* CONFIG_ETHQOS_QCOM_HOSTVM */

signed int ethqos_phylib_mmd_read(struct phy_device *phydev, unsigned int mmd,
				  unsigned int reg, unsigned char msb,
				  unsigned char lsb, unsigned int *pdata)
{
	unsigned int rdata = 0;
	unsigned int mask = 0;

	mask = ((0xFFFFFFFF >> (31 - msb)) ^ ((1 << lsb) - 1));
	rdata =  phy_read_mmd(phydev, mmd, reg);
	*pdata = ((rdata & (mask)) >> (lsb));
	return 0;
}

signed int ethqos_phylib_mmd_write(struct phy_device *phydev, unsigned int mmd,
				   unsigned int reg, unsigned char msb,
				   unsigned char lsb, unsigned int data)
{
	signed int  ret = 0;
	unsigned int mask = 0;

	mask = ((0xFFFFFFFF >> (31 - msb)) ^ ((1 << lsb) - 1));
	ret = phy_modify_mmd(phydev, mmd, reg, mask, (data << lsb));

	return ret;
}

void ethqos_phylib_udelay(unsigned int usec)
{
	if (usec >= 1000) {
		mdelay(usec / 1000);
		usec = usec % 1000;
	}
	udelay(usec);
}

int ethqos_phylib_826xb_sds_read(struct phy_device *phydev, unsigned int page,
				 unsigned int reg, unsigned char msb,
				 unsigned char lsb, unsigned int *pdata)
{
	unsigned int rdata = 0;
	unsigned int op = (page & 0x3f) | ((reg & 0x1f) << 6) | (0x8000);
	unsigned int i = 0;
	unsigned int mask = 0;

	mask = ((0xFFFFFFFF >> (31 - msb)) ^ ((1 << lsb) - 1));
	ethqos_phylib_mmd_write(phydev, 30, 323, 15, 0, op);

	for (i = 0; i < 10; i++) {
		ethqos_phylib_mmd_read(phydev, 30, 323, 15, 15, &rdata);
		if (rdata == 0)
			break;
		ethqos_phylib_udelay(10);
	}
	if (i == 10)
		return -1;

	ethqos_phylib_mmd_read(phydev, 30, 322, 15, 0, &rdata);
	*pdata = ((rdata & (mask)) >> (lsb));

	return 0;
}

int ethqos_phylib_826xb_sds_write(struct phy_device *phydev, unsigned int page,
				  unsigned int reg, unsigned char msb,
				  unsigned char lsb, unsigned int data)
{
	unsigned int wdata = 0, rdata = 0;
	unsigned int op = (page & 0x3f) | ((reg & 0x1f) << 6) | (0x8800);
	unsigned int mask = 0;

	mask = ((0xFFFFFFFF >> (31 - msb)) ^ ((1 << lsb) - 1));
	ethqos_phylib_826xb_sds_read(phydev, page, reg, 15, 0, &rdata);

	wdata = ((rdata & ~(mask)) | ((data << (lsb)) & (mask)));

	ethqos_phylib_mmd_write(phydev, 30, 321, 15, 0, wdata);
	ethqos_phylib_mmd_write(phydev, 30, 323, 15, 0, op);

	return 0;
}

enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
	struct qcom_ethqos *ethqos = pethqos[0];

	ETHQOSINFO("%s: Time_out happen\n", __func__);
	ethqos->peer_resp = TIME_OUT;
	if (ethqos->init_complete)
		complete(&ethqos->thermal_timer);
	return HRTIMER_NORESTART;
}

int qcom_ethqos_bring_up_phy_if(struct device *dev, bool client_mode)
{
	int ret = 0;
	int res = 0;
	struct net_device *ndev;
	struct stmmac_priv *priv;
	struct qcom_ethqos *ethqos = pethqos[0];
	struct phy_device *phydev = NULL;
	struct net *net;
	ktime_t ktime;
	struct property *speed_prop;
	static u32 speed;
	struct device_node *fixed_phy_node;
	struct platform_device *pdev;
	enum eth_mode data = 0;

	ndev = platform_get_drvdata(ethqos->pdev);
	if (!ndev) {
		ETHQOSERR("Netdevice is NULL\n");
		return -EINVAL;
	}

	net = dev_net(ndev);
	if (!net)
		return -EINVAL;

	priv = netdev_priv(ndev);
	if (!priv) {
		ETHQOSERR("priv is NULL\n");
		return -EINVAL;
	}

	pdev = ethqos->pdev;
	if (!pdev) {
		ETHQOSERR("pdev is NULL\n");
		return -EINVAL;
	}

	if (priv->plat->fixed_phy_mode) {
		if (!client_mode) {
			switch (ethqos->current_mode) {
			case USXGMII_10G:
				priv->plat->interface = PHY_INTERFACE_MODE_USXGMII;
				priv->plat->phy_interface = PHY_INTERFACE_MODE_USXGMII;
				break;
			case USXGMII_5G:
				priv->plat->interface = PHY_INTERFACE_MODE_5GBASER;
				priv->plat->phy_interface = PHY_INTERFACE_MODE_5GBASER;
				break;
			case SGMII_1G:
				priv->plat->interface = PHY_INTERFACE_MODE_SGMII;
				priv->plat->phy_interface = PHY_INTERFACE_MODE_SGMII;
				break;
			default:
				ETHQOSERR("Invalid mode %d\n", ethqos->current_mode);
				goto error;
			}
		}
		if (client_mode) {
			switch (priv->plat->interface) {
			case PHY_INTERFACE_MODE_USXGMII:
				data = USXGMII_10G;
				break;
			case PHY_INTERFACE_MODE_5GBASER:
				data = USXGMII_5G;
				break;
			case PHY_INTERFACE_MODE_SGMII:
				data = SGMII_1G;
				break;
			default:
				ETHQOSERR("Invalid I/F %d\n", priv->plat->interface);
				goto error;
			}

			ret = ethqos_send_netlink_socket(data, THERM_NEW_LOCAL_ETH_MODE, SUCCESS);
			if (ret < 0)
				goto error;

			if (ethqos->curr_phy_state == ETHQOS_PHY_STATE_DOWN)
				netif_device_attach(ndev);
			init_completion(&ethqos->thermal_timer);
			ethqos->init_complete = true;
			ktime = ktime_set(TIMEOUT_SEC, TIMEOUT_NSEC);
			hrtimer_init(&ethqos->hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
			ethqos->hr_timer.function = &timer_callback;
			hrtimer_start(&ethqos->hr_timer, ktime, HRTIMER_MODE_REL);
			wait_for_completion(&ethqos->thermal_timer);
			hrtimer_cancel(&ethqos->hr_timer);
			netif_device_detach(ndev);

			if (ethqos->peer_resp == TIME_OUT) {
				ret = ethqos_send_netlink_socket(data,
								 THERM_NEW_LOCAL_ETH_MODE_UPDATE,
								 TIMEOUT);
				if (ret < 0)
					goto error;
				goto phy_state_down;
			} else if (ethqos->peer_resp >= FAILED) {
				ret = ethqos_send_netlink_socket(data,
								 THERM_NEW_LOCAL_ETH_MODE_UPDATE,
								 FAIL);
				if (ret < 0)
					goto error;
				goto phy_state_down;
			} else if (ethqos->peer_cap != SGMII_1G &&
				   ethqos->peer_cap != USXGMII_5G &&
				   ethqos->peer_cap != USXGMII_10G &&
				   ethqos->peer_cap != (SGMII_1G | USXGMII_5G) &&
				   ethqos->peer_cap != (SGMII_1G | USXGMII_10G) &&
				   ethqos->peer_cap != (USXGMII_5G | USXGMII_10G) &&
				   ethqos->peer_cap != (SGMII_1G | USXGMII_5G | USXGMII_10G)) {
				ret = ethqos_send_netlink_socket(data,
								 THERM_NEW_LOCAL_ETH_MODE_UPDATE,
								 INVALID_SPEED);
				if (ret < 0)
					goto error;
				goto phy_state_down;
			}

			ethqos->curr_phy_state = ETHQOS_PHY_STATE_UP;
		}
		fixed_phy_node = of_get_child_by_name(pdev->dev.of_node, "fixed-link");
		if (fixed_phy_node && of_device_is_available(fixed_phy_node)) {

			speed_prop = kzalloc(sizeof(*speed_prop), GFP_KERNEL);

			if (!speed_prop) {
				ETHQOSERR("kzalloc failed\n");
				return -ENOMEM;
			}

			if (priv->plat->interface == PHY_INTERFACE_MODE_SGMII)
				speed = cpu_to_be32(SPEED_1000);
			else if (priv->plat->interface ==  PHY_INTERFACE_MODE_USXGMII)
				speed = cpu_to_be32(SPEED_10000);
			else if (priv->plat->interface == PHY_INTERFACE_MODE_5GBASER)
				speed = cpu_to_be32(SPEED_5000);

			speed_prop->name = kstrdup("speed", GFP_KERNEL);
			speed_prop->value = &speed;
			speed_prop->length = sizeof(u32);

			if (!(of_update_property(fixed_phy_node, speed_prop) == 0)) {
				kfree(speed_prop);
				ETHQOSERR("Fixed-link speed update failed\n");
				return -ENOENT;
			}
		}
		of_node_put(fixed_phy_node);
	}

	if (!priv->plat->mac2mac_en) {
		res = stmmac_phy_setup(priv);
		if (res) {
			ETHQOSERR("failed to setup phy (%d)\n", res);
			goto error;
		}
	}

	ret = ethqos_xpcs_init(ndev);
	if (ret < 0) {
		ETHQOSERR("failed to init xpcs (%d)\n", ret);
		goto error;
	}

	if (!priv->plat->mac2mac_en && !priv->plat->fixed_phy_mode) {
		if (priv->phydev && priv->phydev->drv->get_features &&
		    priv->plat->interface ==  PHY_INTERFACE_MODE_USXGMII &&
		    !priv->plat->mac2mac_en)
			priv->phydev->drv->get_features(priv->phydev);

		if (!priv->plat->mac2mac_en && priv->phydev) {
			phydev = priv->phydev;
			rtnl_lock();
			phylink_connect_phy(priv->phylink, priv->phydev);
			rtnl_unlock();

			if (phydev && phydev->drv->phy_id == ETH_RTK_PHY_ID_RTL8261N) {
				if (phydev->interface == PHY_INTERFACE_MODE_USXGMII) {
					ETHQOSDBG("set_max_speed 10G\n");
					phy_set_max_speed(phydev, SPEED_10000);
				}

				if (phydev->interface == PHY_INTERFACE_MODE_SGMII) {
					/* disabling serdes for usxgmii */
					ethqos_phylib_826xb_sds_write(phydev, 7, 17, 0, 0, 0);

					ETHQOSDBG("set_max_speed 1G\n");
					phy_set_max_speed(phydev, SPEED_1000);

					/* enabling  serdes  and putting into fore mode for sgmii */
					ethqos_phylib_826xb_sds_write(phydev, 0, 2, 9, 9, 1);
					ethqos_phylib_826xb_sds_write(phydev, 0, 2, 8, 8, 0);

					/* Register Access APIs */
					ethqos_phylib_mmd_write(phydev, 30, 0x105, 0, 0, 0x1);
					ethqos_phylib_mmd_write(phydev, 30, 0xc3, 4, 0, 0x2);
					ethqos_phylib_mmd_write(phydev, 30, 0xc2, 9, 5, 0x0);
					ethqos_phylib_mmd_write(phydev, 30, 0x2a2, 7, 7, 0x0);
				}
			}

			if (priv->plat->phy_intr_en_extn_stm && phydev) {
				ETHQOSDBG("PHY interrupt Mode enabled\n");
				phydev->irq = PHY_MAC_INTERRUPT;
				phydev->interrupts =  PHY_INTERRUPT_ENABLED;

				if (phydev->drv->config_intr &&
				    !phydev->drv->config_intr(phydev))
					ETHQOSDBG("config_phy_intr successful after phy on\n");
			} else if (!priv->plat->phy_intr_en_extn_stm && phydev) {
				phydev->irq = PHY_POLL;
				ETHQOSDBG("PHY Polling Mode enabled\n");
			} else {
				ETHQOSERR("phydev is null , intr value=%d\n",
					  priv->plat->phy_intr_en_extn_stm);
			}

			if (!priv->phy_irq_enabled && !priv->plat->mac2mac_en)
				priv->plat->phy_irq_enable(priv);
		}
	}

	ret = stmmac_resume(&ethqos->pdev->dev);

	if (priv->plat->fixed_phy_mode && client_mode) {
		if (ethqos->peer_resp == 1 || ethqos->peer_resp == 2) {
			ret = ethqos_send_netlink_socket(data,
							 THERM_NEW_LOCAL_ETH_MODE_UPDATE,
							 SUCCESS);
			if (ret < 0)
				goto error;
		}
	}
	if (priv->plat->interface ==  PHY_INTERFACE_MODE_USXGMII) {
		rtnl_lock();
		if (netif_running(ndev)) {
			ret = ndev->netdev_ops->ndo_stop(ndev);
			if (ret) {
				rtnl_unlock();
				goto error;
			}
			ret = ndev->netdev_ops->ndo_open(ndev);
			if (ret) {
				rtnl_unlock();
				goto error;
			}
		}
		rtnl_unlock();
	}

	return ret;
error:
	pr_info("%s %d bring_up failed\n", __func__, __LINE__);
	return -1;
phy_state_down:
	ethqos->curr_phy_state = ETHQOS_PHY_STATE_DOWN;
	return 0;
}

int qcom_ethqos_bring_down_phy_if(struct device *dev)
{
	struct net_device *netdev;
	struct stmmac_priv *priv;
	struct qcom_ethqos *ethqos = pethqos[0];
	struct resource *resource = NULL;
	unsigned long xpcs_size = 0;
	void __iomem *xpcs_addr;
	int ret = 0;

	if (ethqos->curr_phy_state == ETHQOS_PHY_STATE_UP) {
		netdev = platform_get_drvdata(ethqos->pdev);
		if (!netdev) {
			ETHQOSERR("netdev is NULL\n");
			return -EINVAL;
		}
		priv = netdev_priv(netdev);
		if (!priv) {
			ETHQOSERR("priv is NULL\n");
			return -EINVAL;
		}

		if (!priv->plat) {
			ETHQOSERR("priv->plat is NULL\n");
			return -EINVAL;
		}

		ethqos->init_complete = false;

		resource = platform_get_resource_byname(ethqos->pdev,
							IORESOURCE_MEM, "xpcs");
		if (!resource) {
			ETHQOSERR("Resource xpcs not found\n");
			return -1;
		}
		xpcs_size = resource_size(resource);

		ret = stmmac_suspend(&ethqos->pdev->dev);
		netif_device_attach(netdev);

		if (priv->hw->qxpcs) {
			if (priv->hw->qxpcs->intr_en) {
				cancel_work_sync(&ethqos->emac_pcs_work);
				flush_work(&ethqos->emac_pcs_work);
				free_irq(priv->hw->qxpcs->pcs_intr, priv);
			}
			xpcs_addr = priv->hw->qxpcs->addr;
			qcom_xpcs_destroy(priv->hw->qxpcs);
			iounmap(xpcs_addr);
			xpcs_addr = NULL;
			priv->hw->qxpcs = NULL;
			release_mem_region(resource->start, xpcs_size);
		}

		if (priv->phy_irq_enabled && !priv->plat->mac2mac_en) {
			priv->plat->phy_irq_disable(priv);

			if (priv->phylink) {
				rtnl_lock();
				phylink_disconnect_phy(priv->phylink);
				rtnl_unlock();
			}
			if (!priv->plat->mac2mac_en && priv->phylink)
				phylink_destroy(priv->phylink);
		}
	}
	return ret;
}

static ssize_t change_if_sysfs_read(struct device *dev,
				    struct device_attribute *attr,
				    char *user_buf)
{
	struct net_device *netdev = to_net_dev(dev);
	struct stmmac_priv *priv;

	if (!netdev)
		return -EINVAL;

	priv = netdev_priv(netdev);
	if (!priv)
		return -EINVAL;

	if (priv->plat->interface ==  PHY_INTERFACE_MODE_USXGMII)
		return scnprintf(user_buf, BUFF_SZ,
				"Current-Interface :: PHY_INTERFACE_MODE_USXGMII");
	else if (priv->plat->interface ==  PHY_INTERFACE_MODE_SGMII)
		return scnprintf(user_buf, BUFF_SZ,
				"Current-Interface :: PHY_INTERFACE_MODE_SGMII");

	return scnprintf(user_buf, BUFF_SZ,
			"Current-Interface :: unknown");
}

static ssize_t change_if_sysfs_write(struct device *dev,
				     struct device_attribute *attr,
				     const char *user_buf,
				     size_t count)
{
	s8 option = 0;
	struct net_device *ndev = to_net_dev(dev);
	struct stmmac_priv *priv;

	if (!ndev) {
		ETHQOSERR("changing I/F not possible\n");
		return -EINVAL;
	}
	priv = netdev_priv(ndev);
	if (!priv) {
		ETHQOSERR("priv is NULL\n");
		return -EINVAL;
	}
	if (kstrtos8(user_buf, 0, &option))
		return -EFAULT;

	if (priv->plat->tm_u5g_en) {
		if (option == 0) {
			priv->plat->interface = PHY_INTERFACE_MODE_USXGMII;
			priv->plat->phy_interface = PHY_INTERFACE_MODE_USXGMII;
		} else if (option == 1) {
			priv->plat->interface = PHY_INTERFACE_MODE_5GBASER;
			priv->plat->phy_interface = PHY_INTERFACE_MODE_5GBASER;
		}
	} else {
		if (option == 0) {
			priv->plat->interface = PHY_INTERFACE_MODE_USXGMII;
			priv->plat->phy_interface = PHY_INTERFACE_MODE_USXGMII;
		} else if (option == 1) {
			priv->plat->interface = PHY_INTERFACE_MODE_SGMII;
			priv->plat->phy_interface = PHY_INTERFACE_MODE_SGMII;
		}
	}

	return count;
}

static DEVICE_ATTR(change_if, TN_SYSFS_DEV_ATTR_PERMS,
		   change_if_sysfs_read,
		   change_if_sysfs_write);

static int ethqos_change_if_cleanup_sysfs(struct qcom_ethqos *ethqos)
{
	struct net_device *netdev;

	if (!ethqos) {
		ETHQOSERR("ethqos is NULL\n");
		return -EINVAL;
	}

	netdev = platform_get_drvdata(ethqos->pdev);

	if (!netdev) {
		ETHQOSERR("netdev is NULL\n");
		return -EINVAL;
	}

	sysfs_remove_file(&netdev->dev.kobj,
			  &dev_attr_change_if.attr);
	return 0;
}

/**
 * ethqos_change_if_create_sysfs() - Called to create sysfs node
 * for creating change_if entry.
 */
static int ethqos_change_if_create_sysfs(struct qcom_ethqos *ethqos)
{
	int ret;
	struct net_device *netdev;

	if (!ethqos) {
		ETHQOSERR("ethqos is NULL\n");
		return -EINVAL;
	}

	netdev = platform_get_drvdata(ethqos->pdev);

	if (!netdev) {
		ETHQOSERR("netdev is NULL\n");
		return -EINVAL;
	}

	ret = sysfs_create_file(&netdev->dev.kobj,
				&dev_attr_change_if.attr);
	if (ret) {
		ETHQOSERR("unable to create change_if sysfs node\n");
		return -ENOMEM;
	}
	return 0;
}

static ssize_t thermal_netlink_sysfs_read(struct device *dev,
					  struct device_attribute *attr,
					  char *user_buf)
{
	return scnprintf(user_buf, BUFF_SZ,
			"option 1 for bringdown, 2 for bringup");
}

static ssize_t thermal_netlink_sysfs_write(struct device *dev,
					   struct device_attribute *attr,
					   const char *user_buf,
					   size_t count)
{
	s8 option = 0;
	int ret = 0;

	if (kstrtos8(user_buf, 0, &option))
		return -EFAULT;

	if (option == 1) {
		ret = qcom_ethqos_bring_down_phy_if(dev);
		if (ret < 0) {
			ETHQOSERR("%s failed %d\n", __func__, __LINE__);
			return -EINVAL;
		}
	} else if (option == 0) {
		ret = qcom_ethqos_bring_up_phy_if(dev, 1);
		if (ret < 0) {
			ETHQOSERR("%s failed %d\n", __func__, __LINE__);
			return -EINVAL;
		}
	}

	return count;
}

static DEVICE_ATTR(thermal_netlink, TN_SYSFS_DEV_ATTR_PERMS,
		   thermal_netlink_sysfs_read,
		   thermal_netlink_sysfs_write);

static int ethqos_thermal_netlink_cleanup_sysfs(struct qcom_ethqos *ethqos)
{
	struct net_device *netdev;

	if (!ethqos) {
		ETHQOSERR("ethqos is NULL\n");
		return -EINVAL;
	}

	netdev = platform_get_drvdata(ethqos->pdev);

	if (!netdev) {
		ETHQOSERR("netdev is NULL\n");
		return -EINVAL;
	}

	sysfs_remove_file(&netdev->dev.kobj,
			  &dev_attr_thermal_netlink.attr);
	return 0;
}

/**
 * ethqos_thermal_netlink_create_sysfs() - Called to create sysfs node
 * for creating thermal_netlink entry.
 */
static int ethqos_thermal_netlink_create_sysfs(struct qcom_ethqos *ethqos)
{
	int ret;
	struct net_device *netdev;

	if (!ethqos) {
		ETHQOSERR("ethqos is NULL\n");
		return -EINVAL;
	}

	netdev = platform_get_drvdata(ethqos->pdev);

	if (!netdev) {
		ETHQOSERR("netdev is NULL\n");
		return -EINVAL;
	}

	ret = sysfs_create_file(&netdev->dev.kobj,
				&dev_attr_thermal_netlink.attr);
	if (ret) {
		ETHQOSERR("unable to create thermal_netlink sysfs node\n");
		return -ENOMEM;
	}
	return 0;
}

static void qcom_ethqos_init_aux_ts(struct qcom_ethqos *ethqos,
				    struct plat_stmmacenet_data *plat_dat,
				    struct stmmac_priv *priv)
{
	struct device_node *np = ethqos->pdev->dev.of_node;
	int i = 0;

	of_property_read_u32(np, "qcom,aux_ts_num_pins", &priv->aux_ts_num_pins);

	for (i = 0; i < priv->aux_ts_num_pins; i++)
		plat_dat->ext_snapshot_num |= (1 << (i + 4));

	dev_info(&ethqos->pdev->dev, "ext_snapshot_num = %d\n", plat_dat->ext_snapshot_num);
}

static void ethqos_get_dt_qos_config(struct device_node *np)
{
	const char *qoscfg;
	int err;

	err = of_property_read_string(np, "config-qos", &qoscfg);
	if (err < 0) {
		mparams.qoscfg_name[0] = '\0';
		return;
	}

	strscpy(mparams.qoscfg_name, qoscfg, sizeof(mparams.qoscfg_name));
}

#ifdef MODULE
static int ethqos_set_early_eth_params(void)
{
	int ret = 0;

	if (eipv4)
		ret = set_early_ethernet_ipv4(eipv4);

	if (eipv6)
		ret = set_early_ethernet_ipv6(eipv6);

	if (ermac)
		ret = set_early_ethernet_mac(ermac);

	if (eiface)
		ret = set_ethernet_interface(eiface);

	if (eqos)
		ret = set_ethernet_qos_cfg(eqos);

	if (ewait_switch_rdy)
		ret = set_ethernet_wait_switch_rdy(ewait_switch_rdy);

	if (eautoneg)
		ret = set_ethernet_autoneg(eautoneg);
	return ret;
}
#endif

static int qcom_ethqos_probe_config_dt(struct platform_device *pdev,
				       struct stmmac_resources *stmmac_res)
{
	int ret = 0;

	plat_dat = stmmac_probe_config_dt(pdev, stmmac_res->mac);
	if (IS_ERR(plat_dat)) {
		dev_err(&pdev->dev, "dt configuration failed\n");
		return PTR_ERR(plat_dat);
	}

	if (strlen(mparams.qoscfg_name) != 0) {
		strscpy(plat_dat->qoscfg, mparams.qoscfg_name, sizeof(mparams.qoscfg_name));
		memset(plat_dat->rx_queues_cfg, 0,
		       (plat_dat->rx_queues_to_use * sizeof(struct stmmac_rxq_cfg)));
		memset(plat_dat->tx_queues_cfg, 0,
		       (plat_dat->tx_queues_to_use * sizeof(struct stmmac_txq_cfg)));
		ret = stmmac_mtl_setup(pdev, plat_dat);
		if (ret) {
			stmmac_remove_config_dt(pdev, plat_dat);
			return ret;
		}
	}

	ETHQOSINFO("RX QOS queues = %d, TX QOS queues = %d RX queues = %d TX queues = %d\n",
		   plat_dat->rx_qos_queues_to_use,
		   plat_dat->tx_qos_queues_to_use,
		   plat_dat->rx_queues_to_use,
		   plat_dat->tx_queues_to_use);
	return ret;
}

static void ethqos_serdes_phy_soft_reset(void *priv)
{
	struct qcom_ethqos *ethqos = (struct qcom_ethqos *)priv;

	qcom_ethqos_serdes_phy_soft_reset(ethqos);
}

static int qcom_ethqos_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct stmmac_resources stmmac_res;
	struct qcom_ethqos *ethqos = NULL;
	int i, ret;
	struct resource	*rgmii_io_block;
	struct net_device *ndev;
	struct stmmac_priv *priv;
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	u32 err = 0;
#endif

	ret = pinctrl_pm_select_default_state(&pdev->dev);
	if (ret)
		ETHQOSERR("pinctrl default select state failed %d\n", ret);

	if (of_device_is_compatible(pdev->dev.of_node,
				    "qcom,emac-smmu-embedded"))
		return emac_emb_smmu_cb_probe(pdev, plat_dat);

#ifdef CONFIG_MSM_BOOT_TIME_MARKER
	update_marker("M - Ethernet probe start");
#endif

#ifdef MODULE
	ethqos_set_early_eth_params();
#endif

	ipc_stmmac_log_ctxt = ipc_log_context_create(IPCLOG_STATE_PAGES,
						     "emac", 0);
	if (!ipc_stmmac_log_ctxt)
		ETHQOSERR("Error creating logging context for emac\n");
	else
		ETHQOSDBG("IPC logging has been enabled for emac\n");
	ret = stmmac_get_platform_resources(pdev, &stmmac_res);
	if (ret)
		return ret;

	ethqos = devm_kzalloc(&pdev->dev, sizeof(*ethqos), GFP_KERNEL);
	if (!ethqos) {
		return -ENOMEM;
	}

	ethqos->pdev = pdev;

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	/* Set ref count to 1 as clocks are initially turned on */
	refcount_set(&ethqos->ps_refcount, 1);
	mutex_init(&ethqos->ps_lock);
	ethqos->clk_active = true;
#endif

	ethqos_init_regulators(ethqos);

	ethqos_init_gpio(ethqos);

	ethqos_get_qoe_dt(ethqos, np);

	/* Use phy address passed from the partition only when the address is a valid one */
	if (phyaddr_pt_param >= 0 && phyaddr_pt_param < PHY_MAX_ADDR) {
		if (ethqos_update_phyid(np))
			goto err_mem;
	}

	if (!(strlen(mparams.qoscfg_name) != 0))
		ethqos_get_dt_qos_config(np);

	qcom_ethqos_probe_config_dt(pdev, &stmmac_res);

	if (mparams.is_valid_eth_intf) {
		plat_dat->interface = mparams.eth_intf;
		plat_dat->phy_interface = mparams.eth_intf;
		plat_dat->is_valid_eth_intf = mparams.is_valid_eth_intf;

		/* Go through default phy detection logic when the
		 * address from the partition is not a valid one
		 */
		if (phyaddr_pt_param < 0 || phyaddr_pt_param >= PHY_MAX_ADDR)
			plat_dat->phy_addr = -1;
	}

	if (pdev->name) {
		if (strnstr(pdev->name, "emac1", strlen(pdev->name)))
			plat_dat->port_num = 1;
		else
			plat_dat->port_num = 0;
	}

	ethqos->socket_nl = netlink_kernel_create(&init_net, NETLINK_TESTFAMILY, NULL);
	if (!ethqos->socket_nl) {
		ETHQOSINFO("netlink_kernel_create : FAILED\n");
		return -1;
	}

	ethqos->rgmii_base = devm_platform_ioremap_resource_byname(pdev, "rgmii");
	if (IS_ERR(ethqos->rgmii_base)) {
		ret = PTR_ERR(ethqos->rgmii_base);
		goto err_mem;
	}

	rgmii_io_block = platform_get_resource_byname(pdev, IORESOURCE_MEM, "rgmii");
	{
		if (!rgmii_io_block) {
			dev_err(&pdev->dev, "Invalid RGMII IO macro phy address\n");
			ret = ENXIO;
			goto err_mem;
		}
	}
	ethqos->rgmii_phy_base = rgmii_io_block->start;

	if (plat_dat->interface == PHY_INTERFACE_MODE_RGMII ||
	    plat_dat->interface == PHY_INTERFACE_MODE_RGMII_ID ||
	    plat_dat->interface == PHY_INTERFACE_MODE_RGMII_RXID ||
	    plat_dat->interface == PHY_INTERFACE_MODE_RGMII_TXID) {
		ethqos->rgmii_clk = devm_clk_get(&pdev->dev, "rgmii");
		if (IS_ERR(ethqos->rgmii_clk)) {
			ret = PTR_ERR(ethqos->rgmii_clk);
			goto err_mem;
		}

		ret = clk_prepare_enable(ethqos->rgmii_clk);
		if (ret)
			goto err_mem;
	} else if (plat_dat->interface == PHY_INTERFACE_MODE_SGMII ||
		   plat_dat->interface ==  PHY_INTERFACE_MODE_USXGMII ||
		   plat_dat->interface == PHY_INTERFACE_MODE_2500BASEX ||
		   plat_dat->interface == PHY_INTERFACE_MODE_5GBASER) {
		ret = ethqos_init_sgmii_regulators(ethqos);
		if (ret)
			goto err_mem;
		ret = ethqos_enable_sgmii_usxgmii_clks(ethqos, plat_dat->interface);
		if (ret)
			goto err_mem;
	}

	/* Read mac address from fuse register */
	read_mac_addr_from_fuse_reg(np);

	/*Initialize Early ethernet to false*/
	ethqos->early_eth_enabled = false;

	/*Check for valid mac, ip address to enable Early eth*/
	if (pparams.is_valid_mac_addr &&
	    (pparams.is_valid_ipv4_addr || pparams.is_valid_ipv6_addr)) {
		/* For 1000BASE-T mode, auto-negotiation is required and
		 * always used to establish a link.
		 * Configure phy and MAC in 100Mbps mode with autoneg
		 * disable as link up takes more time with autoneg
		 * enabled.
		 */
		ethqos->early_eth_enabled = true;
		ETHQOSINFO("Early ethernet is enabled\n");
	}

	if (plat_dat->interface == PHY_INTERFACE_MODE_SGMII ||
	    plat_dat->interface == PHY_INTERFACE_MODE_USXGMII ||
	    plat_dat->interface == PHY_INTERFACE_MODE_2500BASEX ||
	    plat_dat->interface == PHY_INTERFACE_MODE_5GBASER)
		qcom_ethqos_serdes_configure_dt(ethqos, plat_dat->interface);

	ethqos->axi_icc_path = of_icc_get(&pdev->dev, "axi_icc_path");
	if (!ethqos->axi_icc_path || IS_ERR(ethqos->axi_icc_path)) {
		ETHQOSERR("Interconnect not found for Emac->Axi path\n");
		ethqos->emac_axi_icc = NULL;
	} else {
		ethqos->emac_axi_icc = emac_axi_icc_data;
	}

	ethqos->apb_icc_path = of_icc_get(&pdev->dev, "apb_icc_path");
	if (!ethqos->apb_icc_path || IS_ERR(ethqos->apb_icc_path)) {
		ETHQOSERR("Interconnect not found for Emac->Apb path\n");
		ethqos->emac_apb_icc = NULL;
	} else {
		ethqos->emac_apb_icc = emac_apb_icc_data;
	}

	ethqos->speed = SPEED_10;
	ethqos_update_clk_and_bus_cfg(ethqos, SPEED_10, plat_dat->interface);

	plat_dat->early_eth = ethqos->early_eth_enabled;
	plat_dat->bsp_priv = ethqos;
	mutex_init(&ethqos->mdio_icc_lock);
	INIT_DELAYED_WORK(&ethqos->mdio_icc_idle_work, ethqos_mdio_icc_idle_handler);
	plat_dat->mdio_icc_acquire = ethqos_mdio_icc_acquire;
	plat_dat->mdio_icc_release = ethqos_mdio_icc_release;
	plat_dat->mdio_icc_cancel  = ethqos_mdio_icc_cancel;
	plat_dat->fix_mac_speed = ethqos_fix_mac_speed;
	plat_dat->serdes_update_speed = ethqos_serdes_update;
	plat_dat->serdes_phy_soft_reset = ethqos_serdes_phy_soft_reset;
	plat_dat->dump_debug_regs = rgmii_dump;
	plat_dat->tx_select_queue = dwmac_qcom_select_queue;
	plat_dat->get_plat_tx_coal_frames =  dwmac_qcom_get_plat_tx_coal_frames;
	/* Set mdio phy addr probe capability to c22 .
	 * If c22_c45 is set then multiple phy is getting detected.
	 */
	if (of_property_read_bool(np, "eth-c22-mdio-probe")) {
		plat_dat->has_c22_mdio_probe_capability = 1;
		plat_dat->has_c45_mdio_probe_capability = 0;
	} else if (of_property_read_bool(np, "eth-c45-mdio-probe")) {
		plat_dat->has_c45_mdio_probe_capability = 1;
		plat_dat->has_c22_mdio_probe_capability = 0;
	}

	if (!!of_find_property(np, "eth_aux_ts_enabled", NULL))
		plat_dat->enable_aux_ts = true;

	ETHQOSDBG("Aux Timestamp Feature  = %d\n", plat_dat->enable_aux_ts);

	plat_dat->tso_en = of_property_read_bool(np, "snps,tso");
	plat_dat->handle_prv_ioctl = ethqos_handle_prv_ioctl;
	plat_dat->request_phy_wol = qcom_ethqos_request_phy_wol;
	plat_dat->init_pps = ethqos_init_pps;
	plat_dat->phy_irq_enable = ethqos_phy_irq_enable;
	plat_dat->phy_irq_disable = ethqos_phy_irq_disable;
	plat_dat->get_eth_type = dwmac_qcom_get_eth_type;
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	plat_dat->set_skb_prio = dwmac_qcom_set_skb_prio;
	plat_dat->is_skprio_routing = is_skprio_routing_set;
#endif
	plat_dat->mac_err_rec = of_property_read_bool(np, "mac_err_rec");
	plat_dat->probe_invoke_if_up = of_property_read_bool(np, "probe_invoke_if_up");
	if (plat_dat->mac_err_rec)
		plat_dat->handle_mac_err = dwmac_qcom_handle_mac_err;
	plat_dat->enable_pfc = of_property_read_bool(np, "enable-pfc");
	plat_dat->qos_use_skprio = of_property_read_bool(np, "qos-use-skprio");
	if (plat_dat->has_xgmac)
		plat_dat->insert_ts_pktid = true;

	if (plat_dat->interface == PHY_INTERFACE_MODE_SGMII ||
	    plat_dat->interface == PHY_INTERFACE_MODE_USXGMII ||
	    plat_dat->interface == PHY_INTERFACE_MODE_2500BASEX ||
	    plat_dat->interface == PHY_INTERFACE_MODE_5GBASER) {
		plat_dat->serdes_powerup = ethqos_serdes_power_up;
		plat_dat->serdes_powersaving = ethqos_serdes_power_saving;
		plat_dat->xpcs_linkup = ethqos_xpcs_link_up;
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
		plat_dat->xpcs_powersaving = ethqos_xpcs_power_saving;
#endif
	}

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	plat_dat->enable_power_saving = ethqos_enable_power_saving;
#endif
	plat_dat->c37_an_en = mparams.c37_an_en;

	if (of_property_read_bool(pdev->dev.of_node, "qcom,arm-smmu")) {
		emac_emb_smmu_ctx.pdev_master = pdev;
		ret = of_platform_populate(pdev->dev.of_node,
					   qcom_ethqos_match, NULL, &pdev->dev);
		if (ret)
			ETHQOSERR("Failed to populate EMAC platform\n");
		if (emac_emb_smmu_ctx.ret) {
			ETHQOSERR("smmu probe failed\n");
			of_platform_depopulate(&pdev->dev);
			ret = emac_emb_smmu_ctx.ret;
			emac_emb_smmu_ctx.ret = 0;
		}
	}

	if (ethqos_fixed_link_check(pdev))
		goto err_mem;

	if (!plat_dat->mac2mac_en && !plat_dat->fixed_phy_mode) {
		if (!ethqos->early_eth_enabled) {
			if (of_property_read_bool(pdev->dev.of_node,
						  "emac-phy-off-suspend")) {
				ret = of_property_read_u32(pdev->dev.of_node,
							   "emac-phy-off-suspend",
							   &ethqos->current_phy_mode);
				if (ret) {
					ETHQOSDBG(":resource emac-phy-off-suspend! ");
					ETHQOSDBG("not in dtsi\n");
					ethqos->current_phy_mode = 0;
				}
			}
		} else {
			ethqos->current_phy_mode = DISABLE_PHY_ON_OFF;
		}
	}

	ETHQOSINFO("emac-phy-off-suspend = %d\n",
		   ethqos->current_phy_mode);

	ethqos->ioaddr = (&stmmac_res)->addr;

	if (of_property_read_bool(pdev->dev.of_node, "emac-core-version")) {
		/* Read emac core version value from dtsi */
		ret = of_property_read_u32(pdev->dev.of_node,
					   "emac-core-version",
					   &ethqos->emac_ver);
		if (ret) {
			ETHQOSDBG(":resource emac-hw-ver! not in dtsi\n");
			ethqos->emac_ver = EMAC_HW_NONE;
			WARN_ON(1);
		}
	} else {
	/* emac ver cannot be read in case of following XGMAC device id */
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
		if (plat_dat->has_xgmac &&
		    (0x31 == (readl(stmmac_res.addr + GMAC4_VERSION) & GENMASK(7, 0)))) {
			ETHQOSINFO("has_xgmac = %d GMAC4_version_id = 0x%x\n",
				   plat_dat->has_xgmac,
				   (readl(stmmac_res.addr + GMAC4_VERSION) & GENMASK(7, 0)));
			ethqos->emac_ver = EMAC_HW_v4_0_0;
		} else {
#endif
			ethqos->emac_ver =
			rgmii_readl(ethqos, EMAC_I0_EMAC_CORE_HW_VERSION_RGOFFADDR);
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
		}
#endif
	}
	ETHQOSINFO("emac_core_version = %d\n", ethqos->emac_ver);

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
		/* For xgmac specific version get maxspeed from secure environment */
		qcom_scm_call_get_emac_maxspeed(ethqos->rgmii_phy_base, &plat_dat->max_speed);
		ETHQOSERR("%d %s max speed = %d", __LINE__, __func__, plat_dat->max_speed);

		/* Allocate mem for hsr sequence programming */
		err = qtee_shmbridge_allocate_shm(RGMII_BLOCK_SIZE, &ethqos->shm_rgmii_hsr);
		if (err) {
			ETHQOSERR("Rgmii register hsr mem alloc failure\n");
			ret = -ENOMEM;
			goto err_mem;
		}

		/* Allocate mem for dumping rgmii regs */
		err = qtee_shmbridge_allocate_shm(RGMII_BLOCK_SIZE, &ethqos->shm_rgmii_local);
		if (err) {
			dev_err(&ethqos->pdev->dev, "Rgmii register dump - mem alloc failure\n");
			ret = -ENOMEM;
			goto err_mem;
		}
	}
#endif

	if (of_property_read_bool(pdev->dev.of_node, "gdsc-off-on-suspend"))
		ethqos->gdsc_off_on_suspend = true;
	ETHQOSDBG("gdsc-off-on-suspend = %d\n", ethqos->gdsc_off_on_suspend);
	if (of_device_is_compatible(np, "qcom,qcs404-ethqos"))
		plat_dat->rx_clk_runs_in_lpi = 1;

	if (!!of_find_property(np, "qcom,ioss", NULL)) {
		ETHQOSDBG("%s: IPA ENABLED", __func__);
		ethqos->ipa_enabled = true;
	}
	if (plat_dat->mac_err_rec)
		ethqos_mac_rec_init(ethqos);
	if (of_property_read_bool(np, "mdio-drv-str") || of_property_read_bool(np, "mdc-drv-str"))
		ethqos_update_mdio_drv_strength(ethqos, np);
	ret = stmmac_dvr_probe(&pdev->dev, plat_dat, &stmmac_res);
	if (ret)
		goto err_clk;

	for (i = 0; i < ETH_MAX_NICS; i++) {
		if (!pethqos[i]) {
			pethqos[i] = ethqos;
			break;
		}
	}

	ndev = dev_get_drvdata(&ethqos->pdev->dev);
	priv = netdev_priv(ndev);
	ethqos->priv = priv;
	ethqos->power_state = true;

	qcom_ethqos_init_aux_ts(ethqos, plat_dat, priv);

	/*Configure EMAC for 10 Mbps mode*/
	ethqos->probed = true;
	if (plat_dat->fixed_phy_mode)
		plat_dat->serdes_update_speed(plat_dat->bsp_priv, 10);
	plat_dat->fix_mac_speed(plat_dat->bsp_priv, 10);

	if (of_property_read_bool(np, "pcs-v3")) {
		plat_dat->pcs_v3 = true;
	} else {
		plat_dat->pcs_v3 = false;
		ETHQOSDBG(":pcs-v3 not in dtsi\n");
	}

	if (of_property_read_bool(np, "separate_wol_pin")) {
		priv->plat->separate_wol_pin = true;
	} else {
		priv->plat->separate_wol_pin = false;
		ETHQOSDBG("separate_wol_pin not in dtsi\n");
	}

	if (ethqos->emac_ver != EMAC_HW_v3_1_0 && plat_dat->mdio_bus_data &&
	    (plat_dat->phy_interface == PHY_INTERFACE_MODE_SGMII ||
	     plat_dat->phy_interface == PHY_INTERFACE_MODE_USXGMII ||
	     plat_dat->phy_interface == PHY_INTERFACE_MODE_2500BASEX ||
	     plat_dat->phy_interface == PHY_INTERFACE_MODE_5GBASER))
		plat_dat->mdio_bus_data->has_xpcs = true;

	if (plat_dat->mdio_bus_data && plat_dat->mdio_bus_data->has_xpcs) {
		ret = ethqos_xpcs_init(ndev);
		if (ret < 0)
			goto err_clk;
	}

	/* Get sw path tx desc count from device tree */
	if (of_property_read_u32(np, "sw-dma-tx-desc-cnt",
				 &priv->dma_tx_size))
		ETHQOSDBG("sw_dma_tx_desc_cnt not found in dtsi\n");

	/* Get sw path rx desc count from device tree */
	if (of_property_read_u32(np, "sw-dma-rx-desc-cnt",
				 &priv->dma_rx_size))
		ETHQOSDBG("sw_dma_rx_desc_cnt not found in dtsi\n");

	if (!priv->plat->mac2mac_en && !priv->plat->fixed_phy_mode) {
		if (!ethqos_phy_intr_config(ethqos))
			ethqos_phy_intr_enable(ethqos);
		else
			ETHQOSERR("Phy interrupt configuration failed");

		if (priv->plat->separate_wol_pin) {
			if (!ethqos_wol_intr_config(ethqos)) {
				ETHQOSDBG("WOL found in dtsi\n");
				ethqos_wol_intr_enable(ethqos);
			} else {
				ETHQOSERR("WOL interrupt configuration failed");
			}
		}
	}
	if (ethqos->emac_ver == EMAC_HW_v2_3_2_RG) {
		ethqos_pps_irq_config(ethqos);
		create_pps_interrupt_device_node(&ethqos->avb_class_a_dev_t,
						 &ethqos->avb_class_a_cdev,
						 &ethqos->avb_class_a_class,
						 AVB_CLASS_A_POLL_DEV_NODE);

		create_pps_interrupt_device_node(&ethqos->avb_class_b_dev_t,
						 &ethqos->avb_class_b_cdev,
						 &ethqos->avb_class_b_class,
						 AVB_CLASS_B_POLL_DEV_NODE);
	}

	/* Read en_wol from device tree */
	priv->en_wol = of_property_read_bool(np, "enable-wol");
	if (of_property_read_bool(np, "disable-frame-preem"))
		priv->dma_cap.fpesel = 0;

	/* If valid mac address is present from emac partition
	 * Enable the mac address in the device.
	 */
	if (pparams.is_valid_mac_addr) {
		ether_addr_copy(dev_addr, pparams.mac_addr);
		memcpy(priv->dev->dev_addr, dev_addr, ETH_ALEN);
	}

	if (of_property_read_bool(np, "avb-vlan-id"))
		of_property_read_u32(np, "avb-vlan-id",
				     &priv->avb_vlan_id);
	else
		priv->avb_vlan_id = 0;

	/* enable safety feature from device tree */
	if (of_property_read_bool(np, "safety-feat") && priv->dma_cap.asp)
		priv->dma_cap.asp = 1;
	else
		priv->dma_cap.asp = 0;

	ret = qcom_ethmsgq_init(&pdev->dev);
	if (ret < 0)
		goto err_clk;

	if (qcom_ethqos_register_panic_notifier(ethqos))
		ETHQOSERR("Failed to register panic notifier");

	if (ethqos->qoe_mode) {
		ethqos_create_emac_device_node(&ethqos->emac_dev_t,
					       &ethqos->emac_cdev,
					       &ethqos->emac_class,
					       "emac");
	}

	if (priv->plat->mac2mac_en || priv->plat->fixed_phy_mode)
		priv->plat->mac2mac_link = 0;
	if (plat_dat->mac_err_rec)
		ethqos_create_emac_rec_device_node(&ethqos->emac_rec_dev_t,
						   &ethqos->emac_rec_cdev,
						   &ethqos->emac_rec_class,
						   "emac_rec");

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	priv->plat->pm_lite = true;
	plat_dat->enable_wol = ethqos_enable_wol;
#endif

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_HOSTVM)
	if (ethqos->early_eth_enabled)
		ethqos->passthrough_en = 1;
	else
		ethqos->passthrough_en = 0;
#else
	ethqos->cv2x_priority = 0;
	if (of_property_read_bool(np, "cv2x-queue"))
		of_property_read_u32(np, "cv2x-queue",
				     &ethqos->cv2x_queue);
	else
		ethqos->cv2x_queue = CV2X_TAG_TX_CHANNEL;
#endif

	if (priv->plat->separate_wol_pin) {
		plat_dat->wol_irq_enable = ethqos_wol_irq_enable;
		plat_dat->wol_irq_disable = ethqos_wol_irq_disable;
	}

	qcom_ethqos_register_vm_notifier(ethqos);

	qcom_ethmsgq_register_notify(qcom_ethsvm_command_req, priv);
	atomic_set(&priv->plat->phy_clks_suspended, 0);
	ethqos_create_sysfs(ethqos);

	if (plat_dat->phy_interface == PHY_INTERFACE_MODE_USXGMII) {
		ethqos_change_if_create_sysfs(ethqos);
		ethqos_thermal_netlink_create_sysfs(ethqos);
	}

	INIT_WORK(&ethqos->emac_phy_state_work, ethqos_phy_interface_reset);
	plat_dat->tm_u5g_en = of_property_read_bool(pdev->dev.of_node, "tm-u5g-en");

#if IS_ENABLED(CONFIG_DEBUG_FS)
	ethqos_create_debugfs(ethqos);
#endif
	ethqos_create_sysfs_nodes(ethqos);

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	if (priv->plat->enable_power_saving)
		priv->plat->enable_power_saving(ndev, true);
#endif

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	/* Send the probe completion event early so that early eth is not impacted. */
	complete_all(&priv->probe_done);
#endif

	if (ethqos->early_eth_enabled) {
		if (plat_dat->probe_invoke_if_up || plat_dat->fixed_phy_mode ||
		    plat_dat->interface == PHY_INTERFACE_MODE_RGMII ||
		    plat_dat->interface == PHY_INTERFACE_MODE_RGMII_ID ||
		    plat_dat->interface == PHY_INTERFACE_MODE_RGMII_RXID ||
		    plat_dat->interface == PHY_INTERFACE_MODE_RGMII_TXID) {
			/* Initialize work*/
			INIT_WORK(&ethqos->early_eth,
				  qcom_ethqos_bringup_iface);
			/* Queue the work*/
			queue_work(system_wq, &ethqos->early_eth);

			/*Set early eth parameters*/
			ethqos_set_early_eth_param(priv, ethqos);
		}

		if (priv->plat && priv->plat->mdio_bus_data)
			priv->plat->mdio_bus_data->phy_mask =
			priv->plat->mdio_bus_data->phy_mask | DUPLEX_FULL | SPEED_100;
	}
#ifdef CONFIG_MSM_BOOT_TIME_MARKER
		update_marker("M - Ethernet probe end");
#endif

	return ret;

err_clk:
	if (plat_dat->interface == PHY_INTERFACE_MODE_SGMII ||
	    plat_dat->interface ==  PHY_INTERFACE_MODE_USXGMII ||
	    plat_dat->interface == PHY_INTERFACE_MODE_2500BASEX ||
	    plat_dat->interface == PHY_INTERFACE_MODE_5GBASER) {
		ethqos_disable_sgmii_usxgmii_clks(ethqos);
		qcom_ethqos_disable_serdes_clocks(ethqos);
	}

	if (ethqos->rgmii_clk)
		clk_disable_unprepare(ethqos->rgmii_clk);

err_mem:
	stmmac_remove_config_dt(pdev, plat_dat);
	ethqos_disable_regulators(ethqos);
	return ret;
}

static int qcom_ethqos_remove(struct platform_device *pdev)
{
	struct qcom_ethqos *ethqos;
	int i, ret;
	struct stmmac_priv *priv;
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	struct net_device *ndev = platform_get_drvdata(pdev);
#endif

	if (of_device_is_compatible(pdev->dev.of_node, "qcom,emac-smmu-embedded")) {
		of_platform_depopulate(&pdev->dev);
		return 0;
	}

	ethqos = get_stmmac_bsp_priv(&pdev->dev);
	if (!ethqos)
		return -ENODEV;

	priv = qcom_ethqos_get_priv(ethqos);

	if (priv->hw->qxpcs) {
		if (priv->hw->qxpcs->intr_en) {
			free_irq(priv->hw->qxpcs->pcs_intr, priv);
			priv->pcs_irq_enabled = false;
		}
		qcom_xpcs_destroy(priv->hw->qxpcs);
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
		priv->hw->qxpcs = NULL;
		priv->plat->xpcs_powersaving = NULL;
#endif
	}

	ethqos_remove_sysfs(ethqos);
	qcom_ethqos_unregister_vm_notifier(ethqos);
	ethqos_change_if_cleanup_sysfs(ethqos);
	ethqos_thermal_netlink_cleanup_sysfs(ethqos);
	ret = stmmac_pltfr_remove(pdev);
	netlink_kernel_release(ethqos->socket_nl);
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	ethqos_enable_clock_gating(ndev);
#endif

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
	if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
		/* Free SHM memory */
		qtee_shmbridge_free_shm(&ethqos->shm_rgmii_hsr);
		ethqos->shm_rgmii_hsr.paddr = 0;
		ethqos->shm_rgmii_hsr.vaddr = NULL;

		qtee_shmbridge_free_shm(&ethqos->shm_rgmii_local);
		ethqos->shm_rgmii_local.paddr = 0;
		ethqos->shm_rgmii_local.vaddr = NULL;
	}
#endif

	qcom_ethmsgq_deinit(priv->device);

	if (ethqos->rgmii_clk)
		clk_disable_unprepare(ethqos->rgmii_clk);
#if IS_ENABLED(CONFIG_DWMAC_QCOM_VER3)
	if (priv->plat->phy_interface == PHY_INTERFACE_MODE_SGMII ||
	    priv->plat->phy_interface ==  PHY_INTERFACE_MODE_USXGMII ||
	    priv->plat->phy_interface ==  PHY_INTERFACE_MODE_2500BASEX ||
	    priv->plat->phy_interface == PHY_INTERFACE_MODE_5GBASER) {
		ethqos_disable_sgmii_usxgmii_clks(ethqos);
		qcom_ethqos_disable_serdes_clocks(ethqos);
	}
#endif
	icc_put(ethqos->axi_icc_path);

	icc_put(ethqos->apb_icc_path);

	atomic_set(&priv->plat->phy_clks_suspended, 1);

	if (plat_dat->mac_err_rec) {
		ethqos_delete_emac_rec_device_node(&ethqos->emac_rec_dev_t,
						   &ethqos->emac_rec_cdev,
						   &ethqos->emac_rec_class);
	}

#if IS_ENABLED(CONFIG_DEBUG_FS)
	debugfs_remove_recursive(ethqos->debugfs_dir);
#endif
	// Remove sysfs nodes
	if (ethqos->sysfs_kobj)
		kobject_put(ethqos->sysfs_kobj);

	if (priv->plat->phy_intr_en_extn_stm)
		free_irq(ethqos->phy_intr, ethqos);

	priv->phy_irq_enabled = false;
	if (priv->plat->separate_wol_pin) {
		if (priv->wol_irq_enabled)
			free_irq(ethqos->wol_intr, ethqos);
		priv->wol_irq_enabled = false;
	}

	if (priv->plat->phy_intr_en_extn_stm)
		cancel_work_sync(&ethqos->emac_phy_work);

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	mutex_destroy(&ethqos->ps_lock);
#endif
	cancel_work_sync(&ethqos->emac_phy_state_work);
	emac_emb_smmu_exit();
	ethqos_disable_regulators(ethqos);
	qcom_ethqos_unregister_panic_notifier(ethqos);

	for (i = 0; i < ETH_MAX_NICS; i++) {
		if (pethqos[i] == ethqos) {
			pethqos[i] = NULL;
			break;
		}
	}

	platform_set_drvdata(pdev, NULL);
	of_platform_depopulate(&pdev->dev);

	ret = pinctrl_select_state(pdev->dev.pins->p, pdev->dev.pins->init_state);
	if (ret)
		ETHQOSERR("pinctrl inactive select state failed %d\n", ret);

	return ret;
}

static int qcom_ethqos_suspend(struct device *dev)
{
	struct qcom_ethqos *ethqos;
	struct net_device *ndev = NULL;
	int ret;
	struct stmmac_priv *priv;
	struct plat_stmmacenet_data *plat;

	if (of_device_is_compatible(dev->of_node, "qcom,emac-smmu-embedded")) {
		ETHQOSDBG("smmu return\n");
		return 0;
	}
#ifdef CONFIG_MSM_BOOT_TIME_MARKER
	update_marker("M - Ethernet Suspend start");
#endif

	ethqos = get_stmmac_bsp_priv(dev);
	if (!ethqos)
		return -ENODEV;

	ndev = dev_get_drvdata(dev);
	priv = netdev_priv(ndev);
	plat = priv->plat;

	if (!ndev)
		return -EINVAL;

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	if (priv->plat->enable_power_saving) {
		if (priv->plat->enable_power_saving(ndev, false) < 0)
			return -EINVAL;
	}
#endif

	if (priv->plat->interface == PHY_INTERFACE_MODE_USXGMII ||
	    priv->plat->interface == PHY_INTERFACE_MODE_SGMII ||
	    priv->plat->interface == PHY_INTERFACE_MODE_2500BASEX ||
	    priv->plat->interface == PHY_INTERFACE_MODE_5GBASER)
		ethqos_xpcs_irq_disable(ndev);

	if (ethqos->current_phy_mode == DISABLE_PHY_AT_SUSPEND_ONLY ||
	    ethqos->current_phy_mode == DISABLE_PHY_SUSPEND_ENABLE_RESUME) {
		ethqos_trigger_phylink(ethqos, false);
	}

	ret = stmmac_suspend(dev);

	disable_irq(priv->dev->irq);

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	if (priv->plat->enable_power_saving) {
		if (priv->plat->enable_power_saving(ndev, true) < 0)
			return -EINVAL;
	}
#else
	if (ethqos->vreg_a_sgmii_1p2 && ethqos->vreg_a_sgmii_0p9) {
		ethqos_disable_sgmii_usxgmii_clks(ethqos);
		qcom_ethqos_disable_serdes_clocks(ethqos);

		if (priv->plat->serdes_powersaving)
			priv->plat->serdes_powersaving(ndev, priv->plat->bsp_priv, false, false);
	}
#endif

	qcom_ethqos_phy_suspend_clks(ethqos);

	if (ethqos->current_phy_mode == DISABLE_PHY_AT_SUSPEND_ONLY ||
	    ethqos->current_phy_mode == DISABLE_PHY_SUSPEND_ENABLE_RESUME) {
		ETHQOSINFO("disable phy at suspend\n");
		ethqos_phy_power_off(ethqos);
	}

	if (ethqos->gdsc_off_on_suspend) {
		if (ethqos->gdsc_emac) {
			regulator_disable(ethqos->gdsc_emac);
			ETHQOSDBG("Disabled <%s>\n", EMAC_GDSC_EMAC_NAME);
		}
	}

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	ethqos_enable_clock_gating(ndev);
#endif
#ifdef CONFIG_MSM_BOOT_TIME_MARKER
	place_marker("M - Ethernet Suspend End");
#endif
	priv->boot_kpi = false;

	ETHQOSDBG(" ret = %d\n", ret);
	return ret;
}

static int qcom_ethqos_resume(struct device *dev)
{
	struct net_device *ndev = NULL;
	struct qcom_ethqos *ethqos;
	int ret;
	struct stmmac_priv *priv;

	ETHQOSDBG("Resume Enter\n");
	if (of_device_is_compatible(dev->of_node, "qcom,emac-smmu-embedded"))
		return 0;

#ifdef CONFIG_MSM_BOOT_TIME_MARKER
	update_marker("M - Ethernet Resume start");
#endif

	ethqos = get_stmmac_bsp_priv(dev);

	if (!ethqos)
		return -ENODEV;

	if (ethqos->gdsc_off_on_suspend) {
		if (ethqos->gdsc_emac) {
			ret = regulator_enable(ethqos->gdsc_emac);
			if (ret) {
				ETHQOSERR("Can not enable <%s>\n", EMAC_GDSC_EMAC_NAME);
				return ret;
			}
		}
		ETHQOSDBG("Enabled <%s>\n", EMAC_GDSC_EMAC_NAME);
	}

	ndev = dev_get_drvdata(dev);
	priv = netdev_priv(ndev);

	if (!ndev) {
		ETHQOSERR(" Resume not possible\n");
		return -EINVAL;
	}

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	ethqos_disable_clock_gating(ndev);
#endif

	if (ethqos->current_phy_mode == DISABLE_PHY_SUSPEND_ENABLE_RESUME) {
		ETHQOSINFO("enable phy at resume\n");
		ethqos_phy_power_on(ethqos);
	}

	qcom_ethqos_phy_resume_clks(ethqos);

	if (ethqos->gdsc_off_on_suspend)
		ethqos_set_func_clk_en(ethqos);

	enable_irq(priv->dev->irq);

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	if (priv->plat->enable_power_saving) {
		if (priv->plat->enable_power_saving(ndev, false) < 0)
			return -EINVAL;
	}
#else
	if (ethqos->vreg_a_sgmii_1p2 && ethqos->vreg_a_sgmii_0p9) {

		ret = qcom_ethqos_enable_serdes_clocks(ethqos);
		if (ret)
			return -EINVAL;

		ret = ethqos_resume_sgmii_usxgmii_clks(ethqos);
		if (ret)
			return -EINVAL;

		if (priv->plat->serdes_powersaving && priv->speed != SPEED_UNKNOWN)
			priv->plat->serdes_powersaving(ndev, priv->plat->bsp_priv, true, true);
	}
#endif

	if (ethqos->current_phy_mode == DISABLE_PHY_AT_SUSPEND_ONLY) {
		/* Temp Enable LOOPBACK_EN.
		 * TX clock needed for reset As Phy is off
		 */

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
		/*Invoke SCM call */
		if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
			qcom_scm_call_loopback_configure(ethqos->rgmii_phy_base,
							 ENABLE_IO_MACRO_LOOPBACK, 0);
		}
#else
		rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
			      RGMII_CONFIG_LOOPBACK_EN,
			      RGMII_IO_MACRO_CONFIG);
#endif
		ETHQOSINFO("Loopback EN Enabled\n");
	}

	ret = stmmac_resume(dev);

	if (priv->plat->interface == PHY_INTERFACE_MODE_USXGMII ||
	    priv->plat->interface == PHY_INTERFACE_MODE_SGMII ||
	    priv->plat->interface == PHY_INTERFACE_MODE_2500BASEX ||
	    priv->plat->interface == PHY_INTERFACE_MODE_5GBASER)
		ethqos_xpcs_irq_enable(ndev);

	if (ethqos->current_phy_mode == DISABLE_PHY_SUSPEND_ENABLE_RESUME) {
		ETHQOSINFO("reset phy after clock\n");
		ethqos_trigger_phylink(ethqos, true);
	}

	if (ethqos->current_phy_mode == DISABLE_PHY_AT_SUSPEND_ONLY) {
		//Disable  LOOPBACK_EN

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_SCM)
		/*Invoke SCM call */
		if (ethqos->emac_ver == EMAC_HW_v4_0_0) {
			qcom_scm_call_loopback_configure(ethqos->rgmii_phy_base,
							 ENABLE_IO_MACRO_LOOPBACK, 0);
		}
#else

		rgmii_updatel(ethqos, RGMII_CONFIG_LOOPBACK_EN,
			      0, RGMII_IO_MACRO_CONFIG);
#endif
		ETHQOSINFO("Loopback EN Disabled\n");
	}

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	if (priv->plat->enable_power_saving)
		ret = priv->plat->enable_power_saving(ndev, true);
#endif
#ifdef CONFIG_MSM_BOOT_TIME_MARKER
	update_marker("M - Ethernet Resume End");
#endif
	ETHQOSDBG("<--Resume Exit\n");
	return ret;
}

static int qcom_ethqos_enable_clks(struct qcom_ethqos *ethqos, struct device *dev)
{
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);
	int ret = 0;

	/* clock setup */
	priv->plat->stmmac_clk = devm_clk_get(dev,
					      STMMAC_RESOURCE_NAME);
	if (IS_ERR(priv->plat->stmmac_clk)) {
		dev_warn(dev, "stmmac_clk clock failed\n");
		ret = PTR_ERR(priv->plat->stmmac_clk);
		priv->plat->stmmac_clk = NULL;
	} else {
		ret = clk_prepare_enable(priv->plat->stmmac_clk);
		if (ret)
			ETHQOSINFO("stmmac_clk clk failed\n");
	}

	priv->plat->pclk = devm_clk_get(dev, "pclk");
	if (IS_ERR(priv->plat->pclk)) {
		dev_warn(dev, "pclk clock failed\n");
		ret = PTR_ERR(priv->plat->pclk);
		priv->plat->pclk = NULL;
		goto error_pclk_get;
	} else {
		ret = clk_prepare_enable(priv->plat->pclk);
		if (ret) {
			ETHQOSINFO("pclk clk failed\n");
			goto error_pclk_get;
		}
	}

	ethqos->rgmii_clk = devm_clk_get(dev, "rgmii");
	if (IS_ERR(ethqos->rgmii_clk)) {
		dev_warn(dev, "rgmii clock failed\n");
		ret = PTR_ERR(ethqos->rgmii_clk);
		goto error_rgmii_get;
	} else {
		ret = clk_prepare_enable(ethqos->rgmii_clk);
		if (ret) {
			ETHQOSINFO("rgmmi clk failed\n");
			goto error_rgmii_get;
		}
	}
	return 0;

error_rgmii_get:
	clk_disable_unprepare(priv->plat->pclk);
error_pclk_get:
	clk_disable_unprepare(priv->plat->stmmac_clk);
	return ret;
}

static void qcom_ethqos_disable_clks(struct qcom_ethqos *ethqos, struct device *dev)
{
	struct stmmac_priv *priv = qcom_ethqos_get_priv(ethqos);

	ETHQOSINFO("Enter\n");

	if (priv->plat->stmmac_clk)
		clk_disable_unprepare(priv->plat->stmmac_clk);

	if (priv->plat->pclk)
		clk_disable_unprepare(priv->plat->pclk);

	if (ethqos->rgmii_clk)
		clk_disable_unprepare(ethqos->rgmii_clk);

	ETHQOSINFO("Exit\n");
}

static int qcom_ethqos_hib_restore(struct device *dev)
{
	struct qcom_ethqos *ethqos;
	struct stmmac_priv *priv;
	struct net_device *ndev = NULL;
	int ret = 0;

	if (of_device_is_compatible(dev->of_node, "qcom,emac-smmu-embedded"))
		return 0;

	ethqos = get_stmmac_bsp_priv(dev);
	if (!ethqos)
		return -ENODEV;

	ndev = dev_get_drvdata(dev);

	if (!ndev)
		return -EINVAL;

	priv = netdev_priv(ndev);

	ret = ethqos_init_regulators(ethqos);
	if (ret)
		return ret;

	ret = ethqos_init_sgmii_regulators(ethqos);
	if (ret)
		return ret;

	ret = ethqos_init_gpio(ethqos);
	if (ret)
		return ret;

	ret = qcom_ethqos_enable_clks(ethqos, dev);
	if (ret)
		return ret;

	ethqos_update_clk_and_bus_cfg(ethqos, ethqos->speed, priv->plat->interface);

	ethqos_set_func_clk_en(ethqos);

#ifdef DWC_ETH_QOS_CONFIG_PTP
	if (priv->plat->clk_ptp_ref) {
		ret = clk_prepare_enable(priv->plat->clk_ptp_ref);
		if (ret < 0) {
			netdev_warn(priv->dev, "failed to enable PTP reference clock: %d\n", ret);
		} else {
			ret = stmmac_init_ptp(priv);
			if (ret == -EOPNOTSUPP) {
				netdev_warn(priv->dev, "PTP not supported by HW\n");
			} else if (ret) {
				netdev_warn(priv->dev, "PTP init failed\n");
			} else {
				clk_set_rate(priv->plat->clk_ptp_ref,
					     priv->plat->clk_ptp_rate);
			}

			ret = priv->plat->init_pps(priv);
		}
	}

#endif /* end of DWC_ETH_QOS_CONFIG_PTP */

	/* issue software reset to device */
	ret = stmmac_reset(priv, priv->ioaddr);
	if (ret) {
		dev_err(priv->device, "Failed to reset\n");
		return ret;
	}

	if (!netif_running(ndev)) {
		rtnl_lock();
		dev_open(ndev, NULL);
		rtnl_unlock();
		ETHQOSINFO("calling open\n");
	}

	ETHQOSINFO("end\n");

	return ret;
}

static int qcom_ethqos_hib_freeze(struct device *dev)
{
	struct qcom_ethqos *ethqos;
	struct stmmac_priv *priv;
	int ret = 0;
	struct net_device *ndev = NULL;

	if (of_device_is_compatible(dev->of_node, "qcom,emac-smmu-embedded"))
		return 0;

	ethqos = get_stmmac_bsp_priv(dev);
	if (!ethqos)
		return -ENODEV;

	ndev = dev_get_drvdata(dev);

	if (!ndev)
		return -EINVAL;

	priv = netdev_priv(ndev);

	ETHQOSINFO("start\n");

	if (netif_running(ndev)) {
		rtnl_lock();
		dev_close(ndev);
		rtnl_unlock();
		ETHQOSINFO("calling netdev off\n");
	}

#ifdef DWC_ETH_QOS_CONFIG_PTP
	stmmac_release_ptp(priv);
#endif /* end of DWC_ETH_QOS_CONFIG_PTP */

	qcom_ethqos_disable_clks(ethqos, dev);

	ethqos_disable_regulators(ethqos);

	ethqos_free_gpios(ethqos);

	ETHQOSINFO("end\n");

	return ret;
}

MODULE_DEVICE_TABLE(of, qcom_ethqos_match);

static const struct dev_pm_ops qcom_ethqos_pm_ops = {
	.freeze = qcom_ethqos_hib_freeze,
	.restore = qcom_ethqos_hib_restore,
	.thaw = qcom_ethqos_hib_restore,
	.suspend = qcom_ethqos_suspend,
	.resume = qcom_ethqos_resume,
};

static struct platform_driver qcom_ethqos_driver = {
	.probe  = qcom_ethqos_probe,
	.remove = qcom_ethqos_remove,
	.driver = {
		.name           = DRV_NAME,
		.pm		= &qcom_ethqos_pm_ops,
		.of_match_table = of_match_ptr(qcom_ethqos_match),
	},
};

static int __init qcom_ethqos_init_module(void)
{
	int ret = 0;

	ETHQOSDBG("\n");

	ret = platform_driver_register(&qcom_ethqos_driver);
	if (ret < 0) {
		ETHQOSINFO("qcom-ethqos: Driver registration failed");
		return ret;
	}

	ETHQOSDBG("\n");

	return ret;
}

static void __exit qcom_ethqos_exit_module(void)
{
	ETHQOSDBG("\n");

	platform_driver_unregister(&qcom_ethqos_driver);

	if (!ipc_stmmac_log_ctxt)
		ipc_log_context_destroy(ipc_stmmac_log_ctxt);

	if (!ipc_stmmac_log_ctxt_low)
		ipc_log_context_destroy(ipc_stmmac_log_ctxt_low);

	ipc_stmmac_log_ctxt = NULL;
	ipc_stmmac_log_ctxt_low = NULL;
	ETHQOSDBG("\n");
}

/*!
 * \brief Macro to register the driver registration function.
 *
 * \details A module always begin with either the init_module or the function
 * you specify with module_init call. This is the entry function for modules;
 * it tells the kernel what functionality the module provides and sets up the
 * kernel to run the module's functions when they're needed. Once it does this,
 * entry function returns and the module does nothing until the kernel wants
 * to do something with the code that the module provides.
 */

device_initcall(qcom_ethqos_init_module)

/*!
 * \brief Macro to register the driver un-registration function.
 *
 * \details All modules end by calling either cleanup_module or the function
 * you specify with the module_exit call. This is the exit function for modules;
 * it undoes whatever entry function did. It unregisters the functionality
 * that the entry function registered.
 */

module_exit(qcom_ethqos_exit_module)

MODULE_DESCRIPTION("Qualcomm ETHQOS driver");
MODULE_LICENSE("GPL v2");
