#include "S32K142.h"
#include <stdint.h>
#include <stdbool.h>

#define BME_CS_PORT IP_PTB
#define BME_CS_PIN  5

static uint8_t current_spi_page = 0xFF;
static double ambient_temperature = 25.0; // Tracked for gas heater calculations

// Complete factory calibration structure including Gas[cite: 1]
struct bme680_calib_data {
    uint16_t par_t1; int16_t par_t2; int8_t par_t3;
    uint16_t par_p1; int16_t par_p2; int8_t par_p3; int16_t par_p4; int16_t par_p5;
    int8_t par_p6; int8_t par_p7; int16_t par_p8; int16_t par_p9; uint8_t par_p10;
    uint16_t par_h1; uint16_t par_h2; int8_t par_h3; int8_t par_h4; int8_t par_h5;
    uint8_t par_h6; int8_t par_h7;
    int8_t par_g1; int16_t par_g2; int8_t par_g3;
    uint8_t res_heat_range; int8_t res_heat_val; int8_t range_sw_err;
} cal;

// Gas Resistance Math Constants[cite: 1]
static const double const_array1[16] = {1.0, 1.0, 1.0, 1.0, 1.0, 0.99, 1.0, 0.992, 1.0, 1.0, 0.998, 0.995, 1.0, 0.99, 1.0, 1.0};
static const double const_array2[16] = {8000000.0, 4000000.0, 2000000.0, 1000000.0, 499500.4995, 248262.1648, 125000.0, 63004.03226, 31281.28128, 15625.0, 7812.5, 3906.25, 1953.125, 976.5625, 488.28125, 244.140625};

static void bme680_cs_delay(void) {
    for(volatile int i = 0; i < 20; i++) __asm volatile("nop");
}

// =========================================================
// LOW LEVEL SPI HARDWARE
// =========================================================

void LPSPI0_Init(void) {
    IP_SCG->FIRCDIV |= SCG_FIRCDIV_FIRCDIV2(1);
    IP_PCC->PCCn[PCC_PORTB_INDEX] |= PCC_PCCn_CGC_MASK;
    IP_PCC->PCCn[PCC_LPSPI0_INDEX] &= ~PCC_PCCn_CGC_MASK;
    IP_PCC->PCCn[PCC_LPSPI0_INDEX] = PCC_PCCn_PCS(3) | PCC_PCCn_CGC_MASK;

    IP_PORTB->PCR[2] = PORT_PCR_MUX(3) | PORT_PCR_DSE_MASK;
    IP_PORTB->PCR[3] = PORT_PCR_MUX(3) | PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
    IP_PORTB->PCR[4] = PORT_PCR_MUX(3) | PORT_PCR_DSE_MASK;
    IP_PORTB->PCR[5] = PORT_PCR_MUX(1) | PORT_PCR_DSE_MASK;

    BME_CS_PORT->PDDR |= (1 << BME_CS_PIN);
    BME_CS_PORT->PSOR = (1 << BME_CS_PIN);

    IP_LPSPI0->CR = 0x00;
    IP_LPSPI0->CR |= LPSPI_CR_RTF_MASK | LPSPI_CR_RRF_MASK;
    IP_LPSPI0->CFGR1 = LPSPI_CFGR1_MASTER_MASK;
    IP_LPSPI0->CCR = LPSPI_CCR_SCKDIV(46);
    IP_LPSPI0->CR |= LPSPI_CR_MEN_MASK;
    IP_LPSPI0->TCR = LPSPI_TCR_FRAMESZ(7) | LPSPI_TCR_CPOL(0) | LPSPI_TCR_CPHA(0);

    for(volatile int i=0; i<10000; i++) __asm volatile("nop");
    BME_CS_PORT->PCOR = (1 << BME_CS_PIN);
    for(volatile int i=0; i<10000; i++) __asm volatile("nop");
    BME_CS_PORT->PSOR = (1 << BME_CS_PIN);
    for(volatile int i=0; i<10000; i++) __asm volatile("nop");
}

uint8_t LPSPI0_Transfer(uint8_t data) {
    IP_LPSPI0->SR = LPSPI_SR_RDF_MASK | LPSPI_SR_TCF_MASK | LPSPI_SR_TEF_MASK;
    while((IP_LPSPI0->SR & LPSPI_SR_TDF_MASK) == 0);
    IP_LPSPI0->TDR = data;
    while((IP_LPSPI0->SR & LPSPI_SR_RDF_MASK) == 0);
    return (uint8_t)IP_LPSPI0->RDR;
}

// =========================================================
// BME680 PROTOCOL & MATH
// =========================================================

static void bme680_set_page(uint8_t reg_addr) {
    uint8_t target_page = (reg_addr > 0x7F) ? 0 : 1;
    if (target_page != current_spi_page) {
        BME_CS_PORT->PCOR = (1 << BME_CS_PIN); bme680_cs_delay();
        LPSPI0_Transfer(0x73 & 0x7F); LPSPI0_Transfer(target_page << 4);
        bme680_cs_delay(); BME_CS_PORT->PSOR = (1 << BME_CS_PIN);
        current_spi_page = target_page; bme680_cs_delay();
    }
}

