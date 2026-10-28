// SPDX-License-Identifier: GPL-2.0-only

/* Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries. */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/kthread.h>
#include <linux/of.h>

#include "dwmac-qcom-serdes.h"

void qcom_ethqos_serdes_power_down(struct qcom_ethqos *ethqos)
{
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_PCS_TX_MID_TERM_CTRL2);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	usleep_range(100, 200);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_PHY_START);
	writel_relaxed(0x0, ethqos->sgmii_base + QSERDES_PCS_POWER_DOWN_CONTROL);

	/* Set current speed to 0 so that serdes will be reprogrammed on next link up. */
	ethqos->curr_serdes_speed = 0;
}

static int qcom_ethqos_serdes3_soft_reset(struct qcom_ethqos *ethqos)
{
	int ret = 0;
	unsigned int val;

	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES3_PCS_SW_RESET);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_PCS_SW_RESET);
	usleep_range(3000, 5000);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES3_PCS_PHY_START);
	usleep_range(3000, 5000);

	ret = readl_poll_timeout(ethqos->sgmii_base + QSERDES3_COM_C_READY_STATUS,
				 val, val & QSERDES3_COM_C_READY, 1000, 500000);
	if (ret) {
		ETHQOSERR("QSERDES3_COM_C_READY_STATUS timed out\n");
		goto err_ret;
	}

	ret = readl_poll_timeout(ethqos->sgmii_base + QSERDES3_PCS_PCS_READY_STATUS,
				 val, val & QSERDES3_PCS_READY, 1000, 500000);
	if (ret) {
		ETHQOSERR("PCS_READY timed out\n");
		goto err_ret;
	}

	ret = readl_poll_timeout(ethqos->sgmii_base + QSERDES3_PCS_PCS_READY_STATUS,
				 val, val & QSERDES3_PCS_SGMIIPHY_READY, 1000, 500000);
	if (ret) {
		ETHQOSERR("SGMIIPHY_READY timed out\n");
		goto err_ret;
	}

	ret = readl_poll_timeout(ethqos->sgmii_base + QSERDES3_COM_CMN_STATUS,
				 val, val & QSERDES3_COM_C_PLL_LOCKED, 1000, 5000000);
	if (ret) {
		ETHQOSERR("PLL Lock Status timed out\n");
		goto err_ret;
	}

	return ret;

err_ret:
	ETHQOSERR("Serdes soft reset failed\n");
	return ret;
}

