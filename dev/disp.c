// SPDX-License-Identifier: BSD-2-Clause
// Copyright (c) 2021 Amol Surati

#include <lib/assert.h>
#include <lib/string.h>

#include <sys/err.h>
#include <sys/cpu.h>
#include <sys/vmm.h>

#include <dev/dev.h>
#include <dev/con.h>
#include <dev/cm.h>
#include <dev/pv.h>
#include <dev/hdmi.h>
#include <dev/hvs.h>
#include <dev/ddc.h>

// 642x480 71hz
// The PLLH is set to 600MHz, and PLLH_PIX divctl is set to 2. Thus, the
// pixel clock rate is (600/10)/2 = 30 MHz. The refresh rate then is
// (30 * 10^6)/(800 * 525) = 71.42 Hz

volatile uint32_t *g_cm_regs;
volatile uint32_t *g_pv1_regs;
volatile uint32_t *g_pv2_regs;
volatile uint32_t *g_hvs_regs;
volatile uint32_t *g_txp_regs;
volatile uint32_t *g_hdmi_regs;
volatile uint32_t *g_hd_regs;
volatile uint32_t *g_ddc_regs;

// Display Mode
struct mode {
	uint32_t			hap;	// Active Pixels
	uint32_t			hfp;	// Front Porch
	uint32_t			hsw;	// HSync Width
	uint32_t			hbp;	// Back Porch
	uint32_t			val;	// Active Lines
	uint32_t			vfp;	// Front Porch
	uint32_t			vsw;	// VSync Width
	uint32_t			vbp;	// Back Porch

	int				hsp;	// HSync Polarity
	int				vsp;	// VSync Polarity
	int				vic;	// CEA Video ID Code.

	int				pllh_ndiv;	// Integer divisor.
	int				pllh_fdiv;	// Fractional divisor.
	int				pllh_pix_div;	// Integer divisor.
};

static int g_mode = 2;
struct mode g_disp_modes[] = {
	{640, 16, 96, 48, 480, 10, 2, 33, -1, -1, 1, 0x1a, 0x40000, 0x2},
	{800, 40, 128, 88, 600, 1, 4, 23, 1, 1, 0, 0x14, 0xd5556, 0x1},
	{1920, 88, 44, 148, 1080, 4, 5, 36, 1, 1, 16, 0x4d, 0x58000, 0x1},
};

#define HAP				(g_disp_modes[g_mode].hap)
#define HFP				(g_disp_modes[g_mode].hfp)
#define HSW				(g_disp_modes[g_mode].hsw)
#define HBP				(g_disp_modes[g_mode].hbp)
#define HSP				(g_disp_modes[g_mode].hsp)
#define VAL				(g_disp_modes[g_mode].val)
#define VFP				(g_disp_modes[g_mode].vfp)
#define VSW				(g_disp_modes[g_mode].vsw)
#define VBP				(g_disp_modes[g_mode].vbp)
#define VSP				(g_disp_modes[g_mode].vsp)
#define VIC				(g_disp_modes[g_mode].vic)

#if 0
#define HAP				1920ul
#define HFP				88ul
#define HSW				44ul
#define HBP				148ul
#define VAL				1080ul
#define VFP				4ul
#define VSW				5ul
#define VBP				36ul

#define NDIV				0x4d
#define FDIV				0x58000
#endif

#if 0
#define HAP				640ul
#define HFP				16ul
#define HSW				96ul
#define HBP				48ul
#define VAL				480ul
#define VFP				10ul
#define VSW				2ul
#define VBP				33ul

#define NDIV				0xd
#define FDIV				0x20000
#endif

#if 0
#define HAP				800ul
#define HFP				40ul
#define HSW				128ul
#define HBP				88ul
#define VAL				600ul
#define VFP				1ul
#define VSW				4ul
#define VBP				23ul
#endif

#if 0
#define HAP				640ul
#define HFP				16ul
#define HSW				160ul
#define HBP				80ul
#define VAL				360ul
#define VFP				3ul
#define VSW				5ul
#define VBP				8ul
#endif

