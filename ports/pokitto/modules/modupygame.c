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
#include <string.h>

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"

#if 1 //MICROPY_PYGAME

#include "PythonBindings.h"

extern const mp_obj_module_t mp_module_mixer;

// *** surface member data
typedef struct _mp_obj_surface_t {
    mp_obj_base_t base;
    mp_obj_t buf_obj; // need to store this to prevent GC from reclaiming buf
    void *buf;
    uint16_t width, height, stride;
    uint8_t format;
} mp_obj_surface_t;

// *** Rect member data
typedef struct _mp_obj_rect_t {
    mp_obj_base_t base;
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
} mp_obj_rect_t;

// *** eventtype member data
typedef struct _mp_obj_eventtype_t {
    mp_obj_base_t base;
    uint8_t type;
    uint8_t key;
} mp_obj_eventtype_t;


// *** Defines
#define PYGAME_NOEVENT	(0)
#define PYGAME_KEYDOWN  (1)
#define PYGAME_KEYUP    (2)
#define PYGAME_K_z    	(3)  // Pokitto C
#define PYGAME_K_x    	(4)  // Pokitto B
#define PYGAME_K_c    	(5)  // Pokitto A
#define PYGAME_QUIT    	(6)
#define PYGAME_K_UP		(7)
#define PYGAME_K_DOWN	(8)
#define PYGAME_K_LEFT	(9)
#define PYGAME_K_RIGHT	(10)
#define PYGAME_K_ABUT    (11)
#define PYGAME_K_BBUT    (12)
#define PYGAME_K_CBUT    (13)

#define max(a,b) ((a)>(b)?(a):(b))
#define min(a,b) ((a)<(b)?(a):(b))

// *** Common functions

STATIC void generic_method_lookup(mp_obj_t obj, qstr attr, mp_obj_t *dest) {
    mp_obj_type_t *type = mp_obj_get_type(obj);
    if (type->locals_dict != NULL) {
         // generic method lookup
         // this is a lookup in the object (ie not class or type)
         assert(type->locals_dict->base.type == &mp_type_dict); // MicroPython restriction, for now
         mp_map_t *locals_map = &type->locals_dict->map;
         mp_map_elem_t *elem = mp_map_lookup(locals_map, MP_OBJ_NEW_QSTR(attr), MP_MAP_LOOKUP);
         if (elem != NULL) {
             mp_convert_member_lookup(obj, type, elem->value, dest);
         }
    }
}

// *** event module

STATIC mp_obj_t eventtype_attr_op(mp_obj_t self_in, qstr attr, mp_obj_t set_val) {
    mp_obj_eventtype_t *self = MP_OBJ_TO_PTR(self_in);

    if (set_val == MP_OBJ_NULL) {
        if(attr == MP_QSTR_key)
            return mp_obj_new_int(self->key);
        else if(attr == MP_QSTR_type)
            return mp_obj_new_int(self->type);
    }

    // Should be unreachable once all cases are handled
    return MP_OBJ_NULL;
}

STATIC void eventtype_attr(mp_obj_t obj, qstr attr, mp_obj_t *dest) {

    if(attr == MP_QSTR_key || attr == MP_QSTR_type) {

        if (dest[0] == MP_OBJ_NULL) {
            // load attribute
            mp_obj_t val = eventtype_attr_op(obj, attr, MP_OBJ_NULL);
            dest[0] = val;
        }
    }
    else {
        generic_method_lookup(obj, attr, dest);
    }
}

STATIC const mp_rom_map_elem_t eventtype_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_type), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_key), MP_ROM_INT(0) },
};
STATIC MP_DEFINE_CONST_DICT(eventtype_locals_dict, eventtype_locals_dict_table);

STATIC mp_obj_t eventtype_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

	 // Check params.
	 mp_arg_check_num(n_args, n_kw, 2, 2, false);

	 // Create member data
     mp_obj_eventtype_t *o = m_new_obj(mp_obj_eventtype_t);
     o->base.type = type;  // class
     o->type = mp_obj_get_int(args[0]);
     o->key = mp_obj_get_int(args[1]);

    return MP_OBJ_FROM_PTR(o);
 }

