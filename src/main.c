#include "S32K142.h"
#include "uart.h"
#include "lpi2c.h"
#include "hdc3020.h"
#include "inf.h"
#include <stdbool.h>
#include <stdint.h>


volatile uint8_t rolling_window_size = 16;
#define MAX_WINDOW_SIZE 32


#define ARM_CM_DEMCR      (*((volatile uint32_t *)0xE000EDFC))
#define ARM_CM_DWT_CTRL   (*((volatile uint32_t *)0xE0001000))
#define ARM_CM_DWT_CYCCNT (*((volatile uint32_t *)0xE0001004))
#define ARM_CM4_SCB_CPACR (*((volatile uint32_t *)0xE000ED88))

#define CPU_CLOCK_HZ 48000000
#define CYCLES_PER_MS (CPU_CLOCK_HZ / 1000)


static uint32_t last_hw_cycles = 0;
static uint64_t accumulated_cycles = 0;
static uint8_t  dwt_is_operational = 0;


static float _sqrt_(float number) {
    if (number <= 0.0f) return 0.0f;


    union {
        float f;
        uint32_t i;
    } conv;

    conv.f = number;
    conv.i = 0x5f3759df - (conv.i >> 1);
    float y = conv.f;


    y = y * (1.5f - (number * 0.5f * y * y));
    y = y * (1.5f - (number * 0.5f * y * y));
    y = y * (1.5f - (number * 0.5f * y * y));


    return number * y;
}

typedef struct {
    float buffer[MAX_WINDOW_SIZE];
    uint8_t index;
    uint8_t count;
    uint8_t window_size;
}  rol_stat;


void  rol_stat_SetSize( rol_stat *rs, uint8_t new_size) {
    if (new_size > MAX_WINDOW_SIZE) new_size = MAX_WINDOW_SIZE;
    if (rs->window_size != new_size) {
        rs->window_size = new_size;
        rs->count = 0;
        rs->index = 0;
    }
}


void  rol_stat_Add( rol_stat *rs, float new_val) {
    rs->buffer[rs->index] = new_val;
    rs->index = (rs->index + 1) % rs->window_size;
    if (rs->count < rs->window_size) {
        rs->count++;
    }
}

// Calculate Standard Deviation chronologically
float  rol_stat_GetStdDev( rol_stat *rs) {
    if (rs->count < 2) return 0.0f;

    float sum = 0.0f;
    for (uint8_t i = 0; i < rs->count; i++) {
        sum += rs->buffer[i];
    }
    float mean = sum / rs->count;

    float sum_sq_diff = 0.0f;
    for (uint8_t i = 0; i < rs->count; i++) {
        float diff = rs->buffer[i] - mean;
        sum_sq_diff += diff * diff;
    }

    float variance = sum_sq_diff / (float)(rs->count - 1);

    // Call the compiler-proof bit-hacked square root
    return _sqrt_(variance);
}

// Extract Median without destroying main buffer order
float  rol_stat_GetMedian( rol_stat *rs) {
    if (rs->count == 0) return 0.0f;

    float temp[MAX_WINDOW_SIZE];
    for (uint8_t i = 0; i < rs->count; i++) {
        temp[i] = rs->buffer[i];
    }

    // Fast Insertion Sort for ascending order
    for (uint8_t i = 1; i < rs->count; i++) {
        float key = temp[i];
        int8_t j = i - 1;
        while (j >= 0 && temp[j] > key) {
            temp[j + 1] = temp[j];
            j--;
        }
        temp[j + 1] = key;
    }

    if (rs->count % 2 == 0) {
        return (temp[rs->count / 2 - 1] + temp[rs->count / 2]) * 0.5f;
    } else {
        return temp[rs->count / 2];
    }
}

uint64_t Get_Cycles64(void) {
    if (!dwt_is_operational) {
        static uint64_t dummy_ticker = 0;
        return dummy_ticker++;
    }
    uint32_t current_hw_cycles = ARM_CM_DWT_CYCCNT;
    if (current_hw_cycles < last_hw_cycles) {
        accumulated_cycles += 0x100000000ULL;
    }
    last_hw_cycles = current_hw_cycles;
    return accumulated_cycles + current_hw_cycles;
}

