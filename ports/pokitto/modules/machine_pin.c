    /*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014, 2015 Damien P. George
 * Copyright (c) 2016 Linaro Limited
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mphal.h"
#include "modmachine.h"
#include "gpio_api.h"
#include "virtpin.h"

const mp_obj_base_t machine_pin_obj_template = {&machine_pin_type};

STATIC void machine_pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    machine_pin_obj_t *self = self_in;
    mp_printf(print, "<Pin %d>", self->pinNum);
}

// pin.init(mode, pull=None, *, value)
STATIC mp_obj_t machine_pin_obj_init_helper(machine_pin_obj_t *self, size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_mode, /*ARG_pull,*/ ARG_value };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_mode, MP_ARG_REQUIRED | MP_ARG_INT },
        //{ MP_QSTR_pull, MP_ARG_OBJ, {.u_obj = mp_const_none}},
        { MP_QSTR_value, MP_ARG_KW_ONLY | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL}},
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get io mode
    uint mode = args[ARG_mode].u_int;

    // get pull mode
    //uint pull = GPIO_PUD_NORMAL;
    //if (args[ARG_pull].u_obj != mp_const_none) {
    //    pull = mp_obj_get_int(args[ARG_pull].u_obj);
    //}

    //int ret = gpio_pin_configure(self->port, self->pin, mode | pull);
    if( mode == PIN_INPUT )
        gpio_init_in(self->gpio, self->pinNum);
    else // PIN_OUTPUT
        gpio_init_out(self->gpio, self->pinNum);
    //if (ret) {
    //    mp_raise_ValueError("invalid pin");
    //}

    // set initial value
    if (args[ARG_value].u_obj != MP_OBJ_NULL) {
        //(void)gpio_pin_write(self->port, self->pin, mp_obj_is_true(args[ARG_value].u_obj));
        mp_hal_pin_write(self->gpio, mp_obj_is_true(args[ARG_value].u_obj));
    }
    
    return mp_const_none;
}

// constructor(pin, ...)
mp_obj_t mp_pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 1, MP_OBJ_FUN_ARGS_MAX, true);

    // get the wanted port
    //if (!MP_OBJ_IS_TYPE(args[0], &mp_type_tuple)) {
    //    mp_raise_ValueError("Pin id must be tuple of (\"GPIO_x\", pin#)");
    //}
    //mp_obj_t *items;
    //mp_obj_get_array_fixed_n(args[0], 2, &items);
    //const char *drv_name = mp_obj_str_get_str(items[0]);
    int wanted_pin = mp_obj_get_int(args[0]);


    //struct device *wanted_port = device_get_binding(drv_name);
    //if (!wanted_port) {
    //    mp_raise_ValueError("invalid port");
    //}

    machine_pin_obj_t *pin = m_new_obj(machine_pin_obj_t);
    pin->base = machine_pin_obj_template;
    pin->gpio = gc_alloc(sizeof(gpio_t), false);  //!!HV LEAKS MEMORY. FIX THIS!
    memset(pin->gpio, 0, sizeof(gpio_t));
    pin->pinNum = wanted_pin;

    if (n_args > 1 || n_kw > 0) {
        // pin mode given, so configure this GPIO
        mp_map_t kw_args;
        mp_map_init_fixed_table(&kw_args, n_kw, args + n_args);
        machine_pin_obj_init_helper(pin, n_args - 1, args + 1, &kw_args);
    }

    return (mp_obj_t)pin;
}

// fast method for getting/setting pin value
STATIC mp_obj_t machine_pin_call(mp_obj_t self_in, size_t n_args, size_t n_kw, const mp_obj_t *args) {
    mp_arg_check_num(n_args, n_kw, 0, 1, false);
    machine_pin_obj_t *self = self_in;
    if (n_args == 0) {
        uint32_t pin_val;
        //(void)gpio_pin_read(self->port, self->pin, &pin_val);
        pin_val = mp_hal_pin_read(self->gpio);
        return MP_OBJ_NEW_SMALL_INT(pin_val);
    } else {
        //(void)gpio_pin_write(self->port, self->pin, mp_obj_is_true(args[0]));
        mp_hal_pin_write(self->gpio, mp_obj_is_true(args[0]));
        return mp_const_none;
    }
}

// pin.init(mode, pull)
STATIC mp_obj_t machine_pin_obj_init(size_t n_args, const mp_obj_t *args, mp_map_t *kw_args) {
    return machine_pin_obj_init_helper(args[0], n_args - 1, args + 1, kw_args);
}
MP_DEFINE_CONST_FUN_OBJ_KW(machine_pin_init_obj, 1, machine_pin_obj_init);

