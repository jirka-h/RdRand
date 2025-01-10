/* vim: set expandtab cindent fdm=marker ts=4 sw=4: */
/*
 * Copyright (C) 2013-2020 Jan Tulak <jan@tulak.me>
 * Copyright (C) 2013-2025 Jirka Hladky hladky DOT jiri AT gmail DOT com
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

/*
    Now the legal stuff is done. This file contain AES methods for the library.
*/
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <openssl/rand.h>
#include <openssl/evp.h>
#include "./librdrand.h"
#include "./librdrand-aes.private.h"
#include "./librdrand-aes.h"

//memory locking
#include <sys/mman.h>

//getrlimit - memory locking
#include <sys/time.h>
#include <sys/resource.h>


/*****************************************************************************/
// {{{ misc

// t_buffer* BUFFER;

aes_cfg_t AES_CFG = {.keys={.amount=0}};

/**
 * Test if number is power of two.
 * http://stackoverflow.com/questions/600293/how-to-check-if-a-number-is-a-power-of-2
 * @param  x [description]
 * @return   [description]
 */
int isPowerOfTwo(ulong x) {
    return x && (x & (x - 1)) == 0;
}
// bind current key to openssl, return 0 on failure
int key_to_openssl() {
    if ( EVP_CipherInit_ex( 
                AES_CFG.en,
                EVP_aes_128_ctr(),
                NULL,
                AES_CFG.keys.key_current,
                AES_CFG.keys.nonce_current,
                1 ) != 1 ) { 
        // enc=1 => encryption, enc=0 => decryption
        perror("EVP_CipherInit_ex");
        return 0;
    };
    return 1;
}
void memDump(unsigned char *mem, unsigned int length) {
        unsigned i;
            for (i=0; length > i; i++){
                        printf("%02x",mem[i]);
                            }
                printf("\n");
}
// }}} misc

// {{{ keys_allocate/free
int keys_allocate(unsigned int amount, size_t key_length) {
    // test for valid numbers
    if (!isPowerOfTwo(key_length) || amount == 0)
        return 0;
    AES_CFG.keys.amount = amount;
    AES_CFG.keys.key_length = key_length;
    //AES_CFG.keys.nonce_length = key_length/2; 

    // init OpenSSL
    EVP_CIPHER_CTX_init( AES_CFG.en );

    // allocate first level of array
    AES_CFG.keys.keys = malloc(sizeof(char*) *  amount);
    AES_CFG.keys.nonces = malloc(sizeof(char*) * amount);
    if (AES_CFG.keys.keys == NULL || AES_CFG.keys.nonces == NULL)
        return 0;
    // and lock it
    keys_mem_lock(AES_CFG.keys.keys, amount*sizeof(char*));
    keys_mem_lock(AES_CFG.keys.nonces, amount*sizeof(char*));

    // block
    {
        unsigned int i;
        for (i=0; i < amount; i++){
            // for keys and nonces
            // allocate strings
            AES_CFG.keys.keys[i]=malloc(key_length * sizeof(char));
            // set it zero
            memset(AES_CFG.keys.keys[i], 0, AES_CFG.keys.key_length);
            // lock it
            keys_mem_lock (AES_CFG.keys.keys[i], AES_CFG.keys.key_length);

            AES_CFG.keys.nonces[i]=malloc(key_length * sizeof(char));
            memset(AES_CFG.keys.nonces[i], 0, key_length);
            keys_mem_lock (AES_CFG.keys.nonces[i], key_length);
        }
    }
    // set current to [0]
    AES_CFG.keys.key_current = AES_CFG.keys.keys[0];
    AES_CFG.keys.nonce_current = AES_CFG.keys.nonces[0];

    return 1;
}

/**
 * Destroy saved keys and free the memory.
 */
void keys_free() {
    if (AES_CFG.keys.keys == NULL) {
        // If there is nothing to free
        return;
    }



    AES_CFG.keys.key_current = NULL;
    AES_CFG.keys.nonce_current = NULL;
    AES_CFG.keys.index = 0;

    // Destroy keays in memory.
    // Overwrite all keys and nonces, then free them.
    // At the end, do the same for the arrays.
    {
        unsigned int i;
        for (i=0; i < AES_CFG.keys.amount; i++) {
            memset(AES_CFG.keys.keys[i], 0, AES_CFG.keys.key_length);
            keys_mem_unlock (AES_CFG.keys.keys[i], AES_CFG.keys.key_length);
            free(AES_CFG.keys.keys[i]);

            memset(AES_CFG.keys.nonces[i], 0, AES_CFG.keys.key_length);
            keys_mem_unlock(AES_CFG.keys.nonces[i], AES_CFG.keys.key_length);
            free(AES_CFG.keys.nonces[i]);
        }
    }

    memset(AES_CFG.keys.keys, 0, AES_CFG.keys.amount*sizeof(char*));
    memset(AES_CFG.keys.nonces, 0, AES_CFG.keys.amount*sizeof(char*));

    keys_mem_unlock(AES_CFG.keys.keys, AES_CFG.keys.amount*sizeof(char*));
    keys_mem_unlock(AES_CFG.keys.nonces, AES_CFG.keys.amount*sizeof(char*));
    
    free(AES_CFG.keys.keys);
    free(AES_CFG.keys.nonces);

    AES_CFG.keys.keys = NULL;
    AES_CFG.keys.nonces = NULL;

    // clean openssl
    if ( EVP_CIPHER_CTX_cleanup(AES_CFG.en) != 1 ) {
        perror("EVP_CIPHER_CTX_cleanup");
    }
}
// }}} keys_allocate/free

