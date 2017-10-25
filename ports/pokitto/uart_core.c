#include <unistd.h>
#include "py/mpconfig.h"

/*
 * Core UART functions to implement for a port
 */

//!!HV
#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC int pc_putc(int c);

EXTERNC int pc_getc();


#if 0 //!!HV MICROPY_MIN_USE_STM32_MCU
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
} periph_uart_t;
#define USART1 ((periph_uart_t*)0x40011000)
#endif

// Receive single character
int mp_hal_stdin_rx_chr(void) {
    unsigned char c = 0;
#if 0 //!!HV MICROPY_MIN_USE_STDOUT
    int r = read(0, &c, 1);
    (void)r;
#elif 0 //!!HV  MICROPY_MIN_USE_STM32_MCU
    // wait for RXNE
    while ((USART1->SR & (1 << 5)) == 0) {
    }
    c = USART1->DR;
#endif

	c = pc_getc();

    return c;
}

// Send string of given length
void mp_hal_stdout_tx_strn(const char *str, mp_uint_t len) {
#if  0 //!!HV MICROPY_MIN_USE_STDOUT
    int r = write(1, str, len);
    (void)r;
#elif  0 //!!HV MICROPY_MIN_USE_STM32_MCU
    while (len--) {
        // wait for TXE
        while ((USART1->SR & (1 << 7)) == 0) {
        }
        USART1->DR = *str++;
    }
#endif

    while (len--) {
        pc_putc(*str++);
    }

}
