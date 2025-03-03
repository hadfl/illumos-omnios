/*
 * Derived from:
 * $OpenBSD: bcm2711_pcie.c,v 1.13 2024/03/27 15:15:00 patrick Exp $
 *
 * Copyright (c) 2020 Mark Kettenis <kettenis@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * Copyright 2025 Richard Lowe
 */

#include <sys/ddi.h>
#include <sys/sunddi.h>
#include <sys/sunndi.h>

#include <sys/pci_cfgspace.h>
#include <sys/pci_cfgacc.h>
#include <sys/pcie_impl.h>

#define	BCM2711_DEV_CFG_DATA	0x8000 /* (read/write) config space data */
#define	BCM2711_DEV_CFG_INDEX	0x9000 /* (write) config space index  */

typedef struct {
	dev_info_t *bc_dip;
	ddi_acc_handle_t bc_handle;
	caddr_t bc_base;
	kmutex_t bc_lock;
} bcm2711_pcie_softc_t;

static void *bcm2711_pcie_soft_state;

static uint64_t
bcm2711_cfg_read_root(bcm2711_pcie_softc_t *softc, int bus, int dev, int func,
    int reg, size_t size)
{
	/*
	 * Requests to bus 0, the root complex, must also have device 0, per
	 * the BSD driver
	 */
	VERIFY0(dev);

	switch (size * NBBY) {
	case 8:
		return (ddi_get8(softc->bc_handle,
		    (uint8_t *)(softc->bc_base + PCIE_CADDR_ECAM(bus, dev, func,
		    reg))));
	case 16:
		return (ddi_get16(softc->bc_handle,
		    (uint16_t *)(softc->bc_base + PCIE_CADDR_ECAM(bus, dev,
		    func, reg))));
	case 32:
		return (ddi_get32(softc->bc_handle,
		    (uint32_t *)(softc->bc_base + PCIE_CADDR_ECAM(bus, dev,
		    func, reg))));
	case 64:
		return (ddi_get64(softc->bc_handle,
		    (uint64_t *)(softc->bc_base + PCIE_CADDR_ECAM(bus, dev,
		    func, reg))));

	}

	dev_err(softc->bc_dip, CE_PANIC, "weird %ld bit config space access",
	    size * NBBY);
	/* Unreachable */
	return (PCI_EINVAL64);
}

static uint64_t
bcm2711_cfg_read_dev(bcm2711_pcie_softc_t *softc, int bus, int dev, int func,
    int reg, size_t size)
{
	VERIFY3S(bus, !=, 0);

	ddi_put32(softc->bc_handle,
	    (uint32_t *)(softc->bc_base + BCM2711_DEV_CFG_INDEX),
	    PCIE_CADDR_ECAM(bus, dev, func, 0));

	switch (size * NBBY) {
	case 8:
		return (ddi_get8(softc->bc_handle,
		    (uint8_t *)(softc->bc_base + BCM2711_DEV_CFG_DATA + reg)));
	case 16:
		return (ddi_get16(softc->bc_handle,
		    (uint16_t *)(softc->bc_base + BCM2711_DEV_CFG_DATA + reg)));
	case 32:
		return (ddi_get32(softc->bc_handle,
		    (uint32_t *)(softc->bc_base + BCM2711_DEV_CFG_DATA + reg)));
	case 64:
		return (ddi_get32(softc->bc_handle,
		    (uint32_t *)(softc->bc_base + BCM2711_DEV_CFG_DATA + reg)));
	}

	dev_err(softc->bc_dip, CE_PANIC, "weird %ld bit config space access",
	    size * NBBY);
	/* Unreachable */
	return (PCI_EINVAL64);
}

static uint64_t
bcm2711_cfg_read(dev_info_t *dip, int bus, int dev, int func, int reg,
    size_t size)
{
	bcm2711_pcie_softc_t *softc =
	    ddi_get_soft_state(bcm2711_pcie_soft_state, ddi_get_instance(dip));
	uint32_t ret = 0;

	mutex_enter(&softc->bc_lock);

	if (bus == 0) {
		ret = bcm2711_cfg_read_root(softc, bus, dev, func, reg, size);
	} else {
		ret = bcm2711_cfg_read_dev(softc, bus, dev, func, reg, size);
	}

	mutex_exit(&softc->bc_lock);

	return (ret);
}

