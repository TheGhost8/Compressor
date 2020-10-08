#pragma once

#define BITS_IN_BYTE 8
#define NUM_OF_SYM 256 // 256 symbols
#define MAX_FREQUENCY 0x0000FFFF // max frequency for 1 symbol, other symbols will have 64 min segments
#define FILE_BUFFERS_SIZE 8192 // input and output buffers size

void update_frequency(u_int32_t freq[NUM_OF_SYM+1], u_int32_t index);
u_int32_t bits_plus_follow(unsigned char out_buff[FILE_BUFFERS_SIZE], u_int32_t *out_buff_size, unsigned char *byte_out);
void compress_ari(char *ifile, char *ofile);
void decompress_ari(char *ifile, char *ofile);