/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Paul Sokolovsky
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

#include "py/runtime.h"
#include "py/stackctrl.h"

/* CSRB: taking the address of a local is not a reliable way of asking where
 * the machine stack currently is.  AddressSanitizer moves address taken locals
 * into a per thread "fake stack" allocated from the heap, so &local in one
 * frame and &local in another are unrelated addresses: their difference says
 * nothing about depth, and the result is fed to both the recursion check and
 * the GC's conservative stack scan.  The frame address is the real one either
 * way. */
#if defined(__GNUC__) || defined(__clang__)
#define MP_STACK_HERE() ((char*)__builtin_frame_address(0))
#else
/* No frame address builtin: the address of a local is the best available
 * answer, and is the right one wherever nothing is relocating locals. */
STATIC char *mp_stack_here(void) {
    volatile int stack_dummy;
    return (char*)&stack_dummy;
}
#define MP_STACK_HERE() mp_stack_here()
#endif

void mp_stack_ctrl_init(void) {
    MP_STATE_THREAD(stack_top) = MP_STACK_HERE();
}

void mp_stack_set_top(void *top) {
    MP_STATE_THREAD(stack_top) = (char *)top;
}

mp_uint_t mp_stack_usage(void) {
    // Assumes descending stack
    return MP_STATE_THREAD(stack_top) - MP_STACK_HERE();
}

#if MICROPY_STACK_CHECK

void mp_stack_set_limit(mp_uint_t limit) {
    MP_STATE_THREAD(stack_limit) = limit;
}

void mp_stack_check(void) {
    if (mp_stack_usage() >= MP_STATE_THREAD(stack_limit)) {
        mp_raise_recursion_depth();
    }
}

#endif // MICROPY_STACK_CHECK
