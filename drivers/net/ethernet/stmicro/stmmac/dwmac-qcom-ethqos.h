/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (c) 2021, The Linux Foundation. All rights reserved.
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 */
#ifndef	_DWMAC_QCOM_ETHQOS_H
#define	_DWMAC_QCOM_ETHQOS_H

//#include <linux/msm-bus.h>

#include <linux/inetdevice.h>
#include <linux/inet.h>
#include <linux/refcount.h>

#include <net/addrconf.h>
#include <net/ipv6.h>
#include <net/inet_common.h>

#include <linux/uaccess.h>
#include <linux/ipc_logging.h>
#include <linux/interconnect.h>
#include <linux/qtee_shmbridge.h>
#include "dwmac-qcom-msgq-pvm.h"

#define QCOM_ETH_QOS_MAC_ADDR_LEN 6
#define QCOM_ETH_QOS_MAC_ADDR_STR_LEN 18

extern void *ipc_stmmac_log_ctxt;
extern void *ipc_stmmac_log_ctxt_low;

#define IPCLOG_STATE_PAGES 50
#define IPC_RATELIMIT_BURST 1
#define __FILENAME__ (strrchr(__FILE__, '/') ? \
		strrchr(__FILE__, '/') + 1 : __FILE__)

#define DRV_NAME "qcom-ethqos"
#define ETHQOSDBG(fmt, args...) \
do {\
	pr_debug(DRV_NAME " %s:%d " fmt, __func__, __LINE__, ## args);\
	if (ipc_stmmac_log_ctxt) { \
		ipc_log_string(ipc_stmmac_log_ctxt, \
		"%s: %s[%u]:[stmmac] DEBUG:" fmt, __FILENAME__,\
		__func__, __LINE__, ## args); \
	} \
} while (0)
#define ETHQOSERR(fmt, args...) \
do {\
	pr_err(DRV_NAME " %s:%d " fmt, __func__, __LINE__, ## args);\
	if (ipc_stmmac_log_ctxt) { \
		ipc_log_string(ipc_stmmac_log_ctxt, \
		"%s: %s[%u]:[stmmac] ERROR:" fmt, __FILENAME__,\
		__func__, __LINE__, ## args); \
	} \
} while (0)
#define ETHQOSINFO(fmt, args...) \
do {\
	pr_info(DRV_NAME " %s:%d " fmt, __func__, __LINE__, ## args);\
	if (ipc_stmmac_log_ctxt) { \
		ipc_log_string(ipc_stmmac_log_ctxt, \
		"%s: %s[%u]:[stmmac] INFO:" fmt, __FILENAME__,\
		__func__, __LINE__, ## args); \
	} \
} while (0)

#define IPC_LOW(fmt, args...) \
do {\
	if (ipc_stmmac_log_ctxt_low) { \
		ipc_log_string(ipc_stmmac_log_ctxt_low, \
		"%s: %s[%u]:[stmmac] DEBUG:" fmt, __FILENAME__, \
		__func__, __LINE__, ## args); \
	} \
} while (0)

/* Printing one error message in 5 seconds if multiple error messages
 * are coming back to back.
 */
#define pr_err_ratelimited_ipc(fmt, ...) \
	printk_ratelimited_ipc(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)
#define printk_ratelimited_ipc(fmt, ...) \
({ \
	static DEFINE_RATELIMIT_STATE(_rs, DEFAULT_RATELIMIT_INTERVAL, \
				       IPC_RATELIMIT_BURST); \
	if (__ratelimit(&_rs)) \
		pr_err(fmt, ##__VA_ARGS__); \
})
#define IPCERR_RL(fmt, args...) \
	pr_err_ratelimited_ipc(DRV_NAME " %s:%d " fmt, __func__,\
	__LINE__, ## args)

#define RGMII_IO_MACRO_CONFIG		0x0
#define SDCC_HC_REG_DLL_CONFIG		0x4
#define SDCC_TEST_CTL			0x8
#define SDCC_HC_REG_DDR_CONFIG		0xC
#define SDCC_HC_REG_DLL_CONFIG2		0x10
#define SDC4_STATUS			0x14
#define SDCC_USR_CTL			0x18
#define RGMII_IO_MACRO_CONFIG2		0x1C
#define EMAC_WRAPPER_USXGMII_DIV_CONFIG 0x1CC
#ifdef CONFIG_DWMAC_QCOM_VER3
#define EMAC_WRAPPER_SGMII_PHY_CNTRL0		0xF0
#define EMAC_WRAPPER_SGMII_PHY_CNTRL1		0xF4
#else
#define EMAC_WRAPPER_SGMII_PHY_CNTRL0		0x170
#define EMAC_WRAPPER_SGMII_PHY_CNTRL1		0x174
#endif
#define EMAC_WRAPPER_USXGMII_MUX_SEL		0x1D0
#define RGMII_IO_MACRO_SCRATCH_2		0x44
#define RGMII_IO_MACRO_BYPASS		0x16C

#define EMAC_HW_NONE 0
#define EMAC_HW_v2_1_1 0x20010001
#define EMAC_HW_v2_1_2 0x20010002
#define EMAC_HW_v2_3_0 0x20030000
#define EMAC_HW_v2_3_1 0x20030001
#define EMAC_HW_v3_0_0_RG 0x30000000
#define EMAC_HW_v3_1_0 0x30010000
#define EMAC_HW_v4_0_0 0x40000000
#define EMAC_HW_vMAX 9

#define ETHQOS_CONFIG_PPSOUT_CMD 44
#define ETHQOS_AVB_ALGORITHM 27
#define ETHQOS_THERM_GET_REM_RESP	(ETHQOS_AVB_ALGORITHM + 1)
#define ETHQOS_THERM_SET_LOCAL_ETH_CAP	(ETHQOS_THERM_GET_REM_RESP + 1)
#define ETHQOS_THERM_GET_REM_CAP	(ETHQOS_THERM_SET_LOCAL_ETH_CAP + 1)
#define ETHQOS_THERM_SET_LOCAL_ETH_MODE	(ETHQOS_THERM_GET_REM_CAP + 1)

#define PPS_MAXIDX(x)			((((x) + 1) * 8) - 1)
#define PPS_MINIDX(x)			((x) * 8)
#define MCGRENX(x)			BIT(PPS_MAXIDX(x))
#define PPSEN0				BIT(4)

#if IS_ENABLED(CONFIG_DWXGMAC_QCOM_4K)
	#define MAC_PPS_CONTROL		0x000007070
	#define MAC_PPSX_TARGET_TIME_SEC(x)	(0x00007080 + ((x) * 0x10))
	#define MAC_PPSX_TARGET_TIME_NSEC(x)	(0x00007084 + ((x) * 0x10))
	#define MAC_PPSX_INTERVAL(x)		(0x00007088 + ((x) * 0x10))
	#define MAC_PPSX_WIDTH(x)		(0x0000708c + ((x) * 0x10))
#else
	#define MAC_PPS_CONTROL		0x00000b70
	#define MAC_PPSX_TARGET_TIME_SEC(x)	(0x00000b80 + ((x) * 0x10))
	#define MAC_PPSX_TARGET_TIME_NSEC(x)	(0x00000b84 + ((x) * 0x10))
	#define MAC_PPSX_INTERVAL(x)		(0x00000b88 + ((x) * 0x10))
	#define MAC_PPSX_WIDTH(x)		(0x00000b8c + ((x) * 0x10))
#endif

#define TRGTBUSY0			BIT(31)
#define TTSL0				GENMASK(30, 0)

#define PPS_START_DELAY 100000000
#define ONE_NS 1000000000
#define PPS_ADJUST_NS 32

#define DWC_ETH_QOS_PPS_CH_0 0
#define DWC_ETH_QOS_PPS_CH_1 1
#define DWC_ETH_QOS_PPS_CH_2 2
#define DWC_ETH_QOS_PPS_CH_3 3

#define AVB_CLASS_A_POLL_DEV_NODE "avb_class_a_intr"

#define AVB_CLASS_B_POLL_DEV_NODE "avb_class_b_intr"

#define AVB_CLASS_A_CHANNEL_NUM 2
#define AVB_CLASS_B_CHANNEL_NUM 3

#define VOTE_IDX_0MBPS 0
#define VOTE_IDX_10MBPS 1
#define VOTE_IDX_100MBPS 2
#define VOTE_IDX_1000MBPS 3
#define VOTE_IDX_2500MBPS 4
#define VOTE_IDX_5000MBPS 5
#define VOTE_IDX_10000MBPS 6

//Mac config
#define XGMAC_RX_CONFIG		0x00000004
#define XGMAC_CONFIG_LM			BIT(10)

//Mac config
#define XGMAC_RX_CONFIG		0x00000004
#define XGMAC_CONFIG_LM			BIT(10)

#define TLMM_BASE_ADDRESS (tlmm_central_base_addr)

#define TLMM_MDC_HDRV_PULL_CTL_ADDRESS\
	(((unsigned long *)\
	  (TLMM_BASE_ADDRESS + 0xA1000)))

#define TLMM_MDIO_HDRV_PULL_CTL_ADDRESS\
	(((unsigned long *)\
	  (TLMM_BASE_ADDRESS + 0xA0000)))

#define TLMM_MDC_HDRV_PULL_CTL_RGWR(data)\
	iowrite32(data, (void __iomem *)TLMM_MDC_HDRV_PULL_CTL_ADDRESS)
#define TLMM_MDC_HDRV_PULL_CTL_RGRD(data)\
	((data) = ioread32((void __iomem *)TLMM_MDC_HDRV_PULL_CTL_ADDRESS))

#define TLMM_MDIO_HDRV_PULL_CTL_RGWR(data)\
	iowrite32(data, (void __iomem *)TLMM_MDIO_HDRV_PULL_CTL_ADDRESS)
#define TLMM_MDIO_HDRV_PULL_CTL_RGRD(data)\
	((data) = ioread32((void __iomem *)TLMM_MDIO_HDRV_PULL_CTL_ADDRESS))

#define TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_2MA ((unsigned long)(0x0))
#define TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_4MA ((unsigned long)(0x1))
#define TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_6MA ((unsigned long)(0x2))
#define TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_8MA ((unsigned long)(0x3))
#define TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_10MA ((unsigned long)(0x4))
#define TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_12MA ((unsigned long)(0x5))
#define TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_14MA ((unsigned long)(0x6))
#define TLMM_MDIO_HDRV_PULL_CTL1_TX_HDRV_16MA ((unsigned long)(0x7))

#define MAC_REG_SIZE 4

enum eth_mode {
	UNKNOWN         = 0,            /** Unknown */
	USXGMII_10G     = (1 << 0),     /** USXGMII 10G data rate */
	USXGMII_5G      = (1 << 1),     /** USXGMII 5G data rate */
	USXGMII_2_5G    = (1 << 2),     /** USXGMII 2.5G data rate */
	USXGMII_1G      = (1 << 3),     /** USXGMII 1G data rate */
	USXGMII_100M    = (1 << 4),     /** USXGMII 100M data rate */
	USXGMII_10M     = (1 << 5),     /** USXGMII 10M data rate */
	SGMII_2_5G      = (1 << 6),     /** SGMII 2.5G data rate */
	SGMII_1G        = (1 << 7),     /** SGMII 1G data rate */
	SGMII_100M      = (1 << 8)      /** SGMII 100M data rate */
};

enum link_mode_change_status {
	ACCEPTED        = 1,        /** Request accepted to update link mode */
	COMPLETED       = 2,        /** Successfully completed link mode update */
	FAILED          = 3,        /** Request failed to update link mode */
	REJECTED        = 4,         /** Request rejected to update link mode */
	TIME_OUT	= 5
};

enum phy_mode {
	ETHQOS_PHY_STATE_UP,
	ETHQOS_PHY_STATE_DOWN
};

enum nlmsg_flags {
	SUCCESS,
	FAIL,
	TIMEOUT,
	INVALID_SPEED
};

enum nlmsg_type {
	THERM_SET_LOCAL_ETH_CAP		= 1,
	THERM_NEW_LOCAL_ETH_MODE	= 2,
	THERM_NEW_LOCAL_ETH_MODE_UPDATE = 3,
	THERM_NEW_PEER_ETH_MODE		= 4
};

static inline u32 PPSCMDX(u32 x, u32 val)
{
	return (GENMASK(PPS_MINIDX(x) + 3, PPS_MINIDX(x)) &
	((val) << PPS_MINIDX(x)));
}

static inline u32 TRGTMODSELX(u32 x, u32 val)
{
	return (GENMASK(PPS_MAXIDX(x) - 1, PPS_MAXIDX(x) - 2) &
	((val) << (PPS_MAXIDX(x) - 2)));
}

static inline u32 PPSX_MASK(u32 x)
{
	return GENMASK(PPS_MAXIDX(x), PPS_MINIDX(x));
}

enum IO_MACRO_PHY_MODE {
	RGMII_MODE,
	RMII_MODE,
	MII_MODE
};

enum loopback_mode {
	DISABLE_LOOPBACK = 0,
	ENABLE_IO_MACRO_LOOPBACK,
	ENABLE_MAC_LOOPBACK,
	ENABLE_PHY_LOOPBACK,
	ENABLE_SERDES_LOOPBACK,
};

enum nw_loopback_mode {
	DISABLE_NW_LOOPBACK = 0,
	ENABLE_PCS_NW_LOOPBACK,
};

enum loopback_direction {
	HOST_LOOPBACK_MODE = 1,
	NETWORK_LOOPBACK_MODE,
};

enum phy_power_mode {
	DISABLE_PHY_IMMEDIATELY = 1,
	ENABLE_PHY_IMMEDIATELY,
	DISABLE_PHY_AT_SUSPEND_ONLY,
	DISABLE_PHY_SUSPEND_ENABLE_RESUME,
	DISABLE_PHY_ON_OFF,
};

enum current_phy_state {
	PHY_IS_ON = 0,
	PHY_IS_OFF,
};

struct ethqos_vlan_info {
	u16 vlan_id;
	u32 vlan_offset;
	u32 rx_queue;
	bool available;
};

struct ethqos_emac_por {
	unsigned int offset;
	unsigned int value;
};

struct ethqos_emac_driver_data {
	struct ethqos_emac_por *por;
	unsigned int num_por;
};

struct emac_icc_data {
	const char *name;
	u32 average_bandwidth;
	u32 peak_bandwidth;
};

static struct emac_icc_data emac_axi_icc_data[] = {
	{
		.name = "SPEED_0Mbps",
		.average_bandwidth = 0,
		.peak_bandwidth = 0,
	},
	{
		.name = "SPEED_10Mbps",
		.average_bandwidth = 2500,
		.peak_bandwidth = 2500,
	},
	{
		.name = "SPEED_100Mbps",
		.average_bandwidth = 25000,
		.peak_bandwidth = 25000,
	},
	{
		.name = "SPEED_1Gbps",
		.average_bandwidth = 250000,
		.peak_bandwidth = 250000,
	},
	{
		.name = "SPEED_2.5Gbps",
		.average_bandwidth = 625000,
		.peak_bandwidth = 625000,
	},
	{
		.name = "SPEED_5Gbps",
		.average_bandwidth = 825000,
		.peak_bandwidth = 825000,
	},
	{
		.name = "SPEED_10Gbps",
		.average_bandwidth = 1100000,
		.peak_bandwidth = 1100000,
	},
	{
		.name = "SPEED_peak",
		.average_bandwidth = 13106400,
		.peak_bandwidth = 13106400,
	},
};

static struct emac_icc_data emac_apb_icc_data[] = {
	{
		.name = "SPEED_0Mbps",
		.average_bandwidth = 0,
		.peak_bandwidth = 0,
	},
	{
		.name = "SPEED_10Mbps",
		.average_bandwidth = 0,
		.peak_bandwidth = 2500,
	},
	{
		.name = "SPEED_100Mbps",
		.average_bandwidth = 0,
		.peak_bandwidth = 25000,
	},
	{
		.name = "SPEED_1Gbps",
		.average_bandwidth = 0,
		.peak_bandwidth = 250000,
	},
	{
		.name = "SPEED_2.5Gbps",
		.average_bandwidth = 0,
		.peak_bandwidth = 625000,
	},
	{
		.name = "SPEED_5Gbps",
		.average_bandwidth = 0,
		.peak_bandwidth = 825000,
	},
	{
		.name = "SPEED_10Gbps",
		.average_bandwidth = 0,
		.peak_bandwidth = 1100000,
	},
	{
		.name = "SPEED_peak",
		.average_bandwidth = 655320,
		.peak_bandwidth = 655320,
	},
};

struct mac_csr_data {
	u32 offset;
	u32 value;
};

struct qcom_ethqos {
	struct platform_device *pdev;
	void __iomem *rgmii_base;
	void __iomem *sgmii_base;
	void __iomem *ioaddr;
	u32 rgmii_phy_base;
	struct qtee_shm shm_rgmii_hsr;
	struct qtee_shm shm_rgmii_local;
	phys_addr_t phys_rgmii_hsr_por;
	struct msm_bus_scale_pdata *bus_scale_vec;
	u32 bus_hdl;
	unsigned int rgmii_clk_rate;
	struct clk *rgmii_clk;
	struct clk *phyaux_clk;
	struct clk *sgmiref_clk;

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	/* Clocks required for SGMII and USXGMII interfaces */
	struct clk *clk_eee;
	struct clk *sgmii_rx_clk;
	struct clk *sgmii_tx_clk;
	struct clk *pcs_rx_clk;
	struct clk *pcs_tx_clk;
	struct clk *xgxs_rx_clk;
	struct clk *xgxs_tx_clk;
	struct clk *sgmii_rx_clk_src;
	struct clk *sgmii_rclk;
	struct clk *sgmii_tx_clk_src;
	struct clk *sgmii_tclk;
	struct clk *sgmii_mac_rx_clk_src;
	struct clk *sgmii_mac_rclk;
	struct clk *sgmii_mac_tx_clk_src;
	struct clk *sgmii_mac_tclk;
#endif

	unsigned int speed;
	unsigned int vote_idx;

	/* Serdes clocks will be set based on PHY max speed */
	unsigned int usxgmii_mode;

	int gpio_phy_intr_redirect;
	u32 phy_intr;
	u32 wol_intr;

	/* Work struct for handling phy interrupt */
	struct work_struct emac_phy_work;

	/* Work struct for handling phy interrupt */
	struct work_struct emac_wol_work;

	/* Work struct for handling pcs interrupt */
	struct work_struct emac_pcs_work;

	/* Work struct for handling phy state */
	struct work_struct emac_phy_state_work;

	struct delayed_work mdio_icc_idle_work;  /* MDIO ICC idle-timer debounce */
	struct mutex mdio_icc_lock;              /* serialises ICC peak vote access */
	bool mdio_icc_peak_active;              /* serialised by mdio_icc_lock */

	struct ethqos_emac_por *por;
	unsigned int num_por;
	unsigned int emac_ver;

	struct regulator *gdsc_emac;
	struct regulator *reg_rgmii;
	struct regulator *reg_emac_phy;
	struct regulator *reg_rgmii_io_pads;
	struct regulator *vreg_a_sgmii_1p2;
	struct regulator *vreg_a_sgmii_0p9;

	int pps_class_a_irq;
	int pps_class_b_irq;

	struct pinctrl_state *emac_pps_0;

	/* avb_class_a dev node variables*/
	dev_t avb_class_a_dev_t;
	struct cdev *avb_class_a_cdev;
	struct class *avb_class_a_class;

	/* avb_class_b dev node variables*/
	dev_t avb_class_b_dev_t;
	struct cdev *avb_class_b_cdev;
	struct class *avb_class_b_class;
	struct icc_path *axi_icc_path;
	struct emac_icc_data *emac_axi_icc;
	struct icc_path *apb_icc_path;
	struct emac_icc_data *emac_apb_icc;

	dev_t emac_dev_t;
	struct cdev *emac_cdev;
	struct class *emac_class;

	unsigned long avb_class_a_intr_cnt;
	unsigned long avb_class_b_intr_cnt;

	/* Mac recovery dev node variables*/
	dev_t emac_rec_dev_t;
	struct cdev *emac_rec_cdev;
	struct class *emac_rec_class;

	/* saving state for Wake-on-LAN */
	int wolopts;
	/* state of enabled wol options in PHY*/
	u32 phy_wol_wolopts;
	/* state of supported wol options in PHY*/
	u32 phy_wol_supported;
	/* Boolean to check if clock is suspended*/
	int clks_suspended;
	/* Structure which holds done and wait members */
	struct completion clk_enable_done;
	/* early ethernet parameters */
	struct work_struct early_eth;
	struct delayed_work ipv4_addr_assign_wq;
	struct delayed_work ipv6_addr_assign_wq;
	bool early_eth_enabled;
	/* Key Performance Indicators */
	bool print_kpi;
	/* Debug nodes*/
#if IS_ENABLED(CONFIG_DEBUG_FS)
	struct dentry *debugfs_dir;
#endif
	struct kobject *sysfs_kobj;

	int curr_serdes_speed;
	unsigned int emac_phy_off_suspend;
	int loopback_speed;
	u32 max_speed_enforce;
	enum phy_power_mode current_phy_mode;
	enum current_phy_state phy_state;
	/*Backup variable for phy loopback*/
	int backup_duplex;
	int backup_speed;
	u32 bmcr_backup;
	/*Backup variable for suspend resume*/
	int backup_suspend_speed;
	u32 backup_bmcr;
	unsigned backup_autoneg:1;
	bool probed;
	bool ipa_enabled;
	struct notifier_block panic_nb;
	struct notifier_block vm_nb;
	bool panic_notifier_registered;

	struct stmmac_priv *priv;

	/* QMI over ethernet parameter */
	u32 qoe_mode;
	struct ethqos_vlan_info qoe_vlan;
	bool cv2x_pvm_only_enabled;
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_HOSTVM)
	bool last_event_linkup;
	s8 passthrough_en;
#else
	s8 cv2x_priority;
#endif
	unsigned int cv2x_queue;

	/* Mac recovery parameters */
	int mac_err_cnt[MAC_ERR_CNT];
	bool mac_rec_en[MAC_ERR_CNT];
	bool mac_rec_fail[MAC_ERR_CNT];
	int mac_rec_cnt[MAC_ERR_CNT];
	int mac_rec_threshold[MAC_ERR_CNT];
	struct delayed_work tdu_rec;
	bool tdu_scheduled;
	int tdu_chan;
	bool init_complete;

	struct mac_csr_data *mac_reg_list;
	bool power_state;
	bool gdsc_off_on_suspend;
	enum link_mode_change_status peer_resp;
	struct completion thermal_timer;
	struct hrtimer hr_timer;
	enum phy_mode curr_phy_state;
	struct sock *socket_nl;
	struct nlmsghdr *nlh;
	enum eth_mode peer_cap;
	enum eth_mode current_mode;
#if IS_ENABLED(CONFIG_ETHQOS_QCOM_VER4)
	/* Protects clocks and reference count */
	struct mutex ps_lock;
	refcount_t ps_refcount;
	bool clk_active;
	bool clk_gated;
#endif
};

struct pps_cfg {
	unsigned int ptpclk_freq;
	unsigned int ppsout_freq;
	unsigned int ppsout_ch;
	unsigned int ppsout_duty;
	unsigned int ppsout_start;
	unsigned int ppsout_align;
	unsigned int ppsout_align_ns;
};

struct ifr_data_struct {
	unsigned int flags;
	unsigned int qinx; /* dma channel no to be configured */
	unsigned int cmd;
	unsigned int context_setup;
	unsigned int connected_speed;
	unsigned int rwk_filter_values[8];
	unsigned int rwk_filter_length;
	int command_error;
	int test_done;
	void *ptr;
};

struct pps_info {
	int channel_no;
};

struct ip_params {
	bool is_valid_mac_addr;
	char ipv4_addr_str[32];
	struct in_addr ipv4_addr;
	bool is_valid_ipv4_addr;
	char ipv6_addr_str[48];
	struct in6_ifreq ipv6_addr;
	bool is_valid_ipv6_addr;
	unsigned char mac_addr[QCOM_ETH_QOS_MAC_ADDR_LEN];
};

struct mac_params {
	phy_interface_t eth_intf;
	bool is_valid_eth_intf;
	unsigned int link_speed;
	char qoscfg_name[4];
	bool wait_switch_rdy;
	bool c37_an_en;
};

int qcom_ethqos_bring_down_phy_if(struct device *dev);
int qcom_ethqos_bring_up_phy_if(struct device *dev, bool client_mode);
int ethqos_init_sgmii_regulators(struct qcom_ethqos *ethqos);
int ethqos_enable_serdes_consumers(struct qcom_ethqos *ethqos);
int ethqos_disable_serdes_consumers(struct qcom_ethqos *ethqos);
int ethqos_init_regulators(struct qcom_ethqos *ethqos);
void ethqos_disable_regulators(struct qcom_ethqos *ethqos);
int ethqos_init_gpio(struct qcom_ethqos *ethqos);
void ethqos_free_gpios(struct qcom_ethqos *ethqos);
void *qcom_ethqos_get_priv(struct qcom_ethqos *ethqos);
int create_pps_interrupt_device_node(dev_t *pps_dev_t,
				     struct cdev **pps_cdev,
				     struct class **pps_class,
				     char *pps_dev_node_name);
bool qcom_ethqos_is_phy_link_up(struct qcom_ethqos *ethqos);
void *qcom_ethqos_get_priv(struct qcom_ethqos *ethqos);

int ppsout_config(struct stmmac_priv *priv, struct pps_cfg *eth_pps_cfg);
int ethqos_phy_power_on(struct qcom_ethqos *ethqos);
void  ethqos_phy_power_off(struct qcom_ethqos *ethqos);

u16 dwmac_qcom_select_queue(struct net_device *dev,
			    struct sk_buff *skb,
			    struct net_device *sb_dev);

#define QTAG_VLAN_ETH_TYPE_OFFSET 16
#define QTAG_UCP_FIELD_OFFSET 14
#define QTAG_ETH_TYPE_OFFSET 12
#define PTP_UDP_EV_PORT 0x013F
#define PTP_UDP_GEN_PORT 0x0140

#define IPA_DMA_TX_CH 0
#define IPA_DMA_RX_CH 0

#define QMI_TAG_TX_CHANNEL 2
#define CV2X_TAG_TX_CHANNEL 4

#define VLAN_TAG_UCP_SHIFT 13
#define CLASS_A_TRAFFIC_UCP 3
#define CLASS_A_TRAFFIC_TX_CHANNEL 3

#define CLASS_B_TRAFFIC_UCP 2
#define CLASS_B_TRAFFIC_TX_CHANNEL 2

#define NON_TAGGED_IP_TRAFFIC_TX_CHANNEL 1
#define ALL_OTHER_TRAFFIC_TX_CHANNEL 1
#define ALL_OTHER_TX_TRAFFIC_IPA_DISABLED 0

#define DEFAULT_INT_MOD 1
#define AVB_INT_MOD 8
#define IP_PKT_INT_MOD 32
#define PTP_INT_MOD 1

#define PPS_19_2_FREQ 19200000

#define ETH_MAX_NICS 2

enum dwmac_qcom_queue_operating_mode {
	DWMAC_QCOM_QDISABLED = 0X0,
	DWMAC_QCOM_QAVB,
	DWMAC_QCOM_QDCB,
	DWMAC_QCOM_QGENERIC
};

struct dwmac_qcom_avb_algorithm_params {
	unsigned int idle_slope;
	unsigned int send_slope;
	unsigned int hi_credit;
	unsigned int low_credit;
};

struct dwmac_qcom_avb_algorithm {
	unsigned int qinx;
	unsigned int algorithm;
	unsigned int cc;
	struct dwmac_qcom_avb_algorithm_params speed100params;
	struct dwmac_qcom_avb_algorithm_params speed1000params;
	enum dwmac_qcom_queue_operating_mode op_mode;
};

void qcom_ethqos_request_phy_wol(void *plat_n);
void ethqos_trigger_phylink(struct qcom_ethqos *ethqos, bool status);
void  ethqos_phy_power_off(struct qcom_ethqos *ethqos);
int ethqos_phy_power_on(struct qcom_ethqos *ethqos);
void dwmac_qcom_program_avb_algorithm(struct stmmac_priv *priv,
				      struct ifr_data_struct *req);
unsigned int dwmac_qcom_get_plat_tx_coal_frames(struct sk_buff *skb);
int ethqos_init_pps(void *priv);
unsigned int dwmac_qcom_get_eth_type(unsigned char *buf);
bool is_c37_an_supported(struct stmmac_priv *priv);
void ethqos_set_mac_speed_mode_duplex(struct stmmac_priv *priv, int speed,
				      phy_interface_t interface, int duplex);

#if IS_ENABLED(CONFIG_ETHQOS_QCOM_HOSTVM)
void qcom_ethstate_update(struct plat_stmmacenet_data *plat, enum eth_state event);
#else
static inline void qcom_ethstate_update(struct plat_stmmacenet_data *plat, enum eth_state event)
{
	/* Not enabled */
}
#endif /* CONFIG_ETHQOS_QCOM_HOSTVM */

#define EMAC_GDSC_EMAC_NAME "gdsc_emac"

#endif
