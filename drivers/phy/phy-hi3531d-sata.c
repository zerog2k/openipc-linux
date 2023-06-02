/*
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/delay.h>
#include <linux/io.h>
#include <linux/mfd/syscon.h>
#include <linux/module.h>
#include <linux/phy/phy.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <mach/io.h>
#include <mach/platform.h>

static unsigned int mplx_port0;
static unsigned int sata_port_nr;

enum {
	HI_SATA_PERI_CTRL		= IO_ADDRESS(0x12040000),
	HI_SATA_PERI_CRG72		= (HI_SATA_PERI_CTRL + 0x120),
	HI_SATA_PERI_CRG74		= (HI_SATA_PERI_CTRL + 0x128),

	HI_SATA_PHY0_REFCLK_SEL_MASK = (0x3 << 4),
	HI_SATA_PHY0_REFCLK_SEL = (0x1 << 4),
	HI_SATA_PHY1_REFCLK_SEL_MASK = (0x3 << 6),
	HI_SATA_PHY1_REFCLK_SEL = (0x1 << 6),
	HI_SATA_PHY2_REFCLK_SEL_MASK = (0x3 << 12),
	HI_SATA_PHY2_REFCLK_SEL = (0x1 << 12),
	HI_SATA_PHY3_REFCLK_SEL_MASK = (0x3 << 14),
	HI_SATA_PHY3_REFCLK_SEL = (0x1 << 14),

	HI_SATA_PHY0_CLK_EN		= (1 << 0),
	HI_SATA_PHY1_CLK_EN		= (1 << 1),
	HI_SATA_PHY2_CLK_EN		= (1 << 8),
	HI_SATA_PHY3_CLK_EN		= (1 << 9),

	HI_SATA_PHY0_RST		= (1 << 2),
	HI_SATA_PHY1_RST		= (1 << 3),
	HI_SATA_PHY2_RST		= (1 << 10),
	HI_SATA_PHY3_RST		= (1 << 11),

	HI_SATA_PHY3_RST_BACK_MASK	= (1 << 7),
	HI_SATA_PHY2_RST_BACK_MASK	= (1 << 6),
	HI_SATA_PHY1_RST_BACK_MASK	= (1 << 5),
	HI_SATA_PHY0_RST_BACK_MASK	= (1 << 4),

	HI_SATA_BUS_CKEN		= (1 << 0),
	HI_SATA_BUS_SRST_REQ	= (1 << 8),
	HI_SATA_CKO_ALIVE_CKEN	= (1 << 2),
	HI_SATA_CKO_ALIVE_SRST_REQ	= (1 << 9),
	HI_SATA_RX0_CKEN		= (1 << 1),
	HI_SATA_TX0_CKEN		= (1 << 3),
	HI_SATA_RX0_SRST_REQ	= (1 << 10),
	HI_SATA0_SRST_REQ		= (1 << 11),
	HI_SATA_RX1_CKEN		= (1 << 12),
	HI_SATA_TX1_CKEN		= (1 << 13),
	HI_SATA_RX1_SRST_REQ	= (1 << 14),
	HI_SATA1_SRST_REQ		= (1 << 15),
	HI_SATA_RX2_CKEN        = (1 << 16),
	HI_SATA_TX2_CKEN        = (1 << 17),
	HI_SATA_RX2_SRST_REQ    = (1 << 18),
	HI_SATA2_SRST_REQ       = (1 << 19),
	HI_SATA_RX3_CKEN        = (1 << 20),
	HI_SATA_TX3_CKEN        = (1 << 21),
	HI_SATA_RX3_SRST_REQ    = (1 << 22),
	HI_SATA3_SRST_REQ       = (1 << 23),

	HI_SATA_SYS_CTRL		= IO_ADDRESS(0x1205008C),
	HI_SATA_PCIE_MODE		= 12,
};


static unsigned int hi_sata_port_nr(void)
{
	unsigned int val, mode, port_nr;

	val = readl((void *)HI_SATA_SYS_CTRL);

	mode = (val >> HI_SATA_PCIE_MODE) & 0xf;
	switch (mode) {
	case 0x0:
		port_nr = 4;
		sata_port_map = 0xf;
		break;

	case 0x1:
		port_nr = 3;
		sata_port_map = 0x7;
		break;

	case 0x3:
		port_nr = 2;
		sata_port_map = 0x3;
		break;

	case 0x8:
		port_nr = 3;
		sata_port_map = 0xe;
		break;

	case 0x9:
		port_nr = 2;
		sata_port_map = 0x6;
		break;

	case 0xb:
		port_nr = 1;
		sata_port_map = 0x2;
		break;

	default:
		port_nr = 0;
		break;
	}

	mplx_port0 = (mode & 0x8) ? 1 : 0;
	sata_port_nr = port_nr;

	return port_nr;
}

static void hi_sata_poweron(void)
{
	/* msleep(20); */
}

