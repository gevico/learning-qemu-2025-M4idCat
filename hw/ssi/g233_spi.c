/*
 * G233 SPI Controller (minimal model for test-spi-jedec)
 *
 * Single-byte blocking transfers via DR, simple status flags,
 * CS0 enable/active mapping to a low-active SSI CS line.
 */

#include "qemu/osdep.h"
#include "qemu/log.h"
#include "qemu/module.h"
#include "hw/irq.h"
#include "hw/ssi/ssi.h"
#include "hw/ssi/g233_spi.h"
#include "migration/vmstate.h"

static void g233_spi_update_cs(G233SPIState *s)
{
    qemu_set_irq(s->cs_lines[0], (s->cs0_en && s->cs0_act) ? 0 : 1);
    qemu_set_irq(s->cs_lines[1], (s->cs1_en && s->cs1_act) ? 0 : 1);
}

static void g233_spi_update_irq(G233SPIState *s)
{
    bool irq = false;
    /* TXE interrupt when enabled and TXE=1 */
    if ((s->cr2 & (1u << 7)) && (s->sr & G233_SPI_SR_TXE)) {
        irq = true;
    }
    /* RXNE interrupt when enabled and RXNE=1 */
    if ((s->cr2 & (1u << 6)) && (s->sr & G233_SPI_SR_RXNE)) {
        irq = true;
    }
    /* Error interrupt when enabled and any error flag set */
    if ((s->cr2 & (1u << 5)) && (s->sr & (G233_SPI_SR_UDR | G233_SPI_SR_OVR))) {
        irq = true;
    }
    qemu_set_irq(s->cs_lines[2], irq);
}
static void g233_spi_reset(DeviceState *dev)
{
    G233SPIState *s = G233_SPI(dev);

    s->cr1 = 0;
    s->cr2 = 0;
    s->sr  = G233_SPI_SR_TXE; /* TXE=1, RXNE=0, BSY=0 */

    s->rx_data = 0;
    s->spe = false;
    s->mstr = false;
    s->cs0_en = false;
    s->cs0_act = false;
    s->cs1_en = false;
    s->cs1_act = false;
    g233_spi_update_cs(s);
    g233_spi_update_irq(s);
}

static void g233_spi_do_transfer(G233SPIState *s, uint8_t tx)
{
    /* start: set BSY=1, clear TXE */
    s->sr &= ~G233_SPI_SR_TXE;
    s->sr |= G233_SPI_SR_BSY;

    /* one 8-bit transfer on the SSI bus */
    uint32_t rx = ssi_transfer(s->ssi, tx);
    /* Overrun if previous RX not read yet */
    if (s->sr & G233_SPI_SR_RXNE) {
        s->sr |= G233_SPI_SR_OVR;
    }
    s->rx_data = (uint8_t)rx;

    /* complete: set RXNE, set TXE, clear BSY */
    s->sr |= G233_SPI_SR_RXNE;
    s->sr |= G233_SPI_SR_TXE;
    s->sr &= ~G233_SPI_SR_BSY;

    g233_spi_update_irq(s);

}

static uint64_t g233_spi_read(void *opaque, hwaddr addr, unsigned size)
{
    G233SPIState *s = opaque;
    switch (addr) {
        case G233_SPI_CR1:
            return s->cr1;
        case G233_SPI_CR2:
            return s->cr2;
        case G233_SPI_SR:
            return s->sr; 
        case G233_SPI_DR: {
            /* read returns last rx byte; reading DR clears RXNE */
            uint32_t val = s->rx_data;
            s->sr &= ~G233_SPI_SR_RXNE;
            /* Clear OVR together with RXNE, then re-evaluate IRQ */
            s->sr &= ~G233_SPI_SR_OVR;
            g233_spi_update_irq(s);
            return val;
        }
        case G233_SPI_CSCTRL:
            return s->csctrl;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                      "g233-spi: bad read offset 0x%" HWADDR_PRIx "\n", addr);
            return 0;
    }
}

