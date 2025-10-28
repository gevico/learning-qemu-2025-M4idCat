/* G233 SPI Controller (minimal, single-byte xfer, CS0 only) */

#ifndef HW_SSI_G233_SPI_H
#define HW_SSI_G233_SPI_H

#include "hw/sysbus.h"
#include "hw/ssi/ssi.h"
#include "qom/object.h"

/* MMIO layout to match test-spi-jedec */
#define G233_SPI_CR1     0x00
#define G233_SPI_CR2     0x04
#define G233_SPI_SR      0x08
#define G233_SPI_DR      0x0C
#define G233_SPI_CSCTRL  0x10

/* CR1 bits */
#define G233_SPI_CR1_SPE   (1u << 6)
#define G233_SPI_CR1_MSTR  (1u << 2)

/* SR bits */
#define G233_SPI_SR_RXNE   (1u << 0)  /* Receive not empty */
#define G233_SPI_SR_TXE    (1u << 1)  /* Transmit buffer empty */
#define G233_SPI_SR_UDR    (1u << 2)  /* Underrun flag       ← 改成 bit2 */
#define G233_SPI_SR_OVR    (1u << 3)  /* Overrun flag        ← 改成 bit3 */
#define G233_SPI_SR_BSY    (1u << 7)  /* Busy */

/* CSCTRL bits */
#define G233_SPI_CS0_ENABLE  (1u << 0)
#define G233_SPI_CS1_ENABLE  (1u << 1)
#define G233_SPI_CS0_ACTIVE  (1u << 4)
#define G233_SPI_CS1_ACTIVE  (1u << 5)

#define TYPE_G233_SPI "g233-spi"
OBJECT_DECLARE_SIMPLE_TYPE(G233SPIState, G233_SPI)

typedef struct G233SPIState {
    /* <private> */
    SysBusDevice parent_obj;

    /* <public> */
    MemoryRegion mmio;
    SSIBus *ssi;

    /* Unnamed GPIO outputs:
    * index 0: CS0 output (wire to flash0's ssi-gpio-cs[0], low-active)
    * index 1: CS1 output (wire to flash1's ssi-gpio-cs[1], low-active)
    * index 2: IRQ output (wire to PLIC source)
    */
    qemu_irq cs_lines[3];

    /* registers */
    uint32_t cr1;
    uint32_t cr2;
    uint32_t sr;
    uint32_t dr;
    uint32_t csctrl;

    /* internal state */
    uint8_t  rx_data;   /* last received byte */
    bool spe, mstr;
    bool cs0_en, cs0_act;
    bool cs1_en, cs1_act;
    
    /* interrupt enable register (reuse CR2 like test uses) */
    /* bits: TXEIE (7), RXNEIE (6), ERRIE (5). We store entire cr2 value. */
} G233SPIState;
#endif /* HW_SSI_G233_SPI_H */