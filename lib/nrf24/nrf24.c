#include "nrf24.h"
#include "spi_driver.h"
#include <sam.h>
#include "delay.h"
//#include "debug.h"

#define CE_PIN 8
#define CE_GROUP 1
#define CS_PIN 9
#define CS_GROUP 1
#define NRF24_CHANNEL 76
#define NRF24_STARTUP_TIME_US 5000
#define ADDRESS_WIDTH_5_BYTES 0x03
static uint8_t nrf24_payload_size;

/* Memory Map */
#define CONFIG      0x00
#define EN_AA       0x01
#define EN_RXADDR   0x02
#define SETUP_AW    0x03
#define SETUP_RETR  0x04
#define RF_CH       0x05
#define RF_SETUP    0x06
#define STATUS      0x07
#define OBSERVE_TX  0x08
#define CD          0x09
#define RX_ADDR_P0  0x0A
#define RX_ADDR_P1  0x0B
#define RX_ADDR_P2  0x0C
#define RX_ADDR_P3  0x0D
#define RX_ADDR_P4  0x0E
#define RX_ADDR_P5  0x0F
#define TX_ADDR     0x10
#define RX_PW_P0    0x11
#define RX_PW_P1    0x12
#define RX_PW_P2    0x13
#define RX_PW_P3    0x14
#define RX_PW_P4    0x15
#define RX_PW_P5    0x16
#define FIFO_STATUS 0x17
#define DYNPD       0x1C
#define FEATURE     0x1D

/* Bit Mnemonics */
#define MASK_RX_DR  6
#define MASK_TX_DS  5
#define MASK_MAX_RT 4
#define EN_CRC      3
#define CRCO        2
#define PWR_UP      1
#define PRIM_RX     0
#define ENAA_P5     5
#define ENAA_P4     4
#define ENAA_P3     3
#define ENAA_P2     2
#define ENAA_P1     1
#define ENAA_P0     0
#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0
#define AW          0
#define ARD         4
#define ARC         0
#define PLL_LOCK    4
#define CONT_WAVE   7
#define RF_DR       3
#define RF_PWR      1
#define RX_DR       6
#define TX_DS       5
#define MAX_RT      4
#define RX_P_NO     1
#define TX_FULL     0
#define PLOS_CNT    4
#define ARC_CNT     0
#define TX_REUSE    6
#define FIFO_FULL   5
#define TX_EMPTY    4 
#define RX_FULL     1
#define RX_EMPTY    0
#define DPL_P5      5
#define DPL_P4      4
#define DPL_P3      3
#define DPL_P2      2
#define DPL_P1      1
#define DPL_P0      0
#define EN_DPL      2
#define EN_ACK_PAY  1
#define EN_DYN_ACK  0

/* Instruction Mnemonics */
#define R_REGISTER      0x00
#define W_REGISTER      0x20
#define REGISTER_MASK   0x1F
#define ACTIVATE        0x50
#define R_RX_PL_WID     0x60
#define R_RX_PAYLOAD    0x61
#define W_TX_PAYLOAD    0xA0
#define W_ACK_PAYLOAD   0xA8
#define FLUSH_TX        0xE1
#define FLUSH_RX        0xE2
#define REUSE_TX_PL     0xE3
#define NOP             0xFF

static const uint8_t tx_addr[5] = {
    0xA1,
    0xB2, 
    0xC3, 
    0xD4, 
    0xE5
};

static const uint8_t rx_addr_p0[5] = {
    0xA1,
    0xB2, 
    0xC3, 
    0xD4, 
    0xE5
};


static void cs_low(void) {
    PORT->Group[CS_GROUP].OUTCLR.reg = (1 << CS_PIN);
}

static void ce_low(void) {
    PORT->Group[CE_GROUP].OUTCLR.reg = (1 << CE_PIN);
}

static void cs_high(void) {
    PORT->Group[CS_GROUP].OUTSET.reg = (1 << CS_PIN);
}

static void ce_high(void) {
    PORT->Group[CE_GROUP].OUTSET.reg = (1 << CE_PIN);
}


void nrf24_setup(void) {
    spi_setup();

    PM->APBBMASK.bit.PORT_ = 1;
    // PB8: CE
    // PB9: CSN
    PORT->Group[CE_GROUP].PINCFG[CE_PIN].bit.PMUXEN = 0;
    PORT->Group[CS_GROUP].PINCFG[CS_PIN].bit.PMUXEN = 0;
    PORT->Group[CE_GROUP].DIRSET.reg = (1 << CE_PIN);
    PORT->Group[CS_GROUP].DIRSET.reg = (1 << CS_PIN);
    // Set initial states
    PORT->Group[CE_GROUP].OUTCLR.reg = (1 << CE_PIN);
    PORT->Group[CS_GROUP].OUTSET.reg = (1 << CS_PIN);
}