// block is 0 or 1.
static
void read_edid_block(int block, uint8_t *buf)
{
	int len, i;
	uint32_t val;

	val = 0;
	val |= bits_on(DDC_CTRL_START);
	val |= bits_on(DDC_CTRL_I2C_EN);
	g_ddc_regs[DDC_ADDR] = 0x50;
	g_ddc_regs[DDC_DLEN] = 1;
	g_ddc_regs[DDC_CTRL] = val;

	// Wait for TXW.
	while (1) {
		val = g_ddc_regs[DDC_STS];
		con_out("txsts: %x", val);
		if (bits_get(val, DDC_STS_TXW))
			break;
	}

	// Write into FIFO
	// TODO: If TXD is 0, start a new transfer.
	g_ddc_regs[DDC_FIFO] = block * 0x80;

	// Wait for RXR or DONE.
	while (1) {
		val = g_ddc_regs[DDC_STS];
		con_out("rxsts: %x", val);
		if (bits_get(val, DDC_STS_RXR))
			break;
		if (bits_get(val, DDC_STS_DONE))
			break;
	}

	// Read the block
	len = 0x80;
	val = 0;
	val |= bits_on(DDC_CTRL_START);
	val |= bits_on(DDC_CTRL_I2C_EN);
	val |= bits_on(DDC_CTRL_READ);
	g_ddc_regs[DDC_ADDR] = 0x50;
	g_ddc_regs[DDC_DLEN] = len;
	g_ddc_regs[DDC_CTRL] = val;

	for (i = 0; i < len;) {
		val = g_ddc_regs[DDC_STS];
		if (!bits_get(val, DDC_STS_RXD))
			continue;
		buf[i] = g_ddc_regs[DDC_FIFO];
		++i;
	}
}

static
void read_edid()
{
	int i;
	static uint8_t buf[0x100];
	uint32_t *p;

	read_edid_block(0, &buf[0]);
	read_edid_block(1, &buf[0x80]);

	p = (uint32_t *)&buf[0];
	for (i = 0; i < 64; ++i)
		con_out("0x%x,", p[i]);
}

static
void pv_disable()
{
	g_pv2_regs[PV_CTRL] &= bits_off(PV_CTRL_EN);
	g_pv2_regs[PV_CTRL] |= bits_on(PV_CTRL_FIFO_CLR);
}

static
void pv_disable_video()
{
	g_pv2_regs[PV_VID_CTRL] &= bits_off(PV_VID_CTRL_EN);
	while (bits_get(g_pv2_regs[PV_VID_CTRL], PV_VID_CTRL_EN))
		;
}

static
void pv_enable()
{
	uint32_t val;

	pv_disable();

	g_pv2_regs[PV_HORZA] = HSW | (HBP << 16);
	g_pv2_regs[PV_HORZB] = HAP | (HFP << 16);
	g_pv2_regs[PV_VERTA] = VSW | (VBP << 16);
	g_pv2_regs[PV_VERTB] = VAL | (VFP << 16);
	g_pv2_regs[PV_VID_CTRL] = bits_on(PV_VID_CTRL_CONTINUOUS);

	val = 0;
	val |= bits_on(PV_CTRL_FIFO_CLR);
	val |= bits_on(PV_CTRL_CLR_AT_START);
	val |= bits_on(PV_CTRL_TRIG_UFLOW);
	val |= bits_on(PV_CTRL_WAIT_HSTART);
	val |= (1 << 2);	// HDMI clock select.
	val |= (0x2e << 15);
	g_pv2_regs[PV_CTRL] = val;

	g_pv2_regs[PV_CTRL] |= bits_on(PV_CTRL_EN);
	con_out("pve: %x", g_pv2_regs[PV_CTRL]);
}

static
void pv_enable_video()
{
	g_pv2_regs[PV_VID_CTRL] |= bits_on(PV_VID_CTRL_EN);
	con_out("pvev: %x", g_pv2_regs[PV_VID_CTRL]);
}

