#include <stdlib.h>
#include <stdio.h>

#include "ari.h"

#define BITS_IN_BYTE 8
#define NUM_OF_SYM 257 // 256 symbols + EOF
#define MAX_FREQUENCY 16383 // max frequency for 1 symbol
#define FILE_BUFFERS_SIZE 8192 // input and output buffers size


void update_frequency(size_t freq[NUM_OF_SYM])
{
    for (size_t i = 0; i < NUM_OF_SYM; ++i)
    {
        freq[i] = (size_t)((freq[i]+1)/2);
    }
}

size_t bits_plus_follow(unsigned char out_buff[FILE_BUFFERS_SIZE], size_t *out_buff_size, unsigned char *byte_out)
{
    out_buff[*out_buff_size] = *byte_out;
    byte_out = 0;
    ++(*out_buff_size);
    return FILE_BUFFERS_SIZE - *out_buff_size;
}

void compress_ari(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    /** PUT YOUR CODE HERE
      * implement an arithmetic encoding algorithm for compression
      * don't forget to change header file `ari.h`
    */

    size_t freq[NUM_OF_SYM] = {1};
    size_t l_i = 0;
    size_t h_i = 65535;
    size_t i = 0;
    size_t first_qtr = (h_i+1)/4; // = 16384
    size_t half = first_qtr*2; // = 32768
    size_t third_qtr = first_qtr*3; // = 49152
    size_t bits_to_follow = 0;// Сколько бит сбрасывать
    size_t bytes_read, bytes_written, ascii_code, l_i_prev, h_i_prev;
    unsigned char buff[FILE_BUFFERS_SIZE];
    unsigned char out_buff[FILE_BUFFERS_SIZE];
    size_t out_buff_size = 0;
    unsigned char byte_out = 0;
    size_t bits_in_byte_out = 0;

    fseek(ifp, 0, SEEK_END);
    size_t size = ftell(ifp);
    fwrite(&size, 1, sizeof(size), ofp); // write size of file
    rewind(ifp);

    do
    {
        bytes_read = fread(buff, 1, sizeof buff, ifp);
        for (size_t j = 0; j < bytes_read; ++j)
        {
            ascii_code = (int)buff[j];
            l_i_prev = l_i;
            h_i_prev = h_i;
            l_i = l_i_prev + (ascii_code-1)*(h_i_prev - l_i_prev + 1)/256;
            h_i = l_i_prev + ascii_code*(h_i_prev - l_i_prev + 1)/256 - 1;
            for (;;)
            { // Обрабатываем варианты
                if (h_i < half)// переполнения
                {
                    if (bits_in_byte_out == BITS_IN_BYTE) // if the byte full push it to output buffer
                    {
                        if (!bits_plus_follow(out_buff, &out_buff_size, &byte_out)) // if the buffer is full push it to output file
                            bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
                        bits_in_byte_out = 0;
                    }
                    byte_out = byte_out << 1; // push 0
                    ++bits_in_byte_out;
                    while (bits_to_follow)
                    {
                        if (bits_in_byte_out == BITS_IN_BYTE)
                        {
                            if (!bits_plus_follow(out_buff, &out_buff_size, &byte_out))
                                bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
                            bits_in_byte_out = 0;
                        }
                        byte_out = (byte_out << 1) | 1; // push 1
                        ++bits_in_byte_out;
                        --bits_to_follow;
                    }
                }
                else if (l_i >= half)
                {
                    if (bits_in_byte_out == BITS_IN_BYTE) // if the byte full push it to output buffer
                    {
                        if (!bits_plus_follow(out_buff, &out_buff_size, &byte_out)) // if the buffer is full push it to output file
                            bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
                        bits_in_byte_out = 0;
                    }
                    byte_out = (byte_out << 1) | 1; // push 1
                    ++bits_in_byte_out;
                    while (bits_to_follow)
                    {
                        if (bits_in_byte_out == BITS_IN_BYTE)
                        {
                            if (!bits_plus_follow(out_buff, &out_buff_size, &byte_out))
                                bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
                            bits_in_byte_out = 0;
                        }
                        byte_out = byte_out << 1; // push 0
                        ++bits_in_byte_out;
                        --bits_to_follow;
                    }
                    l_i -= half;
                    h_i -= half;
                }
                else if ((l_i >= first_qtr) && (h_i < third_qtr))
                {
                    bits_to_follow++;
                    l_i -= first_qtr;
                    h_i -= first_qtr;
                }
                else break;
                l_i += l_i;
                h_i += h_i + 1;
            }
            /*if (freq[ascii_code] >= MAX_FREQUENCY)
                update_frequency(freq);
            ++freq[ascii_code];
            */
            /* dont forget to update the table
             *  ++i;
             *  li = li - 1 + b[j - 1] * (hi - 1 - li - 1 + 1) / delitel;
                hi = li - 1 + b[j] * (hi - 1 - li - 1 + 1) / delitel - 1;
            */
        }

    } while((!feof(ifp)) && (bytes_read == bytes_written));

    if (out_buff_size)
    {
        if (bits_in_byte_out)
        {
            bits_plus_follow(out_buff, &out_buff_size, &byte_out);
        }
        bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
    }

    /* This is an implementation of simple copying
    size_t n, m;
    unsigned char buff[8192];

    do {
        n = fread(buff, 1, sizeof buff, ifp);
        if (n)
            m = fwrite(buff, 1, n, ofp);
        else 
            m = 0;
    } while ((n > 0) && (n == m));
    */

    fclose(ifp);
    fclose(ofp);
}