uint8_t bme680_read_register(uint8_t reg_addr) {
    bme680_set_page(reg_addr);
    BME_CS_PORT->PCOR = (1 << BME_CS_PIN); bme680_cs_delay();
    LPSPI0_Transfer(reg_addr | 0x80);
    uint8_t data = LPSPI0_Transfer(0x00);
    bme680_cs_delay(); BME_CS_PORT->PSOR = (1 << BME_CS_PIN);
    return data;
}

void bme680_write_register(uint8_t reg_addr, uint8_t data) {
    bme680_set_page(reg_addr);
    BME_CS_PORT->PCOR = (1 << BME_CS_PIN); bme680_cs_delay();
    LPSPI0_Transfer(reg_addr & 0x7F); LPSPI0_Transfer(data);
    bme680_cs_delay(); BME_CS_PORT->PSOR = (1 << BME_CS_PIN);
}

void bme680_read_burst(uint8_t start_addr, uint8_t *buffer, uint8_t len) {
    bme680_set_page(start_addr);
    BME_CS_PORT->PCOR = (1 << BME_CS_PIN); bme680_cs_delay();
    LPSPI0_Transfer(start_addr | 0x80);
    for(int i = 0; i < len; i++) buffer[i] = LPSPI0_Transfer(0x00);
    bme680_cs_delay(); BME_CS_PORT->PSOR = (1 << BME_CS_PIN);
}

// Calculates dynamic Gas Wait Time multiplier[cite: 1]
static uint8_t bme680_calc_heater_duration(uint16_t duration_ms) {
    uint8_t factor = 0;
    while (duration_ms > 0x3F) {
        duration_ms /= 4;
        factor++;
    }
    return (uint8_t)(duration_ms + (factor * 64));
}

// Translates target temperature into device-specific heater resistance code[cite: 1]
static uint8_t bme680_calc_heater_res(uint16_t target_temp_c) {
    double var1 = ((double)cal.par_g1 / 16.0) + 49.0;
    double var2 = (((double)cal.par_g2 / 32768.0) * 0.0005) + 0.00235;
    double var3 = (double)cal.par_g3 / 1024.0;
    double var4 = var1 * (1.0 + (var2 * (double)target_temp_c));
    double var5 = var4 + (var3 * ambient_temperature);
    return (uint8_t)(3.4 * ((var5 * (4.0 / (4.0 + (double)cal.res_heat_range)) * (1.0 / (1.0 + ((double)cal.res_heat_val * 0.002)))) - 25));
}

void bme680_init_sensor(void) {
    bme680_write_register(0xE0, 0xB6); // Soft Reset
    for(volatile int i=0; i<100000; i++) __asm volatile("nop");

    // Read T, P, H Calibration Data
    cal.par_t1 = (bme680_read_register(0xEA) << 8) | bme680_read_register(0xE9);
    cal.par_t2 = (bme680_read_register(0x8B) << 8) | bme680_read_register(0x8A);
    cal.par_t3 = bme680_read_register(0x8C);

    cal.par_p1 = (bme680_read_register(0x8F) << 8) | bme680_read_register(0x8E);
    cal.par_p2 = (bme680_read_register(0x91) << 8) | bme680_read_register(0x90);
    cal.par_p3 = bme680_read_register(0x92);
    cal.par_p4 = (bme680_read_register(0x95) << 8) | bme680_read_register(0x94);
    cal.par_p5 = (bme680_read_register(0x97) << 8) | bme680_read_register(0x96);
    cal.par_p6 = bme680_read_register(0x99);
    cal.par_p7 = bme680_read_register(0x98);
    cal.par_p8 = (bme680_read_register(0x9D) << 8) | bme680_read_register(0x9C);
    cal.par_p9 = (bme680_read_register(0x9F) << 8) | bme680_read_register(0x9E);
    cal.par_p10 = bme680_read_register(0xA0);

    uint8_t e1 = bme680_read_register(0xE1), e2 = bme680_read_register(0xE2), e3 = bme680_read_register(0xE3);
    cal.par_h1 = (e3 << 4) | (e2 & 0x0F);
    cal.par_h2 = (e1 << 4) | (e2 >> 4);
    cal.par_h3 = bme680_read_register(0xE4);
    cal.par_h4 = bme680_read_register(0xE5);
    cal.par_h5 = bme680_read_register(0xE7);
    cal.par_h6 = bme680_read_register(0xE6);
    cal.par_h7 = bme680_read_register(0xE8);

    // Read Gas Calibration Data[cite: 1]
    cal.par_g1 = (int8_t)bme680_read_register(0xED);
    cal.par_g2 = (int16_t)((bme680_read_register(0xEC) << 8) | bme680_read_register(0xEB));
    cal.par_g3 = (int8_t)bme680_read_register(0xEE);
    cal.res_heat_range = (bme680_read_register(0x02) & 0x30) >> 4;
    cal.res_heat_val = (int8_t)bme680_read_register(0x00);

    // Range switching error is a 4-bit signed integer in the upper nibble[cite: 1]
    int8_t raw_err = bme680_read_register(0x04);
    cal.range_sw_err = (raw_err & 0x80) ? ((raw_err >> 4) | 0xF0) : (raw_err >> 4);
}