STATIC const mp_obj_type_t mp_type_eventtype = {
    { &mp_type_type },
    .name = MP_QSTR_EventType,
    .make_new = eventtype_make_new,
    .locals_dict = (mp_obj_dict_t*)&eventtype_locals_dict,
    .attr = eventtype_attr,
};

STATIC mp_obj_t event_poll(void) {

	// Is key held? Add to the buffer.
	if( Pok_Core_buttons_held(BTN_UP, 1) )
        Pok_addToRingBuffer( PYGAME_KEYDOWN, PYGAME_K_UP);
	if( Pok_Core_buttons_held(BTN_DOWN, 1) )
        Pok_addToRingBuffer( PYGAME_KEYDOWN, PYGAME_K_DOWN);
	if( Pok_Core_buttons_held(BTN_LEFT, 1) )
        Pok_addToRingBuffer( PYGAME_KEYDOWN, PYGAME_K_LEFT);
	if( Pok_Core_buttons_held(BTN_RIGHT, 1) )
        Pok_addToRingBuffer( PYGAME_KEYDOWN, PYGAME_K_RIGHT);
	if( Pok_Core_buttons_held(BTN_A, 1) )
        Pok_addToRingBuffer( PYGAME_KEYDOWN, PYGAME_K_ABUT);
	if( Pok_Core_buttons_held(BTN_B, 1) )
        Pok_addToRingBuffer( PYGAME_KEYDOWN, PYGAME_K_BBUT);
	if( Pok_Core_buttons_held(BTN_C, 1) )
        Pok_addToRingBuffer( PYGAME_KEYDOWN, PYGAME_K_CBUT);

    // Is key released? Add to the buffer.
    if( Pok_Core_buttons_released(BTN_UP) )
        Pok_addToRingBuffer( PYGAME_KEYUP, PYGAME_K_UP);
    if( Pok_Core_buttons_released(BTN_DOWN) )
        Pok_addToRingBuffer( PYGAME_KEYUP, PYGAME_K_DOWN);
    if( Pok_Core_buttons_released(BTN_LEFT) )
        Pok_addToRingBuffer( PYGAME_KEYUP, PYGAME_K_LEFT);
    if( Pok_Core_buttons_released(BTN_RIGHT) )
        Pok_addToRingBuffer( PYGAME_KEYUP, PYGAME_K_RIGHT);
    if( Pok_Core_buttons_released(BTN_A) )
        Pok_addToRingBuffer( PYGAME_KEYUP, PYGAME_K_ABUT);
    if( Pok_Core_buttons_released(BTN_B) )
        Pok_addToRingBuffer( PYGAME_KEYUP, PYGAME_K_BBUT);
    if( Pok_Core_buttons_released(BTN_C) )
        Pok_addToRingBuffer( PYGAME_KEYUP, PYGAME_K_CBUT);

    // Read the oldest item, if any.
    EventRingBufferItem item;
    if( Pok_readAndRemoveFromRingBuffer(&item)){
		// Create eventtype object instance
		mp_obj_eventtype_t *o = m_new_obj(mp_obj_eventtype_t);
		o->base.type = &mp_type_eventtype;
		o->type = item.type;
		o->key = item.key;
		return o;
	}
	else {
		return mp_obj_new_int(PYGAME_NOEVENT);
	}
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(event_poll_obj, event_poll);

STATIC const mp_rom_map_elem_t event_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_event) },  // event module name
    { MP_ROM_QSTR(MP_QSTR_poll), MP_ROM_PTR(&event_poll_obj) },		// Method: poll
    { MP_ROM_QSTR(MP_QSTR_EventType), MP_ROM_PTR(&mp_type_eventtype) },	// Event class
 };

STATIC MP_DEFINE_CONST_DICT(event_module_globals, event_module_globals_table);

const mp_obj_module_t mp_module_event = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&event_module_globals,
};

// *** Rect class

STATIC bool DoRectsIntersect (mp_obj_rect_t *A, mp_obj_rect_t *B)
{
    //A.topleft < B.bottomright &&
    //A.bottomright > B.topleft
    return (A->x < B->x + B->w && A->y < B->y + B->h &&
            A->x + A->w > B->x && A->y + A->h > B->y);
}