void qcom_ethqos_serdes_soft_reset(struct qcom_ethqos *ethqos)
{
	int ret = 0;
	int retry = 500;
	unsigned int val;

	if (ethqos->emac_ver == EMAC_HW_v3_1_0) {
		ret = qcom_ethqos_serdes3_soft_reset(ethqos);
		if (ret)
			ETHQOSERR("SerDes3 soft reset failed\n");
		return;
	}

	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	usleep_range(3000, 5000);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_PHY_START);
	usleep_range(3000, 5000);

	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_COM_CMN_STATUS);
		val &= BIT(1);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("PLL Lock Status timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_COM_C_READY_STATUS);
		val &= BIT(0);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("C_READY_STATUS timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_PCS_READY_STATUS);
		val &= BIT(7);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("PCS_READY timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_PCS_READY_STATUS);
		val &= BIT(0);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("SGMIIPHY_READY timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	return;

err_ret:
	ETHQOSERR("Serdes soft reset failed\n");

}
EXPORT_SYMBOL(qcom_ethqos_serdes_soft_reset);

void qcom_ethqos_disable_serdes_clocks(struct qcom_ethqos *ethqos)
{
	if (ethqos->phyaux_clk)
		clk_disable_unprepare(ethqos->phyaux_clk);

	if (ethqos->sgmiref_clk)
		clk_disable_unprepare(ethqos->sgmiref_clk);
}
EXPORT_SYMBOL(qcom_ethqos_disable_serdes_clocks);

int qcom_ethqos_enable_serdes_clocks(struct qcom_ethqos *ethqos)
{
	int ret = 0;

	if (ethqos->phyaux_clk) {
		ret = clk_prepare_enable(ethqos->phyaux_clk);
		if (ret) {
			ETHQOSERR("Failed to enable phyaux clock: %d\n", ret);
			return ret;
		}
	}

	if (ethqos->sgmiref_clk) {
		ret = clk_prepare_enable(ethqos->sgmiref_clk);
		if (ret) {
			ETHQOSERR("Failed to enable SerDes reference clock: %d\n", ret);
			return ret;
		}
	}

	return ret;
}
EXPORT_SYMBOL(qcom_ethqos_enable_serdes_clocks);

static int qcom_ethqos_serdes3_sgmii_1Gb(struct qcom_ethqos *ethqos)
{
	int retry = 500;
	unsigned int val;
	int ret = 0;

	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES3_PCS_SW_RESET);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES3_PCS_POWER_DOWN_CONTROL);

	/***************** MODULE: QSERDES3_COM_SGMII_QMP_PLL*********/
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES3_COM_PLL_IVCO);
	writel_relaxed(0x06, ethqos->sgmii_base + QSERDES3_COM_CP_CTRL_MODE0);
	writel_relaxed(0x16, ethqos->sgmii_base + QSERDES3_COM_PLL_RCTRL_MODE0);
	writel_relaxed(0x36, ethqos->sgmii_base + QSERDES3_COM_PLL_CCTRL_MODE0);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES3_COM_SYSCLK_EN_SEL);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES3_COM_LOCK_CMP1_MODE0);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES3_COM_LOCK_CMP2_MODE0);
	writel_relaxed(0x82, ethqos->sgmii_base + QSERDES3_COM_DEC_START_MODE0);
	writel_relaxed(0x55, ethqos->sgmii_base + QSERDES3_COM_DIV_FRAC_START1_MODE0);
	writel_relaxed(0x55, ethqos->sgmii_base + QSERDES3_COM_DIV_FRAC_START2_MODE0);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES3_COM_DIV_FRAC_START3_MODE0);
	writel_relaxed(0x24, ethqos->sgmii_base + QSERDES3_COM_VCO_TUNE1_MODE0);

	writel_relaxed(0x02, ethqos->sgmii_base + QSERDES3_COM_VCO_TUNE2_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_COM_VCO_TUNE_INITVAL2);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES3_COM_HSCLK_SEL);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_COM_HSCLK_HS_SWITCH_SEL);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES3_COM_CORECLK_DIV_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_COM_CORE_CLK_EN);
	writel_relaxed(0xB9, ethqos->sgmii_base + QSERDES3_COM_BIN_VCOCAL_CMP_CODE1_MODE0);
	writel_relaxed(0x1E, ethqos->sgmii_base + QSERDES3_COM_BIN_VCOCAL_CMP_CODE2_MODE0);
	writel_relaxed(0x11, ethqos->sgmii_base + QSERDES3_COM_BIN_VCOCAL_HSCLK_SEL);

	/******************MODULE: QSERDES3_TX0_SGMII_QMP_TX***********************/
	writel_relaxed(0x05, ethqos->sgmii_base + QSERDES3_TX_TX_BAND);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES3_TX_SLEW_CNTL);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES3_TX_RES_CODE_LANE_OFFSET_TX);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES3_TX_RES_CODE_LANE_OFFSET_RX);
	writel_relaxed(0x05, ethqos->sgmii_base + QSERDES3_TX_LANE_MODE_1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_TX_LANE_MODE_3);
	writel_relaxed(0x12, ethqos->sgmii_base + QSERDES3_TX_RCV_DETECT_LVL_2);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES3_TX_TRAN_DRVR_EMP_EN);

	/*****************MODULE: QSERDES3_RX0_SGMII_QMP_RX*******************/
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES3_RX_UCDR_FO_GAIN);
	writel_relaxed(0x06, ethqos->sgmii_base + QSERDES3_RX_UCDR_SO_GAIN);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES3_RX_UCDR_FASTLOCK_FO_GAIN);
	writel_relaxed(0x7F, ethqos->sgmii_base + QSERDES3_RX_UCDR_SO_SATURATION_AND_ENABLE);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_RX_UCDR_FASTLOCK_COUNT_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES3_RX_UCDR_FASTLOCK_COUNT_HIGH);
	writel_relaxed(0x81, ethqos->sgmii_base + QSERDES3_RX_UCDR_PI_CONTROLS);
	writel_relaxed(0x80, ethqos->sgmii_base + QSERDES3_RX_UCDR_PI_CTRL2);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES3_RX_RX_TERM_BW);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES3_RX_VGA_CAL_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES3_RX_GM_CAL);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES3_RX_RX_EQU_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_RX_RX_EQU_ADAPTOR_CNTRL2);
	writel_relaxed(0x4A, ethqos->sgmii_base + QSERDES3_RX_RX_EQU_ADAPTOR_CNTRL3);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES3_RX_RX_EQU_ADAPTOR_CNTRL4);
	writel_relaxed(0x80, ethqos->sgmii_base + QSERDES3_RX_RX_IDAC_TSETTLE_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES3_RX_RX_IDAC_TSETTLE_HIGH);
	writel_relaxed(0x20, ethqos->sgmii_base + QSERDES3_RX_RX_IDAC_MEASURE_TIME);
	writel_relaxed(0x17, ethqos->sgmii_base + QSERDES3_RX_RX_EQ_OFFSET_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_RX_RX_OFFSET_ADAPTOR_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES3_RX_SIGDET_CNTRL);
	writel_relaxed(0x1E, ethqos->sgmii_base + QSERDES3_RX_SIGDET_DEGLITCH_CNTRL);
	writel_relaxed(0x05, ethqos->sgmii_base + QSERDES3_RX_RX_BAND);
	writel_relaxed(0xE0, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_00_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_00_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_00_HIGH2);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_00_HIGH3);
	writel_relaxed(0xB1, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_00_HIGH4);
	writel_relaxed(0xE0, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_01_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_01_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_01_HIGH2);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_01_HIGH3);
	writel_relaxed(0xB1, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_01_HIGH4);
	writel_relaxed(0xE0, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_10_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_10_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_10_HIGH2);
	writel_relaxed(0x3B, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_10_HIGH3);
	writel_relaxed(0xB7, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_10_HIGH4);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES3_RX_DCC_CTRL1);

	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES3_PCS_LINE_RESET_TIME);
	writel_relaxed(0x1F, ethqos->sgmii_base + QSERDES3_PCS_TX_LARGE_AMP_DRV_LVL);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES3_PCS_TX_SMALL_AMP_DRV_LVL);
	writel_relaxed(0x83, ethqos->sgmii_base + QSERDES3_PCS_TX_MID_TERM_CTRL1);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES3_PCS_TX_MID_TERM_CTRL2);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES3_PCS_SGMII_MISC_CTRL8);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_PCS_SW_RESET);

	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES3_PCS_PHY_START);

	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES3_COM_C_READY_STATUS);
		val &= QSERDES3_COM_C_READY;
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("QSERDES3_COM_C_READY_STATUS timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES3_PCS_PCS_READY_STATUS);
		val &= QSERDES3_PCS_READY;
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("PCS_READY timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES3_PCS_PCS_READY_STATUS);
		val &= QSERDES3_PCS_SGMIIPHY_READY;
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("SGMIIPHY_READY timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 5000;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES3_COM_CMN_STATUS);
		val &= QSERDES3_COM_C_PLL_LOCKED;
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("PLL Lock Status timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

err_ret:
		return ret;
}

static int qcom_ethqos_serdes3_sgmii_2_5Gb(struct qcom_ethqos *ethqos)
{
	int retry = 500;
	unsigned int val;
	int ret = 0;

	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES3_PCS_SW_RESET);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES3_PCS_POWER_DOWN_CONTROL);

	/***************** MODULE: QSERDES_COM_SGMII_QMP_PLL*********/
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES3_COM_PLL_IVCO);
	writel_relaxed(0x06, ethqos->sgmii_base + QSERDES3_COM_CP_CTRL_MODE0);
	writel_relaxed(0x16, ethqos->sgmii_base + QSERDES3_COM_PLL_RCTRL_MODE0);
	writel_relaxed(0x36, ethqos->sgmii_base + QSERDES3_COM_PLL_CCTRL_MODE0);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES3_COM_SYSCLK_EN_SEL);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES3_COM_LOCK_CMP1_MODE0);
	writel_relaxed(0x41, ethqos->sgmii_base + QSERDES3_COM_LOCK_CMP2_MODE0);
	writel_relaxed(0x7A, ethqos->sgmii_base + QSERDES3_COM_DEC_START_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_COM_DIV_FRAC_START1_MODE0);
	writel_relaxed(0x20, ethqos->sgmii_base + QSERDES3_COM_DIV_FRAC_START2_MODE0);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES3_COM_DIV_FRAC_START3_MODE0);
	writel_relaxed(0xA1, ethqos->sgmii_base + QSERDES3_COM_VCO_TUNE1_MODE0);

	writel_relaxed(0x02, ethqos->sgmii_base + QSERDES3_COM_VCO_TUNE2_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_COM_VCO_TUNE_INITVAL2);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES3_COM_HSCLK_SEL);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_COM_HSCLK_HS_SWITCH_SEL);
	writel_relaxed(0x05, ethqos->sgmii_base + QSERDES3_COM_CORECLK_DIV_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_COM_CORE_CLK_EN);
	writel_relaxed(0xCD, ethqos->sgmii_base + QSERDES3_COM_BIN_VCOCAL_CMP_CODE1_MODE0);
	writel_relaxed(0x1C, ethqos->sgmii_base + QSERDES3_COM_BIN_VCOCAL_CMP_CODE2_MODE0);
	writel_relaxed(0x11, ethqos->sgmii_base + QSERDES3_COM_BIN_VCOCAL_HSCLK_SEL);

	/******************MODULE: QSERDES_TX0_SGMII_QMP_TX***********************/
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES3_TX_TX_BAND);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES3_TX_SLEW_CNTL);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES3_TX_RES_CODE_LANE_OFFSET_TX);
	writel_relaxed(0x02, ethqos->sgmii_base + QSERDES3_TX_RES_CODE_LANE_OFFSET_RX);
	writel_relaxed(0x05, ethqos->sgmii_base + QSERDES3_TX_LANE_MODE_1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_TX_LANE_MODE_3);
	writel_relaxed(0x12, ethqos->sgmii_base + QSERDES3_TX_RCV_DETECT_LVL_2);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES3_TX_TRAN_DRVR_EMP_EN);

	/*****************MODULE: QSERDES_RX0_SGMII_QMP_RX*******************/
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES3_RX_UCDR_FO_GAIN);
	writel_relaxed(0x06, ethqos->sgmii_base + QSERDES3_RX_UCDR_SO_GAIN);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES3_RX_UCDR_FASTLOCK_FO_GAIN);
	writel_relaxed(0x7F, ethqos->sgmii_base + QSERDES3_RX_UCDR_SO_SATURATION_AND_ENABLE);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_RX_UCDR_FASTLOCK_COUNT_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES3_RX_UCDR_FASTLOCK_COUNT_HIGH);
	writel_relaxed(0x81, ethqos->sgmii_base + QSERDES3_RX_UCDR_PI_CONTROLS);
	writel_relaxed(0x80, ethqos->sgmii_base + QSERDES3_RX_UCDR_PI_CTRL2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_RX_RX_TERM_BW);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES3_RX_VGA_CAL_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES3_RX_GM_CAL);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES3_RX_RX_EQU_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_RX_RX_EQU_ADAPTOR_CNTRL2);
	writel_relaxed(0x4A, ethqos->sgmii_base + QSERDES3_RX_RX_EQU_ADAPTOR_CNTRL3);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES3_RX_RX_EQU_ADAPTOR_CNTRL4);
	writel_relaxed(0x80, ethqos->sgmii_base + QSERDES3_RX_RX_IDAC_TSETTLE_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES3_RX_RX_IDAC_TSETTLE_HIGH);
	writel_relaxed(0x20, ethqos->sgmii_base + QSERDES3_RX_RX_IDAC_MEASURE_TIME);
	writel_relaxed(0x17, ethqos->sgmii_base + QSERDES3_RX_RX_EQ_OFFSET_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_RX_RX_OFFSET_ADAPTOR_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES3_RX_SIGDET_CNTRL);
	writel_relaxed(0x1E, ethqos->sgmii_base + QSERDES3_RX_SIGDET_DEGLITCH_CNTRL);
	writel_relaxed(0x18, ethqos->sgmii_base + QSERDES3_RX_RX_BAND);
	writel_relaxed(0x18, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_00_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_00_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_00_HIGH2);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_00_HIGH3);
	writel_relaxed(0xB8, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_00_HIGH4);
	writel_relaxed(0xE0, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_01_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_01_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_01_HIGH2);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_01_HIGH3);
	writel_relaxed(0xB1, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_01_HIGH4);
	writel_relaxed(0xE0, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_10_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_10_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_10_HIGH2);
	writel_relaxed(0x3B, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_10_HIGH3);
	writel_relaxed(0xB7, ethqos->sgmii_base + QSERDES3_RX_RX_MODE_10_HIGH4);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES3_RX_DCC_CTRL1);

	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES3_PCS_LINE_RESET_TIME);
	writel_relaxed(0x1F, ethqos->sgmii_base + QSERDES3_PCS_TX_LARGE_AMP_DRV_LVL);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES3_PCS_TX_SMALL_AMP_DRV_LVL);
	writel_relaxed(0x83, ethqos->sgmii_base + QSERDES3_PCS_TX_MID_TERM_CTRL1);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES3_PCS_TX_MID_TERM_CTRL2);
	writel_relaxed(0x8C, ethqos->sgmii_base + QSERDES3_PCS_SGMII_MISC_CTRL8);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES3_PCS_SW_RESET);

	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES3_PCS_PHY_START);

	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES3_COM_C_READY_STATUS);
		val &= QSERDES3_COM_C_READY;
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("QSERDES3_COM_C_READY_STATUS timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES3_PCS_PCS_READY_STATUS);
		val &= QSERDES3_PCS_READY;
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("PCS_READY timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES3_PCS_PCS_READY_STATUS);
		val &= QSERDES3_PCS_SGMIIPHY_READY;
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("SGMIIPHY_READY timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 5000;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES3_COM_CMN_STATUS);
		val &= QSERDES3_COM_C_PLL_LOCKED;
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("PLL Lock Status timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

err_ret:
		return ret;
}

static int qcom_ethqos_serdes_sgmii_1Gb(struct qcom_ethqos *ethqos)
{
	int ret = 0;
	int retry = 500;
	unsigned int val;

	if (ethqos->emac_ver == EMAC_HW_v3_1_0)
		return qcom_ethqos_serdes3_sgmii_1Gb(ethqos);

	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_POWER_DOWN_CONTROL);

	/***************** MODULE: QSERDES_COM_SGMII_QMP_PLL*********/
	writel_relaxed(0x0E, ethqos->sgmii_base + QSERDES_COM_BG_TIMER);
	writel_relaxed(0x07, ethqos->sgmii_base + QSERDES_COM_BIAS_EN_CLKBUFLR_EN);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_COM_CLK_ENABLE1);
	writel_relaxed(0x1F, ethqos->sgmii_base + QSERDES_COM_PLL_IVCO);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_COM_CP_CTRL_MODE0);
	writel_relaxed(0x16, ethqos->sgmii_base + QSERDES_COM_PLL_RCTRL_MODE0);
	writel_relaxed(0x36, ethqos->sgmii_base + QSERDES_COM_PLL_CCTRL_MODE0);
	writel_relaxed(0X1F, ethqos->sgmii_base + QSERDES_COM_INTEGLOOP_GAIN0_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_INTEGLOOP_GAIN1_MODE0);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_COM_PLL_EN);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES_COM_SYSCLK_EN_SEL);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES_COM_LOCK_CMP1_MODE0);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES_COM_LOCK_CMP2_MODE0);
	writel_relaxed(0x82, ethqos->sgmii_base + QSERDES_COM_DEC_START_MODE0);
	writel_relaxed(0x55, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START1_MODE0);
	writel_relaxed(0x55, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START2_MODE0);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START3_MODE0);
	writel_relaxed(0x84, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE1_MODE0);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE2_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE_INITVAL2);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_COM_HSCLK_SEL_1);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_COM_HSCLK_HS_SWITCH_SEL_1);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES_COM_CORECLK_DIV_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_CORE_CLK_EN);
	writel_relaxed(0x16, ethqos->sgmii_base + QSERDES_COM_CMN_CONFIG_1);
	writel_relaxed(0xB9, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_CMP_CODE1_MODE0);
	writel_relaxed(0x1E, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_CMP_CODE2_MODE0);
	writel_relaxed(0x11, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_HSCLK_SEL_1);

	/******************MODULE: QSERDES_TX0_SGMII_QMP_TX***********************/
	writel_relaxed(0x0D, ethqos->sgmii_base + QSERDES_TX_CLKBUF_ENABLE);
	writel_relaxed(0x05, ethqos->sgmii_base + QSERDES_TX0_TX_BAND);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES_TX0_SLEW_CNTL);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_TX0_RES_CODE_LANE_OFFSET_TX);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_TX0_RES_CODE_LANE_OFFSET_RX);
	writel_relaxed(0xF5, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_1);
	writel_relaxed(0x02, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_3);
	writel_relaxed(0x3F, ethqos->sgmii_base + QSERDES_TX_LANE_MODE_4);
	writel_relaxed(0x5F, ethqos->sgmii_base + QSERDES_TX_LANE_MODE_5);
	writel_relaxed(0x12, ethqos->sgmii_base + QSERDES_TX0_RCV_DETECT_LVL_2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_TX0_TRAN_DRVR_EMP_EN);
	writel_relaxed(0x2B, ethqos->sgmii_base + QSERDES_TX_TX_EMP_POST1_LVL);

	/*****************MODULE: QSERDES_RX0_SGMII_QMP_RX*******************/
	writel_relaxed(0x0D, ethqos->sgmii_base + QSERDES_RX0_UCDR_FO_GAIN);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_RX0_UCDR_SO_GAIN);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_FO_GAIN);
	writel_relaxed(0x7F, ethqos->sgmii_base + QSERDES_RX0_UCDR_SO_SATURATION_AND_ENABLE);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_COUNT_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_COUNT_HIGH);
	writel_relaxed(0x81, ethqos->sgmii_base + QSERDES_RX0_UCDR_PI_CONTROLS);
	writel_relaxed(0x81, ethqos->sgmii_base + QSERDES_RX0_UCDR_PI_CTRL2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX_UCDR_SB2_THRESH1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX_UCDR_SB2_THRESH2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_RX_TERM_BW);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_RX0_VGA_CAL_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_RX0_GM_CAL);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL2);
	writel_relaxed(0x4A, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL3);
	writel_relaxed(0x5A, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL4);
	writel_relaxed(0x80, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_TSETTLE_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_TSETTLE_HIGH);
	writel_relaxed(0x20, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_MEASURE_TIME);
	writel_relaxed(0x17, ethqos->sgmii_base + QSERDES_RX0_RX_EQ_OFFSET_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_RX_OFFSET_ADAPTOR_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CNTRL);
	writel_relaxed(0x1E, ethqos->sgmii_base + QSERDES_RX0_SIGDET_DEGLITCH_CNTRL);
	writel_relaxed(0x05, ethqos->sgmii_base + QSERDES_RX0_RX_BAND);
	writel_relaxed(0x18, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH2);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH3);
	writel_relaxed(0xB8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH4);
	writel_relaxed(0xE0, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH2);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH3);
	writel_relaxed(0xB1, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH4);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES_RX0_DCC_CTRL1);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES_PCS_LINE_RESET_TIME);

	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_PCS_TX_SMALL_AMP_DRV_LVL);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES_PCS_TX_LARGE_AMP_DRV_LVL);
	writel_relaxed(0x0B, ethqos->sgmii_base + QSERDES_PCS_TX_LARGE_AMP_POST_EMP_LVL);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_TX_SMALL_AMP_POST_EMP_LVL);
	writel_relaxed(0x83, ethqos->sgmii_base + QSERDES_PCS_TX_MID_TERM_CTRL1);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_PCS_TX_MID_TERM_CTRL2);
	writel_relaxed(0x10, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CAL_CTRL1);
	writel_relaxed(0x77, ethqos->sgmii_base + QSERDES_PCS_RX_SIGDET_CTRL2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CAL_CTRL2_AND_CDR_LOCK_EDGE);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_SGMII_MISC_CTRL7);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES_PCS_SGMII_MISC_CTRL8);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_RETIME_BUFFER_EN);

	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_PHY_START);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_PHY_START);
	usleep_range(3000, 5000);

	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_COM_CMN_STATUS);
		val &= BIT(1);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR(" %s : PLL Lock Status timedout with retry = %d\n",
			  __func__,
			  retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_COM_C_READY_STATUS);
		val &= BIT(0);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("%s : C_READY_STATUS timedout with retry = %d\n",
			  __func__,
			  retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_PCS_READY_STATUS);
		val &= BIT(7);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("%s : PCS_READY timedout with retry = %d\n",
			  __func__,
			  retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_PCS_READY_STATUS);
		val &= BIT(0);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("%s : SGMIIPHY_READY timedout with retry = %d\n",
			  __func__,
			  retry);
		ret = -1;
		goto err_ret;
	}

	return ret;