static
void hdmi_enable_video()
{
	int i;
	uint8_t *p;
	uint32_t val;
	static struct hdmi_avi_infoframe avi;

	val = 0;
	val |= bits_on(HDMI_VID_CTRL_EN);
	if (HSP < 0)
		val |= bits_on(HDMI_VID_CTRL_NHSYNC);
	if (VSP < 0)
		val |= bits_on(HDMI_VID_CTRL_NVSYNC);
#if 0
	if (g_mode == 0) {
		// vga has negative syncs.
		val |= bits_on(HDMI_VID_CTRL_NHSYNC);
		val |= bits_on(HDMI_VID_CTRL_NVSYNC);
	}
#endif
	g_hd_regs[HDMI_VID_CTRL] = val;
	//g_hd_regs[HDMI_VID_CTRL] &= bits_off(HDMI_VID_CTRL_BLANK);

	// if we skip the sched ctrl and hdmi mode selection, the
	// resoltuion is correct. so the problem does seem to be with
	// infoframes.
	goto do_hdmi;
	goto skip_hdmi;
do_hdmi:

	g_hdmi_regs[HDMI_SCHED_CTRL] |= 1;	// contorl mode hdmi
	while (!(g_hdmi_regs[HDMI_SCHED_CTRL] & (1 << 1)))
		;

	//g_hdmi_regs[HDMI_SCHED_CTRL] |= 1 << 3;	// alwasy keppout
	g_hdmi_regs[HDMI_RAM_PKT_CFG] = 1 << 16;

	// Write the AVI Infoframe
	memset(&avi, 0, sizeof(avi));
	avi.type = 0x82;
	avi.version = 2;
	avi.data_len = 13;
	avi.colour_scan = 0x10;
	avi.vic = VIC;

	//0x18 for vga, 0x28 for fhd
	if (g_mode == 0 || g_mode == 1) {
		//avi.colour_aspect = 0x98;
		avi.colour_aspect = 0x18;	// 4:3
	} else {
		//avi.colour_aspect = 0xa8;
		avi.colour_aspect = 0x28;
	}

	val = 0;
	p = (uint8_t *)&avi;
	for (i = 0; i < (int)sizeof(avi); ++i)
		val += p[i];
	avi.chksum = 0x100 - (uint8_t)val;

	g_hdmi_regs[0x112] = p[0] | (p[1] << 8) | (p[2] << 16);
	g_hdmi_regs[0x113] = p[3] | (p[4] << 8) | (p[5] << 16) | (p[6] << 24);
	g_hdmi_regs[0x114] = p[7] | (p[8] << 8) | (p[9] << 16);
	g_hdmi_regs[0x115] = p[10] | (p[11] << 8) | (p[12] << 16) | (p[13] << 24);
	g_hdmi_regs[0x116] = p[14] | (p[15] << 8) | (p[16] << 16);

	con_out("avi %x %x %x %x %x",
		g_hdmi_regs[0x112],
		g_hdmi_regs[0x113],
		g_hdmi_regs[0x114],
		g_hdmi_regs[0x115],
		g_hdmi_regs[0x116]);

	g_hdmi_regs[HDMI_RAM_PKT_CFG] |= 1 << 2;
	while (!(g_hdmi_regs[HDMI_RAM_PKT_STS] & (1 << 2)))
	       ;
	goto recenter_fifo;

skip_hdmi:
	g_hdmi_regs[HDMI_RAM_PKT_CFG] &= ~(1 << 16);
	g_hdmi_regs[HDMI_SCHED_CTRL] &= (~1);
	while ((g_hdmi_regs[HDMI_SCHED_CTRL] & (1 << 1)))
		;

recenter_fifo:
	// recenter fifo
	val = g_hdmi_regs[HDMI_FIFO_CTRL] & 0xefff;
	g_hdmi_regs[HDMI_FIFO_CTRL] = val & (~(1 << 6));
	g_hdmi_regs[HDMI_FIFO_CTRL] = val | (1 << 6);
	for (i = 0; i < 0xffff; ++i)
		;

	g_hdmi_regs[HDMI_FIFO_CTRL] = val & (~(1 << 6));
	g_hdmi_regs[HDMI_FIFO_CTRL] = val | (1 << 6);
	while (!(g_hdmi_regs[HDMI_FIFO_CTRL] & (1 << 14)))
		;
}

