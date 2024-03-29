/*
 * Bit field types.
 *
 * Copyright (C) 2003 Juha Aatrokoski, Timo Lilja,
 *   Leena Salmela, Teemu Takanen, Aleksi Virtanen.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * $Id: types.h,v 1.4 2012/02/03 09:22:11 tlilja Exp $
 *
 */

#ifndef TYPES_H
#define TYPES_H

/* Some handy type definitions to be used in various bitfields */

typedef unsigned char uint8_t;       /* unsigned 8-bit integer */
typedef unsigned short uint16_t;     /* unsigned 16-bit integer */
typedef unsigned int uint32_t;       /* unsigned 32-bit integer */
typedef unsigned long long uint64_t; /* unsigned 64-bit integer */

typedef signed char int8_t;       /* signed 8-bit integer */
typedef signed short int16_t;     /* signed 16-bit integer */
typedef signed int int32_t;       /* signed 32-bit integer */
typedef signed long long int64_t; /* signed 64-bit integer */

typedef uint32_t usr_sem_t;

#define UNUSED __attribute__ ((unused))
#endif