// {{{ keys_mem_lock/unlock
/**
 * Prevent memory from being swapped.
 */
int keys_mem_lock(void * ptr, size_t len) {
    struct rlimit rlim;
    unsigned int mlock_limit;

    // TODO
    //(void) ptr;
    //(void) len;
    //return TRUE;
   
//{{{ getrlimit
  if ( getrlimit(RLIMIT_MEMLOCK, &rlim) == 0 ) {
    mlock_limit = rlim.rlim_cur;
    //fprintf(stderr, "INFO:  getrlimit(RLIMIT_MEMLOCK, rlim) reports: current limit %d, maximum (ceiling) %d.\n", rlim.rlim_cur, rlim.rlim_max);
    mlock_limit /= 4;
  } else {
    mlock_limit = 16384;
    fprintf (stderr, 
        "\nWARNING: keys_mem_lock: "
        "getrlimit(RLIMIT_MEMLOCK, rlim) has failed. "
        "Using %u as default limit. Reported error: %s\n", 
        mlock_limit,  
        strerror (errno) 
    );
    return 0;
  }
//}}}

  if ( len <= mlock_limit ) {
    if ( mlock(ptr, len ) == 0 ) {
    } else {
      fprintf (stderr, 
            "\nWARNING: Function init_buffer: "
            "cannot lock buffer to RAM (preventing that memory "
            "from being paged to the swap area)\n"
            "\tSize of buffer is %lu Bytes. "
            "Reported error: \"%s\". "
            "See man -S2 mlock for the interpretation.\n", 
            len, 
            strerror (errno) 
        );
      return 0;
    }
  } 
  return 1;
}

/**
 * Allow memory to be swapped again.
 */
int keys_mem_unlock(void * ptr, size_t len) {
    // TODO
    //(void) ptr;
    //(void) len;
    //return TRUE;
    
    memset(ptr, 0, len);
    if ( munlock(ptr, len ) != 0) {
      fprintf (stderr, "\nWARNING: Function destroy_buffer: "
              "cannot unlock buffer from RAM (lock prevents memory "
              "from being paged to the swap area).\n"
          "Size of buffer is %lu Bytes. Reported error: %s\n", 
          len, strerror (errno) );
      return 0;
    }
    return 1;
}
// }}} keys_mem_lock/unlock


/**
 * Set manually keys for AES.
 * These keys will be rotated randomly.
 *
 * @param  amount     Count of keys
 * @param  key_length Length of all keys in bytes
 *                    (must be pow(2))
 * @param  nonces     Array of nonces. Nonces have to be half of
 *                    length of keys.
 * @param  keys       Array of keys. All have to be the same length.
 * @return            1 if the keys were successfuly set
*/
// {{{ rdrand_set_aes_keys
int rdrand_set_aes_keys(unsigned int amount,
                        size_t key_length,
                        unsigned char **keys,
                        unsigned char **nonces) {
    // don't allow bad key lengths
    if(key_length <= RDRAND_MIN_KEY_LENGTH || key_length >= RDRAND_MAX_KEY_LENGTH)
        return 0;

    AES_CFG.en = EVP_CIPHER_CTX_new();
    AES_CFG.keys.index=0;
    AES_CFG.keys.next_counter=MAX_COUNTER;
    AES_CFG.keys_type = KEYS_GIVEN;
    AES_CFG.keys.key_current = NULL;
    if (keys_allocate(amount, key_length) == 0) {
        return 0;
    }
    { // subblock for var. i
        unsigned int i;
        for (i=0; i<amount; i++) {
            memcpy(AES_CFG.keys.keys[i], keys[i], key_length);
            memcpy(AES_CFG.keys.nonces[i], nonces[i], (key_length/2));
        }
    }
    key_to_openssl();
    // random index
    //keys_change();
    return 1;
}
// }}} rdrand_set_aes_keys

/**
 * Set automatic key generation.
 * OpenSSL will be used as a key generator.
 */
