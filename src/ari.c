#include <stdlib.h>
#include <stdio.h>

#include "ari.h"


void update_frequency(u_int32_t freq[NUM_OF_SYM+1], u_int32_t index)
{
    for (u_int32_t i = index; i <= NUM_OF_SYM; ++i)
    {
        ++freq[i];
    }
    if (freq[NUM_OF_SYM] >= MAX_FREQUENCY)
    {
        for (u_int32_t i = 0; i <= NUM_OF_SYM; ++i)
        {
            freq[i] = (u_int32_t)((freq[i] + 1)/2);
        }
    }
}

u_int32_t bits_plus_follow(unsigned char out_buff[FILE_BUFFERS_SIZE], u_int32_t *out_buff_size, unsigned char *byte_out)
{
    out_buff[*out_buff_size] = *byte_out;
    *byte_out = 0;
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

    u_int32_t freq[NUM_OF_SYM+1]; // + 1 because 0 cell doesnt count
    for(u_int32_t i = 0; i <= NUM_OF_SYM; ++i)
    {
        freq[i] = i;
    }

    u_int32_t l_i = 0;
    u_int32_t h_i = 65535;
    u_int32_t first_qtr = (h_i+1)/4; // = 16384
    u_int32_t half = first_qtr*2; // = 32768
    u_int32_t third_qtr = first_qtr*3; // = 49152
    u_int32_t bits_to_follow = 0;// Сколько бит сбрасывать
    u_int32_t bytes_read, bytes_written, index, l_i_prev, h_i_prev;
    unsigned char buff[FILE_BUFFERS_SIZE];
    unsigned char out_buff[FILE_BUFFERS_SIZE];
    u_int32_t out_buff_size = 0;
    unsigned char byte_out = 0;
    u_int32_t bits_in_byte_out = 0;

    fseek(ifp, 0, SEEK_END);
    size_t size = ftell(ifp);
    fwrite(&size, 1, sizeof(size), ofp); // write size of file
    rewind(ifp);

    do
    {
        bytes_read = fread(buff, 1, sizeof(buff), ifp);
        for (u_int32_t j = 0; j < bytes_read; ++j)
        {
            index = (int)buff[j] + 1;
            l_i_prev = l_i;
            h_i_prev = h_i;
            l_i = l_i_prev + freq[index-1]*(h_i_prev - l_i_prev + 1)/freq[NUM_OF_SYM];
            h_i = l_i_prev + freq[index]*(h_i_prev - l_i_prev + 1)/freq[NUM_OF_SYM] - 1;
            update_frequency(freq, index);
            for (;;)
            { // Обрабатываем варианты
                if (h_i < half)// переполнения
                {
                    if (bits_in_byte_out == BITS_IN_BYTE) // if the byte full push it to output buffer
                    {
                        //if (!bits_plus_follow(out_buff, &out_buff_size, &byte_out)) // if the buffer is full push it to output file
                            //bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
                        bytes_written = fwrite(&byte_out, 1, sizeof(byte_out), ofp);
                        bits_in_byte_out = 0;
                    };
                    byte_out = byte_out << 1; // push 0
                    ++bits_in_byte_out;
                    while (bits_to_follow)
                    {
                        if (bits_in_byte_out == BITS_IN_BYTE)
                        {
                            //if (!bits_plus_follow(out_buff, &out_buff_size, &byte_out))
                                //bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
                            bytes_written = fwrite(&byte_out, 1, sizeof(byte_out), ofp);
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
                        //if (!bits_plus_follow(out_buff, &out_buff_size, &byte_out)) // if the buffer is full push it to output file
                            //bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
                        bytes_written = fwrite(&byte_out, 1, sizeof(byte_out), ofp);
                        bits_in_byte_out = 0;
                    }
                    byte_out = (byte_out << 1) | 1; // push 1
                    ++bits_in_byte_out;
                    while (bits_to_follow)
                    {
                        if (bits_in_byte_out == BITS_IN_BYTE)
                        {
                            //if (!bits_plus_follow(out_buff, &out_buff_size, &byte_out))
                                //bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
                            bytes_written = fwrite(&byte_out, 1, sizeof(byte_out), ofp);
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
        }

    } while(!feof(ifp));

    ++bits_to_follow;
    if (l_i < first_qtr)
    {
        if (bits_in_byte_out == BITS_IN_BYTE) // if the byte full push it to output buffer
        {
            //if (!bits_plus_follow(out_buff, &out_buff_size, &byte_out)) // if the buffer is full push it to output file
                //bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
            bytes_written = fwrite(&byte_out, 1, sizeof(byte_out), ofp);
            bits_in_byte_out = 0;
        };
        byte_out = byte_out << 1; // push 0
        ++bits_in_byte_out;
        while (bits_to_follow)
        {
            if (bits_in_byte_out == BITS_IN_BYTE)
            {
                //if (!bits_plus_follow(out_buff, &out_buff_size, &byte_out))
                    //bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
                bytes_written = fwrite(&byte_out, 1, sizeof(byte_out), ofp);
                bits_in_byte_out = 0;
            }
            byte_out = (byte_out << 1) | 1; // push 1
            ++bits_in_byte_out;
            --bits_to_follow;
        }
    }
    else
    {
        if (bits_in_byte_out == BITS_IN_BYTE) // if the byte full push it to output buffer
        {
            //if (!bits_plus_follow(out_buff, &out_buff_size, &byte_out)) // if the buffer is full push it to output file
                //bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
            bytes_written = fwrite(&byte_out, 1, sizeof(byte_out), ofp);
            bits_in_byte_out = 0;
        };
        byte_out = (byte_out << 1) | 1; // push 1
        ++bits_in_byte_out;
        while (bits_to_follow)
        {
            if (bits_in_byte_out == BITS_IN_BYTE)
            {
                //if (!bits_plus_follow(out_buff, &out_buff_size, &byte_out))
                    //bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
                bytes_written = fwrite(&byte_out, 1, sizeof(byte_out), ofp);
                bits_in_byte_out = 0;
            }
            byte_out = byte_out << 1; // push 0
            ++bits_in_byte_out;
            --bits_to_follow;
        }
    }

    if ((bits_in_byte_out < 8) && (bits_in_byte_out > 0))
    {
        while ((bits_in_byte_out < BITS_IN_BYTE) && (bits_in_byte_out))
        {
            byte_out = byte_out << 1; // push 0
            ++bits_in_byte_out;
        }
    }
    if (bits_in_byte_out == BITS_IN_BYTE)
    {
        //bits_plus_follow(out_buff, &out_buff_size, &byte_out);
        bytes_written = fwrite(&byte_out, 1, sizeof(byte_out), ofp);
    }
    if (out_buff_size)
    {
        bytes_written = fwrite(out_buff, 1, out_buff_size, ofp);
        out_buff_size = 0;
    }

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

    u_int32_t freq[NUM_OF_SYM+1]; // + 1 because 0 cell doesnt count
    for(u_int32_t i = 0; i <= NUM_OF_SYM; ++i)
    {
        freq[i] = i;
    }

    u_int32_t l_i = 0;
    u_int32_t l_i_prev = l_i;
    u_int32_t h_i = 65535;
    u_int32_t h_i_prev = h_i;
    u_int32_t first_qtr = (h_i+1)/4; // = 16384
    u_int32_t half = first_qtr*2; // = 32768
    u_int32_t third_qtr = first_qtr*3; // = 49152
    u_int8_t bits_left;
    unsigned char c;
    u_int8_t input_buff;
    u_int32_t j;

    size_t size;
    u_int32_t read_bytes = fread(&size, 1, sizeof(size_t), ifp);

    u_int16_t value;
    read_bytes = fread(&input_buff, 1, sizeof(input_buff), ifp);
    value = input_buff;
    value = value << 8;
    read_bytes = fread(&input_buff, 1, sizeof(input_buff), ifp);
    value += input_buff;
    bits_left = 0;

    for(size_t i = 0; i < size; ++i)
    {
        for (j = 1; (int)(freq[j]*(h_i - l_i + 1)/freq[NUM_OF_SYM]) + l_i <= value; ++j); // Находим его индекс
        l_i_prev = l_i;
        h_i_prev = h_i;
        l_i = l_i_prev + freq[j-1]*(h_i_prev - l_i_prev + 1)/freq[NUM_OF_SYM];
        h_i = l_i_prev + freq[j]*(h_i_prev - l_i_prev + 1)/freq[NUM_OF_SYM] - 1;
        update_frequency(freq, j);
        for(;;)
        {
            if(h_i < half)
            {}
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
            value += value + (input_buff >> 7); // Добавляем бит из файла
            input_buff = input_buff << 1;
            --bits_left;
        }
        c = (char)(j-1);
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