// Fetches all 4 parameters simultaneously using a 15-byte burst read
void bme680_get_data(double *temperature, double *pressure, double *humidity, double *gas_res) {
    // 1. Setup Gas Heater Profile 0 (e.g., 320°C for 150ms)[cite: 1]
    bme680_write_register(0x5A, bme680_calc_heater_res(320)); // res_heat_0[cite: 1]
    bme680_write_register(0x64, bme680_calc_heater_duration(150)); // gas_wait_0[cite: 1]
    bme680_write_register(0x71, 0x10); // Enable Gas (run_gas = 1), Select Profile 0[cite: 1]

    // 2. Setup Oversampling: Hum 1x, Temp 2x, Press 16x[cite: 1]
    bme680_write_register(0x72, 0x01);

    // 3. Trigger Forced Mode (mode = 01)[cite: 1]
    bme680_write_register(0x74, 0x55);

    // 4. Wait for New Data Flag (Bit 7 in 0x1D)[cite: 1]
    while ((bme680_read_register(0x1D) & 0x80) == 0);

    // 5. Burst Read 15 bytes starting from 0x1D (Status, Press, Temp, Hum, Gas)[cite: 1]
    uint8_t raw[15];
    bme680_read_burst(0x1D, raw, 15);

    uint32_t press_adc = ((uint32_t)raw[2] << 12) | ((uint32_t)raw[3] << 4) | (raw[4] >> 4);
    uint32_t temp_adc  = ((uint32_t)raw[5] << 12) | ((uint32_t)raw[6] << 4) | (raw[7] >> 4);
    uint16_t hum_adc   = ((uint16_t)raw[8] << 8)  | raw[9];
    uint16_t gas_adc   = ((uint16_t)raw[13] << 2) | (raw[14] >> 6);
    uint8_t  gas_range = raw[14] & 0x0F;
    bool gas_valid     = (raw[14] & 0x20) != 0;

    // 6. Calculate Temperature[cite: 1]
    double var1 = (((double)temp_adc / 16384.0) - ((double)cal.par_t1 / 1024.0)) * (double)cal.par_t2;
    double var2 = ((((double)temp_adc / 131072.0) - ((double)cal.par_t1 / 8192.0)) *
                   (((double)temp_adc / 131072.0) - ((double)cal.par_t1 / 8192.0))) * ((double)cal.par_t3 * 16.0);
    double t_fine = var1 + var2;
    *temperature = t_fine / 5120.0;
    ambient_temperature = *temperature; // Save for next heater calibration

    // 7. Calculate Pressure[cite: 1]
    var1 = (t_fine / 2.0) - 64000.0;
    var2 = var1 * var1 * ((double)cal.par_p6 / 131072.0);
    var2 = var2 + (var1 * (double)cal.par_p5 * 2.0);
    var2 = (var2 / 4.0) + ((double)cal.par_p4 * 65536.0);
    var1 = (((double)cal.par_p3 * var1 * var1) / 16384.0) + (((double)cal.par_p2 * var1) / 524288.0);
    var1 = (1.0 + (var1 / 32768.0)) * (double)cal.par_p1;
    double press_comp = 1048576.0 - (double)press_adc;
    if (var1 != 0.0) {
        press_comp = ((press_comp - (var2 / 4096.0)) * 6250.0) / var1;
        var1 = ((double)cal.par_p9 * press_comp * press_comp) / 2147483648.0;
        var2 = press_comp * ((double)cal.par_p8 / 32768.0);
        double var3 = (press_comp / 256.0) * (press_comp / 256.0) * (press_comp / 256.0) * ((double)cal.par_p10 / 131072.0);
        press_comp = press_comp + (var1 + var2 + var3 + ((double)cal.par_p7 * 128.0)) / 16.0;
    }
    *pressure = press_comp / 100.0; // Return as hPa

    // 8. Calculate Humidity[cite: 1]
    double calc_temp = *temperature;
    var1 = hum_adc - (((double)cal.par_h1 * 16.0) + (((double)cal.par_h3 / 2.0) * calc_temp));
    var2 = var1 * (((double)cal.par_h2 / 262144.0) * (1.0 + (((double)cal.par_h4 / 16384.0) * calc_temp) + (((double)cal.par_h5 / 1048576.0) * calc_temp * calc_temp)));
    double var3 = (double)cal.par_h6 / 16384.0;
    double var4 = (double)cal.par_h7 / 2097152.0;
    *humidity = var2 + ((var3 + (var4 * calc_temp)) * var2 * var2);

    // 9. Calculate Gas Resistance (Ohms)[cite: 1]
    if (gas_valid) {
        var1 = (1340.0 + 5.0 * (double)cal.range_sw_err) * const_array1[gas_range];
        *gas_res = (var1 * const_array2[gas_range]) / ((double)gas_adc - 512.0 + var1);
    } else {
        *gas_res = 0.0; // Gas measurement failed or not ready
    }
}