STATIC mp_obj_t rect_colliderect(size_t n_args, const mp_obj_t *args)
{
    mp_obj_rect_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_rect_t *aRect = MP_OBJ_TO_PTR(args[1]);

    return mp_obj_new_bool(DoRectsIntersect (self, aRect));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(rect_colliderect_obj, 2, 2, rect_colliderect);

STATIC mp_obj_t rect_attr_op(mp_obj_t self_in, qstr attr, mp_obj_t set_val) {
    mp_obj_rect_t *self = MP_OBJ_TO_PTR(self_in);

    if (set_val == MP_OBJ_NULL) {
        if(attr == MP_QSTR_x)
            return mp_obj_new_int(self->x);
        else if(attr == MP_QSTR_y)
            return mp_obj_new_int(self->y);
        else if(attr == MP_QSTR_width)
            return mp_obj_new_int(self->w);
        else if(attr == MP_QSTR_height)
            return mp_obj_new_int(self->h);
        else if(attr == MP_QSTR_centerx)
            return mp_obj_new_int(self->x + (self->w/2));
        else if(attr == MP_QSTR_height)
            return mp_obj_new_int(self->y + (self->h/2));
    }
    else {
        if(attr == MP_QSTR_x)
            self->x = (mp_int_t)mp_obj_get_int(set_val);
        else if(attr == MP_QSTR_y)
            self->y = (mp_int_t)mp_obj_get_int(set_val);
        else if(attr == MP_QSTR_width)
            self->w = (mp_int_t)mp_obj_get_int(set_val);
        else if(attr == MP_QSTR_height)
            self->h = (mp_int_t)mp_obj_get_int(set_val);

        return set_val; // just !MP_OBJ_NULL
    }

    // Should be unreachable once all cases are handled
    return MP_OBJ_NULL;
}

STATIC void rect_attr(mp_obj_t obj, qstr attr, mp_obj_t *dest) {

    if(attr == MP_QSTR_x || attr == MP_QSTR_y || attr == MP_QSTR_width || attr == MP_QSTR_height ||
       attr == MP_QSTR_centerx || attr == MP_QSTR_centery ) {

        if (dest[0] == MP_OBJ_NULL) {
            if(attr == MP_QSTR_x || attr == MP_QSTR_y || attr == MP_QSTR_width || attr == MP_QSTR_height ||
               attr == MP_QSTR_centerx || attr == MP_QSTR_centery ) {

                // load attribute
                mp_obj_t val = rect_attr_op(obj, attr, MP_OBJ_NULL);
                dest[0] = val;
            }
         } else {
            if(attr == MP_QSTR_x || attr == MP_QSTR_y || attr == MP_QSTR_width || attr == MP_QSTR_height ) {
                // delete/store attribute
                if (rect_attr_op(obj, attr, dest[1]) != MP_OBJ_NULL) {
                    dest[0] = MP_OBJ_NULL; // indicate success
                }
            }
        }
    }
    else {
        generic_method_lookup(obj, attr, dest);
    }
}

STATIC const mp_rom_map_elem_t rect_locals_dict_table[] = {

    // Instance attributes
    { MP_ROM_QSTR(MP_QSTR_x), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_y), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_width), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_height), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_centerx), MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_centery), MP_ROM_INT(0) },

    // Methods
    { MP_ROM_QSTR(MP_QSTR_colliderect), MP_ROM_PTR(&rect_colliderect_obj) },
};
STATIC MP_DEFINE_CONST_DICT(rect_locals_dict, rect_locals_dict_table);

STATIC mp_obj_t rect_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

    // Check params.
	mp_arg_check_num(n_args, n_kw, 1, 4, false);

    // Create member data struct.
    mp_obj_rect_t *o = m_new_obj(mp_obj_rect_t);
    o->base.type = type;  // class

   if (n_args == 1) {

        // Init from rect
        // Create member data
        mp_obj_rect_t *aRect = MP_OBJ_TO_PTR(args[0]);
        o->x = aRect->x;
        o->y = aRect->y;
        o->w = aRect->w;
        o->h = aRect->h;
    }
    else {

        // Init from x,y,w,h

        // Create member data
        o->x = mp_obj_get_int(args[0]);
        o->y = mp_obj_get_int(args[1]);
        o->w = mp_obj_get_int(args[2]);
        o->h = mp_obj_get_int(args[3]);
    }

    return MP_OBJ_FROM_PTR(o);
 }

 STATIC void rect_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    mp_obj_rect_t *self = self_in;
    mp_printf(print, "Rect(%u, %u, %u, %u)", self->x, self->y, self->w, self->h );
}