static
void hdmi_disable_audio()
{
	return;
	g_hd_regs[HDMI_MAI_CTRL] = 1;
	g_hd_regs[HDMI_MAI_CTRL] = 2;
	g_hd_regs[HDMI_MAI_CTRL] = 1 << 9;
}

static
void hdmi_disable_video()
{
	uint32_t val;

	g_hdmi_regs[HDMI_RAM_PKT_CFG] = 0;

	val = 0;
	val |= bits_on(HDMI_VID_CTRL_CLR_SYNC);
	val |= bits_on(HDMI_VID_CTRL_CLR_RGB);
	g_hd_regs[HDMI_VID_CTRL] |= val;

	g_hd_regs[HDMI_VID_CTRL] |= bits_on(HDMI_VID_CTRL_BLANK);
}

static
void hdmi_phy_enable()
{
	g_hdmi_regs[HDMI_TX_PHY_RESET_CTRL] = 0xf << 16;
	g_hdmi_regs[HDMI_TX_PHY_RESET_CTRL] = 0;
}

static
void hdmi_phy_disable()
{
	g_hdmi_regs[HDMI_TX_PHY_RESET_CTRL] = 0xf << 16;
}

static
void hdmi_disable()
{
	uint32_t val;

	hdmi_phy_disable();

	g_hdmi_regs[HDMI_VID_CTRL] &= bits_off(HDMI_VID_CTRL_EN);

	// Disable the pixel clock, PLLH_PIX and its source, PLLH.

	// Turn off the load mask and turn on the hold mask (0 here).
	val = g_cm_regs[CM_PLLH];
	val &= bits_off(CM_PLLH_LOAD_PIX);
	g_cm_regs[CM_PLLH] = val | CM_PASSWORD;

	// Disable.
	val = bits_on(A2W_PLL_CHANNEL_DISABLE) | CM_PASSWORD;
	g_cm_regs[A2W_PLLH_PIX] |= val;

	// Disable PLLH.
	g_cm_regs[CM_PLLH] = bits_on(CM_PLL_ANA_RESET) | CM_PASSWORD;
	g_cm_regs[A2W_PLLH_CTRL] |= bits_on(A2W_PLL_CTRL_PWRDN) | CM_PASSWORD;
	con_out("hdmi_dis: %x %x", g_cm_regs[CM_PLLH], g_cm_regs[A2W_PLLH_CTRL]);
}

