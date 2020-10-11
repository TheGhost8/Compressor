#pragma once

#define FILE_BUFFERS_SIZE 8192 // input and output buffers size
#define BITS_IN_BYTE 8
#define NUM_OF_SYM 256 // 256 symbols
#define MAX_FREQUENCY_PPM 0x0000FFFF
#define BITNESS 0xFFFFFFFF

void update_frequency_ppm(u_int32_t freq[NUM_OF_SYM+1], u_int32_t index);
void bits_plus_follow_ppm(FILE *ofp, unsigned char *byte_out, u_int8_t *bits_in_byte_out, u_int32_t *bits_to_follow, u_int8_t bit);
void compress_ppm(char *ifile, char *ofile);
void decompress_ppm(char *ifile, char *ofile);