static void hi_sata_poweroff(void)
{
}

void hisi_sata_reset_rxtx_assert(unsigned int port_no)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HI_SATA_PERI_CRG74);

	if (port_no == 0)
		tmp_val |= HI_SATA_RX0_SRST_REQ
			| HI_SATA0_SRST_REQ;
	else if (port_no == 1)
		tmp_val |= HI_SATA_RX1_SRST_REQ
			| HI_SATA1_SRST_REQ;
	else if (port_no == 2)
		tmp_val |= HI_SATA_RX2_SRST_REQ
			| HI_SATA2_SRST_REQ;
	else if (port_no == 3)
		tmp_val |= HI_SATA_RX3_SRST_REQ
			| HI_SATA3_SRST_REQ;

	writel(tmp_val, (void *)HI_SATA_PERI_CRG74);
}
EXPORT_SYMBOL(hisi_sata_reset_rxtx_assert);

void hisi_sata_reset_rxtx_deassert(unsigned int port_no)
{
	unsigned int tmp_val;

	tmp_val = readl((void *)HI_SATA_PERI_CRG74);

	if (port_no == 0)
		tmp_val &= ~(HI_SATA_RX0_SRST_REQ
			| HI_SATA0_SRST_REQ);
	else if (port_no == 1)
		tmp_val &= ~(HI_SATA_RX1_SRST_REQ
			| HI_SATA1_SRST_REQ);
	else if (port_no == 2)
		tmp_val &= ~(HI_SATA_RX2_SRST_REQ
			| HI_SATA2_SRST_REQ);
	else if (port_no == 3)
		tmp_val &= ~(HI_SATA_RX3_SRST_REQ
			| HI_SATA3_SRST_REQ);

	writel(tmp_val, (void *)HI_SATA_PERI_CRG74);
}
EXPORT_SYMBOL(hisi_sata_reset_rxtx_deassert);

static void hi_sata_reset(void)
{
	unsigned int tmp_val, nport;

	nport = sata_port_nr;

	tmp_val = readl((void *)HI_SATA_PERI_CRG74);

	tmp_val |= HI_SATA_BUS_SRST_REQ | HI_SATA_CKO_ALIVE_SRST_REQ;

	if (nport == 4) {
		tmp_val |= HI_SATA_RX0_SRST_REQ
				| HI_SATA0_SRST_REQ
				| HI_SATA_RX1_SRST_REQ
				| HI_SATA1_SRST_REQ
				| HI_SATA_RX2_SRST_REQ
				| HI_SATA2_SRST_REQ
				| HI_SATA_RX3_SRST_REQ
				| HI_SATA3_SRST_REQ;
	} else if (nport == 3) {
		if (mplx_port0) {
			tmp_val |= HI_SATA_RX1_SRST_REQ
					| HI_SATA1_SRST_REQ
					| HI_SATA_RX2_SRST_REQ
					| HI_SATA2_SRST_REQ
					| HI_SATA_RX3_SRST_REQ
					| HI_SATA3_SRST_REQ;
		} else {
			tmp_val |= HI_SATA_RX0_SRST_REQ
					| HI_SATA0_SRST_REQ
					| HI_SATA_RX1_SRST_REQ
					| HI_SATA1_SRST_REQ
					| HI_SATA_RX2_SRST_REQ
					| HI_SATA2_SRST_REQ;
		}
	} else if (nport == 2) {
		if (mplx_port0) {
			tmp_val |= HI_SATA_RX1_SRST_REQ
					| HI_SATA1_SRST_REQ
					| HI_SATA_RX2_SRST_REQ
					| HI_SATA2_SRST_REQ;
		} else {
			tmp_val |= HI_SATA_RX0_SRST_REQ
					| HI_SATA0_SRST_REQ
					| HI_SATA_RX1_SRST_REQ
					| HI_SATA1_SRST_REQ;
		}
	} else if (nport == 1) {
			tmp_val |= HI_SATA_RX1_SRST_REQ
					| HI_SATA1_SRST_REQ;
	}

	writel(tmp_val, (void *)HI_SATA_PERI_CRG74);
}