STATIC const mp_obj_type_t mp_type_rect = {
    { &mp_type_type },
    .name = MP_QSTR_Rect,
    .make_new = rect_make_new,
    .locals_dict = (mp_obj_dict_t*)&rect_locals_dict,
    .print = rect_print,
    .attr = rect_attr,
};

// *** SURFACE  module ***

STATIC mp_obj_t surface_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args) {

	 // Check params.
	 mp_arg_check_num(n_args, n_kw, 2, 3, false);

	 // Create member data
     mp_obj_surface_t *o = m_new_obj(mp_obj_surface_t);
     o->base.type = type;  // class
     o->width = mp_obj_get_int(args[0]);
     o->height = mp_obj_get_int(args[1]);

	 // bytebuffer argument given?
	 if (n_args >= 3) {
		o->buf_obj = args[2];
		mp_buffer_info_t bufinfo;
		mp_get_buffer_raise(args[2], &bufinfo, MP_BUFFER_READ);
		o->buf = bufinfo.buf;
	 }
	 else {
		int len = o->width * o->height / 2;
		o->buf = m_new(byte, 1 * len);
	 }

    // o->format = mp_obj_get_int(args[3]);
    // if (n_args >= 5) {
        // o->stride = mp_obj_get_int(args[4]);
    // } else {
    o->stride = o->width;
    // }


    return MP_OBJ_FROM_PTR(o);
 }

STATIC mp_int_t surface_get_buffer(mp_obj_t self_in, mp_buffer_info_t *bufinfo, mp_uint_t flags) {
    (void)flags;
    mp_obj_surface_t *self = MP_OBJ_TO_PTR(self_in);
    bufinfo->buf = self->buf;
    bufinfo->len = self->stride * self->height / 2;  // 4-bit color depth
    bufinfo->typecode = 'B'; // view framebuf as bytes
    return 0;
}


