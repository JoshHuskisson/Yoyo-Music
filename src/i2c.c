#include "i2c.h"
#include <stdio.h>
#include "sam.h"

void setup(void){
    // Turn on power
    PM->APBCMASK.bit.SERCOM0_ = 1;

    // Enable clock
    GCLK->CLKCTRL.reg = 
        GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_SERCOM0_CORE) |
        GCLK_CLKCTRL_GEN(GCLK_CLKCTRL_GEN_GCLK0) |
        GCLK_CLKCTRL_CLKEN;

    while (GCLK->STATUS.bit.SYNCBUSY);

    // Define pins
    PORT->Group[0].PINCFG[8].reg = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;
    PORT->Group[0].PINCFG[9].reg = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;

    PORT->Group[0].PMUX[8/2].bit.PMUXE = PORT_PMUX_PMUXE_C_Val; // PA08
    PORT->Group[0].PMUX[9/2].bit.PMUXO = PORT_PMUX_PMUXO_C_Val; // PA09

    // Software reset
    SERCOM0->I2CM.CTRLA.bit.SWRST = 1;
    while (SERCOM0->I2CM.SYNCBUSY.bit.SWRST);

    // Configure CTRLA
    SERCOM0->I2CM.CTRLA.reg = 
        SERCOM_I2CM_CTRLA_MODE_I2C_MASTER |
        SERCOM_I2CM_CTRLA_SPEED(0) |
        SERCOM_I2CM_CTRLA_SDAHOLD(0x2);
                               
    // Configure CTRLB
    SERCOM0->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;
    while (SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);

    // Set BAUD
    SERCOM0->I2CM.BAUD.reg = 235;

    // Enable SERCOM
    SERCOM0->I2CM.CTRLA.bit.ENABLE = 1;
    while (SERCOM0->I2CM.SYNCBUSY.bit.ENABLE);

    // Set bus state to IDLE
    SERCOM0->I2CM.STATUS.bit.BUSSTATE = 1;
    while (SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);
}

int write_read(uint8_t addr, uint8_t reg, uint8_t *buff, int size) {
    SERCOM0->I2CM.ADDR.bit.ADDR = (addr << 1) | 0;  // Write command
    while (!SERCOM0->I2CM.INTFLAG.bit.MB);
    if (SERCOM0->I2CM.STATUS.bit.RXNACK) {
        SERCOM0->I2CM.CTRLB.bit.CMD = 3;    // Issue stop
        while (SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);
        return 0;
    }

    SERCOM0->I2CM.DATA.reg = reg;               // Send register to read
    while (!SERCOM0->I2CM.INTFLAG.bit.MB);
    if (SERCOM0->I2CM.STATUS.bit.RXNACK) {
        SERCOM0->I2CM.CTRLB.bit.CMD = 3;    // Issue stop
        while (SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);
        return 0;
    }

    SERCOM0->I2CM.ADDR.bit.ADDR = (addr << 1) | 1;  // Read command
    while (!SERCOM0->I2CM.INTFLAG.bit.SB);
    if (SERCOM0->I2CM.STATUS.bit.RXNACK) {
        SERCOM0->I2CM.CTRLB.bit.CMD = 3;    // Issue stop
        while (SERCOM0->I2CM.SYNCBUSY.bit.SYSOP);
        return 0;
    }
    for (int i = 1; i < size; i++) {
        buff[i-1] = SERCOM0->I2CM.DATA.reg;
    }
    // set NACK CONDITION
    // read from DATA again

    
}