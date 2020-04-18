/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2017 Hannu Viitala
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

#include "py/runtime.h"
#include "py/obj.h"

#include "extmod/machine_mem.h"
#include "extmod/machine_pinbase.h"
#include "extmod/machine_signal.h"
#include "extmod/machine_pulse.h"
#include "modmachine.h"

#include "PythonBindings.h"

#define POK_TRACE(str) printf("%s (%d): %s", __FILE__, __LINE__,str)
#define POK_TRACE1(str, p1) printf("%s (%d): %s ==> %d\n", __FILE__, __LINE__, str, p1 );

#if MICROPY_PLAT_DEV_MEM
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#define MICROPY_PAGE_SIZE 4096
#define MICROPY_PAGE_MASK (MICROPY_PAGE_SIZE - 1)
#endif

#if MICROPY_PY_MACHINE

// Copied from modframebuf.c
typedef struct _mp_obj_framebuf_t {
	mp_obj_base_t base;
	mp_obj_t buf_obj; // need to store this to prevent GC from reclaiming buf
	void *buf;
	uint16_t width, height, stride;
	uint8_t format;
} mp_obj_framebuf_t;

// *** cookie member data
typedef struct _mp_obj_cookie_t {
    mp_obj_base_t base;
    mp_obj_t buf_obj; // need to store this to prevent GC from reclaiming buf
    void *buf;
    uint16_t len;
    void* cookiePtr;
} mp_obj_cookie_t;


uintptr_t mod_machine_mem_get_addr(mp_obj_t addr_o, uint align) {
    uintptr_t addr = mp_obj_int_get_truncated(addr_o);
    if ((addr & (align - 1)) != 0) {
        nlr_raise(mp_obj_new_exception_msg_varg(&mp_type_ValueError, "address %08x is not aligned to %d bytes", addr, align));
    }
    #if MICROPY_PLAT_DEV_MEM
    {
        // Not thread-safe
        static int fd;
        static uintptr_t last_base = (uintptr_t)-1;
        static uintptr_t map_page;
        if (!fd) {
            fd = open("/dev/mem", O_RDWR | O_SYNC);
            if (fd == -1) {
                mp_raise_OSError(errno);
            }
        }

        uintptr_t cur_base = addr & ~MICROPY_PAGE_MASK;
        if (cur_base != last_base) {
            map_page = (uintptr_t)mmap(NULL, MICROPY_PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, cur_base);
            last_base = cur_base;
        }
        addr = map_page + (addr & MICROPY_PAGE_MASK);
    }
    #endif

    return addr;
}

STATIC mp_obj_t mod_machine_blit_framebuf(size_t n_args, const mp_obj_t *args) {
	
    mp_obj_framebuf_t *source = MP_OBJ_TO_PTR(args[0]);
    mp_int_t x = mp_obj_get_int(args[1]);
    mp_int_t y = mp_obj_get_int(args[2]);
    mp_int_t key = -1;
    if (n_args > 3) {
        key = mp_obj_get_int(args[3]);
    }

	//TODO: take source->stride into account also
	//TODO: check that source->format is acceptable
    Pok_Display_blitFrameBuffer(x, y, source->width, source->height, false, false, key, source->buf);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_machine_blit_framebuf_obj, 3, 4, mod_machine_blit_framebuf);

// STATIC mp_obj_t mod_machine_buttons_repeat(size_t n_args, const mp_obj_t *args) {
	// uint8_t button = mp_obj_get_int(args[0]);
	// uint8_t period = mp_obj_get_int(args[1]);
	// Pok_Core_buttons_repeat(button, period);
	// return mp_const_none;
// }
// STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_machine_buttons_repeat_obj, 2, 2, mod_machine_buttons_repeat);

STATIC mp_obj_t mod_machine_draw_text(size_t n_args, const mp_obj_t *args) {
	uint8_t x = mp_obj_get_int(args[0]);
	uint8_t y = mp_obj_get_int(args[1]);
    const char *str = mp_obj_str_get_str(args[2]);
	uint8_t color = mp_obj_get_int(args[3]);
	Pok_Display_print(x, y, str, color);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_machine_draw_text_obj, 4, 4, mod_machine_draw_text);

