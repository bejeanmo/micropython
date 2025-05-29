/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2014 Damien P. George
 * Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
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

#include "py/obj.h"
#include "py/stream.h"
#include "py/mpstate.h"
#include "py/mphal.h"
#include "py/lexer.h"
#include "extmod/misc.h"
#include "shared/runtime/pyexec.h"
#include "sblservice.h"

//uint32_t setjmp(BASE_LIBRARY_JUMP_BUFFER *jump_buf)
//{
//  return SetJump (jump_buf);
//}
//
//void longjmp(BASE_LIBRARY_JUMP_BUFFER *jump_buf, UINTN value)
//{
//  LongJump (jump_buf, value);
//}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *ss1 = s1, *ss2 = s2;
    while (n--) {
        int c = *ss1++ - *ss2++;
        if (c) {
            return c;
        }
    }
    return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    if (s < d && d < s + n) {
        // Need to copy backwards.
        d += n - 1;
        s += n - 1;
        while (n--) {
            *d-- = *s--;
        }
    } else {
        // Can copy forwards.
        while (n--) {
            *d++ = *s++;
        }
    }
    return dest;
}

void *memcpy(void *dest, const void *src, size_t n) {
    return memmove(dest, src, n);
}

void *memset(void *s, int c, size_t n) {
    unsigned char *ss = s;
    while (n--) {
        *ss++ = c;
    }
    return s;
}

size_t strlen(const char *s) {
    const char *ss = s;
    while (*ss) {
        ++ss;
    }
    return ss - s;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == c) {
            return (char *)s;
        }
        ++s;
    }
    return NULL;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n > 0 && *s1 && *s2) {
        int c = *s1++ - *s2++;
        --n;
        if (c) {
            return c;
        }
    }
    if (n == 0) {
        return 0;
    }
    return *s1 - *s2;
}

int strcmp(const char *s1, const char *s2) {
    return strncmp(s1, s2, 0x7fffffff);
}

VOID CpuPause (VOID)
{
  __asm__ __volatile__ ("pause");
}

NORETURN VOID CpuDeadLoop (VOID)
{
  volatile UINTN  Index;

  for (Index = 0; Index == 0;) {
    CpuPause ();
  }
	for (;;); //make compiler happy
}

void __stack_chk_fail(void) {
    static bool failed_once;

    if (failed_once) {
        return;
    }
    failed_once = true;
    sbl_service->DebugPrint (DEBUG_ERROR, "Stack corruption detected !\n");
    CpuDeadLoop ();
}

void __assert_fail(const char *__assertion, const char *__file,
    unsigned int __line, const char *__function) {
    sbl_service->DebugPrint (DEBUG_ERROR,"Assert at %s:%d:%s() \"%s\" failed\n", __file, __line, __function, __assertion);
    CpuDeadLoop ();
}

void debug_assert (char *file_name, uint32_t line, char *msg)
{
  sbl_service->DebugPrint (DEBUG_ERROR, "ASSERT %a(%d):%a\n", file_name, line, msg);
  CpuDeadLoop ();
}

/**
  Helper method used to print error message.
**/
void stderr_print_strn (void *env, const char *str, size_t len)
{
	sbl_service->ConsoleWrite ((char*)str, len);
}

const mp_print_t mp_stderr_print = {NULL, stderr_print_strn};

/**
  Helper method required by MicroPython to handle unrecoverable exception.
**/
void nlr_jump_fail (void *val)
{
  mp_hal_stdout_tx_str ("FATAL: uncaught NLR\n");
	for (;;); // needed to silence compiler warning
}

int usleep (useconds_t us)
{
	sbl_service->MicroSecondDelay (us);
	return 0;
}

void mp_hal_delay_ms(mp_uint_t ms) {
  usleep((ms) * 1000);
}

void mp_hal_delay_us (mp_uint_t us)
{
	usleep (us);
}

UINT64 ReadMsr64 (UINT32 Msr)
{
	UINT64  Value;
	__asm__ __volatile__ (
		"rdmsr"
		: "=A"(Value)
		: "c"(Msr)
	);
	return Value;
}

UINT64 ReadTsc (VOID)
{
	UINT64  Value;
	__asm__ __volatile__ (
		"rdtsc"
		: "=A"(Value)
	);
	return Value;
}

mp_uint_t mp_hal_ticks_us (void)
{
	UINT8 Ratio;
	Ratio = (UINT8)(ReadMsr64 (0xCE) >> 8);
	if (Ratio == 0) {
		// This might be QEMU case
		Ratio = 8;
	}
	return ReadTsc()/(Ratio * 100);
}

mp_uint_t mp_hal_ticks_ms (void)
{
	return (mp_hal_ticks_us()/1000);
}

mp_uint_t mp_hal_ticks_cpu (void)
{
	return ReadTsc();
}

mp_uint_t mp_hal_stdout_tx_strn (const char *str, size_t len)
{
	sbl_service->ConsoleWrite ((char*)str, len);
	return mp_os_dupterm_tx_strn (str, len);
}

int mp_hal_stdin_rx_chr (void)
{
	UINT8   ch;

  ch = 0;
	sbl_service->ConsoleRead (&ch, 1);

	return (int)ch;
}

int byte_num_to_dec_string(uint8_t num, char *str, size_t str_size)
{
		int start = 0;
		if (str_size < 4) {
				return -1; // Not enough space for an 8-bit decimal number + null
		}
		if (num >= 100) {
			str[start++] = '0' + (num / 100); // Hundreds place
		} else if (num >= 10) {
			str[start++] = '0' + ((num / 10) % 10); // Tens place
		} else {
			str[start++] = '0' + (num % 10); // ones place
		}
		str[start++] = '\0'; // Null-terminate the string
		return start - 1;
}

// simplify this to avoid implementing snprintf
void mp_hal_move_cursor_back(unsigned int pos) {
    if (pos <= 4) {
        // fast path for most common case of 1 step back
        mp_hal_stdout_tx_strn("\b\b\b\b", pos);
    } else {
        char vt100_command[6];
        // snprintf needs space for the terminating null character
				vt100_command[0] = 0x1B;
				vt100_command[1] = '[';
				int n = byte_num_to_dec_string(pos, &vt100_command[2], sizeof(vt100_command) - 2);
        //int n = snprintf(&vt100_command[0], sizeof(vt100_command), "\x1b[%u", pos);

        if (n > 0) {
            vt100_command[n] = 'D'; // replace null char
            mp_hal_stdout_tx_strn(vt100_command, n + 1);
        }
    }
}

void mp_hal_erase_line_from_cursor(unsigned int n_chars_to_erase) {
    (void)n_chars_to_erase;
    mp_hal_stdout_tx_strn("\x1b[K", 3);
}


uintptr_t mp_hal_stdio_poll (uintptr_t poll_flags)
{
	uintptr_t ret = 0;
	if ((poll_flags & MP_STREAM_POLL_RD) && sbl_service->ConsolePoll()) {
		ret |= MP_STREAM_POLL_RD;
	}
	return ret;
}