static void hi_sata_unreset(void)
{
	unsigned int tmp_val, nport;

	nport = sata_port_nr;

	tmp_val = readl((void *)HI_SATA_PERI_CRG74);

	tmp_val &= ~(HI_SATA_BUS_SRST_REQ | HI_SATA_CKO_ALIVE_SRST_REQ);

	if (nport == 4) {
		tmp_val &= ~(HI_SATA_RX0_SRST_REQ
				| HI_SATA0_SRST_REQ
				| HI_SATA_RX1_SRST_REQ
				| HI_SATA1_SRST_REQ
				| HI_SATA_RX2_SRST_REQ
				| HI_SATA2_SRST_REQ
				| HI_SATA_RX3_SRST_REQ
				| HI_SATA3_SRST_REQ);
	} else if (nport == 3) {
		if (mplx_port0) {
			tmp_val &= ~(HI_SATA_RX1_SRST_REQ
					| HI_SATA1_SRST_REQ
					| HI_SATA_RX2_SRST_REQ
					| HI_SATA2_SRST_REQ
					| HI_SATA_RX3_SRST_REQ
					| HI_SATA3_SRST_REQ);
		} else {
			tmp_val &= ~(HI_SATA_RX0_SRST_REQ
					| HI_SATA0_SRST_REQ
					| HI_SATA_RX1_SRST_REQ
					| HI_SATA1_SRST_REQ
					| HI_SATA_RX2_SRST_REQ
					| HI_SATA2_SRST_REQ);
		}
	} else if (nport == 2) {
		if (mplx_port0) {
			tmp_val &= ~(HI_SATA_RX1_SRST_REQ
					| HI_SATA1_SRST_REQ
					| HI_SATA_RX2_SRST_REQ
					| HI_SATA2_SRST_REQ);
		} else {
			tmp_val &= ~(HI_SATA_RX0_SRST_REQ
					| HI_SATA0_SRST_REQ
					| HI_SATA_RX1_SRST_REQ
					| HI_SATA1_SRST_REQ);
		}
	} else if (nport == 1) {
			tmp_val &= ~(HI_SATA_RX1_SRST_REQ
					| HI_SATA1_SRST_REQ);
	}

	writel(tmp_val, (void *)HI_SATA_PERI_CRG74);
}

static void hi_sata_phy_reset(void)
{
	unsigned int tmp_val, nport;

	tmp_val = readl((void *)HI_SATA_PERI_CRG72);

	nport = sata_port_nr;

	if (nport == 4) {
		tmp_val |= HI_SATA_PHY0_RST
				| HI_SATA_PHY1_RST
				| HI_SATA_PHY2_RST
				| HI_SATA_PHY3_RST;
	} else if (nport == 3) {
		if (mplx_port0) {
			tmp_val |= HI_SATA_PHY1_RST
					| HI_SATA_PHY2_RST
					| HI_SATA_PHY3_RST;
		} else {
			tmp_val |= HI_SATA_PHY0_RST
					| HI_SATA_PHY1_RST
					| HI_SATA_PHY2_RST;
		}
	} else if (nport == 2) {
		if (mplx_port0) {
			tmp_val |= HI_SATA_PHY1_RST
					| HI_SATA_PHY2_RST;
		} else {
			tmp_val |= HI_SATA_PHY0_RST
					| HI_SATA_PHY1_RST;
		}
	} else if (nport == 1) {
			tmp_val |= HI_SATA_PHY1_RST;
	}

	writel(tmp_val, (void *)HI_SATA_PERI_CRG72);
}

