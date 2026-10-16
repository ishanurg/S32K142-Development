#include "inf.h"
#include "lpi2c.h"
#include "S32K142.h"

// --- ARM Cortex-M4 Cycle Counter for Perfect Timing ---
#define ARM_CM_DWT_CYCCNT (*((volatile uint32_t *)0xE0001004))
#define CYCLES_PER_MS 48000 // FIXED: S32K142 Default 48 MHz Clock
#define CYCLES_PER_US 48

static void inf_hw_delay_ms(uint32_t ms) {
    uint32_t start = ARM_CM_DWT_CYCCNT;
    uint32_t ticks = ms * CYCLES_PER_MS;
    while ((ARM_CM_DWT_CYCCNT - start) < ticks) {}
}

static void inf_hw_delay_us(uint32_t us) {
    uint32_t start = ARM_CM_DWT_CYCCNT;
    uint32_t ticks = us * CYCLES_PER_US;
    while ((ARM_CM_DWT_CYCCNT - start) < ticks) {}
}

// Constants and Filter Globals
#define TCI_CMD_CONC    0xA8
#define TCI_CMD_TEMP    0xA9
#define TCI_CMD_READ_ID 0xC2

static double leaky_acc_1 = 0.0;
static double leaky_acc_2 = 0.0;
static const double jmp_thresh = 10.0;
static const double samp_proxy = 0.02;
static const double jmp_thresh_2 = 15.0;
static const double samp_proxy_2 = 0.01;
static bool first_sample = true;

static uint16_t inf_crc16(const uint8_t *data, uint8_t len) {
    uint16_t crc = 0xFFFF;
    for (uint8_t i = 0; i < len; i++) {
        crc ^= ((uint16_t)data[i]) << 8;
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x8000) crc = (uint16_t)((crc << 1) ^ 0x1021);
            else              crc = (uint16_t)(crc << 1);
        }
    }
    return crc;
}

static bool inf_write(const uint8_t *payload, uint8_t len) {
    uint8_t tx_buf[16];
    uint16_t crc = inf_crc16(payload, len);
    for (uint8_t i = 0; i < len; i++) { tx_buf[i] = payload[i]; }
    tx_buf[len] = (uint8_t)(crc >> 8);
    tx_buf[len + 1] = (uint8_t)(crc & 0xFF);
    return LPI2C0_transmit(INF_I2C_ADDR, tx_buf, len + 2);
}

static bool inf_read(uint8_t *buf, uint8_t len) {
    if(!LPI2C0_receive(INF_I2C_ADDR, buf, len)) return false;
    uint16_t crc = inf_crc16(buf, len - 2);
    uint16_t rx  = ((uint16_t)buf[len - 2] << 8) | buf[len - 1];
    return (crc == rx);
}

void INF_WakeSequence(void) {
    IP_LPI2C0->MCR &= ~LPI2C_MCR_MEN_MASK;
    IP_PORTA->PCR[3] = PORT_PCR_MUX(1) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    IP_PTA->PDDR |= (1 << 3);
    IP_PTA->PCOR = (1 << 3);
    inf_hw_delay_ms(2);
    IP_PTA->PSOR = (1 << 3);
    inf_hw_delay_us(50);
    IP_PORTA->PCR[3] = PORT_PCR_MUX(3) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    LPI2C0_init();
    inf_hw_delay_us(600);
}

void INF_InitLeaky(void) { first_sample = true; }

static double calculate_leaky(double raw_val, double current_leaky, double thresh, double proxy) {
    double diff = raw_val - current_leaky;
    if (diff < 0) diff = -diff;
    if (diff > thresh) return raw_val;
    else return ((raw_val - current_leaky) * proxy) + current_leaky;
}

// --- NEW: Split Trigger and Read functions for Multitasking ---
bool INF_TriggerConc(void) {
    uint8_t cmd[5] = {TCI_CMD_CONC, 0x03, 50, 25, 100};
    return inf_write(cmd, 5);
}

bool INF_ReadConcData(int16_t *out_raw) {
    uint8_t rx[5];
    if (inf_read(rx, 5)) {
        *out_raw = (int16_t)(((uint16_t)rx[1] << 8) | rx[2]);
        return true;
    }
    return false;
}

bool INF_TriggerTemp(void) {
    uint8_t cmd[1] = {TCI_CMD_TEMP};
    return inf_write(cmd, 1);
}

bool INF_ReadTempData(int8_t *out_temp) {
    uint8_t rx[4];
    if (inf_read(rx, 4)) {
        *out_temp = (int8_t)rx[1];
        return true;
    }
    return false;
}

// Legacy functions intact just in case
bool INF_ReadRawConc(int16_t *out_raw) {
    if (INF_TriggerConc()) { inf_hw_delay_ms(35); return INF_ReadConcData(out_raw); }
    return false;
}
bool INF_ReadTemp(int8_t *out_temp) {
    if (INF_TriggerTemp()) { inf_hw_delay_ms(2); return INF_ReadTempData(out_temp); }
    return false;
}

void INF_ProcessLeaky(int16_t raw_conc, double *out_leaky1, double *out_leaky2) {
    double raw_val = (double)raw_conc;
    if (first_sample) { leaky_acc_1 = raw_val; leaky_acc_2 = raw_val; first_sample = false; }
    else {
        leaky_acc_1 = calculate_leaky(raw_val, leaky_acc_1, jmp_thresh, samp_proxy);
        leaky_acc_2 = calculate_leaky(raw_val, leaky_acc_2, jmp_thresh_2, samp_proxy_2);
    }
    *out_leaky1 = leaky_acc_1; *out_leaky2 = leaky_acc_2;
}