err_ret:
	return ret;
}

static int qcom_ethqos_serdes_sgmii_2p5Gb(struct qcom_ethqos *ethqos)
{
	int ret = 0;
	int retry = 500;
	unsigned int val;

	if (ethqos->emac_ver == EMAC_HW_v3_1_0)
		return qcom_ethqos_serdes3_sgmii_2_5Gb(ethqos);

	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_POWER_DOWN_CONTROL);

	/***************** MODULE: QSERDES_COM_SGMII_QMP_PLL*********/
	writel_relaxed(0x0E, ethqos->sgmii_base + QSERDES_COM_BG_TIMER);
	writel_relaxed(0x07, ethqos->sgmii_base + QSERDES_COM_BIAS_EN_CLKBUFLR_EN);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_COM_CLK_ENABLE1);
	writel_relaxed(0x1F, ethqos->sgmii_base + QSERDES_COM_PLL_IVCO);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_COM_CP_CTRL_MODE0);
	writel_relaxed(0x16, ethqos->sgmii_base + QSERDES_COM_PLL_RCTRL_MODE0);
	writel_relaxed(0x36, ethqos->sgmii_base + QSERDES_COM_PLL_CCTRL_MODE0);
	writel_relaxed(0X1F, ethqos->sgmii_base + QSERDES_COM_INTEGLOOP_GAIN0_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_INTEGLOOP_GAIN1_MODE0);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_COM_PLL_EN);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES_COM_SYSCLK_EN_SEL);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES_COM_LOCK_CMP1_MODE0);
	writel_relaxed(0x41, ethqos->sgmii_base + QSERDES_COM_LOCK_CMP2_MODE0);
	writel_relaxed(0x7A, ethqos->sgmii_base + QSERDES_COM_DEC_START_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START1_MODE0);
	writel_relaxed(0x20, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START2_MODE0);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START3_MODE0);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE1_MODE0);
	writel_relaxed(0x02, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE2_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE_INITVAL2);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_COM_HSCLK_SEL_1);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_COM_HSCLK_HS_SWITCH_SEL_1);
	writel_relaxed(0x05, ethqos->sgmii_base + QSERDES_COM_CORECLK_DIV_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_CORE_CLK_EN);
	writel_relaxed(0x16, ethqos->sgmii_base + QSERDES_COM_CMN_CONFIG_1);
	writel_relaxed(0xCD, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_CMP_CODE1_MODE0);
	writel_relaxed(0x1C, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_CMP_CODE2_MODE0);
	writel_relaxed(0x11, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_HSCLK_SEL_1);

	/******************MODULE: QSERDES_TX0_SGMII_QMP_TX***********************/
	writel_relaxed(0x0D, ethqos->sgmii_base + QSERDES_TX_CLKBUF_ENABLE);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_TX0_TX_BAND);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES_TX0_SLEW_CNTL);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_TX0_RES_CODE_LANE_OFFSET_TX);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_TX0_RES_CODE_LANE_OFFSET_RX);
	writel_relaxed(0xF5, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_1);
	writel_relaxed(0x02, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_3);
	writel_relaxed(0x3F, ethqos->sgmii_base + QSERDES_TX_LANE_MODE_4);
	writel_relaxed(0x5F, ethqos->sgmii_base + QSERDES_TX_LANE_MODE_5);
	writel_relaxed(0x12, ethqos->sgmii_base + QSERDES_TX0_RCV_DETECT_LVL_2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_TX0_TRAN_DRVR_EMP_EN);
	writel_relaxed(0x2B, ethqos->sgmii_base + QSERDES_TX_TX_EMP_POST1_LVL);

	/*****************MODULE: QSERDES_RX0_SGMII_QMP_RX*******************/
	writel_relaxed(0x0D, ethqos->sgmii_base + QSERDES_RX0_UCDR_FO_GAIN);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_RX0_UCDR_SO_GAIN);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_FO_GAIN);
	writel_relaxed(0x7F, ethqos->sgmii_base + QSERDES_RX0_UCDR_SO_SATURATION_AND_ENABLE);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_COUNT_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_COUNT_HIGH);
	writel_relaxed(0x81, ethqos->sgmii_base + QSERDES_RX0_UCDR_PI_CONTROLS);
	writel_relaxed(0x81, ethqos->sgmii_base + QSERDES_RX0_UCDR_PI_CTRL2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX_UCDR_SB2_THRESH1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX_UCDR_SB2_THRESH2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_RX_TERM_BW);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_RX0_VGA_CAL_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_RX0_GM_CAL);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL2);
	writel_relaxed(0x4A, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL3);
	writel_relaxed(0x5A, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL4);
	writel_relaxed(0x80, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_TSETTLE_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_TSETTLE_HIGH);
	writel_relaxed(0x20, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_MEASURE_TIME);
	writel_relaxed(0x17, ethqos->sgmii_base + QSERDES_RX0_RX_EQ_OFFSET_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_RX_OFFSET_ADAPTOR_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CNTRL);
	writel_relaxed(0x1E, ethqos->sgmii_base + QSERDES_RX0_SIGDET_DEGLITCH_CNTRL);
	writel_relaxed(0x18, ethqos->sgmii_base + QSERDES_RX0_RX_BAND);
	writel_relaxed(0x18, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH2);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH3);
	writel_relaxed(0xB8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH4);
	writel_relaxed(0xE0, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH2);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH3);
	writel_relaxed(0xB1, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH4);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES_RX0_DCC_CTRL1);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES_PCS_LINE_RESET_TIME);

	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_PCS_TX_SMALL_AMP_DRV_LVL);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES_PCS_TX_LARGE_AMP_DRV_LVL);
	writel_relaxed(0x0B, ethqos->sgmii_base + QSERDES_PCS_TX_LARGE_AMP_POST_EMP_LVL);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_TX_SMALL_AMP_POST_EMP_LVL);
	writel_relaxed(0x83, ethqos->sgmii_base + QSERDES_PCS_TX_MID_TERM_CTRL1);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_PCS_TX_MID_TERM_CTRL2);

	writel_relaxed(0x10, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CAL_CTRL1);
	writel_relaxed(0x77, ethqos->sgmii_base + QSERDES_PCS_RX_SIGDET_CTRL2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CAL_CTRL2_AND_CDR_LOCK_EDGE);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_SGMII_MISC_CTRL7);
	writel_relaxed(0x8c, ethqos->sgmii_base + QSERDES_PCS_SGMII_MISC_CTRL8);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_RETIME_BUFFER_EN);

	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_PHY_START);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_PHY_START);
	usleep_range(3000, 5000);
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_COM_CMN_STATUS);
		val &= BIT(1);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("%s : PLL Lock Status timedout with retry = %d\n",
			  __func__,
			  retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_COM_C_READY_STATUS);
		val &= BIT(0);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("%s : C_READY_STATUS timedout with retry = %d\n",
			  __func__,
			  retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_PCS_READY_STATUS);
		val &= BIT(7);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("%s : PCS_READY timedout with retry = %d\n",
			  __func__,
			  retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_PCS_READY_STATUS);
		val &= BIT(0);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("%s : SGMIIPHY_READY timedout with retry = %d\n",
			  __func__,
			  retry);
		ret = -1;
		goto err_ret;
	}

	return ret;