static void hi_sata_phy_unreset(void)
{
	unsigned int tmp_val, nport;

	tmp_val = readl((void *)HI_SATA_PERI_CRG72);

	nport = sata_port_nr;

	if (nport == 4) {
		tmp_val &= ~(HI_SATA_PHY0_RST
				| HI_SATA_PHY1_RST
				| HI_SATA_PHY2_RST
				| HI_SATA_PHY3_RST);
	} else if (nport == 3) {
		if (mplx_port0) {
			tmp_val &= ~(HI_SATA_PHY1_RST
					| HI_SATA_PHY2_RST
					| HI_SATA_PHY3_RST);
		} else {
			tmp_val &= ~(HI_SATA_PHY0_RST
					| HI_SATA_PHY1_RST
					| HI_SATA_PHY2_RST);
		}
	} else if (nport == 2) {
		if (mplx_port0) {
			tmp_val &= ~(HI_SATA_PHY1_RST
					| HI_SATA_PHY2_RST);
		} else {
			tmp_val &= ~(HI_SATA_PHY0_RST
					| HI_SATA_PHY1_RST);
		}
	} else if (nport == 1) {
			tmp_val &= ~HI_SATA_PHY1_RST;
	}

	writel(tmp_val, (void *)HI_SATA_PERI_CRG72);
}

static void hi_sata_clk_enable(void)
{
	unsigned int tmp_val, tmp_reg, nport;

	nport = sata_port_nr;

	tmp_val = readl((void *)HI_SATA_PERI_CRG72);
	tmp_reg = readl((void *)HI_SATA_PERI_CRG74);

	tmp_reg |= HI_SATA_BUS_CKEN
			| HI_SATA_CKO_ALIVE_CKEN;

	if (nport == 4) {
		tmp_val |= HI_SATA_PHY0_CLK_EN;
		tmp_val |= HI_SATA_PHY1_CLK_EN;
		tmp_val |= HI_SATA_PHY2_CLK_EN;
		tmp_val |= HI_SATA_PHY3_CLK_EN;

		tmp_reg |= HI_SATA_RX0_CKEN
				| HI_SATA_TX0_CKEN
				| HI_SATA_RX1_CKEN
				| HI_SATA_TX1_CKEN
				| HI_SATA_RX2_CKEN
				| HI_SATA_TX2_CKEN
				| HI_SATA_RX3_CKEN
				| HI_SATA_TX3_CKEN;

	} else if (nport == 3) {
		if (mplx_port0) {
			tmp_val |= HI_SATA_PHY1_CLK_EN;
			tmp_val |= HI_SATA_PHY2_CLK_EN;
			tmp_val |= HI_SATA_PHY3_CLK_EN;

			tmp_reg |= HI_SATA_RX1_CKEN
					| HI_SATA_TX1_CKEN
					| HI_SATA_RX2_CKEN
					| HI_SATA_TX2_CKEN
					| HI_SATA_RX3_CKEN
					| HI_SATA_TX3_CKEN;
		} else {
			tmp_val |= HI_SATA_PHY0_CLK_EN;
			tmp_val |= HI_SATA_PHY1_CLK_EN;
			tmp_val |= HI_SATA_PHY2_CLK_EN;

			tmp_reg |= HI_SATA_RX0_CKEN
					| HI_SATA_TX0_CKEN
					| HI_SATA_RX1_CKEN
					| HI_SATA_TX1_CKEN
					| HI_SATA_RX2_CKEN
					| HI_SATA_TX2_CKEN;
		}
	} else if (nport == 2) {
		if (mplx_port0) {
			tmp_val |= HI_SATA_PHY1_CLK_EN;
			tmp_val |= HI_SATA_PHY2_CLK_EN;

			tmp_reg |= HI_SATA_RX1_CKEN
					| HI_SATA_TX1_CKEN
					| HI_SATA_RX2_CKEN
					| HI_SATA_TX2_CKEN;
		} else {
			tmp_val |= HI_SATA_PHY0_CLK_EN;
			tmp_val |= HI_SATA_PHY1_CLK_EN;

			tmp_reg |= HI_SATA_RX0_CKEN
					| HI_SATA_TX0_CKEN
					| HI_SATA_RX1_CKEN
					| HI_SATA_TX1_CKEN;
		}
	} else if (nport == 1) {
		tmp_val |= HI_SATA_PHY1_CLK_EN;

		tmp_reg |= HI_SATA_RX1_CKEN
				| HI_SATA_TX1_CKEN;
	} else
		return;

	writel(tmp_val, (void *)HI_SATA_PERI_CRG72);
	writel(tmp_reg, (void *)HI_SATA_PERI_CRG74);

}
static void hi_sata_clk_disable(void)
{
}