static void g233_spi_write(void *opaque, hwaddr addr, uint64_t val64, unsigned size)
{
    G233SPIState *s = opaque;
    uint32_t val = val64;

    switch (addr) {
        case G233_SPI_CR1:
            s->cr1 = val;
            s->spe  = (val & G233_SPI_CR1_SPE)  != 0;
            s->mstr = (val & G233_SPI_CR1_MSTR) != 0;
            return;
        case G233_SPI_CR2:
            /* Interrupt enables live here (TXEIE/RXNEIE/ERRIE) */
            s->cr2 = val;
            g233_spi_update_irq(s);
            return;
        case G233_SPI_SR:
            /* status is read-only for this minimal model */
            return;
        case G233_SPI_DR: {
            s->dr = val & 0xFF;
            /* transfer only if enabled, master, and CS active */
            bool cs0_active = (s->cs0_en && s->cs0_act);
            bool cs1_active = (s->cs1_en && s->cs1_act);
            if (s->spe && s->mstr && (cs0_active ^ cs1_active)){
                g233_spi_do_transfer(s, (uint8_t)s->dr);
            }else {
            /* If not active, keep TXE=1, RXNE unchanged; BSY stays 0 */
            }
            return;
        }
        case G233_SPI_CSCTRL:
            s->csctrl = val;
            s->cs0_en  = (val & G233_SPI_CS0_ENABLE) != 0;
            s->cs1_en  = (val & G233_SPI_CS1_ENABLE) != 0;
            s->cs0_act = (val & G233_SPI_CS0_ACTIVE) != 0;
            s->cs1_act = (val & G233_SPI_CS1_ACTIVE) != 0;
            g233_spi_update_cs(s);
            return;
        default:
            qemu_log_mask(LOG_GUEST_ERROR,
                        "g233-spi: bad write offset 0x%" HWADDR_PRIx " val=0x%x\n",
                        addr, val);
            return;
    }    
}
static const MemoryRegionOps g233_spi_ops = {
    .read = g233_spi_read,
    .write = g233_spi_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid = { .min_access_size = 4, .max_access_size = 4 },
};

static const VMStateDescription vmstate_g233_spi = {
    .name = TYPE_G233_SPI,
    .version_id = 1,
    .minimum_version_id = 1,
    .fields = (const VMStateField[]) {
        VMSTATE_UINT32(cr1, G233SPIState),
        VMSTATE_UINT32(cr2, G233SPIState),
        VMSTATE_UINT32(sr,  G233SPIState),
        VMSTATE_UINT32(dr,  G233SPIState),
        VMSTATE_UINT32(csctrl, G233SPIState),
        VMSTATE_UINT8(rx_data, G233SPIState),
        VMSTATE_BOOL(spe, G233SPIState),
        VMSTATE_BOOL(mstr, G233SPIState),
        VMSTATE_BOOL(cs0_en, G233SPIState),
        VMSTATE_BOOL(cs0_act, G233SPIState),
        VMSTATE_BOOL(cs1_en, G233SPIState),
        VMSTATE_BOOL(cs1_act, G233SPIState),
        VMSTATE_END_OF_LIST()
    }
};

static void g233_spi_realize(DeviceState *dev, Error **errp)
{
    G233SPIState *s = G233_SPI(dev);
    memory_region_init_io(&s->mmio, OBJECT(dev), &g233_spi_ops, s, 
                        TYPE_G233_SPI, 0X1000);
    sysbus_init_mmio(SYS_BUS_DEVICE(dev), &s->mmio);

    /* Expose unnamed GPIO outputs:
     * 0: CS0 (low-active)
     * 1: IRQ line to PLIC
     */
    qdev_init_gpio_out(DEVICE(dev), s->cs_lines, 3);

    s->ssi = ssi_create_bus(DEVICE(dev), "ssi");

}

static void g233_spi_class_init(ObjectClass *klass, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    device_class_set_legacy_reset(dc, g233_spi_reset);
    dc->realize = &g233_spi_realize;
    dc->vmsd = &vmstate_g233_spi;
}
static const TypeInfo g233_spi_info = {
    .name          = TYPE_G233_SPI,
    .parent        = TYPE_SYS_BUS_DEVICE,
    .instance_size = sizeof(G233SPIState),
    .class_init    = g233_spi_class_init,
};

static void g233_spi_register_types(void)
{
    type_register_static(&g233_spi_info);
}
type_init(g233_spi_register_types)