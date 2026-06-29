#include "spi_driver.h"
#include <stdio.h>
#include "sam.h"

void spi_setup(void){
    // Turn on power
    PM->APBCMASK.bit.SERCOM0_ = 1;

    // Enable clock
    GCLK->CLKCTRL.reg = 
        GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_SERCOM0_CORE) |
        GCLK_CLKCTRL_GEN(GCLK_CLKCTRL_GEN_GCLK0) |
        GCLK_CLKCTRL_CLKEN;

    while (GCLK->STATUS.bit.SYNCBUSY);

    // Define pins
    // PA5: MISO
    // PA6: MOSI
    // PA7: SCK
    PORT->Group[0].PINCFG[5].reg = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;
    PORT->Group[0].PINCFG[6].reg = PORT_PINCFG_PMUXEN;
    PORT->Group[0].PINCFG[7].reg = PORT_PINCFG_PMUXEN;

    PORT->Group[0].PMUX[5/2].bit.PMUXO = PORT_PMUX_PMUXO_D_Val; // PA05
    PORT->Group[0].PMUX[6/2].bit.PMUXE = PORT_PMUX_PMUXE_D_Val; // PA06
    PORT->Group[0].PMUX[7/2].bit.PMUXO = PORT_PMUX_PMUXO_D_Val; // PA07

    // Software reset
    SERCOM0->SPI.CTRLA.bit.SWRST = 1;
    while (SERCOM0->SPI.CTRLA.bit.SWRST ||
           SERCOM0->SPI.SYNCBUSY.bit.SWRST);

    // Configure CTRLA
    SERCOM0->SPI.CTRLA.reg = 
        SERCOM_SPI_CTRLA_MODE_SPI_MASTER |
        SERCOM_SPI_CTRLA_DOPO(1) |
        SERCOM_SPI_CTRLA_DIPO(1);
        // CPHA = 0
        // CPOL = 0
        // DORD = 0
    
    // Configure CTRLB
    SERCOM0->SPI.CTRLB.reg = SERCOM_SPI_CTRLB_RXEN;
    while (SERCOM0->SPI.SYNCBUSY.bit.CTRLB);

    // Set BAUD
    SERCOM0->SPI.BAUD.reg = 5;  // 4Mhz SPI Clock

    // Enable SERCOM
    SERCOM0->SPI.CTRLA.bit.ENABLE = 1;
    while(SERCOM0->SPI.SYNCBUSY.bit.ENABLE);
}

uint8_t spi_transfer(uint8_t txByte) {
    while (!SERCOM0->SPI.INTFLAG.bit.DRE);  // wait for data reg to clear
    SERCOM0->SPI.DATA.reg = txByte;
    while (!SERCOM0->SPI.INTFLAG.bit.RXC);  // wait for received data
    uint8_t rxByte = SERCOM0->SPI.DATA.reg;

    return rxByte;
}