bool nrf24_init_tx (uint8_t payload_size) {
    nrf24_setup();
    spi_command(FLUSH_RX);
    spi_command(FLUSH_TX);
    spi_write_reg(STATUS, 
                 (1 << RX_DR) |
                 (1 << TX_DS) |
                 (1 << MAX_RT));
    spi_write_reg(EN_AA, (1 << ENAA_P0));       // Auto Ack pipe 0
    spi_write_reg(SETUP_AW, ADDRESS_WIDTH_5_BYTES);

    uint8_t setup_rtr = (2 << ARD) | (3 << ARC);    // wait 750us retransmit 3
    spi_write_reg(SETUP_RETR, setup_rtr); 
    spi_write_reg(RF_CH, NRF24_CHANNEL);

    uint8_t rf_set = (0 << RF_DR) | (3 << RF_PWR);
    spi_write_reg(RF_SETUP, rf_set);
    spi_multi_write_reg(TX_ADDR, tx_addr, 5);
    spi_multi_write_reg(RX_ADDR_P0, rx_addr_p0, 5);

    nrf24_payload_size = payload_size;
    spi_write_reg(RX_PW_P0, nrf24_payload_size);
    spi_write_reg(EN_RXADDR, (1 << ERX_P0));
    uint8_t config = (1 << PWR_UP) | 
                     (0 << PRIM_RX) | 
                     (1 << EN_CRC);     // CRC 1 byte         
    spi_write_reg(CONFIG, config);
    delay_us(NRF24_STARTUP_TIME_US);

    if (spi_read_reg(CONFIG) != config) {
        return false;
    }
    
    return true;
}

bool nrf24_init_rx (uint8_t payload_size) {
    nrf24_setup();
    spi_command(FLUSH_RX);
    spi_command(FLUSH_TX);
    spi_write_reg(STATUS, 
                 (1 << RX_DR) |
                 (1 << TX_DS) |
                 (1 << MAX_RT));
    spi_write_reg(EN_AA, (1 << ENAA_P0));       // Auto Ack pipe 0
    spi_write_reg(SETUP_AW, ADDRESS_WIDTH_5_BYTES);

    uint8_t setup_rtr = (2 << ARD) | (3 << ARC);    // wait 750us retransmit 3
    spi_write_reg(SETUP_RETR, setup_rtr);
    spi_write_reg(RF_CH, NRF24_CHANNEL);

    uint8_t rf_set = (0 << RF_DR) | (3 << RF_PWR);
    spi_write_reg(RF_SETUP, rf_set);
    spi_multi_write_reg(RX_ADDR_P0, rx_addr_p0, 5);

    nrf24_payload_size = payload_size;
    spi_write_reg(RX_PW_P0, nrf24_payload_size);
    spi_write_reg(EN_RXADDR, (1 << ERX_P0));
    uint8_t config = (1 << PWR_UP) | 
                     (1 << PRIM_RX) | 
                     (1 << EN_CRC);    // CRCO 1 byte
    spi_write_reg(CONFIG, config);
    delay_us(NRF24_STARTUP_TIME_US);
    
    if (spi_read_reg(CONFIG) != config) {
        return false;
    }

    ce_high();
    
    return true;
}

uint8_t spi_multi_write_reg(uint8_t reg, uint8_t *data, int size){
    cs_low();

    uint8_t status = spi_transfer(W_REGISTER | reg);
    for (int i = 0; i < size; i++){
        spi_transfer(data[i]);
    }

    cs_high();

    return status;
}

uint8_t spi_write_reg(uint8_t reg, uint8_t data) {
    cs_low();

    spi_transfer(W_REGISTER | reg);
    uint8_t status = spi_transfer(data);

    cs_high();

    return status;
}

uint8_t spi_read_reg(uint8_t reg) {
    cs_low();

    spi_transfer(R_REGISTER | reg);
    uint8_t read_data = spi_transfer(NOP);

    cs_high();

    return read_data;
}

uint8_t spi_command(uint8_t command) {
    cs_low();

    uint8_t status = spi_transfer(command);

    cs_high();

    return status;
}

bool nrf24_send(const void *buffer, uint8_t length) {
    const uint8_t *data = (const uint8_t *)buffer;

    if(length != nrf24_payload_size) {
        return false;
    }
    
    cs_low();

    spi_transfer(W_TX_PAYLOAD);
    for(int i = 0; i < length; i++) {
        spi_transfer(data[i]);
    }

    cs_high();

    uint8_t fifo_status = spi_read_reg(FIFO_STATUS);

    ce_high();
    delay_us(50);
    ce_low();

    uint8_t status = spi_read_reg(STATUS);
    while(!(status & (1 << TX_DS)) && !(status & (1 << MAX_RT))){
        status = spi_read_reg(STATUS);
    }

    if(status & (1 << TX_DS)) {
        spi_write_reg(STATUS, 
                     (1 << TX_DS) | 
                     (1 << MAX_RT));
        return true;
    }
    else {
        spi_write_reg(STATUS, 
                     (1 << TX_DS) | 
                     (1 << MAX_RT));
        spi_command(FLUSH_TX);
        return false;
    }
}

bool nrf24_receive(void *buff, uint8_t length) {
    uint8_t *buffer = (uint8_t *)buff;
    uint8_t status = spi_read_reg(STATUS);

    if(!(status & (1 << RX_DR))){   // If there's no new data
        return false;
    }

    cs_low();
    spi_transfer(R_RX_PAYLOAD);

    for (int i = 0; i < nrf24_payload_size; i++) {
        buffer[i] = spi_transfer(NOP);
    }

    cs_high();

    spi_write_reg(STATUS, (1 << RX_DR));

    return true;

}