STATIC mp_obj_t surface_get_rect(size_t n_args, const mp_obj_t *args) {

    mp_obj_surface_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_obj_rect_t *retRect = m_new_obj(mp_obj_rect_t);
    retRect->base.type = &mp_type_rect;
    retRect->x = 0;
    retRect->y = 0;
    retRect->w = self->width;
    retRect->h = self->height;

	return MP_OBJ_FROM_PTR(retRect);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(surface_get_rect_obj, 1, 1, surface_get_rect);

// TODO: should only work for the screen surface
STATIC mp_obj_t surface_fill(size_t n_args, const mp_obj_t *args) {

    mp_obj_surface_t *self = MP_OBJ_TO_PTR(args[0]);
    mp_int_t color = mp_obj_get_int(args[1]);
    if(n_args > 2) {
        // fill the rect with the color
        mp_obj_rect_t *rect = MP_OBJ_TO_PTR(args[2]);
        Pok_Display_fillRectangle(rect->x, rect->y, rect->w, rect->h, color);
    }
    else {
        // fill the whole screen  with the color
        Pok_Display_fillRectangle(0, 0, self->width, self->height, color);
    }

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(surface_fill_obj, 2, 3   , surface_fill);

// TODO: should only work for the screen surface
STATIC mp_obj_t surface_blit(size_t n_args, const mp_obj_t *args) {

	// For now, always draws the source surface to the screen! Does not draw to this surface!
    mp_obj_surface_t *source = MP_OBJ_TO_PTR(args[1]);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);
    mp_int_t transparentColor = 0;
    if( n_args > 4)
        transparentColor = mp_obj_get_int(args[4]);

	//TODO: take source->stride into account also
	//TODO: check that source->format is acceptable
	Pok_Display_blitFrameBuffer(x, y, source->width, source->height, transparentColor, source->buf);
	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(surface_blit_obj, 4, 5, surface_blit);

STATIC mp_obj_t surface_setHwSprite(size_t n_args, const mp_obj_t *args) {

    mp_int_t index = mp_obj_get_int(args[3]);

	if(args[1] == mp_const_none) {
        Pok_Display_setSprite(index, 0, 0, 0, 0, 0, NULL, NULL, false);
        return mp_const_none;
    }

    // Get source surface.
    mp_obj_surface_t *source = MP_OBJ_TO_PTR(args[1]);

    // Create the array of 16-bit ints from the list.
    mp_obj_list_t *aListOfInts = MP_OBJ_TO_PTR(args[2]);
    uint16_t palette16x16bit[16];
    for(int i = 0; i < 16; i++) {
        uint16_t color16bit = 0;
        if (i < aListOfInts->len)
            color16bit = mp_obj_get_int(aListOfInts->items[i]);
        palette16x16bit[i] = color16bit;
    }

    mp_int_t x = mp_obj_get_int(args[4]);
    mp_int_t y = mp_obj_get_int(args[5]);
    bool doResetDirtyRect = mp_obj_is_true(args[6]);

    //mp_obj_bool_t *self = MP_OBJ_TO_PTR(self_in);
    mp_int_t transparentColor = 0;
    if( n_args > 7)
        transparentColor = mp_obj_get_int(args[7]);

	//TODO: take source->stride into account also
	//TODO: check that source->format is acceptable
	Pok_Display_setSprite(index, x, y, source->width, source->height, transparentColor, source->buf, palette16x16bit, doResetDirtyRect);

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(surface_setHwSprite_obj, 7, 8, surface_setHwSprite);

STATIC mp_obj_t surface_setHwSpritePos(size_t n_args, const mp_obj_t *args) {

    mp_int_t index = mp_obj_get_int(args[1]);
    mp_int_t x = mp_obj_get_int(args[2]);
    mp_int_t y = mp_obj_get_int(args[3]);

	//TODO: take source->stride into account also
	//TODO: check that source->format is acceptable
	Pok_Display_setSpritePos(index, x, y);

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(surface_setHwSpritePos_obj, 4, 4, surface_setHwSpritePos);

STATIC mp_obj_t surface_set_clip(size_t n_args, const mp_obj_t *args) {

    if(n_args > 1) {
        // Set clip rect for the screen buffer.
        mp_obj_rect_t *rect = MP_OBJ_TO_PTR(args[1]);
        Pok_Display_setClipRect(rect->x, rect->y, rect->w, rect->h);
    }
    else {
        // Remove clip rect from the screen buffer.
        Pok_Display_setClipRect(0, 0, 0, 0);
    }


	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(surface_set_clip_obj, 1, 2, surface_set_clip);

STATIC const mp_rom_map_elem_t surface_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get_rect), MP_ROM_PTR(&surface_get_rect_obj) },
    { MP_ROM_QSTR(MP_QSTR_fill), MP_ROM_PTR(&surface_fill_obj) },
    { MP_ROM_QSTR(MP_QSTR_blit), MP_ROM_PTR(&surface_blit_obj) },
    { MP_ROM_QSTR(MP_QSTR_setHwSprite), MP_ROM_PTR(&surface_setHwSprite_obj) },
    { MP_ROM_QSTR(MP_QSTR_setHwSpritePos), MP_ROM_PTR(&surface_setHwSpritePos_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_clip), MP_ROM_PTR(&surface_set_clip_obj) },
};
STATIC MP_DEFINE_CONST_DICT(surface_locals_dict, surface_locals_dict_table);

STATIC const mp_obj_type_t mp_type_surface = {
    { &mp_type_type },
    .name = MP_QSTR_Surface,
    .make_new = surface_make_new,
    .buffer_p = { .get_buffer = surface_get_buffer },
    .locals_dict = (mp_obj_dict_t*)&surface_locals_dict,
};

STATIC const mp_rom_map_elem_t surface_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_surface) },  // surface module name
    { MP_ROM_QSTR(MP_QSTR_Surface), MP_ROM_PTR(&mp_type_surface) },
 };