// {{{ rdrand_set_aes_random_key
int rdrand_set_aes_random_key() {
    AES_CFG.en = EVP_CIPHER_CTX_new();
    AES_CFG.keys_type = KEYS_GENERATED;
    AES_CFG.keys.index=0;
    AES_CFG.keys.next_counter=0;
    AES_CFG.keys.key_current = NULL;
    
    if (keys_allocate(1, DEFAULT_KEY_LEN) == 0){
        return 0;
    }
    if(key_generate() == 0){
        return 0;
    }

    return 1;
}
//}}} rdrand_set_aes_random_key

/**
 * Perform cleaning of all AES related settings:
 * Discard keys, ...
 */
// {{{ rdrand_clean_aes
void rdrand_clean_aes() {
    keys_free();
    AES_CFG.keys_type=0;
    AES_CFG.keys.amount=0;
    AES_CFG.keys.key_length=0;
//    AES_CFG.keys.nonce_length=0;
    AES_CFG.keys.next_counter=0;
    EVP_CIPHER_CTX_free(AES_CFG.en);

}
// }}} rdrand_clean_aes

/**
 * Encrypt the given buffer.
 *
 * @param src    source data
 * @param dest   destination buffer
 * @param len    length of the buffer
 *
 * @return       1 on success
 */
// {{{ rdrand_enc_buffer
int rdrand_enc_buffer(void* dest, void* src, size_t len) {
    size_t i,chunks, tail;
    int out_len;
    chunks = len / MAX_BUFFER_SIZE;
    tail = len % MAX_BUFFER_SIZE;

    for (i=0; i<chunks; i++) {
        // By placing the counter at the beginning of the cycle
        // avoid situation, when counter would be just few bytes from regenerating,
        // but all MAX_BUFFER_SIZE would be generated with old key.
        if(counter(MAX_BUFFER_SIZE) == 0){
            perror("rdrand_enc_buf: counter chunks");
            return 0;
        }
        
        // encrypt full buffer
        if( EVP_EncryptUpdate(
            AES_CFG.en,
            dest+i*MAX_BUFFER_SIZE, 
            &out_len, 
            src+i*MAX_BUFFER_SIZE, 
            MAX_BUFFER_SIZE) != 1 ) {

            perror("rdrand_enc_buf: EVP_EncryptUpdate");
            return 0;
        }
    }

    if (tail != 0) {
        if( EVP_EncryptUpdate(
            AES_CFG.en,
            dest + i*MAX_BUFFER_SIZE, 
            &out_len, 
            src + i*MAX_BUFFER_SIZE, 
            tail) != 1 ) {

            perror("rdrand_enc_buf: EVP_EncryptUpdate");
            return 0;
        }
    }


    return 1;
}
// }}}

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
 * @param  dest        destination location
 * @param  count       bytes to generate
 * @param  retry_limit how many times to retry the RdRand instruction
 * @return             amount of sucessfully generated and ecrypted bytes
 */
// {{{ rdrand_get_bytes_aes_ctr
unsigned int rdrand_get_bytes_aes_ctr(
    void *dest,
    const unsigned int count,
    int retry_limit) {

    // allow enough space in output buffer for additional block (padding)
    unsigned char output[MAX_BUFFER_SIZE + EVP_MAX_BLOCK_LENGTH];
    unsigned char buf[MAX_BUFFER_SIZE];
    unsigned int buffers, tail, i, generated=0;
    int out_len;

    // keys change and such
    //counter(0);

    memset(output, 0, MAX_BUFFER_SIZE);
    // compute cycles 
    buffers = count/MAX_BUFFER_SIZE;
    tail = count % MAX_BUFFER_SIZE;
        
    for (i=0; i < buffers; i++) {
        // By placing the counter at the beginning of the cycle
        // avoid situation, when counter would be just few bytes from regenerating,
        // but all MAX_BUFFER_SIZE would be generated with old key.
        counter(MAX_BUFFER_SIZE);

        // generate full buffer
        if(rdrand_get_bytes_retry(buf, MAX_BUFFER_SIZE, retry_limit) != MAX_BUFFER_SIZE) {
            return generated;
        }
        // encrypt full buffer
         if( EVP_EncryptUpdate(AES_CFG.en, output, &out_len, buf, MAX_BUFFER_SIZE) != 1 ) {
            perror("EVP_EncryptUpdate");
            return generated;
        };

        memcpy(dest + i*MAX_BUFFER_SIZE, output, MAX_BUFFER_SIZE);
        generated += MAX_BUFFER_SIZE;
    }
    
    if(tail) {
        counter(tail);
        // generate tail
        if(rdrand_get_bytes_retry(buf, tail, retry_limit) != tail) {
            return generated;
        }
        // encrypt tail
         if( EVP_EncryptUpdate(AES_CFG.en, output, &out_len, buf, tail) != 1 ) {
            perror("EVP_EncryptUpdate");
            return generated;
        };
        memcpy(dest + i*MAX_BUFFER_SIZE, output, tail);
        generated += tail;
    }


    return generated;
}

