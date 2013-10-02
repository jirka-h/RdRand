/*
gcc -Wall -Wextra -fopenmp -mrdrnd -O2 -o main main.c -lrt
*/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <x86intrin.h>
#include <time.h>
#include <omp.h>


#include "../src/rdrand.h"

#define RETRY_LIMIT 10

//#define THROUGHPUT
#define CORRECT_SIZES

int fill(uint64_t* buf, int size) {
  int j,k;
  int rc;

  for (j=0; j<size; ++j) {
    k = 0;
    do {
      rc = rdrand64_step ( &buf[j] );
      ++k;
    } while ( rc == 0 && k < RETRY_LIMIT);
    if ( rc == 0 ) return 0;
  }
  return 1;
}


/**
 * Print data given in ptr with size of bytes as a hexa number
 */
void print_numbers(char * ptr, size_t bytes)
{
    size_t i;
    for(i=bytes; i>0;i--)
    {
        fprintf(stdout,"%hhx",ptr[i]);
    }
    fprintf(stdout,"\n");

}

/**
 * Will print anything given into ptr as single bits.
 */
void print_bin(char *ptr, uint64_t bytes)
{
    uint64_t i;
    uint64_t j;
    uint64_t bits = bytes*8;


    unsigned char mask[bytes]; // enough space for all bits
    mask[bytes-1]=0x80;

    j=bytes-1;

    printf("(%llu) ",(long long unsigned int)bits);

    for(i=1; i<=bits; i++)
    {
        if (ptr[j] & mask[j]) // if the masked bit is 1
            printf("1");
        else
            printf("0");

        mask[j] >>= 1; // shift the masked bit to less important bit

        if (i % 8 == 0) //print spaces after each byte
        {
            printf(" (%llu) ",(long long unsigned int)(bits-i));
            j--;
            mask[j]=0x80;
        }
    }
    printf("\n");
}

int main(void) {
 uint64_t* buf;
 const int size=2048;
 const int threads=4;
 const int rounds=1;
 int i, j, k;
 struct timespec t[2];
 double run_time, throughput;
 int thread_error;
 int rc;

 uint16_t uint16;
 uint32_t uint32;
 uint64_t uint64;
 int x;

 omp_set_num_threads(threads);


 buf = malloc(threads * size * sizeof(uint64_t) );


#ifdef THROUGHPUT
 clock_gettime(CLOCK_REALTIME, &t[0]);
 for (i=0; i<rounds;i++)
 {

         thread_error = 0;
         //#pragma omp parallel for reduction(+ : thread_error) schedule(static, 1024)
         #pragma omp parallel for
         for (i=0; i<threads; ++i) {
             rc = fill ( &buf[i*size], size );
           //if ( rc == 0 ) { ++thread_error; }
         }
         //if ( thread_error ) return 1;
         //fwrite(buf, sizeof(uint64_t), threads * size , stdout);
         print_numbers((char*)buf,threads*size);
  }

 clock_gettime(CLOCK_REALTIME, &t[1]);

 run_time = (double)(t[1].tv_sec) - (double)(t[0].tv_sec) +
            ( (double)(t[1].tv_nsec) - (double)(t[0].tv_nsec) ) / 1.0E9;

  throughput = (double) (i) * size * sizeof(uint64_t) / run_time/1024.0/1024.0;

  fprintf(stderr, "Runtime %g, throughput %gMiB/s\n", run_time, throughput);
#endif // THROUGHPUT

#ifdef CORRECT_SIZES

    /* single numbers */

    rdrand_get_uint16_retry(&uint16,RETRY_LIMIT);
    printf("\nA 16 bit number:\n");
    print_numbers((char *)&uint16,sizeof(uint16));
    print_bin((char *)&uint16,2);

    rdrand_get_uint32_retry(&uint32,RETRY_LIMIT);
    printf("\nA 32 bit number:\n");
    print_numbers((char *)&uint32,sizeof(uint32));
    print_bin((char *)&uint32,4);


    rdrand_get_uint64_retry(&uint64,RETRY_LIMIT);
    printf("\nA 64 bit number:\n");
    print_numbers((char *)&uint64,sizeof(uint64));
    print_bin((char *)&uint64,8);

    /* bigger values like arrays */
    printf("------------------------\n");

    printf("\nA 18-bytes array (aligned):\n");
    // to make potential non-generated numbers visible
    memset(buf,0,threads * size * sizeof(uint64_t) );
    rdrand_get_bytes_retry(18,((unsigned char *)(buf)),RETRY_LIMIT);
    print_numbers( (char *)buf,18);
    print_bin( (char *)buf,18);

    printf("\nA 18-bytes array (misaligned by one byte shift):\n");
    // to make potential non-generated numbers visible
    memset(buf,0,threads * size * sizeof(uint64_t) );
    rdrand_get_bytes_retry(18,((unsigned char *)(buf))+1,RETRY_LIMIT);
    print_numbers( (char *)buf+1,18);
    print_bin( (char *)buf+1,18);

    printf("\nA 26-bytes array (misaligned):\n");
    // to make potential non-generated numbers visible
    memset(buf,0,threads * size * sizeof(uint64_t) );
    rdrand_get_bytes_retry(26,((unsigned char *)(buf))+1,RETRY_LIMIT);
    print_numbers( (char *)buf+1,26);
    print_bin( (char *)buf+1,26);

    printf("\n\n");
#endif // CORRECT_SIZES

return 0;
}