static void
bcm2711_cfg_write_root(bcm2711_pcie_softc_t *softc, int bus, int dev, int func,
    int reg, size_t size, uint64_t val)
{
	/*
	 * Requests to bus 0, the root complex, must also have device 0, per
	 * the BSD driver
	 */
	VERIFY0(dev);

	switch (size * NBBY) {
	case 8:
		ddi_put8(softc->bc_handle, (uint8_t *)(softc->bc_base +
		    PCIE_CADDR_ECAM(bus, dev, func, reg)), val);
		break;
	case 16:
		ddi_put16(softc->bc_handle, (uint16_t *)(softc->bc_base +
		    PCIE_CADDR_ECAM(bus, dev, func, reg)), val);
		break;
	case 32:
		ddi_put32(softc->bc_handle, (uint32_t *)(softc->bc_base +
		    PCIE_CADDR_ECAM(bus, dev, func, reg)), val);
		break;
	case 64:
		ddi_put64(softc->bc_handle, (uint64_t *)(softc->bc_base +
		    PCIE_CADDR_ECAM(bus, dev, func, reg)), val);
		break;
	default:
		dev_err(softc->bc_dip, CE_PANIC,
		    "weird %ld bit config space access", size * NBBY);
	}
}

static void
bcm2711_cfg_write_dev(bcm2711_pcie_softc_t *softc, int bus, int dev, int func,
    int reg, size_t size, uint64_t val)
{
	VERIFY3S(bus, !=, 0);

	ddi_put32(softc->bc_handle,
	    (uint32_t *)(softc->bc_base + BCM2711_DEV_CFG_INDEX),
	    PCIE_CADDR_ECAM(bus, dev, func, 0));

	switch (size * NBBY) {
	case 8:
		ddi_put8(softc->bc_handle,
		    (uint8_t *)(softc->bc_base + BCM2711_DEV_CFG_DATA + reg),
		    val);
		break;
	case 16:
		ddi_put16(softc->bc_handle,
		    (uint16_t *)(softc->bc_base + BCM2711_DEV_CFG_DATA + reg),
		    val);
		break;
	case 32:
		ddi_put32(softc->bc_handle,
		    (uint32_t *)(softc->bc_base + BCM2711_DEV_CFG_DATA + reg),
		    val);
		break;
	case 64:
		ddi_put64(softc->bc_handle,
		    (uint64_t *)(softc->bc_base + BCM2711_DEV_CFG_DATA + reg),
		    val);
		break;
	default:
		dev_err(softc->bc_dip, CE_PANIC,
		    "weird %ld bit config space access", size * NBBY);
	}
}

static void
bcm2711_cfg_write(dev_info_t *dip, int bus, int dev, int func, int reg,
    size_t size, uint64_t val)
{
	bcm2711_pcie_softc_t *softc =
	    ddi_get_soft_state(bcm2711_pcie_soft_state, ddi_get_instance(dip));

	mutex_enter(&softc->bc_lock);

	if (bus == 0) {
		bcm2711_cfg_write_root(softc, bus, dev, func, reg, size, val);
	} else {
		bcm2711_cfg_write_dev(softc, bus, dev, func, reg, size, val);
	}

	mutex_exit(&softc->bc_lock);
}

void
bcm2711_cfgspace_acc(pci_cfgacc_req_t *req)
{
	int bus, dev, func, reg;

	bus = PCI_CFGACC_BUS(req);
	dev = PCI_CFGACC_DEV(req);
	func = PCI_CFGACC_FUNC(req);
	reg = req->offset;

	if (!pcie_cfgspace_access_check(bus, dev, func, reg, req->size)) {
		if (!req->write)
			VAL64(req) = -1;
		return;
	}

	if (req->write) {
		bcm2711_cfg_write(req->rcdip, bus, dev, func, reg,
		    req->size, VAL64(req));
	} else {
		VAL64(req) = bcm2711_cfg_read(req->rcdip, bus, dev, func, reg,
		    req->size);
	}
}

/*
 * XXXPCI: This could be in the misc module, and the same for all nexi.
 * This is cut/pated between the two, don't mess that up
 */
static int
bcm2711_pcie_bus_config(dev_info_t *pdip, uint_t flags, ddi_bus_config_op_t op,
    void *arg, dev_info_t **rdip)
{
	int rval = DDI_SUCCESS;
	ndi_devi_enter(pdip);

	/*
	 * XXXPCI: This is the wrong way to be doing this.  We need to do
	 * stuff to rdip no doubt?
	 */
	if (op == BUS_CONFIG_ALL) {
		extern void pci_enumerate(dev_info_t *);
		pci_enumerate(pdip);
	}

	rval = ndi_busop_bus_config(pdip, flags, op, arg, rdip, 0);

	ndi_devi_exit(pdip);

	return (rval);
}