static void hi_sata_clk_reset(void)
{
}

static void hi_sata_phy_clk_sel(void)
{
	unsigned int tmp_val, nport;

	nport = sata_port_nr;

	tmp_val = readl((void *)HI_SATA_PERI_CRG72);

	if (nport == 4) {
		tmp_val &= ~HI_SATA_PHY0_REFCLK_SEL_MASK;
		tmp_val &= ~HI_SATA_PHY1_REFCLK_SEL_MASK;
		tmp_val &= ~HI_SATA_PHY2_REFCLK_SEL_MASK;
		tmp_val &= ~HI_SATA_PHY3_REFCLK_SEL_MASK;

		tmp_val |= HI_SATA_PHY0_REFCLK_SEL;
		tmp_val |= HI_SATA_PHY1_REFCLK_SEL;
		tmp_val |= HI_SATA_PHY2_REFCLK_SEL;
		tmp_val |= HI_SATA_PHY3_REFCLK_SEL;
	} else if (nport == 3) {
		if (mplx_port0) {
			tmp_val &= ~HI_SATA_PHY1_REFCLK_SEL_MASK;
			tmp_val &= ~HI_SATA_PHY2_REFCLK_SEL_MASK;
			tmp_val &= ~HI_SATA_PHY3_REFCLK_SEL_MASK;

			tmp_val |= HI_SATA_PHY1_REFCLK_SEL;
			tmp_val |= HI_SATA_PHY2_REFCLK_SEL;
			tmp_val |= HI_SATA_PHY3_REFCLK_SEL;
		} else {
			tmp_val &= ~HI_SATA_PHY0_REFCLK_SEL_MASK;
			tmp_val &= ~HI_SATA_PHY1_REFCLK_SEL_MASK;
			tmp_val &= ~HI_SATA_PHY2_REFCLK_SEL_MASK;

			tmp_val |= HI_SATA_PHY0_REFCLK_SEL;
			tmp_val |= HI_SATA_PHY1_REFCLK_SEL;
			tmp_val |= HI_SATA_PHY2_REFCLK_SEL;
		}
	} else if (nport == 2) {
		if (mplx_port0) {
			tmp_val &= ~HI_SATA_PHY1_REFCLK_SEL_MASK;
			tmp_val &= ~HI_SATA_PHY2_REFCLK_SEL_MASK;

			tmp_val |= HI_SATA_PHY1_REFCLK_SEL;
			tmp_val |= HI_SATA_PHY2_REFCLK_SEL;
		} else {
			tmp_val &= ~HI_SATA_PHY0_REFCLK_SEL_MASK;
			tmp_val &= ~HI_SATA_PHY1_REFCLK_SEL_MASK;

			tmp_val |= HI_SATA_PHY0_REFCLK_SEL;
			tmp_val |= HI_SATA_PHY1_REFCLK_SEL;

		}
	} else if (nport == 1) {
		tmp_val &= ~HI_SATA_PHY1_REFCLK_SEL_MASK;
		tmp_val |= HI_SATA_PHY1_REFCLK_SEL;
	} else
		return;

	writel(tmp_val, (void *)HI_SATA_PERI_CRG72);
}