STATIC mp_obj_t mod_machine_wait(size_t n_args, const mp_obj_t *args) {
	uint32_t time_ms = mp_obj_get_int(args[0]);
	Pok_Wait(time_ms);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(mod_machine_wait_obj, 1, 1, mod_machine_wait);

STATIC mp_obj_t mod_machine_time_ms(void) {
	uint32_t time_ms = Pok_Time_ms();
	return mp_obj_new_int(time_ms);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_machine_time_ms_obj, mod_machine_time_ms);

// *** Cookie module

// Create
STATIC mp_obj_t cookie_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

	// Check params.
	mp_arg_check_num(n_args, n_kw, 2, 2, false);

	// Create the member data
    mp_obj_cookie_t *o = m_new_obj(mp_obj_cookie_t);
    o->base.type = type;  // class
    
    // Cookie name
    const char *name = mp_obj_str_get_str(args[0]);

	// bytebuffer argument
    o->buf_obj = args[1];
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(args[1], &bufinfo, MP_BUFFER_WRITE);
    o->buf = bufinfo.buf;
    o->len = bufinfo.len;
    
    // Create the class in PokittoLib
    o->cookiePtr = Pok_CreateCookie( (char*)name, o->buf, o->len );
    
    return MP_OBJ_FROM_PTR(o);
 }

 STATIC mp_obj_t cookie_destroy(mp_obj_t self_in) {
    mp_obj_cookie_t *self = self_in;
    Pok_DeleteCookie( self->cookiePtr );
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(cookie_destroy_obj, cookie_destroy);

// Get buffer
STATIC mp_int_t cookie_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    mp_obj_cookie_t *self = MP_OBJ_TO_PTR(self_in);
    bufinfo->buf = self->buf;
    bufinfo->len = self->len;
    bufinfo->typecode = 'B'; // view buffer as bytes
    return 0;
}

// Load the cookie and return the content.
STATIC mp_obj_t cookie_load(size_t n_args, const mp_obj_t *args)
{
    // Load the cookie from EEPROM
    mp_obj_cookie_t *self = MP_OBJ_TO_PTR(args[0]);
	Pok_LoadCookie(self->cookiePtr);
    
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cookie_load_obj, 1, 1, cookie_load);

// Save the cookie and return the content.
STATIC mp_obj_t cookie_save(size_t n_args, const mp_obj_t *args) {

    mp_obj_cookie_t *self = MP_OBJ_TO_PTR(args[0]);
    
    // Save the cookie to the EEPROM.
	Pok_SaveCookie(self->cookiePtr);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(cookie_save_obj, 1, 1, cookie_save);

// Cookie local table
STATIC const mp_rom_map_elem_t cookie_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&cookie_destroy_obj) },
    { MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&cookie_load_obj) },
    { MP_ROM_QSTR(MP_QSTR_save), MP_ROM_PTR(&cookie_save_obj) },
};
STATIC MP_DEFINE_CONST_DICT(cookie_locals_dict, cookie_locals_dict_table);

// Cookie class
STATIC const mp_obj_type_t machine_cookie_type = {
    { &mp_type_type },
    .name = MP_QSTR_Cookie,
    .make_new = cookie_make_new,
    .buffer_p = { .get_buffer = cookie_get_buffer },
    .locals_dict = (mp_obj_dict_t*)&cookie_locals_dict,
};

// *** GLOBALS

STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_umachine) },

    { MP_ROM_QSTR(MP_QSTR_mem8), MP_ROM_PTR(&machine_mem8_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem16), MP_ROM_PTR(&machine_mem16_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem32), MP_ROM_PTR(&machine_mem32_obj) },

    { MP_ROM_QSTR(MP_QSTR_PinBase), MP_ROM_PTR(&machine_pinbase_type) },
	{ MP_ROM_QSTR(MP_QSTR_Signal), MP_ROM_PTR(&machine_signal_type) },
    { MP_ROM_QSTR(MP_QSTR_Pin), MP_ROM_PTR(&machine_pin_type) },
	{ MP_ROM_QSTR(MP_QSTR_blit_framebuf), MP_ROM_PTR(&mod_machine_blit_framebuf_obj) },
	//{ MP_ROM_QSTR(MP_QSTR_buttons_repeat), MP_ROM_PTR(&mod_machine_buttons_repeat_obj) },
	{ MP_ROM_QSTR(MP_QSTR_draw_text), MP_ROM_PTR(&mod_machine_draw_text_obj) },
#if MICROPY_PY_MACHINE_PULSE
    { MP_ROM_QSTR(MP_QSTR_time_pulse_us), MP_ROM_PTR(&machine_time_pulse_us_obj) },
#endif
 	{ MP_ROM_QSTR(MP_QSTR_wait), MP_ROM_PTR(&mod_machine_wait_obj) },
 	{ MP_ROM_QSTR(MP_QSTR_time_ms), MP_ROM_PTR(&mod_machine_time_ms_obj) },
    { MP_ROM_QSTR(MP_QSTR_Cookie), MP_ROM_PTR(&machine_cookie_type) },
};

STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

const mp_obj_module_t mp_module_machine = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&machine_module_globals,
};

#endif // MICROPY_PY_MACHINE
