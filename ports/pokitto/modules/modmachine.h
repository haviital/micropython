#ifndef MICROPY_INCLUDED_POKITTO_MODMACHINE_H
#define MICROPY_INCLUDED_POKITTO_MODMACHINE_H

#if !POKITTO_USE_WIN_SIMULATOR

#include "py/obj.h"
#include "PinNames.h"
#include "gpio_object.h"


// Copied from "Pokitto_extport.h"
#define EXT0 	P1_19
#define EXT1	P0_11
#define EXT2	P0_12
#define EXT3	P0_13
#define EXT4	P0_14
#define EXT5	P0_17
#define EXT6	P0_18
#define EXT7	P0_19
#define EXT8	P1_20
#define EXT9	P1_21
#define EXT10	P1_22
#define EXT11	P1_23
#define EXT12	P1_5
#define EXT13	P1_6
#define EXT14	P1_8
#define EXT15	P1_26
#define EXT16	P1_27
#define EXT17	P0_16


extern const mp_obj_type_t machine_pin_type;

//MP_DECLARE_CONST_FUN_OBJ_0(machine_info_obj);

typedef struct _machine_pin_obj_t {
    mp_obj_base_t base;
    gpio_t* gpio;
    uint32_t pinNum;
    //struct device *port;
    //uint32_t pin;
} machine_pin_obj_t;

#endif // POKITTO_USE_WIN_SIMULATOR

#endif // MICROPY_INCLUDED_POKITTO_MODMACHINE_H