// pin.value([value])
STATIC mp_obj_t machine_pin_value(size_t n_args, const mp_obj_t *args) {
    return machine_pin_call(args[0], n_args - 1, 0, args + 1);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_pin_value_obj, 1, 2, machine_pin_value);

// Sets the pin off.
STATIC mp_obj_t machine_pin_off(mp_obj_t self_in) {
    machine_pin_obj_t *self = self_in;
    //(void)gpio_pin_write(self->port, self->pin, 0);
    mp_hal_pin_low(self->gpio);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_off_obj, machine_pin_off);

// Sets the pin on.
STATIC mp_obj_t machine_pin_on(mp_obj_t self_in) {
    machine_pin_obj_t *self = self_in;
    //(void)gpio_pin_write(self->port, self->pin, 1);
    mp_hal_pin_high(self->gpio);
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(machine_pin_on_obj, machine_pin_on);

// Generic ioctl function called by virtpin.c. 
STATIC mp_uint_t machine_pin_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    (void)errcode;
    machine_pin_obj_t *self = self_in;

    switch (request) {
        case MP_PIN_READ: {
            uint32_t pin_val;
            pin_val = mp_hal_pin_read(self->gpio);
            return pin_val;
        }
        case MP_PIN_WRITE: {
            mp_hal_pin_write(self->gpio, arg);
            return 0;
        }
    }
    return -1;
}

// Pin class methods and constants.
STATIC const mp_rom_map_elem_t machine_pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init),    MP_ROM_PTR(&machine_pin_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_value),   MP_ROM_PTR(&machine_pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_off),     MP_ROM_PTR(&machine_pin_off_obj) },
    { MP_ROM_QSTR(MP_QSTR_on),      MP_ROM_PTR(&machine_pin_on_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_IN),        MP_ROM_INT(PIN_INPUT) },
    { MP_ROM_QSTR(MP_QSTR_OUT),       MP_ROM_INT(PIN_OUTPUT) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP),   MP_ROM_INT(PullUp) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN), MP_ROM_INT(PullDown) },
    { MP_ROM_QSTR(MP_QSTR_EXT0),      MP_ROM_INT(EXT0) },
    { MP_ROM_QSTR(MP_QSTR_EXT1),      MP_ROM_INT(EXT1) },
    { MP_ROM_QSTR(MP_QSTR_EXT2),      MP_ROM_INT(EXT2) },
    { MP_ROM_QSTR(MP_QSTR_EXT3),      MP_ROM_INT(EXT3) },
    { MP_ROM_QSTR(MP_QSTR_EXT4),      MP_ROM_INT(EXT4) },
    { MP_ROM_QSTR(MP_QSTR_EXT5),      MP_ROM_INT(EXT5) },
    { MP_ROM_QSTR(MP_QSTR_EXT6),      MP_ROM_INT(EXT6) },
    { MP_ROM_QSTR(MP_QSTR_EXT7),      MP_ROM_INT(EXT7) },
    { MP_ROM_QSTR(MP_QSTR_EXT8),      MP_ROM_INT(EXT8) },
    { MP_ROM_QSTR(MP_QSTR_EXT9),      MP_ROM_INT(EXT9) },
    { MP_ROM_QSTR(MP_QSTR_EXT10),      MP_ROM_INT(EXT10) },
    { MP_ROM_QSTR(MP_QSTR_EXT11),      MP_ROM_INT(EXT11) },
    { MP_ROM_QSTR(MP_QSTR_EXT12),      MP_ROM_INT(EXT12) },
    { MP_ROM_QSTR(MP_QSTR_EXT13),      MP_ROM_INT(EXT13) },
    { MP_ROM_QSTR(MP_QSTR_EXT14),      MP_ROM_INT(EXT14) },
    { MP_ROM_QSTR(MP_QSTR_EXT15),      MP_ROM_INT(EXT15) },
    { MP_ROM_QSTR(MP_QSTR_EXT16),      MP_ROM_INT(EXT16) },
    { MP_ROM_QSTR(MP_QSTR_EXT17),      MP_ROM_INT(EXT17) },
};

STATIC MP_DEFINE_CONST_DICT(machine_pin_locals_dict, machine_pin_locals_dict_table);


STATIC const mp_pin_p_t machine_pin_pin_p = {
    .ioctl = machine_pin_ioctl,
};


// Pin class type
const mp_obj_type_t machine_pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_Pin,
    .print = machine_pin_print,
    .make_new = mp_pin_make_new,
    .call = machine_pin_call,
    .protocol = &machine_pin_pin_p,
    .locals_dict = (mp_obj_t)&machine_pin_locals_dict,
};
