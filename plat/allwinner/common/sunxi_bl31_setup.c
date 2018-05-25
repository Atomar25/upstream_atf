/*
 * Copyright (c) 2017, ARM Limited and Contributors. All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <arch.h>
#include <assert.h>
#include <console.h>
#include <debug.h>
#include <generic_delay_timer.h>
#include <gicv2.h>
#include <platform.h>
#include <platform_def.h>
#include <plat_arm.h>
#include <sunxi_def.h>
#include <sunxi_mmap.h>

#include "sunxi_private.h"

static entry_point_info_t bl32_image_ep_info;
static entry_point_info_t bl33_image_ep_info;

static const gicv2_driver_data_t sunxi_gic_data = {
	.gicd_base = SUNXI_GICD_BASE,
	.gicc_base = SUNXI_GICC_BASE,
};

void bl31_early_platform_setup(bl31_params_t *from_bl2,
			       void *plat_params_from_bl2)
{
	/* Initialize the debug console as soon as possible */
	console_init(SUNXI_UART0_BASE,
		     SUNXI_UART0_CLK_IN_HZ,
		     SUNXI_UART0_BAUDRATE);

#ifdef BL32_BASE
	/* Populate entry point information for BL32 */
	SET_PARAM_HEAD(&bl32_image_ep_info, PARAM_EP, VERSION_1, 0);
	SET_SECURITY_STATE(bl32_image_ep_info.h.attr, SECURE);
	/* Tell BL32 where the trusted software image
	 * is located and the entry state information
	 */
	bl32_image_ep_info.pc = BL32_BASE;
	bl32_image_ep_info.spsr = arm_get_spsr_for_bl32_entry();
#endif
	/* Populate entry point information for BL33 */
	SET_PARAM_HEAD(&bl33_image_ep_info, PARAM_EP, VERSION_1, 0);
	SET_SECURITY_STATE(bl33_image_ep_info.h.attr, NON_SECURE);
	/*
	 * Tell BL31 where the non-trusted software image
	 * is located and the entry state information
	 */
	bl33_image_ep_info.pc = PLAT_ARM_NS_IMAGE_OFFSET;
	bl33_image_ep_info.spsr = SPSR_64(MODE_EL2, MODE_SP_ELX,
					  DISABLE_ALL_EXCEPTIONS);

	/* Turn off all secondary CPUs */
	sunxi_disable_secondary_cpus(plat_my_core_pos());
}

void bl31_plat_arch_setup(void)
{
	sunxi_configure_mmu_el3(0);
}

void bl31_platform_setup(void)
{
	generic_delay_timer_init();

	/* Configure the interrupt controller */
	gicv2_driver_init(&sunxi_gic_data);
	gicv2_distif_init();
	gicv2_pcpu_distif_init();
	gicv2_cpuif_enable();

	INFO("BL31: Platform setup done\n");
}

entry_point_info_t *bl31_plat_get_next_image_ep_info(uint32_t type)
{
	assert(sec_state_is_valid(type));

	if (type == NON_SECURE)
		return &bl33_image_ep_info;

	else if (type == SECURE && bl32_image_ep_info.pc)
		return &bl32_image_ep_info;

	return NULL;
}