STATIC MP_DEFINE_CONST_DICT(surface_module_globals, surface_module_globals_table);

const mp_obj_module_t mp_module_surface = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&surface_module_globals,
};

// *** display module

STATIC mp_obj_t display_init(void) {
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(display_init_obj, display_init);

STATIC mp_obj_t display_quit(void) {
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(display_quit_obj, display_quit);

STATIC mp_obj_t display_set_mode(void) {

	 // Create special surface
     mp_obj_surface_t *o = m_new_obj(mp_obj_surface_t);
     o->base.type = &mp_type_surface;
     o->width = Pok_Display_getWidth();
     o->height = Pok_Display_getHeight();

    return o;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(display_set_mode_obj, display_set_mode);

// Update the screen and subsustems (audio, events, etc.). Takes in to account FPS limit. Optionally, updates the screen immediately and skip other updates.
STATIC mp_obj_t display_update(size_t n_args, const mp_obj_t *args) {

    bool directDraw = false;
    if( n_args > 0 )
        directDraw = mp_obj_is_true(args[0]);

    mp_obj_rect_t rect = { .x=0, .y=0, .w=Pok_Display_getWidth(), .h=Pok_Display_getHeight() };
    if( n_args > 1 ) {
        // Get the update rect.
        mp_obj_rect_t* rect2 = MP_OBJ_TO_PTR(args[1]);
        rect.x = rect2->x; rect.y = rect2->y; rect.w = rect2->w; rect.h = rect2->h;
    }

    // Get drawNow parameter.
    bool drawNow = false;
    if( n_args > 2 )
        drawNow = mp_obj_is_true(args[2]);

    if( drawNow ) {
        // Draw the screen surface immediately to the display. Do not care about fps limits. Do not run event loops etc.
        Pok_Display_update(directDraw, rect.x, rect.y, rect.w, rect.h);
    }
    else {
        // Run the event loops, audio loops etc. Draws the screen when the fps limit is reached and returns true.

        while( ! Pok_Core_update(directDraw, rect.x, rect.y, rect.w, rect.h) )
        {}

        // This call updates the screen.
        (void)Pok_Core_update(directDraw, rect.x, rect.y, rect.w, rect.h);

    }
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_update_obj, 0, 3, display_update);

STATIC mp_obj_t display_flip(void) {
	bool doUpdate = false;
	while( !doUpdate )
	{
		doUpdate = Pok_Core_update(false, 0, 0, Pok_Display_getWidth(), Pok_Display_getHeight());
		#if UNIX  // windows or unix
			//sleep(10);
		#endif
	}
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(display_flip_obj, display_flip);

// Set custom palette
STATIC mp_obj_t display_set_palette(size_t n_args, const mp_obj_t *args) {

    if (n_args == 1) {

        // Create the array of 16-bit ints from the list.
        mp_obj_list_t *aListOfTuples = MP_OBJ_TO_PTR(args[0]);
        uint16_t palette16[16];
        for(int i = 0; i < min(aListOfTuples->len, 16); i++) {
            mp_obj_tuple_t *rgbTuple = MP_OBJ_TO_PTR(aListOfTuples->items[i]);
            uint8_t r = mp_obj_get_int(rgbTuple->items[0]);
            uint8_t g = mp_obj_get_int(rgbTuple->items[1]);
            uint8_t b = mp_obj_get_int(rgbTuple->items[2]);
            palette16[i] = POK_game_display_RGBto565(r, g, b);
        }

        // Set palette.
        POK_game_display_setPalette(palette16, min(aListOfTuples->len, 16));
    }
    else {

        // TODO: Revert to the default palette
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_set_palette_obj, 0, 1, display_set_palette);

// Set custom palette
STATIC mp_obj_t display_set_palette_16bit(size_t n_args, const mp_obj_t *args) {

    // Create the array of 16-bit ints from the list.
    mp_obj_list_t *aListOfInts = MP_OBJ_TO_PTR(args[0]);
    uint16_t palette16bit[16];
    for(int i = 0; i < min(aListOfInts->len, 16); i++) {
        uint16_t color16bit = mp_obj_get_int(aListOfInts->items[i]);
        palette16bit[i] = color16bit;
    }

    // Set palette.
    POK_game_display_setPalette(palette16bit, min(aListOfInts->len, min(aListOfInts->len, 16)));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(display_set_palette_16bit_obj, 1, 1, display_set_palette_16bit);

STATIC const mp_rom_map_elem_t display_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_display) },  // display module name
    { MP_ROM_QSTR(MP_QSTR_init), MP_ROM_PTR(&display_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_quit), MP_ROM_PTR(&display_quit_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_mode), MP_ROM_PTR(&display_set_mode_obj) },
    { MP_ROM_QSTR(MP_QSTR_flip), MP_ROM_PTR(&display_flip_obj) },
    { MP_ROM_QSTR(MP_QSTR_update), MP_ROM_PTR(&display_update_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_palette), MP_ROM_PTR(&display_set_palette_obj) },
    { MP_ROM_QSTR(MP_QSTR_set_palette_16bit), MP_ROM_PTR(&display_set_palette_16bit_obj) },
 };

STATIC MP_DEFINE_CONST_DICT(display_module_globals, display_module_globals_table);

const mp_obj_module_t mp_module_display = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&display_module_globals,
};

// *** upygame module

STATIC const mp_rom_map_elem_t pygame_module_globals_table[] = {

    // Modules
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_upygame) },  	// pygame module name
    { MP_ROM_QSTR(MP_QSTR_display), MP_ROM_PTR(&mp_module_display) },  	// display module
    { MP_ROM_QSTR(MP_QSTR_surface), MP_ROM_PTR(&mp_module_surface) },  	// surface module
    { MP_ROM_QSTR(MP_QSTR_event), MP_ROM_PTR(&mp_module_event) },  		// event module
    { MP_ROM_QSTR(MP_QSTR_mixer), MP_ROM_PTR(&mp_module_mixer) },  	// Audio Mixer module

    // Classes
    { MP_ROM_QSTR(MP_QSTR_Rect), MP_ROM_PTR(&mp_type_rect) },	// Event class

    // Consts
    { MP_ROM_QSTR(MP_QSTR_NOEVENT), MP_ROM_INT(PYGAME_NOEVENT) },
    { MP_ROM_QSTR(MP_QSTR_KEYDOWN), MP_ROM_INT(PYGAME_KEYDOWN) },
    { MP_ROM_QSTR(MP_QSTR_KEYUP), MP_ROM_INT(PYGAME_KEYUP) },
    { MP_ROM_QSTR(MP_QSTR_KEY_z), MP_ROM_INT(PYGAME_K_z) },
    { MP_ROM_QSTR(MP_QSTR_KEY_x), MP_ROM_INT(PYGAME_K_x) },
    { MP_ROM_QSTR(MP_QSTR_KEY_c), MP_ROM_INT(PYGAME_K_c) },
    { MP_ROM_QSTR(MP_QSTR_QUIT), MP_ROM_INT(PYGAME_QUIT) },
    { MP_ROM_QSTR(MP_QSTR_K_UP), MP_ROM_INT(PYGAME_K_UP) },
    { MP_ROM_QSTR(MP_QSTR_K_DOWN), MP_ROM_INT(PYGAME_K_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_K_LEFT), MP_ROM_INT(PYGAME_K_LEFT) },
    { MP_ROM_QSTR(MP_QSTR_K_RIGHT), MP_ROM_INT(PYGAME_K_RIGHT) },
    { MP_ROM_QSTR(MP_QSTR_BUT_A), MP_ROM_INT(PYGAME_K_ABUT) },
    { MP_ROM_QSTR(MP_QSTR_BUT_B), MP_ROM_INT(PYGAME_K_BBUT) },
    { MP_ROM_QSTR(MP_QSTR_BUT_C), MP_ROM_INT(PYGAME_K_CBUT) },

};

STATIC MP_DEFINE_CONST_DICT(pygame_module_globals, pygame_module_globals_table);

const mp_obj_module_t mp_module_pygame = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&pygame_module_globals,
};

#endif // MICROPY_PY_FRAMEBUF
