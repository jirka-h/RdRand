/* vim: set expandtab cindent fdm=marker ts=2 sw=2: */
/*
 * Copyright (C) 2013  Jan Tulak <jan@tulak.me>
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

/*
 * Usage:
 * 1) Initialize it by one of the two functions
 *     - rdrand_set_aes_keys
 *     - rdrand_set_aes_random_key
 * 2) Generate
 *     - rdrand_get_bytes_aes_ctr
 * 3) Clean
 *     - rdrand_clean_aes
 */
#ifndef LIBRDRAND_AES_H_INCLUDED
#define LIBRDRAND_AES_H_INCLUDED

/**
 * Default length of a key in bits.
 * Used when key is generated.
 */
#define DEFAULT_KEY_LEN 128
#define BYTES(bits) (bits>>3)/*(bits>>3)*/


/**
 * Get an array of 64 bit random values.
 * Will retry up to retry_limit times. Negative retry_limit
 * implies default retry_limit RETRY_LIMIT
 * Returns the number of bytes successfully acquired.
 *
 * All output from rdrand is passed through AES-CTR encryption.
 *
 * Either rdrand_set_aes_keys or rdrand_set_aes_random_key
 * has to be set in advance.
 * 
 * @param  dest        [description]
 * @param  count       [description]
 * @param  retry_limit [description]
 * @return             [description]
 */
unsigned int rdrand_get_bytes_aes_ctr(void *dest,
    const unsigned int count,
    int retry_limit);


/**
 * Set manually keys for AES.
 * These keys will be rotated randomly.
 * 
 *   TODO: rotate just in random times, or also random order?
 *   Maybe a shuffle at the end of the list?
 *
 * @param  amount     Count of keys
 * @param  key_length Length of all keys in bits 
 *                    (must be pow(2))
 * @param  nonces     Array of nonces. Nonces have to be half of 
 *                    length of keys.
 * @param  keys       Array of keys. All have to be the same length.
 * @return            True if the keys were successfuly set
 */
int rdrand_set_aes_keys(size_t amount,
    size_t key_length,
    char **nonces,
    char **keys);

/**
 * Set automatic key generation.
 * /dev/random will be used as a key generator.
 * 
 * @return True if the key was successfuly set
 */
int rdrand_set_aes_random_key();


/**
 * Perform cleaning of all AES related settings:
 * Discard keys, ...
 */
void rdrand_clean_aes();

#endif // LIBRDRAND_AES_H_INCLUDED