// }}} rdrand_get_bytes_aes_ctr

/**
 * Decrement counter and if needed, change used key.
 *
 * @param num   How many changes to counter
 * @return 1 if it went ok
 */
// {{{ counter
int counter(unsigned int num) {


    int result = 0;
    // if the counter would be negative after substraction of "num"
    // (or if is zero, so it will catch even num == 0 in that case)
    // regenerate it
    if (AES_CFG.keys.next_counter == 0 || AES_CFG.keys.next_counter < num) {
        //perror("!!! DEBUG: KEY CHANGED !!!\n");
        if (AES_CFG.keys_type == KEYS_GIVEN) {
            result = keys_change(); // set a new random index
            //keys_randomize(); // set a new random timer
            AES_CFG.keys.next_counter = MAX_COUNTER;
        } else { // KEYS_GENERATED
            result = key_generate(); // generate a new key and nonce
            keys_randomize(); // set a new random timer
        }
    } else {
        AES_CFG.keys.next_counter -= num;
    }

    if(result == 0){
        return 0;
    }
    return 1;
}
// }}}

// {{{ keys and randomizing
/**
 * Set key index (and nonce too) for AES to another random one.
 * Will call keys_shuffle at the end.
 * Used when rdrand_set_aes_keys() was set.
 */

int keys_change(void) {
    /*unsigned int buf;
    if (RAND_bytes((unsigned char*)&buf, sizeof(unsigned int)) != 1) {
        fprintf(stderr, "ERROR: can't change keys index, not enough entropy!\n");
        return 0;
    }
    AES_CFG.keys.index = ((double)buf / UINT_MAX)*AES_CFG.keys.amount;
    */
    AES_CFG.keys.index = (AES_CFG.keys.index+1) % AES_CFG.keys.amount;
    AES_CFG.keys.key_current = AES_CFG.keys.keys[AES_CFG.keys.index];
    AES_CFG.keys.nonce_current = AES_CFG.keys.nonces[AES_CFG.keys.index];

    if(keys_change_rotation() == 0){
        return 0;
    }
    key_to_openssl();
    return 1;
}

/**
 * Encrypt the current key and nonce to prevent reusing the same counter.
 */
int keys_change_rotation(){
    unsigned char K[AES_CFG.keys.key_length];
    unsigned char N[AES_CFG.keys.key_length];
    int tmp;

    if ( AES_CFG.keys.key_current == NULL){
        fprintf(stderr,"An internal error in librdrand-aes.c on line %d\n", __LINE__);
        return 0;
    }

    EVP_EncryptUpdate(
            AES_CFG.en,
            K,
            &tmp,
            AES_CFG.keys.key_current,
            AES_CFG.keys.key_length) ;
    EVP_EncryptUpdate(
            AES_CFG.en,
            N,
            &tmp,
            AES_CFG.keys.nonce_current,
            AES_CFG.keys.key_length) ;

    memcpy(AES_CFG.keys.key_current, K, AES_CFG.keys.key_length);
    memcpy(AES_CFG.keys.nonce_current, N, AES_CFG.keys.key_length);
    return 1;
}


/**
 * Set a random timeout for new key generation/step.
 * Called on every key change.
 */
int keys_randomize() {
    unsigned int buf;
    if (RAND_bytes((unsigned char*)&buf, sizeof(unsigned int)) != 1) { 
        fprintf(stderr, "ERROR: can't change keys index, not enough entropy!\n");
        return 0;
    } 
    AES_CFG.keys.next_counter = ((double)buf/UINT_MAX)*MAX_COUNTER;
    
    return 1;
}

/**
 * Generate a random key.
 * Used when rdrand_set_aes_random_key() was set.
 */
int key_generate() {
    unsigned char buf[RDRAND_MAX_KEY_LENGTH] = {};
    if (RAND_bytes(buf, AES_CFG.keys.key_length) != 1) { 
        fprintf(stderr, "ERROR: can't generate key, not enough entropy!\n");
        return 0;
    }
    memcpy(AES_CFG.keys.keys[0],buf, AES_CFG.keys.key_length);

    if (RAND_bytes(buf, AES_CFG.keys.key_length) != 1) { 
        fprintf(stderr, "ERROR: can't generate nonce, not enough entropy!\n");
        return 0;
    }
    memcpy(AES_CFG.keys.nonces[0],buf, AES_CFG.keys.key_length);
    key_to_openssl();
    return 1;
}
// }}} keys and randomizing

