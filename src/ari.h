#pragma once
#define FILE_BUFFERS_SIZE 8192 // input and output buffers size

void compress_ari(char *ifile, char *ofile);
void decompress_ari(char *ifile, char *ofile);
size_t bits_plus_follow(unsigned char out_buff[FILE_BUFFERS_SIZE], size_t *out_buff_size, unsigned char *byte_out);