static
void hdmi_enable()
{
	int i;
	uint32_t val, ana[4], ndiv, fdiv;

	// Set and enable PLLH.
	//ndiv = NDIV;
	//fdiv = FDIV;

	// Can't set to 252MHZ since, 600MHz is minimum for PLLH.
	//ndiv = 0x1f;
	//fdiv = 0x40000;	// 600MHz min.


	// 640x480 We want pixel clock = 25.2 MHz. So we need 25.2 * 2 * 10 =
	// 504MHz clock
	// printf '%d' $(((19200000 * (0x1a * 2**20 + 0x40000))/(2**20)))
	//ndiv = 0x1a;
	//fdiv = 0x40000;

	// 640x480 We want pixel clock = 25.2 MHz. So we need 25.2 * 3 * 10 =
	// 756MHz clock
	//ndiv = 0x27;
	//fdiv = 0x60000;

	// 640x480 We want pixel clock = 25.175 MHz, so we need
	// 25.175 * 2 * 10 = 503.5 MHz
	//ndiv = 0x1a;
	//fdiv = 0x39556;

	// 800x600, pixel clock = 40MHz. So we need to set
	// 400MHz (if pllh_pix divctl is 1), or 800MHz (if pllh_pix div is 2)
	//ndiv = 0x14;
	//fdiv = 0xd5556;

	// 640x360 We want pixel clock = 18.048 MHz. So we need 18.048 * 30 =
	// 541.44MHz clock
	// printf '%d' $(((19200000 * (0x1a * 2**20 + 0x40000))/(2**20)))
	//ndiv = 0x1c;
	//fdiv = 0x33334;

//	ndiv = 0x11;//0x10
//	fdiv = 0x70000;	// 315.6MHz	locks successfully; 0x60000 doesn't

	ndiv = g_disp_modes[g_mode].pllh_ndiv;
	fdiv = g_disp_modes[g_mode].pllh_fdiv;
	con_out("hdmi_en");

	g_cm_regs[A2W_XOCS_CTRL] |= 1 | CM_PASSWORD;

	con_out("xosc_ctrl %x", g_cm_regs[A2W_XOCS_CTRL]);
	ana[0] = 0x900000;
	ana[1] = 0xc;
	ana[2] = 0;
	ana[3] = 0;
	for (i = 3; i >= 0; --i)
		g_cm_regs[A2W_PLLH_ANA0 + i] = ana[i] | CM_PASSWORD;
	con_out("wrote ana");

	g_cm_regs[A2W_PLLH_FRAC] = fdiv | CM_PASSWORD;
	con_out("wrote frac");

	val = g_cm_regs[A2W_PLLH_CTRL];
	val &= ~0x3ff;
	val &= ~0x7000;
	val |= ndiv;
	val |= 0x1000;	// pdiv.
	g_cm_regs[A2W_PLLH_CTRL] = val | CM_PASSWORD;
	con_out("wrote ndiv");

	// PLLH_PIX rate. Change div to 2 to get (600MH/10)/2 = 30Mhz pixel clock
	//g_cm_regs[A2W_PLLH_PIX] = 0x103 | CM_PASSWORD;
	//g_cm_regs[A2W_PLLH_PIX] = 0x102 | CM_PASSWORD;
	//g_cm_regs[A2W_PLLH_PIX] = 0x101 | CM_PASSWORD;
	val = g_cm_regs[A2W_PLLH_PIX] & (~0xff);
	val |= g_disp_modes[g_mode].pllh_pix_div;
	g_cm_regs[A2W_PLLH_PIX] = val | CM_PASSWORD;

	con_out("pllh_pix.a2w_reg %x", g_cm_regs[A2W_PLLH_PIX]);
	val = g_cm_regs[CM_PLLH];
	g_cm_regs[CM_PLLH] = val | bits_on(CM_PLLH_LOAD_PIX) | CM_PASSWORD;
	g_cm_regs[CM_PLLH] = (val & bits_off(CM_PLLH_LOAD_PIX)) | CM_PASSWORD;

	// PLLH Enable.
	val = g_cm_regs[A2W_PLLH_CTRL];
	val &= bits_off(A2W_PLL_CTRL_PWRDN);
	g_cm_regs[A2W_PLLH_CTRL] = val | CM_PASSWORD;
	con_out("wrote ~pwrdn");

	val = g_cm_regs[CM_PLLH];
	val &= bits_off(CM_PLL_ANA_RESET);
	g_cm_regs[CM_PLLH] = val | CM_PASSWORD;
	con_out("wrote ~anarst %x %x %x", g_cm_regs[CM_PLLH],
		g_cm_regs[A2W_PLLH_CTRL], g_cm_regs[CM_LOCK]);

	while (!bits_get(g_cm_regs[CM_LOCK], CM_LOCK_FLOCKH))
		;
	con_out("got lock %x", g_cm_regs[CM_LOCK]);

	val = bits_on(A2W_PLL_CTRL_PRST_DISABLE);
	g_cm_regs[A2W_PLLH_CTRL] |= val | CM_PASSWORD;
	con_out("wrote ~prst_dis");

	// PLLH_PIX Enable.
	val = g_cm_regs[A2W_PLLH_PIX];
	val &= bits_off(A2W_PLL_CHANNEL_DISABLE);
	g_cm_regs[A2W_PLLH_PIX] = val | CM_PASSWORD;
	con_out("wrote ~ch_dis %x", g_cm_regs[A2W_PLLH_PIX]);

	hdmi_phy_enable();
	con_out("wrote phy_en");

	val = 0;
	//val |= bits_on(HDMI_SCHED_CTRL_MANUAL_FMT);
	//val |= bits_on(HDMI_SCHED_CTRL_IGNORE_VP);
	g_hdmi_regs[HDMI_SCHED_CTRL] |= val;
	con_out("wrote sched");

	val = HAP;
	if (HSP > 0)
		val |= 1 << 13;	// H +ve
	if (VSP > 0)
		val |= 1 << 14;	// V +ve
	g_hdmi_regs[HDMI_HORZA] = val;
#if 0
	if (g_mode == 1)
		g_hdmi_regs[HDMI_HORZA] = HAP | 0x6000;	// 0x6000 for HV POSITIVE
	else
		g_hdmi_regs[HDMI_HORZA] = HAP;
#endif
	g_hdmi_regs[HDMI_HORZB] = HFP | (HSW << 10) | (HBP << 20);
	con_out("wrote horz");

	g_hdmi_regs[HDMI_VERTA0] =
	g_hdmi_regs[HDMI_VERTA1] = VAL | (VFP << 13) | (VSW << 20);

	g_hdmi_regs[HDMI_VERTB0] =
	g_hdmi_regs[HDMI_VERTB1] = VBP;
	con_out("wrote vert");
}