void hisata_v200_set_fifo(void *mmio, int n_ports)
{
	int i, port_no;

	for (i = 0; i < n_ports; i++) {
		port_no = i;
		if (mplx_port0)
			port_no++;

		writel(HI_SATA_FIFOTH_VALUE, (mmio + 0x100 + port_no*0x80
					+ HI_SATA_PORT_FIFOTH));
	}
}

void hisata_phy_init(void *mmio, int phy_mode, int n_ports)
{
	unsigned int tmp, phy_config = HI_SATA_PHY_3G;
	unsigned int phy_sg = HI_SATA_PHY_SG_3G;
	int i, port_no;

	hisata_v200_set_fifo(mmio, n_ports);

	tmp = readl(mmio + HI_SATA_PHY_CTL1);
	tmp |= HI_SATA_BIGENDINE;
	writel(tmp, (mmio + HI_SATA_PHY_CTL1));
	tmp = readl(mmio + HI_SATA_PHY_CTL2);
	tmp |= HI_SATA_BIGENDINE;
	writel(tmp, (mmio + HI_SATA_PHY_CTL2));

	tmp = readl(mmio + HI_SATA_PHY_RST_BACK_MASK);
	tmp &= 0xffffff0f;
	if (n_ports == 1) {
		tmp |= HI_SATA_PHY0_RST_BACK_MASK
			| HI_SATA_PHY2_RST_BACK_MASK
			| HI_SATA_PHY3_RST_BACK_MASK;
	} else if (n_ports == 2) {
		if (mplx_port0) {
			tmp |= HI_SATA_PHY0_RST_BACK_MASK
				| HI_SATA_PHY3_RST_BACK_MASK;
		} else {
			tmp |= HI_SATA_PHY2_RST_BACK_MASK
				| HI_SATA_PHY3_RST_BACK_MASK;
		}
	} else if (n_ports == 3) {
		if (mplx_port0)
			tmp |= HI_SATA_PHY0_RST_BACK_MASK;
		else
			tmp |= HI_SATA_PHY3_RST_BACK_MASK;
	} else if (n_ports == 4) {
		/* Not need mask any port */
	}
	writel(tmp, (mmio + HI_SATA_PHY_RST_BACK_MASK));

	if (phy_mode == HI_SATA_PHY_MODE_1_5G) {
		phy_config = HI_SATA_PHY_1_5G;
		phy_sg = HI_SATA_PHY_SG_1_5G;
	}

	if (phy_mode == HI_SATA_PHY_MODE_3G) {
		phy_config = HI_SATA_PHY_3G;
		phy_sg = HI_SATA_PHY_SG_3G;
	}

	if (phy_mode == HI_SATA_PHY_MODE_6G) {
		phy_config = HI_SATA_PHY_6G;
		phy_sg = HI_SATA_PHY_SG_6G;
	}

	for (i = 0; i < n_ports; i++) {
		port_no = i;
		if (mplx_port0)
			port_no++;

		writel(phy_config, (mmio + 0x100 + port_no*0x80
					+ HI_SATA_PORT_PHYCTL));

		writel(phy_sg, (mmio + 0x100 + port_no*0x80
					+ HI_SATA_PORT_PHYCTL1));
	}
}

