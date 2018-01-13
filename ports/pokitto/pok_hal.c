//#include <stdio.h>
//#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"
//#include "usb.h"
//#include "uart.h"
//#include "Arduino.h"


void mp_hal_gpio_clock_enable(GPIO_TypeDef *gpio) {
}

void extint_register_pin(const void *pin, uint32_t mode, int hard_irq, mp_obj_t callback_obj) {
    mp_raise_NotImplementedError(NULL);
}