static
void hdmi_enable_csc_fifo()
{
	// csc setup. vga doesn't need conversoin, but anything else does.

	g_hd_regs[HDMI_CSC_CTRL] = 0x2d;//2f in firmware
	g_hdmi_regs[HDMI_FIFO_CTRL] = 1;
}

static
void hvs_disable_channel()
{
	g_hvs_regs[HVS_DL1_CTRL] |= bits_on(HVS_DL_CTRL_RESET);
	g_hvs_regs[HVS_DL1_CTRL] &= bits_off(HVS_DL_CTRL_EN);
}

#define HVS_DL_MEM_INDEX		0x800
#define HVS_DL_INDEX			0x400
//volatile uint32_t fb[HAP * VAL];

static
void hvs_enable_channel()
{
	uint32_t val;
	volatile uint32_t *dl;

	g_hvs_regs[HVS_DL0] = 0;
	g_hvs_regs[HVS_DL1] = 0;
	g_hvs_regs[HVS_DL2] = 0;

	dl = &g_hvs_regs[HVS_DL_MEM_INDEX + HVS_DL_INDEX];

	// The input FB (fb) has the format BGRA8888, or  ARGB32.
	// The output FB (out_fb) is expected to be in the same format.
	val = 0;
	val |= bits_set(HVS_DLW_CTL0_PIXEL_FMT, 7);	// RGBA8888
	val |= bits_set(HVS_DLW_CTL0_PIXEL_ORDER, 3);
	val |= bits_set(HVS_DLW_CTL0_RGBA_EXPAND, 3);	// Round??
	val |= bits_set(HVS_DLW_CTL0_NUM_WORDS, 7);	// 7 + End.
	val |= bits_on(HVS_DLW_CTL0_UNITY);
	val |= bits_on(HVS_DLW_CTL0_VALID);
	dl[0] = val;

	dl[1] = bits_on(HVS_DLW_POS0_FIXED_ALPHA);	// 0xff

	val = 0;
	val |= bits_set(HVS_DLW_POS2_SRC_WIDTH, HAP);
	val |= bits_set(HVS_DLW_POS2_SRC_HEIGHT, VAL);
	val |= bits_set(HVS_DLW_POS2_ALPHA_MODE, 1);	// Fixed Alpha.
	dl[2] = val;
	dl[3] = 0;

	//dl[4] = va_to_ba((va_t)fb);
	dl[4] = pa_to_ba(0x2000000);
	dl[5] = 0;
	dl[6] = HAP * 4;	// Source Pitch, for Linear Tiling.
	dl[7] = bits_on(HVS_DLW_CTL0_END);

	g_hvs_regs[HVS_DL1] = HVS_DL_INDEX;

	g_hvs_regs[HVS_DL1_CTRL] = 0;
	g_hvs_regs[HVS_DL1_CTRL] = bits_on(HVS_DL_CTRL_RESET);
	g_hvs_regs[HVS_DL1_CTRL] = 0;

	val = 0;
	val |= bits_on(HVS_DL_CTRL_EN);
	val |= bits_set(HVS_DL_CTRL_WIDTH, HAP);
	val |= bits_set(HVS_DL_CTRL_HEIGHT, VAL);
	g_hvs_regs[HVS_DL1_CTRL] = val;
}

