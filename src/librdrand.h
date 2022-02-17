/* vim: set expandtab cindent fdm=marker ts=2 sw=2: */
/*
 * Copyright (C) 2013-2020 Jan Tulak <jan@tulak.me>
 * Copyright (C) 2013-2022 Jirka Hladky hladky DOT jiri AT gmail DOT com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/*
    Now the legal stuff is done. This file contain the library itself.
*/
#ifndef INTEL_RDRAND_H
#define INTEL_RDRAND_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

/** ********************************************************************
 *                         TESTING NAMES
 * For testing purposes, real generating functions can be renamed
 * and all library will use stub functions with predetermined behaviour.
 * 
 * When STUB_RDRAND is defined, any rdrand calls will set all bits 
 * to 1. For access to the real rdrand, use rdrandXX_step_native()
 */
#ifdef STUB_RDRAND
	int rdrand16_step_native(uint16_t *x);
	int rdrand32_step_native(uint32_t *x);
	int rdrand64_step_native(uint64_t *x);
#endif //STUB_RDRAND


/**
 * Returned by function if a random number(s) was generated correctly.
 */
#define RDRAND_SUCCESS      1

/**
 * Returned by function if a random number(s) was NOT generated correctly.
 */
#define RDRAND_FAILURE      -1

/**
 * Returned by support-test function if the CPU support rdrand
 */
#define RDRAND_SUPPORTED    2
/**
 * Returned by support-test function if the CPU doesn't know rdrand
 */
#define RDRAND_UNSUPPORTED  -2

/**
 * Detect if the CPU support RdRand instruction.
 * Returns RDRAND_SUPPORTED  or RDRAND_UNSUPPORTED.
 */
int rdrand_testSupport();

/**
 * 16 bits of entropy through RDRAND
 *
 * The 16 bit result is zero extended to 32 bits.
 * Returns RDRAND_SUCCESS on success, or RDRAND_FAILURE on underflow.
 */
int rdrand16_step(uint16_t *x);

/**
 * 32 bits of entropy through RDRAND
 *
 * Returns RDRAND_SUCCESS on success, or RDRAND_FAILURE on underflow.
 */
int rdrand32_step(uint32_t *x);

/**
 * 64 bits of entropy through RDRAND
 *
 * Returns RDRAND_SUCCESS on success, or RDRAND_FAILURE on underflow.
 */
int rdrand64_step(uint64_t *x);

/**
 * Get a 16 bit random number
 *
 * The 16 bit result is zero extended to 32 bits.
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT.
 * Returns RDRAND_SUCCESS on success, or RDRAND_FAILURE on underflow.
 */
int rdrand_get_uint16_retry(uint16_t *dest, int retry_limit);

/**
 * Get a 32 bit random number
 *
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT.
 * Returns RDRAND_SUCCESS on success, or RDRAND_FAILURE on underflow.
 */
int rdrand_get_uint32_retry(uint32_t *dest, int retry_limit);

/**
 * Get a 64 bit random number
 *
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT.
 * Returns RDRAND_SUCCESS on success, or RDRAND_FAILURE on underflow.
 */
int rdrand_get_uint64_retry(uint64_t *dest, int retry_limit);

/**
 * Get an array of 16 bit random numbers
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired
 * For higher speed, uses 64bit generating when possible.
 */
unsigned int rdrand_get_uint16_array_retry(uint16_t *dest, const unsigned int count, int retry_limit);

/**
 * Get an array of 32 bit random numbers
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired
 * For higher speed, uses 64bit generating when possible.
 */
unsigned int rdrand_get_uint32_array_retry(uint32_t *dest, const unsigned int count, int retry_limit);

/**
 * Get an array of 64 bit random numbers
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired
 */
unsigned int rdrand_get_uint64_array_retry(uint64_t *dest, const unsigned int count, int retry_limit);

/**
 * Get an array of 8 bit random numbers
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired
 * For higher speed, uses 64bit generating when possible.
 */
unsigned int rdrand_get_uint8_array_retry(uint8_t *dest,  const unsigned int count, int retry_limit);

/**
 * Get bytes of random values.
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired.
 * For higher speed, uses 64bit generating when possible.
 */
size_t rdrand_get_bytes_retry(void *dest, const size_t size, int retry_limit);

/**
 * Write count bytes of random data to a file.
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired.
 */
size_t rdrand_fwrite(FILE *f, const size_t count, int retry_limit);


/**
 * Get an array of 64 bit random values.
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired.
 *
 * Force reseed by waiting few microseconds before each generating.
 */
unsigned int rdrand_get_uint64_array_reseed_delay(uint64_t *dest, const unsigned int count, int retry_limit);


/**
 * Get an array of 64 bit random values.
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired.
 *
 * Force reseed by generating and throwing away 1024 values per one saved.
 */
unsigned int rdrand_get_uint64_array_reseed_skip(uint64_t *dest, const unsigned int count, int retry_limit);

#endif

