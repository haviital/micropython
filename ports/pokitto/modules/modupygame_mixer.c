/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Hannu Viitala
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
#include <string.h>

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"

#if 1 //MICROPY_PYGAME

#include "PythonBindings.h"

#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))

typedef struct _mp_obj_sound_t {
    mp_obj_base_t base;
    mp_obj_t buf_obj; // need to store this to prevent GC from reclaiming buf
    void *buf;
    uint16_t len;
    uint8_t soundBufferIndex;
    uint16_t soundBufferPos;
} mp_obj_sound_t;


// *** Sound module

// Fill buffer helper
STATIC mp_obj_t sound_fill_buffer_helper(mp_obj_sound_t *self, size_t n_args, const mp_obj_t *args) {

	 // bytebuffer argument
     self->buf_obj = args[0];
     self->len = mp_obj_get_int(args[1]);
     mp_buffer_info_t bufinfo;
     mp_get_buffer_raise(args[0], &bufinfo, MP_BUFFER_READ);
     self->buf = bufinfo.buf;
     self->soundBufferIndex = 0;
     self->soundBufferPos = 0;

	 //
	 if (n_args >= 2) {
        self->soundBufferIndex = mp_obj_get_int(args[2]);
     }

	 //
	 if (n_args >= 3) {
        self->soundBufferPos = mp_obj_get_int(args[3]);
     }

    Pok_Sound_FillBuffer(self->buf, self->len, self->soundBufferIndex, self->soundBufferPos);

	return mp_const_none;
}

// Create Sound object: Sound( bufferdata, lenght, bufferIndex, bufferPos)
STATIC mp_obj_t sound_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

	 // Check params.
	 mp_arg_check_num(n_args, n_kw, 0, 4, false);

	 // Create member data
     mp_obj_sound_t *o = m_new_obj(mp_obj_sound_t);
     o->base.type = type;  // class

    //sound_fill_buffer_helper(o, n_args, args);

    //Pok_Sound_Reset();

       /*
	 // bytebuffer argument
     o->buf_obj = args[0];
     o->len = mp_obj_get_int(args[1]);
     mp_buffer_info_t bufinfo;
     mp_get_buffer_raise(args[0], &bufinfo, MP_BUFFER_READ);
     o->buf = bufinfo.buf;
     o->soundBufferIndex = 0;
     o->soundBufferPos = 0;

	 //
	 if (n_args >= 2) {
        o->soundBufferIndex = mp_obj_get_int(args[2]);
     }

	 //
	 if (n_args >= 3) {
        o->soundBufferPos = mp_obj_get_int(args[3]);
     }

    Pok_Sound_Init(o->buf, o->soundBufferIndex, o->soundBufferPos);
    */

    return MP_OBJ_FROM_PTR(o);
 }

// Sound.fill_buffer()
STATIC mp_obj_t sound_fill_buffer(size_t n_args, const mp_obj_t *args) {

    mp_obj_sound_t *self = MP_OBJ_TO_PTR(args[0]);

    sound_fill_buffer_helper(self, n_args-1, &(args[1]));

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(sound_fill_buffer_obj, 3, 5, sound_fill_buffer);

STATIC mp_int_t sound_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    mp_obj_sound_t *self = MP_OBJ_TO_PTR(self_in);
    bufinfo->buf = self->buf;
    bufinfo->len = self->len;
    bufinfo->typecode = 'B'; // view framebuf as bytes
    return 0;
}