void HW_Delay_ms(uint32_t ms) {
    if (!dwt_is_operational) {
        volatile uint32_t count = ms * (CPU_CLOCK_HZ / 13000);
        while (count--) { __asm("nop"); }
        return;
    }
    uint64_t start = Get_Cycles64();
    uint64_t ticks = (uint64_t)ms * CYCLES_PER_MS;
    while ((Get_Cycles64() - start) < ticks) {}
}

void Smart_Loop_Delay_ms(uint64_t loop_start, uint32_t target_ms) {
    if (!dwt_is_operational) {
        HW_Delay_ms(target_ms);
        return;
    }
    uint64_t target_cycles = (uint64_t)target_ms * CYCLES_PER_MS;
    while ((Get_Cycles64() - loop_start) < target_cycles) {}
}

static void print_int_helper(int32_t v) {
    char buf[12];
    uint8_t i = 0;
    bool neg = false;
    if (v < 0) { neg = true; v = -v; }
    if (v == 0) { buf[i++] = '0'; }
    while (v > 0) { buf[i++] = (char)('0' + (v % 10)); v /= 10; }
    if (neg) UART_Write('-');
    while (i--) UART_Write(buf[i]);
}

int main(void) {
    IP_WDOG->CNT = 0xD928C520;
    IP_WDOG->CNT = 0x62E6F7C8;
    while (!(IP_WDOG->CS & (1 << 11)));

    IP_WDOG->TOVAL = 0x0000FFFF;
    IP_WDOG->CS = 0x2120;
    while (IP_WDOG->CS & (1 << 10));

    ARM_CM4_SCB_CPACR |= ((3UL << 10*2) | (3UL << 11*2));
    __asm volatile ("dsb 0xF" ::: "memory");
    __asm volatile ("isb 0xF" ::: "memory");

    PORT_init();
    LPI2C0_init();
    UART_Begin(115200);
    INF_InitLeaky();

    ARM_CM_DEMCR |= (1 << 24);
    ARM_CM_DWT_CTRL |= (1 << 0);
    ARM_CM_DWT_CYCCNT = 0;

    volatile uint32_t safety_timeout = 0;
    while (ARM_CM_DWT_CYCCNT == 0 && safety_timeout < 500) {
        safety_timeout++;
    }

    if (ARM_CM_DWT_CYCCNT > 0) {
        dwt_is_operational = 1;
        last_hw_cycles = ARM_CM_DWT_CYCCNT;
    } else {
        dwt_is_operational = 0;
        last_hw_cycles = 0;
    }
    accumulated_cycles = 0;

     rol_stat raw_stats;
    raw_stats.window_size = 0; // Force immediate update on first pass

    UART_Println("HDC_Temp_C,HDC_RH_%,Inf_Raw,Inf_Temp_C,Inf_Raw_StdDev,Inf_Raw_Median");

    while(1) {
        uint64_t loop_start = Get_Cycles64();

        // Allow on-the-fly dynamic updates to array size
         rol_stat_SetSize(&raw_stats, rolling_window_size);

        double hdc_temp = 0.0, hdc_rh = 0.0;
        int16_t inf_raw = 0;
        int8_t inf_temp = 0;

        INF_WakeSequence();
        HDC3020_TriggerMeasurement();
        INF_TriggerConc();

        HW_Delay_ms(35);

        HDC3020_ReadData(&hdc_temp, &hdc_rh);
        INF_ReadConcData(&inf_raw);

        INF_TriggerTemp();
        HW_Delay_ms(2);
        INF_ReadTempData(&inf_temp);


        rol_stat_Add(&raw_stats, (float)inf_raw);
        float raw_stddev =  rol_stat_GetStdDev(&raw_stats);
        float raw_median =  rol_stat_GetMedian(&raw_stats);

        //HDC Temp
        UART_PrintFloat((float)hdc_temp, 2); UART_Write(','); UART_Write(' ');
        //HDC RH%
        UART_PrintFloat((float)hdc_rh, 2);   UART_Write(','); UART_Write(' ');
        //Inf Raw
        print_int_helper((int32_t)inf_raw);  UART_Write(','); UART_Write(' ');
        //Inf Temp
        print_int_helper((int32_t)inf_temp); UART_Write(','); UART_Write(' ');
        //Inf Raw Standard Deviation
        UART_PrintFloat(raw_stddev, 3);      UART_Write(','); UART_Write(' ');
        //Inf Raw rolling Median
        UART_PrintFloat(raw_median, 2);      UART_Println("");

        Smart_Loop_Delay_ms(loop_start, 250);
    }
}