void decompress_ari(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    /** PUT YOUR CODE HERE
      * implement an arithmetic encoding algorithm for decompression
      * don't forget to change header file `ari.h`
    */

    //size_t freq[NUM_OF_SYM] = {1};
    size_t l_i = 0;
    size_t l_i_prev = l_i;
    size_t h_i = 65535;
    size_t h_i_prev = h_i;
    size_t first_qtr = (h_i+1)/4; // = 16384
    size_t half = first_qtr*2; // = 32768
    size_t third_qtr = first_qtr*3; // = 49152
    u_int8_t bits_left;
    unsigned char c, input_buff;
    size_t j;

    size_t size;
    size_t read_bytes = fread(&size, 1, sizeof(size_t), ifp);
    size_t byte = sizeof(input_buff);

    u_int16_t value;
    /*
    if (size > 2)
    {
        fread(&input_buff, 1, sizeof(input_buff), ifp);
        value = input_buff;
        value << BITS_IN_BYTE;
        fread(&input_buff, 1, sizeof(input_buff), ifp);
        value = value | input_buff;
        size -= 2;
        bits_left = 0;
    }
    else
    {
        fread(&input_buff, 1, sizeof(input_buff), ifp);
        value = input_buff;
        size -= 1;
        bits_left = 0;
    }
    */
    fread(&input_buff, 1, sizeof(input_buff), ifp);
    value = input_buff;
    size -= 1;
    bits_left = 0;

    for(size_t i = 0; i <= size; ++i)
    {
        for (j = 0; (j*256)<=value; ++j); // Находим его индекс
        l_i = l_i_prev + (j-1)*(h_i_prev - l_i_prev + 1)/256;
        h_i = l_i_prev + j*(h_i_prev - l_i_prev + 1)/256 - 1;
        for(;;)
        {
            if(h_i < half);
            else if(l_i >= half)
            {
                value -= half;
                l_i-= half;
                h_i-= half;
            }
            else if((l_i >= first_qtr) && (h_i < third_qtr))
            {
                value -= first_qtr;
                l_i -= first_qtr;
                h_i -= first_qtr;
            } else break;
            l_i += l_i;
            h_i += h_i + 1;
            if (!bits_left)
            {
                fread(&input_buff, 1, sizeof(input_buff), ifp);
                bits_left = 8;
            }
            value += value + ((input_buff & 0b10000000) >> 7); // Добавляем бит из файла
            input_buff = input_buff << 1;
            --bits_left;
        }
        for (j = 0; (j*256)<=value; ++j); // Находим его индекс
        c = (char)(j);
        fwrite(&c, 1, sizeof(c), ofp); // сбрасываем символ в файл
    }

    /* This is an implementation of simple copying
    size_t n, m;
    unsigned char buff[8192];

    do {
        n = fread(buff, 1, sizeof buff, ifp);
        if (n)
            m = fwrite(buff, 1, n, ofp);
        else 
            m = 0;
    } while ((n > 0) && (n == m));
    */

    fclose(ifp);
    fclose(ofp);
}
