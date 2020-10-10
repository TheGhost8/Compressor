#pragma once

#define FILE_BUFFERS_SIZE 8192 // input and output buffers size
#define BITS_IN_BYTE 8
#define NUM_OF_SYM 256 // 256 symbols
#define BITNESS 0xFFFFFFFF


void update_frequency(u_int32_t freq[NUM_OF_SYM+1], u_int32_t index);
void bits_plus_follow(FILE *ofp, unsigned char *byte_out, u_int8_t *bits_in_byte_out, u_int32_t *bits_to_follow, u_int8_t bit);
void compress_ari(char *ifile, char *ofile);
void decompress_ari(char *ifile, char *ofile);