err_ret:
	return ret;
}

static int qcom_ethqos_serdes_usxgmii_2p5Gb(struct qcom_ethqos *ethqos)
{
	int ret = 0;
	int retry = 500;
	unsigned int val;

	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_POWER_DOWN_CONTROL);

	/***************** MODULE: QSERDES_COM_SGMII_QMP_PLL*********/
	writel_relaxed(0x0E, ethqos->sgmii_base + QSERDES_COM_BG_TIMER);
	writel_relaxed(0x07, ethqos->sgmii_base + QSERDES_COM_BIAS_EN_CLKBUFLR_EN);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_COM_CLK_ENABLE1);
	writel_relaxed(0x1F, ethqos->sgmii_base + QSERDES_COM_PLL_IVCO);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_COM_CP_CTRL_MODE0);
	writel_relaxed(0x16, ethqos->sgmii_base + QSERDES_COM_PLL_RCTRL_MODE0);
	writel_relaxed(0x36, ethqos->sgmii_base + QSERDES_COM_PLL_CCTRL_MODE0);
	writel_relaxed(0X1F, ethqos->sgmii_base + QSERDES_COM_INTEGLOOP_GAIN0_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_INTEGLOOP_GAIN1_MODE0);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_COM_PLL_EN);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES_COM_SYSCLK_EN_SEL);
	writel_relaxed(0x91, ethqos->sgmii_base + QSERDES_COM_LOCK_CMP1_MODE0);
	writel_relaxed(0x21, ethqos->sgmii_base + QSERDES_COM_LOCK_CMP2_MODE0);
	writel_relaxed(0x86, ethqos->sgmii_base + QSERDES_COM_DEC_START_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START1_MODE0);
	writel_relaxed(0x70, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START2_MODE0);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START3_MODE0);
	writel_relaxed(0x46, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE1_MODE0);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE2_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE_INITVAL2);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_COM_HSCLK_SEL_1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_HSCLK_HS_SWITCH_SEL_1);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_COM_CORECLK_DIV_MODE0);
	writel_relaxed(0x10, ethqos->sgmii_base + QSERDES_COM_CORE_CLK_EN);
	writel_relaxed(0x16, ethqos->sgmii_base + QSERDES_COM_CMN_CONFIG_1);
	writel_relaxed(0xAE, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_CMP_CODE1_MODE0);
	writel_relaxed(0x1F, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_CMP_CODE2_MODE0);
	writel_relaxed(0x11, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_HSCLK_SEL_1);

	/******************MODULE: QSERDES_TX0_SGMII_QMP_TX***********************/
	writel_relaxed(0x0D, ethqos->sgmii_base + QSERDES_TX_CLKBUF_ENABLE);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_TX0_TX_BAND);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_TX0_SLEW_CNTL);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_TX0_RES_CODE_LANE_OFFSET_TX);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_TX0_RES_CODE_LANE_OFFSET_RX);
	writel_relaxed(0xF5, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_1);
	writel_relaxed(0x06, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_2);
	writel_relaxed(0x3F, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_3);
	writel_relaxed(0x3F, ethqos->sgmii_base + QSERDES_TX_LANE_MODE_4);
	writel_relaxed(0x5F, ethqos->sgmii_base + QSERDES_TX_LANE_MODE_5);
	writel_relaxed(0x12, ethqos->sgmii_base + QSERDES_TX0_RCV_DETECT_LVL_2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_TX0_TRAN_DRVR_EMP_EN);
	writel_relaxed(0x2B, ethqos->sgmii_base + QSERDES_TX_TX_EMP_POST1_LVL);

	/*****************MODULE: QSERDES_RX0_SGMII_QMP_RX*******************/
	writel_relaxed(0x0D, ethqos->sgmii_base + QSERDES_RX0_UCDR_FO_GAIN);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_RX0_UCDR_SO_GAIN);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_FO_GAIN);
	writel_relaxed(0x7F, ethqos->sgmii_base + QSERDES_RX0_UCDR_SO_SATURATION_AND_ENABLE);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_COUNT_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_COUNT_HIGH);
	writel_relaxed(0x81, ethqos->sgmii_base + QSERDES_RX0_UCDR_PI_CONTROLS);
	writel_relaxed(0x80, ethqos->sgmii_base + QSERDES_RX0_UCDR_PI_CTRL2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX_UCDR_SB2_THRESH1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX_UCDR_SB2_THRESH2);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_RX0_RX_TERM_BW);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_RX0_VGA_CAL_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_RX0_GM_CAL);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL2);
	writel_relaxed(0x4A, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL3);
	writel_relaxed(0x5A, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL4);
	writel_relaxed(0x80, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_TSETTLE_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_TSETTLE_HIGH);
	writel_relaxed(0x20, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_MEASURE_TIME);
	writel_relaxed(0x17, ethqos->sgmii_base + QSERDES_RX0_RX_EQ_OFFSET_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_RX_OFFSET_ADAPTOR_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CNTRL);
	writel_relaxed(0x1E, ethqos->sgmii_base + QSERDES_RX0_SIGDET_DEGLITCH_CNTRL);
	writel_relaxed(0x18, ethqos->sgmii_base + QSERDES_RX0_RX_BAND);
	writel_relaxed(0x18, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH2);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH3);
	writel_relaxed(0xB8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH4);
	writel_relaxed(0xE0, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH2);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH3);
	writel_relaxed(0xB1, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH4);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES_RX0_DCC_CTRL1);

	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_PCS_TX_SMALL_AMP_DRV_LVL);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES_PCS_TX_LARGE_AMP_DRV_LVL);
	writel_relaxed(0x0B, ethqos->sgmii_base + QSERDES_PCS_TX_LARGE_AMP_POST_EMP_LVL);
	writel_relaxed(0x2F, ethqos->sgmii_base + QSERDES_PCS_TX_SMALL_AMP_POST_EMP_LVL);
	writel_relaxed(0x83, ethqos->sgmii_base + QSERDES_PCS_TX_MID_TERM_CTRL1);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_PCS_TX_MID_TERM_CTRL2);

	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CAL_CTRL1);
	writel_relaxed(0xCC, ethqos->sgmii_base + QSERDES_PCS_RX_SIGDET_CTRL2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CAL_CTRL2_AND_CDR_LOCK_EDGE);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_SGMII_MISC_CTRL7);
	writel_relaxed(0x14, ethqos->sgmii_base + QSERDES_PCS_SGMII_MISC_CTRL8);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_RETIME_BUFFER_EN);

	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_PHY_START);
	usleep_range(3000, 5000);

	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_COM_CMN_STATUS);
		val &= BIT(1);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("PLL Lock Status timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_COM_C_READY_STATUS);
		val &= BIT(0);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("C_READY_STATUS timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_PCS_READY_STATUS);
		val &= BIT(7);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("PCS_READY timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_PCS_READY_STATUS);
		val &= BIT(0);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("SGMIIPHY_READY timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	ETHQOSINFO("%s : Serdes is up on USXGMII 2.5 Gbps mode", __func__);
	return ret;

err_ret:
	return ret;
}

static int qcom_ethqos_serdes_usxgmii_5Gb(struct qcom_ethqos *ethqos)
{
	int ret = 0;
	int retry = 500;
	unsigned int val;

	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_POWER_DOWN_CONTROL);

	/***************** MODULE: QSERDES_COM_SGMII_QMP_PLL*********/
	writel_relaxed(0x0E, ethqos->sgmii_base + QSERDES_COM_BG_TIMER);
	writel_relaxed(0x07, ethqos->sgmii_base + QSERDES_COM_BIAS_EN_CLKBUFLR_EN);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_COM_CLK_ENABLE1);
	writel_relaxed(0x1F, ethqos->sgmii_base + QSERDES_COM_PLL_IVCO);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_COM_CP_CTRL_MODE0);
	writel_relaxed(0x16, ethqos->sgmii_base + QSERDES_COM_PLL_RCTRL_MODE0);
	writel_relaxed(0x36, ethqos->sgmii_base + QSERDES_COM_PLL_CCTRL_MODE0);
	writel_relaxed(0X1F, ethqos->sgmii_base + QSERDES_COM_INTEGLOOP_GAIN0_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_INTEGLOOP_GAIN1_MODE0);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_COM_PLL_EN);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES_COM_SYSCLK_EN_SEL);
	writel_relaxed(0x23, ethqos->sgmii_base + QSERDES_COM_LOCK_CMP1_MODE0);
	writel_relaxed(0x43, ethqos->sgmii_base + QSERDES_COM_LOCK_CMP2_MODE0);
	writel_relaxed(0x86, ethqos->sgmii_base + QSERDES_COM_DEC_START_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START1_MODE0);
	writel_relaxed(0x70, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START2_MODE0);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START3_MODE0);
	writel_relaxed(0x46, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE1_MODE0);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE2_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE_INITVAL2);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_COM_HSCLK_SEL_1);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_COM_HSCLK_HS_SWITCH_SEL_1);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_COM_CORECLK_DIV_MODE0);
	writel_relaxed(0x10, ethqos->sgmii_base + QSERDES_COM_CORE_CLK_EN);
	writel_relaxed(0x16, ethqos->sgmii_base + QSERDES_COM_CMN_CONFIG_1);
	writel_relaxed(0xAE, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_CMP_CODE1_MODE0);
	writel_relaxed(0x1F, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_CMP_CODE2_MODE0);
	writel_relaxed(0x11, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_HSCLK_SEL_1);

	/******************MODULE: QSERDES_TX0_SGMII_QMP_TX***********************/
	writel_relaxed(0x0D, ethqos->sgmii_base + QSERDES_TX_CLKBUF_ENABLE);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_TX0_TX_BAND);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_TX0_SLEW_CNTL);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_TX0_RES_CODE_LANE_OFFSET_TX);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_TX0_RES_CODE_LANE_OFFSET_RX);
	writel_relaxed(0xF5, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_1);
	writel_relaxed(0x06, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_2);
	writel_relaxed(0x3F, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_3);
	writel_relaxed(0x3F, ethqos->sgmii_base + QSERDES_TX_LANE_MODE_4);
	writel_relaxed(0x5F, ethqos->sgmii_base + QSERDES_TX_LANE_MODE_5);
	writel_relaxed(0x12, ethqos->sgmii_base + QSERDES_TX0_RCV_DETECT_LVL_2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_TX0_TRAN_DRVR_EMP_EN);
	writel_relaxed(0x2B, ethqos->sgmii_base + QSERDES_TX_TX_EMP_POST1_LVL);

	/*****************MODULE: QSERDES_RX0_SGMII_QMP_RX*******************/
	writel_relaxed(0x0D, ethqos->sgmii_base + QSERDES_RX0_UCDR_FO_GAIN);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_RX0_UCDR_SO_GAIN);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_FO_GAIN);
	writel_relaxed(0x7F, ethqos->sgmii_base + QSERDES_RX0_UCDR_SO_SATURATION_AND_ENABLE);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_COUNT_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_COUNT_HIGH);
	writel_relaxed(0x81, ethqos->sgmii_base + QSERDES_RX0_UCDR_PI_CONTROLS);
	writel_relaxed(0x81, ethqos->sgmii_base + QSERDES_RX0_UCDR_PI_CTRL2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX_UCDR_SB2_THRESH1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX_UCDR_SB2_THRESH2);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_RX0_RX_TERM_BW);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_RX0_VGA_CAL_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_RX0_GM_CAL);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL2);
	writel_relaxed(0x4A, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL3);
	writel_relaxed(0x5A, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL4);
	writel_relaxed(0x80, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_TSETTLE_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_TSETTLE_HIGH);
	writel_relaxed(0x20, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_MEASURE_TIME);
	writel_relaxed(0x17, ethqos->sgmii_base + QSERDES_RX0_RX_EQ_OFFSET_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_RX_OFFSET_ADAPTOR_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CNTRL);
	writel_relaxed(0x1E, ethqos->sgmii_base + QSERDES_RX0_SIGDET_DEGLITCH_CNTRL);
	writel_relaxed(0x18, ethqos->sgmii_base + QSERDES_RX0_RX_BAND);
	writel_relaxed(0x1F, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_LOW);
	writel_relaxed(0xBF, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH);
	writel_relaxed(0xFF, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH2);
	writel_relaxed(0xDF, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH3);
	writel_relaxed(0xED, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH4);
	writel_relaxed(0xE0, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH2);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH3);
	writel_relaxed(0xB1, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH4);
	writel_relaxed(0xE0, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_10_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_10_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_10_HIGH2);
	writel_relaxed(0x3B, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_10_HIGH3);
	writel_relaxed(0xB7, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_10_HIGH4);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES_RX0_DCC_CTRL1);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES_PCS_LINE_RESET_TIME);
	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_PCS_TX_SMALL_AMP_DRV_LVL);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES_PCS_TX_LARGE_AMP_DRV_LVL);
	writel_relaxed(0x0B, ethqos->sgmii_base + QSERDES_PCS_TX_LARGE_AMP_POST_EMP_LVL);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_TX_SMALL_AMP_POST_EMP_LVL);
	writel_relaxed(0x83, ethqos->sgmii_base + QSERDES_PCS_TX_MID_TERM_CTRL1);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_PCS_TX_MID_TERM_CTRL2);

	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CAL_CTRL1);
	writel_relaxed(0xCC, ethqos->sgmii_base + QSERDES_PCS_RX_SIGDET_CTRL2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CAL_CTRL2_AND_CDR_LOCK_EDGE);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_SGMII_MISC_CTRL7);
	writel_relaxed(0x14, ethqos->sgmii_base + QSERDES_PCS_SGMII_MISC_CTRL8);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_RETIME_BUFFER_EN);

	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_PHY_START);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_PHY_START);
	usleep_range(3000, 5000);

	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_COM_CMN_STATUS);
		val &= BIT(1);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("PLL Lock Status timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_COM_C_READY_STATUS);
		val &= BIT(0);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("C_READY_STATUS timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_PCS_READY_STATUS);
		val &= BIT(7);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("PCS_READY timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_PCS_READY_STATUS);
		val &= BIT(0);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("SGMIIPHY_READY timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	return ret;

err_ret:
	return ret;
}

static int qcom_ethqos_serdes_usxgmii_10Gb_1Gb(struct qcom_ethqos *ethqos)
{
	int ret = 0;
	int retry = 500;
	unsigned int val;

	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_POWER_DOWN_CONTROL);

	/***************** MODULE: QSERDES_COM_SGMII_QMP_PLL*********/
	writel_relaxed(0x0E, ethqos->sgmii_base + QSERDES_COM_BG_TIMER);
	writel_relaxed(0x07, ethqos->sgmii_base + QSERDES_COM_BIAS_EN_CLKBUFLR_EN);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_COM_CLK_ENABLE1);
	writel_relaxed(0x1F, ethqos->sgmii_base + QSERDES_COM_PLL_IVCO);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_COM_CP_CTRL_MODE0);
	writel_relaxed(0x16, ethqos->sgmii_base + QSERDES_COM_PLL_RCTRL_MODE0);
	writel_relaxed(0x36, ethqos->sgmii_base + QSERDES_COM_PLL_CCTRL_MODE0);
	writel_relaxed(0X1F, ethqos->sgmii_base + QSERDES_COM_INTEGLOOP_GAIN0_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_INTEGLOOP_GAIN1_MODE0);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_COM_PLL_EN);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES_COM_SYSCLK_EN_SEL);
	writel_relaxed(0x46, ethqos->sgmii_base + QSERDES_COM_LOCK_CMP1_MODE0);
	writel_relaxed(0x86, ethqos->sgmii_base + QSERDES_COM_LOCK_CMP2_MODE0);
	writel_relaxed(0x86, ethqos->sgmii_base + QSERDES_COM_DEC_START_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START1_MODE0);
	writel_relaxed(0x70, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START2_MODE0);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_COM_DIV_FRAC_START3_MODE0);
	writel_relaxed(0x46, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE1_MODE0);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE2_MODE0);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_VCO_TUNE_INITVAL2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_HSCLK_SEL_1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_COM_HSCLK_HS_SWITCH_SEL_1);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_COM_CORECLK_DIV_MODE0);
	writel_relaxed(0x10, ethqos->sgmii_base + QSERDES_COM_CORE_CLK_EN);
	writel_relaxed(0x16, ethqos->sgmii_base + QSERDES_COM_CMN_CONFIG_1);
	writel_relaxed(0xAE, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_CMP_CODE1_MODE0);
	writel_relaxed(0x1F, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_CMP_CODE2_MODE0);
	writel_relaxed(0x11, ethqos->sgmii_base + QSERDES_COM_BIN_VCOCAL_HSCLK_SEL_1);

	/******************MODULE: QSERDES_TX0_SGMII_QMP_TX***********************/
	writel_relaxed(0x0D, ethqos->sgmii_base + QSERDES_TX_CLKBUF_ENABLE);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_TX0_TX_BAND);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_TX0_SLEW_CNTL);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_TX0_RES_CODE_LANE_OFFSET_TX);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_TX0_RES_CODE_LANE_OFFSET_RX);
	writel_relaxed(0xF5, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_1);
	writel_relaxed(0x06, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_2);
	writel_relaxed(0x3F, ethqos->sgmii_base + QSERDES_TX0_LANE_MODE_3);
	writel_relaxed(0x3F, ethqos->sgmii_base + QSERDES_TX_LANE_MODE_4);
	writel_relaxed(0x5F, ethqos->sgmii_base + QSERDES_TX_LANE_MODE_5);
	writel_relaxed(0x12, ethqos->sgmii_base + QSERDES_TX0_RCV_DETECT_LVL_2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_TX0_TRAN_DRVR_EMP_EN);
	writel_relaxed(0x2B, ethqos->sgmii_base + QSERDES_TX_TX_EMP_POST1_LVL);

	/*****************MODULE: QSERDES_RX0_SGMII_QMP_RX*******************/
	writel_relaxed(0x0D, ethqos->sgmii_base + QSERDES_RX0_UCDR_FO_GAIN);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_RX0_UCDR_SO_GAIN);
	writel_relaxed(0x0A, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_FO_GAIN);
	writel_relaxed(0x7F, ethqos->sgmii_base + QSERDES_RX0_UCDR_SO_SATURATION_AND_ENABLE);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_COUNT_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_RX0_UCDR_FASTLOCK_COUNT_HIGH);
	writel_relaxed(0x81, ethqos->sgmii_base + QSERDES_RX0_UCDR_PI_CONTROLS);
	writel_relaxed(0x81, ethqos->sgmii_base + QSERDES_RX0_UCDR_PI_CTRL2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX_UCDR_SB2_THRESH1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX_UCDR_SB2_THRESH2);
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_RX0_RX_TERM_BW);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_RX0_VGA_CAL_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_RX0_GM_CAL);
	writel_relaxed(0x04, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL2);
	writel_relaxed(0x4A, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL3);
	writel_relaxed(0x5A, ethqos->sgmii_base + QSERDES_RX0_RX_EQU_ADAPTOR_CNTRL4);
	writel_relaxed(0x80, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_TSETTLE_LOW);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_TSETTLE_HIGH);
	writel_relaxed(0x20, ethqos->sgmii_base + QSERDES_RX0_RX_IDAC_MEASURE_TIME);
	writel_relaxed(0x17, ethqos->sgmii_base + QSERDES_RX0_RX_EQ_OFFSET_ADAPTOR_CNTRL1);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_RX_OFFSET_ADAPTOR_CNTRL2);
	writel_relaxed(0x0F, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CNTRL);
	writel_relaxed(0x1E, ethqos->sgmii_base + QSERDES_RX0_SIGDET_DEGLITCH_CNTRL);
	writel_relaxed(0x18, ethqos->sgmii_base + QSERDES_RX0_RX_BAND);
	writel_relaxed(0x1F, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_LOW);
	writel_relaxed(0xBF, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH);
	writel_relaxed(0xFF, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH2);
	writel_relaxed(0xDF, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH3);
	writel_relaxed(0xED, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_00_HIGH4);
	writel_relaxed(0xE0, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH2);
	writel_relaxed(0x09, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH3);
	writel_relaxed(0xB1, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_01_HIGH4);
	writel_relaxed(0xE0, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_10_LOW);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_10_HIGH);
	writel_relaxed(0xC8, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_10_HIGH2);
	writel_relaxed(0x3B, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_10_HIGH3);
	writel_relaxed(0xB7, ethqos->sgmii_base + QSERDES_RX0_RX_MODE_10_HIGH4);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES_RX0_DCC_CTRL1);
	writel_relaxed(0x0C, ethqos->sgmii_base + QSERDES_PCS_LINE_RESET_TIME);
	/****************MODULE: SGMII_PHY_SGMII_PCS**********************************/
	writel_relaxed(0x03, ethqos->sgmii_base + QSERDES_PCS_TX_SMALL_AMP_DRV_LVL);
	writel_relaxed(0x1A, ethqos->sgmii_base + QSERDES_PCS_TX_LARGE_AMP_DRV_LVL);
	writel_relaxed(0x0B, ethqos->sgmii_base + QSERDES_PCS_TX_LARGE_AMP_POST_EMP_LVL);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_TX_SMALL_AMP_POST_EMP_LVL);
	writel_relaxed(0x83, ethqos->sgmii_base + QSERDES_PCS_TX_MID_TERM_CTRL1);
	writel_relaxed(0x08, ethqos->sgmii_base + QSERDES_PCS_TX_MID_TERM_CTRL2);

	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CAL_CTRL1);
	writel_relaxed(0xCC, ethqos->sgmii_base + QSERDES_PCS_RX_SIGDET_CTRL2);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_RX0_SIGDET_CAL_CTRL2_AND_CDR_LOCK_EDGE);
	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_SGMII_MISC_CTRL7);
	writel_relaxed(0x14, ethqos->sgmii_base + QSERDES_PCS_SGMII_MISC_CTRL8);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_RETIME_BUFFER_EN);

	writel_relaxed(0x00, ethqos->sgmii_base + QSERDES_PCS_SW_RESET);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_PHY_START);
	writel_relaxed(0x01, ethqos->sgmii_base + QSERDES_PCS_PHY_START);
	usleep_range(3000, 5000);

	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_COM_CMN_STATUS);
		val &= BIT(1);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("PLL Lock Status timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_COM_C_READY_STATUS);
		val &= BIT(0);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("C_READY_STATUS timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_PCS_READY_STATUS);
		val &= BIT(7);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("PCS_READY timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	retry = 500;
	do {
		val = readl_relaxed(ethqos->sgmii_base + QSERDES_PCS_READY_STATUS);
		val &= BIT(0);
		if (val)
			break;
		usleep_range(1000, 1500);
		retry--;
	} while (retry > 0);
	if (!retry) {
		ETHQOSERR("SGMIIPHY_READY timedout with retry = %d\n", retry);
		ret = -1;
		goto err_ret;
	}

	return ret;

err_ret:
	return ret;
}

static int qcom_ethqos_serdes_update_sgmii(struct qcom_ethqos *ethqos,
					   int speed)
{
	int ret = 0;

	switch (speed) {
	case SPEED_1000:
		if (ethqos->curr_serdes_speed != SPEED_1000)
			ret = qcom_ethqos_serdes_sgmii_1Gb(ethqos);

		ethqos->curr_serdes_speed = SPEED_1000;
		break;
	case SPEED_2500:
		if (ethqos->curr_serdes_speed != SPEED_2500)
			ret = qcom_ethqos_serdes_sgmii_2p5Gb(ethqos);

		ethqos->curr_serdes_speed = SPEED_2500;
		break;
	case SPEED_100:
	case SPEED_10:
		if (ethqos->curr_serdes_speed != SPEED_1000) {
			ETHQOSINFO("%s : Serdes Speed set to 1GB speed", __func__);
			ret = qcom_ethqos_serdes_sgmii_1Gb(ethqos);
			ethqos->curr_serdes_speed = SPEED_1000;
		}
		break;
	default:
		ETHQOSERR("%s : Serdes Speed not set for speed = %d\n",
			  __func__,
			   speed);
		ethqos->curr_serdes_speed = 0;
		break;
	}

	ETHQOSINFO("%s : Exit serdes speed = %d",
		   __func__,
		   ethqos->curr_serdes_speed);
	return ret;
}

static int qcom_ethqos_serdes_update_usxgmii(struct qcom_ethqos *ethqos,
					     int speed)
{
	int ret = 0;

	switch (ethqos->usxgmii_mode) {
	case USXGMII_MODE_10G:
		if (ethqos->curr_serdes_speed != SPEED_10000)
			ret = qcom_ethqos_serdes_usxgmii_10Gb_1Gb(ethqos);

		ethqos->curr_serdes_speed = SPEED_10000;
		break;
	case USXGMII_MODE_5G:
		if (ethqos->curr_serdes_speed != SPEED_5000)
			ret = qcom_ethqos_serdes_usxgmii_5Gb(ethqos);

		ethqos->curr_serdes_speed = SPEED_5000;
		break;
	case USXGMII_MODE_2P5G:
		if (ethqos->curr_serdes_speed != SPEED_2500)
			ret = qcom_ethqos_serdes_usxgmii_2p5Gb(ethqos);

		ethqos->curr_serdes_speed = SPEED_2500;
		break;
	default:
		ETHQOSERR("%s : Serdes clock not set for USXGMII PHY speed = %d\n",
			  __func__,
			  ethqos->usxgmii_mode);
		ethqos->curr_serdes_speed = 0;
		break;
	}

	ETHQOSINFO("%s : exit ret = %d", __func__, ret);
	return ret;
}

int qcom_ethqos_serdes_update(struct qcom_ethqos *ethqos,
			      int speed,
			      int interface)
{
	int ret = 0;

#ifdef CONFIG_MSM_BOOT_TIME_MARKER
	update_marker("M -Ethernet Serdes power up start");
#endif

	switch (interface) {
	case PHY_INTERFACE_MODE_SGMII:
	case PHY_INTERFACE_MODE_2500BASEX:
		ret = qcom_ethqos_serdes_update_sgmii(ethqos, speed);
		break;
	case PHY_INTERFACE_MODE_5GBASER:
	case PHY_INTERFACE_MODE_USXGMII:
		if (interface == PHY_INTERFACE_MODE_USXGMII)
			ethqos->usxgmii_mode = USXGMII_MODE_10G;
		else if (interface == PHY_INTERFACE_MODE_5GBASER)
			ethqos->usxgmii_mode = USXGMII_MODE_5G;

		ret = qcom_ethqos_serdes_update_usxgmii(ethqos, speed);
		break;
	default:
		ETHQOSINFO("%s : PHY interface not supported", __func__);
		ret = EINVAL;
		break;
	}
#ifdef CONFIG_MSM_BOOT_TIME_MARKER
	update_marker("M - Ethernet Serdes power up end");
#endif

	return ret;
}
EXPORT_SYMBOL(qcom_ethqos_serdes_update);

int qcom_ethqos_serdes_configure_dt(struct qcom_ethqos *ethqos, int interface)
{
	struct resource *res = NULL;
	struct platform_device *pdev = ethqos->pdev;
	int ret;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "serdes");
	ethqos->sgmii_base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ethqos->sgmii_base)) {
		dev_err(&pdev->dev, "Can't get sgmii base\n");
		ret = PTR_ERR(ethqos->sgmii_base);
		goto err_mem;
	}

	ethqos->sgmiref_clk = devm_clk_get(&pdev->dev, "sgmi_ref");
	if (IS_ERR(ethqos->sgmiref_clk)) {
		ETHQOSERR("%s : configure_serdes_dt Failed sgmi_ref", __func__);
		ret = PTR_ERR(ethqos->sgmiref_clk);
		goto err_mem;
	}
	ethqos->phyaux_clk = devm_clk_get(&pdev->dev, "phyaux");
	if (IS_ERR(ethqos->phyaux_clk)) {
		ETHQOSERR("%s : configure_serdes_dt Failed phyaux", __func__);
		ret = PTR_ERR(ethqos->phyaux_clk);
		goto err_mem;
	}

	ret = qcom_ethqos_enable_serdes_clocks(ethqos);
	if (ret)
		goto err_mem;

	return 0;

err_mem:
	ETHQOSERR("%s : configure_serdes_dt Failed", __func__);
	return -1;
}
EXPORT_SYMBOL(qcom_ethqos_serdes_configure_dt);
