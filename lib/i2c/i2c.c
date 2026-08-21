#include "i2c.h"
#include <stdio.h>
#include "sam.h"
//#include "debug.h"

void i2c_setup(void){
    // Turn on power
    PM->APBCMASK.bit.SERCOM2_ = 1;

    // Enable clock
    GCLK->CLKCTRL.reg = 
        GCLK_CLKCTRL_ID(GCLK_CLKCTRL_ID_SERCOM2_CORE) |
        GCLK_CLKCTRL_GEN(GCLK_CLKCTRL_GEN_GCLK0) |
        GCLK_CLKCTRL_CLKEN;

    while (GCLK->STATUS.bit.SYNCBUSY);

    // Define pins
    PORT->Group[0].PINCFG[8].reg = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;
    PORT->Group[0].PINCFG[9].reg = PORT_PINCFG_PMUXEN | PORT_PINCFG_INEN;

    PORT->Group[0].PMUX[8/2].bit.PMUXE = PORT_PMUX_PMUXE_D_Val; // PA08
    PORT->Group[0].PMUX[9/2].bit.PMUXO = PORT_PMUX_PMUXO_D_Val; // PA09

    // Software reset
    SERCOM2->I2CM.CTRLA.bit.SWRST = 1;
    while (SERCOM2->I2CM.SYNCBUSY.bit.SWRST);

    // Configure CTRLA
    SERCOM2->I2CM.CTRLA.reg = 
        SERCOM_I2CM_CTRLA_MODE_I2C_MASTER |
        SERCOM_I2CM_CTRLA_SPEED(0) |
        SERCOM_I2CM_CTRLA_SDAHOLD(0x2);
                               
    // Configure CTRLB
    SERCOM2->I2CM.CTRLB.reg = SERCOM_I2CM_CTRLB_SMEN;
    while (SERCOM2->I2CM.SYNCBUSY.bit.SYSOP);

    // Set BAUD
    SERCOM2->I2CM.BAUD.reg = 235;

    // Enable SERCOM
    SERCOM2->I2CM.CTRLA.bit.ENABLE = 1;
    while (SERCOM2->I2CM.SYNCBUSY.bit.ENABLE);

    // Set bus state to IDLE
    SERCOM2->I2CM.STATUS.bit.BUSSTATE = 1;
    while (SERCOM2->I2CM.SYNCBUSY.bit.SYSOP);
}

/*
return 0 - error
return 1 - success
*/
int i2c_write_read(uint8_t addr, uint8_t reg, uint8_t *buff, uint32_t size) {
    if (size == 0) return 0;

    SERCOM2->I2CM.ADDR.bit.ADDR = (addr << 1) | 0;  // Write command
    while (!SERCOM2->I2CM.INTFLAG.bit.MB);
    if (error_check()) return 0;

    SERCOM2->I2CM.DATA.reg = reg;               // Send register to read
    while (!SERCOM2->I2CM.INTFLAG.bit.MB);
    if (error_check()) return 0;

    SERCOM2->I2CM.ADDR.bit.ADDR = (addr << 1) | 1;  // Read command
    if (size == 1) {
        // Single byte handling
        while (!SERCOM2->I2CM.INTFLAG.bit.SB);
        if (error_check()) return 0;
        SERCOM2->I2CM.CTRLB.bit.ACKACT = 1;
        SERCOM2->I2CM.CTRLB.bit.CMD = 3;
        while (SERCOM2->I2CM.SYNCBUSY.bit.SYSOP);
        buff[0] = SERCOM2->I2CM.DATA.reg;
    }
    else {
        // More than one byte handling
        for (uint32_t i = 0; i < size-1; i++) {
            while (!SERCOM2->I2CM.INTFLAG.bit.SB);
            if (error_check()) return 0;
            SERCOM2->I2CM.CTRLB.bit.ACKACT = 0;
            SERCOM2->I2CM.CTRLB.bit.CMD = 2;
            while (SERCOM2->I2CM.SYNCBUSY.bit.SYSOP);
            buff[i] = SERCOM2->I2CM.DATA.reg;
        }
        // Last byte
        while (!SERCOM2->I2CM.INTFLAG.bit.SB);
        if (error_check()) return 0;
        SERCOM2->I2CM.CTRLB.bit.ACKACT = 1;
        SERCOM2->I2CM.CTRLB.bit.CMD = 3;
        while (SERCOM2->I2CM.SYNCBUSY.bit.SYSOP);
        buff[size-1] = SERCOM2->I2CM.DATA.reg;
    }

    return 1;
}

bool i2c_reg_write(uint8_t addr, uint8_t reg, uint8_t data) {
    SERCOM2->I2CM.ADDR.bit.ADDR = (addr << 1) | 0;  // Write command
    while (!SERCOM2->I2CM.INTFLAG.bit.MB);
    if (error_check()) return false;

    SERCOM2->I2CM.DATA.reg = reg;               // Send register to write to
    while (!SERCOM2->I2CM.INTFLAG.bit.MB);
    if (error_check()) return false;

    SERCOM2->I2CM.DATA.reg = data;              // Send data to write
    while (!SERCOM2->I2CM.INTFLAG.bit.MB);
    if (error_check()) return false;

    SERCOM2->I2CM.CTRLB.bit.CMD = 3;
    while (SERCOM2->I2CM.SYNCBUSY.bit.SYSOP);
    SERCOM2->I2CM.CTRLB.bit.ACKACT = 0;

    return true;
}


int error_check(void){
    if (SERCOM2->I2CM.STATUS.bit.RXNACK) {
        SERCOM2->I2CM.CTRLB.bit.CMD = 3;    // Issue stop
        while (SERCOM2->I2CM.SYNCBUSY.bit.SYSOP);
        return 1;
    }
    return 0;
}