static
void dump_regs()
{
	int i;

	for (i = 0; i < (0x6000 >> 2); ++i)
		con_out("hvs[%x]=%x", i, g_hvs_regs[i]);

	for (i = 0; i < (0x100 >> 2); ++i)
		con_out("pv2[%x]=%x", i, g_pv2_regs[i]);

	for (i = 0; i < (0x600 >> 2); ++i)
		con_out("hdmi[%x]=%x", i, g_hdmi_regs[i]);

	for (i = 0; i < (0x100 >> 2); ++i)
		con_out("hd[%x]=%x", i, g_hd_regs[i]);

	for (i = 0; i < (0x2000 >> 2); ++i)
		con_out("cm[%x]=%x", i, g_cm_regs[i]);
}

// PV2 drives the HDMI encoder.
int disp_config()
{
	//con_out("Before");
	//dump_regs();
	pv_disable_video();
	hdmi_disable_video();
	hdmi_disable_audio();
	pv_disable();
	hvs_disable_channel();
	hdmi_disable();

	hvs_enable_channel();
	hdmi_enable();
	pv_enable();
	hdmi_enable_csc_fifo();
	pv_enable_video();
	hdmi_enable_video();
	return ERR_SUCCESS;
	con_out("After");
	dump_regs();
	return ERR_SUCCESS;
}

// IPL_THREAD
int disp_init()
{
	volatile uint32_t *fb;
	int err, i, num_pages;
	va_t va;
	vpn_t pages, page;
	pfn_t frame;

	err = dev_map_io(HVS_BASE, 0x6000, &va);
	if (err)
		return err;
	g_hvs_regs = (volatile uint32_t *)va;

	err = dev_map_io(TXP_BASE, 0x20, &va);
	if (err)
		return err;
	g_txp_regs = (volatile uint32_t *)va;

	err = dev_map_io(PV1_BASE, 0x100, &va);
	if (err)
		return err;
	g_pv1_regs = (volatile uint32_t *)va;

	err = dev_map_io(PV2_BASE, 0x100, &va);
	if (err)
		return err;
	g_pv2_regs = (volatile uint32_t *)va;

	err = dev_map_io(HDMI_BASE, 0x600, &va);
	if (err)
		return err;
	g_hdmi_regs = (volatile uint32_t *)va;

	err = dev_map_io(HD_BASE, 0x100, &va);
	if (err)
		return err;
	g_hd_regs = (volatile uint32_t *)va;

	err = dev_map_io(CM_BASE, 0x2000, &va);
	if (err)
		return err;
	g_cm_regs = (volatile uint32_t *)va;

	err = dev_map_io(DDC_BASE, 0x1000, &va);
	if (err)
		return err;
	g_ddc_regs = (volatile uint32_t *)va;

	//num_pages = 19;		//VGA
	num_pages = 127;	//FHD
	// 640x480x4 == 19 64KB pages.
	err = vmm_alloc(ALIGN_PAGE, num_pages, &pages);
	if (err)
		goto err0;

	page = pages;
	frame = pa_to_pfn(0x2000000);
	for (i = 0; i < num_pages; ++i, ++page, ++frame) {
		err = mmu_map_page(0, page, frame, ALIGN_PAGE, PROT_RW);
		if (err)
			goto err1;
	}
	fb = (volatile uint32_t *)vpn_to_va(pages);
	for (i = 0; i < (int)(HAP * VAL); ++i)
		fb[i] = 0xffff0000;

	// first and last row
	for (i = 0; i < (int)HAP; ++i) {
		fb[i] = 0xffffffff;
		fb[(VAL - 1) * HAP + i] = 0xffffffff;
	}

	// first and last column
	for (i = 0; i < (int)VAL; ++i) {
		fb[i * HAP] = 0xffffffff;
		fb[i * HAP + (HAP - 1)] = 0xffffffff;
	}

	return ERR_SUCCESS;
err1:
	for (--i, --page; i >= 0; --i, --page)
		mmu_unmap_page(0, page);
	vmm_free(pages, 19);
err0:
	return err;
	(void)read_edid;
}
