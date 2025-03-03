/*
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 */

/*
 * Copyright 2023 Oxide Computer Company
 */

#include <sys/ddi.h>
#include <sys/pci.h>
#include <sys/pcie.h>
#include <sys/spl.h>
#include <sys/sunddi.h>
#include <sys/sysmacros.h>
#include <sys/types.h>

#include <sys/promif.h>

extern kmutex_t pcicfg_mmio_mutex;

boolean_t
pcie_access_check(int bus, int dev, int func, int reg, size_t len)
{
	if (bus < 0 || bus >= PCI_MAX_BUS_NUM) {
		return (B_FALSE);
	}

	if (dev < 0 || dev >= PCI_MAX_DEVICES) {
		return (B_FALSE);
	}

	/*
	 * Due to the advent of ARIs we want to make sure that we're not overly
	 * stringent here. ARIs retool how the bits are used for the device and
	 * function. This means that if dev == 0, allow func to be up to 0xff.
	 */
	if (func < 0 || (dev != 0 && func >= PCI_MAX_FUNCTIONS) ||
	    (dev == 0 && func >= PCIE_ARI_MAX_FUNCTIONS)) {
		return (B_FALSE);
	}

	/*
	 * Technically the maximum register is determined by the parent. At this
	 * point we have no way of knowing what is PCI or PCIe and will rely on
	 * mmio to solve this for us.
	 */
	if (reg < 0 || reg >= PCIE_CONF_HDR_SIZE) {
		return (B_FALSE);
	}

	if (!IS_P2ALIGNED(reg, len)) {
#ifdef	DEBUG
		/*
		 * While there are legitimate reasons we might try to access
		 * nonexistent devices and functions, misaligned accesses are at
		 * least strongly suggestive of kernel bugs.  Let's see what
		 * this finds.
		 */
		cmn_err(CE_WARN, "misaligned PCI config space access at "
		    "%x/%x/%x reg 0x%x len %lu\n", bus, dev, func, reg, len);
#endif
		return (B_FALSE);
	}

	return (B_TRUE);
}

extern int pcie_cfgspace_ecam_init(uintptr_t, size_t);
extern int pcie_cfgspace_none_init(void);

void
pcie_cfgspace_init(void)
{
	pnode_t node;

	/*
	 * XXXPCI: This is the other part of the prototype simplification, we
	 * use /pcie explicitly and assume it to be ECAM
	 */
	if ((node = prom_finddevice("/pcie")) == OBP_NONODE) {
#ifdef DEBUG
		cmn_err(CE_WARN, "system has no PCIe at /pcie");
#endif
		return;
	}

	if (prom_is_compatible(node, "pci-host-ecam-generic")) {
		uint64_t addr;
		uint64_t size;

#ifdef DEBUG
		cmn_err(CE_CONT, "?PCIe: System is ECAM\n");
#endif

		if (prom_get_reg_address(node, 0, &addr) != 0) {
			cmn_err(CE_WARN, "PCIe: Failed to get config "
			    "space address");
			return;
		}

		if (prom_get_reg_size(node, 0, &size) != 0) {
			cmn_err(CE_WARN, "PCIe: Failed to get config "
			    "space size");
			return;
		}

		if (pcie_cfgspace_ecam_init(addr, size) != 0)
			pcie_cfgspace_none_init();
	} else {
		pcie_cfgspace_none_init();
	}

	/*
	 * XXXPCI:
	 *
	 *  else if bcm2711? ....
	 *  else ...
	 *  else assume DEN0115
	 *
	 * Probably in reality just assume the world is either ecam, or
	 * DEN0115 works.
	 */
	mutex_init(&pcicfg_mmio_mutex, NULL, MUTEX_SPIN,
	    (ddi_iblock_cookie_t)ipltospl(DISP_LEVEL));
}

/*
 * This would be called once the device arena was up, to remap into it, except
 * we don't really have one, we just have primordial PSM maps that live
 * forever.
 */
void
pcie_cfgspace_remap(void)
{
}