static void hi_sata_phy_reg_config(void)
{
	unsigned int i, port_no;

	for (i = 0; i < sata_port_nr; i++) {
		port_no = i;
		if (mplx_port0)
			port_no++;

		if (port_no == 0) {
			/* PLL always 6G & CDR <= RATE */
			writel(0xd01, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0xd41, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0xd01, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);

			/* disable SSC */
			writel(0x803, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x843, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x803, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);

			/* EQ set 6'b010000 */
			writel(0x009, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x049, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x009, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);

			writel(0x508, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x548, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x508, (void *)HI_SATA_MISC_COMB_PHY0);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);
		} else if (port_no == 1) {
			/* PLL always 6G & CDR <= RATE */
			writel(0xd01, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0xd41, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0xd01, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);

			/* disable SSC */
			writel(0x803, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x843, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x803, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);

			/* EQ set 6'b010000 */
			writel(0x009, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x049, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x009, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);

			writel(0x508, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x548, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x508, (void *)HI_SATA_MISC_COMB_PHY1);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);
		} else if (port_no == 2) {
			/* PLL always 6G & CDR <= RATE */
			writel(0xd01, (void *)HI_SATA_MISC_COMB_PHY2);
			writel(0xd41, (void *)HI_SATA_MISC_COMB_PHY2);
			writel(0xd01, (void *)HI_SATA_MISC_COMB_PHY2);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY2);

			/* disable SSC */
			writel(0x803, (void *)HI_SATA_MISC_COMB_PHY2);
			writel(0x843, (void *)HI_SATA_MISC_COMB_PHY2);
			writel(0x803, (void *)HI_SATA_MISC_COMB_PHY2);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY2);

			/* EQ set 6'b010000 */
			writel(0x009, (void *)HI_SATA_MISC_COMB_PHY2);
			writel(0x049, (void *)HI_SATA_MISC_COMB_PHY2);
			writel(0x009, (void *)HI_SATA_MISC_COMB_PHY2);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY2);

			writel(0x508, (void *)HI_SATA_MISC_COMB_PHY2);
			writel(0x548, (void *)HI_SATA_MISC_COMB_PHY2);
			writel(0x508, (void *)HI_SATA_MISC_COMB_PHY2);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY2);
		} else if (port_no == 3) {
			/* PLL always 6G & CDR <= RATE */
			writel(0xd01, (void *)HI_SATA_MISC_COMB_PHY3);
			writel(0xd41, (void *)HI_SATA_MISC_COMB_PHY3);
			writel(0xd01, (void *)HI_SATA_MISC_COMB_PHY3);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY3);

			/* disable SSC */
			writel(0x803, (void *)HI_SATA_MISC_COMB_PHY3);
			writel(0x843, (void *)HI_SATA_MISC_COMB_PHY3);
			writel(0x803, (void *)HI_SATA_MISC_COMB_PHY3);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY3);

			/* EQ set 6'b010000 */
			writel(0x009, (void *)HI_SATA_MISC_COMB_PHY3);
			writel(0x049, (void *)HI_SATA_MISC_COMB_PHY3);
			writel(0x009, (void *)HI_SATA_MISC_COMB_PHY3);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY3);

			writel(0x508, (void *)HI_SATA_MISC_COMB_PHY3);
			writel(0x548, (void *)HI_SATA_MISC_COMB_PHY3);
			writel(0x508, (void *)HI_SATA_MISC_COMB_PHY3);
			writel(0x0, (void *)HI_SATA_MISC_COMB_PHY3);
		}
	}
}