// Sound.get_current_soundbuffer_index()
STATIC mp_obj_t sound_get_current_soundbuffer_index(size_t n_args, const mp_obj_t *args) {

    //mp_obj_sound_t *self = MP_OBJ_TO_PTR(args[0]);

    //
    uint8_t index = Pok_Sound_GetCurrentBufferIndex();

	return mp_obj_new_int(index);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(sound_get_current_soundbuffer_index_obj, 1, 1, sound_get_current_soundbuffer_index);

// Sound.get_current_soundbuffer_pos()
STATIC mp_obj_t sound_get_current_soundbuffer_pos(size_t n_args, const mp_obj_t *args) {

    //mp_obj_sound_t *self = MP_OBJ_TO_PTR(args[0]);

    //
    uint32_t posInBuffer = Pok_Sound_GetCurrentBufferPos();

	return mp_obj_new_int(posInBuffer);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(sound_get_current_soundbuffer_pos_obj, 1, 1, sound_get_current_soundbuffer_pos);

// Sound.get_soundbuffer_size()
STATIC mp_obj_t sound_get_soundbuffer_size(size_t n_args, const mp_obj_t *args) {

    //mp_obj_sound_t *self = MP_OBJ_TO_PTR(args[0]);

    //
    uint32_t bufferSize = Pok_Sound_GetBufferSize();

	return mp_obj_new_int(bufferSize);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(sound_get_soundbuffer_size_obj, 1, 1, sound_get_soundbuffer_size);


// Sound.reset()
STATIC mp_obj_t sound_reset(size_t n_args, const mp_obj_t *args) {

    //mp_obj_sound_t *self = MP_OBJ_TO_PTR(args[0]);

    //
    Pok_Sound_Reset();

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(sound_reset_obj, 1, 1, sound_reset);

// Sound.play()
STATIC mp_obj_t sound_play(size_t n_args, const mp_obj_t *args) {

    //mp_obj_sound_t *self = MP_OBJ_TO_PTR(args[0]);

    //
    Pok_Sound_Play();

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(sound_play_obj, 1, 1, sound_play);

// Sound.pause()
STATIC mp_obj_t sound_pause(size_t n_args, const mp_obj_t *args) {

    //mp_obj_sound_t *self = MP_OBJ_TO_PTR(args[0]);

    //
    Pok_Sound_Pause();

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(sound_pause_obj, 1, 1, sound_pause);

STATIC const mp_rom_map_elem_t sound_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_reset), MP_ROM_PTR(&sound_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill_buffer), MP_ROM_PTR(&sound_fill_buffer_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_current_soundbuffer_index), MP_ROM_PTR(&sound_get_current_soundbuffer_index_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_current_soundbuffer_pos), MP_ROM_PTR(&sound_get_current_soundbuffer_pos_obj) },
    { MP_ROM_QSTR(MP_QSTR_get_soundbuffer_size), MP_ROM_PTR(&sound_get_soundbuffer_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_play), MP_ROM_PTR(&sound_play_obj) },
    { MP_ROM_QSTR(MP_QSTR_pause), MP_ROM_PTR(&sound_pause_obj) },
};
STATIC MP_DEFINE_CONST_DICT(sound_locals_dict, sound_locals_dict_table);

STATIC const mp_obj_type_t mp_type_sound = {
    { &mp_type_type },
    .name = MP_QSTR_Sound,
    .make_new = sound_make_new,
    .buffer_p = { .get_buffer = sound_get_buffer },
    .locals_dict = (mp_obj_dict_t*)&sound_locals_dict,
};

//STATIC const mp_rom_map_elem_t sound_module_globals_table[] = {
//    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_Sound) },  // Sound module name
//    { MP_ROM_QSTR(MP_QSTR_Sound), MP_ROM_PTR(&mp_type_sound) },
// };

//STATIC MP_DEFINE_CONST_DICT(sound_module_globals, sound_module_globals_table);

//const mp_obj_module_t mp_module_sound = {
//    .base = { &mp_type_module },
//    .globals = (mp_obj_dict_t*)&sound_module_globals,
//};

STATIC const mp_rom_map_elem_t mixer_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_mixer) },  // Audio Mixer module name
    //{ MP_ROM_QSTR(MP_QSTR_Sound), MP_ROM_PTR(&mp_module_sound) },  	// Sound module
    { MP_ROM_QSTR(MP_QSTR_Sound), MP_ROM_PTR(&mp_type_sound) },      // Sound class
 };

STATIC MP_DEFINE_CONST_DICT(mixer_module_globals, mixer_module_globals_table);

const mp_obj_module_t mp_module_mixer = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mixer_module_globals,
};


#endif // MICROPY_PY_FRAMEBUF