int
bcm2711_pcie_attach(dev_info_t *dip, ddi_attach_cmd_t cmd)
{
	bcm2711_pcie_softc_t *softc = NULL;
	int ret;

	if (cmd == DDI_RESUME)
		return (DDI_SUCCESS);

	int instance = ddi_get_instance(dip);

	if ((ret = ddi_soft_state_zalloc(bcm2711_pcie_soft_state, instance)) !=
	    DDI_SUCCESS) {
		return (ret);
	}

	softc = ddi_get_soft_state(bcm2711_pcie_soft_state, instance);
	VERIFY3P(softc, !=, NULL);

	const ddi_device_acc_attr_t attr = {
		.devacc_attr_version = DDI_DEVICE_ATTR_V1,
		.devacc_attr_endian_flags = DDI_STRUCTURE_LE_ACC,
		.devacc_attr_dataorder = DDI_STRICTORDER_ACC,
		.devacc_attr_access = DDI_DEFAULT_ACC,
	};

	softc->bc_dip = dip;

	if ((ret = ddi_regs_map_setup(dip, 0, &softc->bc_base,
	    0, 0, &attr, &softc->bc_handle)) != DDI_SUCCESS) {
		dev_err(dip, CE_WARN, "failed to map configuration space: %d",
		    ret);
		return (ret);
	}

	VERIFY3U(softc->bc_base, !=, 0);

	/*
	 * XXXPCI: This is not the mechanism I would prefer, but I cannot find
	 * one I prefer.
	 */
	if ((ret = ddi_prop_update_int64(DDI_DEV_T_NONE, dip,
	    OBP_CFGSPACE_HOOK, (intptr_t)&bcm2711_cfgspace_acc)) !=
	    DDI_PROP_SUCCESS) {
		dev_err(dip, CE_WARN, "failed to set cfgspace access "
		    "hook: %d\n", ret);
		return (DDI_FAILURE);
	}

	ddi_report_dev(dip);

	return (DDI_SUCCESS);
}

int
bcm2711_pcie_detach(dev_info_t *dip, ddi_detach_cmd_t cmd)
{
	if (cmd != DDI_DETACH)
		return (DDI_SUCCESS);

	bcm2711_pcie_softc_t *softc =
	    ddi_get_soft_state(bcm2711_pcie_soft_state, ddi_get_instance(dip));

	VERIFY3P(softc, !=, NULL);

	ddi_prop_remove(DDI_DEV_T_NONE, dip, OBP_CFGSPACE_HOOK);

	ddi_regs_map_free(&softc->bc_handle);
	ddi_soft_state_free(bcm2711_pcie_soft_state,
	    ddi_get_instance(dip));

	return (DDI_SUCCESS);
}

static struct bus_ops bcm2711_pcie_bus_ops = {
	.busops_rev = BUSO_REV,
	.bus_config = bcm2711_pcie_bus_config,
};

static struct dev_ops bcm2711_pcie_ops = {
	.devo_rev = DEVO_REV,
	.devo_refcnt = 0,
	.devo_getinfo = ddi_no_info,
	.devo_identify = nulldev,
	.devo_probe = nulldev,
	.devo_attach = bcm2711_pcie_attach,
	.devo_detach = bcm2711_pcie_detach,
	.devo_reset = nodev,
	.devo_bus_ops = &bcm2711_pcie_bus_ops,
	.devo_quiesce = ddi_quiesce_not_needed,
};

static struct modldrv bcm2711_pcie_modldrv = {
	.drv_modops = &mod_driverops,
	.drv_linkinfo = "Broadcom 2711 PCIe",
	.drv_dev_ops = &bcm2711_pcie_ops,
};

static struct modlinkage modlinkage = {
	.ml_rev = MODREV_1,
	.ml_linkage = { &bcm2711_pcie_modldrv, NULL }
};

int
_init(void)
{
	int err;

	if ((err = ddi_soft_state_init(&bcm2711_pcie_soft_state,
	    sizeof (bcm2711_pcie_softc_t), 1)) != DDI_SUCCESS) {
		return (err);
	}

	if ((err = mod_install(&modlinkage)) != 0) {
		ddi_soft_state_fini(&bcm2711_pcie_soft_state);
		return (err);
	}

	return (DDI_SUCCESS);
}

int
_fini(void)
{
	int err;

	if ((err = mod_remove(&modlinkage)) != DDI_SUCCESS) {
		return (err);
	}

	ddi_soft_state_fini(&bcm2711_pcie_soft_state);
	return (DDI_SUCCESS);
}

int
_info(struct modinfo *modinfop)
{
	return (mod_info(&modlinkage, modinfop));
}