void hi_sata_eq_recovery(unsigned int port_no)
{
	if (port_no == 0) {
		/* auto_eq */
		writel(0xf09, (void *)HI_SATA_MISC_COMB_PHY0);
		writel(0xf49, (void *)HI_SATA_MISC_COMB_PHY0);
		writel(0xf09, (void *)HI_SATA_MISC_COMB_PHY0);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);

		writel(0x308, (void *)HI_SATA_MISC_COMB_PHY0);
		writel(0x348, (void *)HI_SATA_MISC_COMB_PHY0);
		writel(0x308, (void *)HI_SATA_MISC_COMB_PHY0);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);
	} else if (port_no == 1) {
		/* auto_eq */
		writel(0xf09, (void *)HI_SATA_MISC_COMB_PHY1);
		writel(0xf49, (void *)HI_SATA_MISC_COMB_PHY1);
		writel(0xf09, (void *)HI_SATA_MISC_COMB_PHY1);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);

		writel(0x308, (void *)HI_SATA_MISC_COMB_PHY1);
		writel(0x348, (void *)HI_SATA_MISC_COMB_PHY1);
		writel(0x308, (void *)HI_SATA_MISC_COMB_PHY1);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);
	} else if (port_no == 2) {
		/* auto_eq */
		writel(0xf09, (void *)HI_SATA_MISC_COMB_PHY2);
		writel(0xf49, (void *)HI_SATA_MISC_COMB_PHY2);
		writel(0xf09, (void *)HI_SATA_MISC_COMB_PHY2);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY2);

		writel(0x308, (void *)HI_SATA_MISC_COMB_PHY2);
		writel(0x348, (void *)HI_SATA_MISC_COMB_PHY2);
		writel(0x308, (void *)HI_SATA_MISC_COMB_PHY2);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY2);
	} else if (port_no == 3) {
		/* auto_eq */
		writel(0xf09, (void *)HI_SATA_MISC_COMB_PHY3);
		writel(0xf49, (void *)HI_SATA_MISC_COMB_PHY3);
		writel(0xf09, (void *)HI_SATA_MISC_COMB_PHY3);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY3);

		writel(0x308, (void *)HI_SATA_MISC_COMB_PHY3);
		writel(0x348, (void *)HI_SATA_MISC_COMB_PHY3);
		writel(0x308, (void *)HI_SATA_MISC_COMB_PHY3);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY3);
	}

	return;
}
EXPORT_SYMBOL(hi_sata_eq_recovery);

void hi_sata_set_eq(unsigned int port_no)
{
	if (port_no == 0) {
		/* EQ set 6'b010000 */
		writel(0x009, (void *)HI_SATA_MISC_COMB_PHY0);
		writel(0x049, (void *)HI_SATA_MISC_COMB_PHY0);
		writel(0x009, (void *)HI_SATA_MISC_COMB_PHY0);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);

		writel(0x508, (void *)HI_SATA_MISC_COMB_PHY0);
		writel(0x548, (void *)HI_SATA_MISC_COMB_PHY0);
		writel(0x508, (void *)HI_SATA_MISC_COMB_PHY0);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY0);
	} else if (port_no == 1) {
		/* EQ set 6'b010000 */
		writel(0x009, (void *)HI_SATA_MISC_COMB_PHY1);
		writel(0x049, (void *)HI_SATA_MISC_COMB_PHY1);
		writel(0x009, (void *)HI_SATA_MISC_COMB_PHY1);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);

		writel(0x508, (void *)HI_SATA_MISC_COMB_PHY1);
		writel(0x548, (void *)HI_SATA_MISC_COMB_PHY1);
		writel(0x508, (void *)HI_SATA_MISC_COMB_PHY1);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY1);
	} else if (port_no == 2) {
		/* EQ set 6'b010000 */
		writel(0x009, (void *)HI_SATA_MISC_COMB_PHY2);
		writel(0x049, (void *)HI_SATA_MISC_COMB_PHY2);
		writel(0x009, (void *)HI_SATA_MISC_COMB_PHY2);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY2);

		writel(0x508, (void *)HI_SATA_MISC_COMB_PHY2);
		writel(0x548, (void *)HI_SATA_MISC_COMB_PHY2);
		writel(0x508, (void *)HI_SATA_MISC_COMB_PHY2);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY2);
	} else if (port_no == 3) {
		/* EQ set 6'b010000 */
		writel(0x009, (void *)HI_SATA_MISC_COMB_PHY3);
		writel(0x049, (void *)HI_SATA_MISC_COMB_PHY3);
		writel(0x009, (void *)HI_SATA_MISC_COMB_PHY3);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY3);

		writel(0x508, (void *)HI_SATA_MISC_COMB_PHY3);
		writel(0x548, (void *)HI_SATA_MISC_COMB_PHY3);
		writel(0x508, (void *)HI_SATA_MISC_COMB_PHY3);
		writel(0x0, (void *)HI_SATA_MISC_COMB_PHY3);
	}

	return;
}
EXPORT_SYMBOL(hi_sata_set_eq);
