#include <stdlib.h>
#include <stdio.h>

#include "ppm.h"

void update_frequency_ppm(u_int32_t freq[NUM_OF_SYM+1], u_int32_t index)
{
    for (u_int32_t i = index; i <= NUM_OF_SYM; ++i)
    {
        freq[i] += PPM_AGGRESSIVNESS;
    }
    if (freq[NUM_OF_SYM] >= MAX_FREQUENCY_PPM)
    {
        u_int32_t mistake = 0;
        for (u_int32_t i = 1; i <= NUM_OF_SYM; ++i)
        {
            freq[i] = (u_int32_t)((freq[i] + 1)/(PPM_AGGRESSIVNESS+1)) + mistake;
            if (freq[i] == freq[i-1])
            {
                ++freq[i];
                ++mistake;
            }
        }
    }
}

void bits_plus_follow_ppm(FILE *ofp, unsigned char *byte_out, u_int8_t *bits_in_byte_out, u_int32_t *bits_to_follow, u_int8_t bit)
{
    if (*bits_in_byte_out == BITS_IN_BYTE) // if the byte full push it to output buffer
    {
        fwrite(byte_out, 1, sizeof(*byte_out), ofp);
        *bits_in_byte_out = 0;
    };
    *byte_out = (*byte_out << 1) | bit; // push bit
    ++(*bits_in_byte_out);
    bit = (~bit) & 0x01; // inverse pushed bit before bits_to_follow
    while (*bits_to_follow)
    {
        if (*bits_in_byte_out == BITS_IN_BYTE)
        {
            fwrite(byte_out, 1, sizeof(*byte_out), ofp);
            *bits_in_byte_out = 0;
        }
        *byte_out = (*byte_out << 1) | bit; // push bit
        ++(*bits_in_byte_out);
        --(*bits_to_follow);
    }
}

void compress_ppm(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    
    u_int32_t *tables[NUM_OF_SYM];
    for (int i = 0; i < NUM_OF_SYM; ++i)
    {
        tables[i] = (u_int32_t*)malloc((NUM_OF_SYM+1) * sizeof(u_int32_t));
        for(u_int32_t j = 0; j <= NUM_OF_SYM; ++j)
        {
            tables[i][j] = j;
        }
    }


    size_t l_i = 0;
    size_t h_i = BITNESS;
    u_int32_t first_qtr = (h_i+1)/4;
    u_int32_t half = first_qtr*2;
    u_int32_t third_qtr = first_qtr*3;
    u_int32_t bits_to_follow = 0;
    size_t l_i_prev, h_i_prev;
    u_int32_t bytes_read;
    u_int32_t pre_index = 0;
    u_int32_t index = 1;

    unsigned char buff[FILE_BUFFERS_SIZE];
    unsigned char byte_out = 0;
    u_int8_t bits_in_byte_out = 0;

    fseek(ifp, 0, SEEK_END);
    u_int32_t size = ftell(ifp);
    fwrite(&size, 1, sizeof(size), ofp); // write size of file
    rewind(ifp);

    do
    {
        bytes_read = fread(buff, 1, sizeof(buff), ifp);
        for (u_int32_t j = 0; j < bytes_read; ++j)
        {
            pre_index = index - 1;
            index = (int)buff[j] + 1;
            l_i_prev = l_i;
            h_i_prev = h_i;
            l_i = l_i_prev + tables[pre_index][index-1]*(h_i_prev - l_i_prev + 1)/tables[pre_index][NUM_OF_SYM];
            h_i = l_i_prev + tables[pre_index][index]*(h_i_prev - l_i_prev + 1)/tables[pre_index][NUM_OF_SYM] - 1;
            update_frequency_ppm(tables[pre_index], index);
            for (;;)
            { // Обрабатываем варианты
                if (h_i < half)// переполнения
                {
                    bits_plus_follow_ppm(ofp, &byte_out, &bits_in_byte_out, &bits_to_follow, 0);
                }
                else if (l_i >= half)
                {
                    bits_plus_follow_ppm(ofp, &byte_out, &bits_in_byte_out, &bits_to_follow, 1);
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

    } while (!feof(ifp));

    ++bits_to_follow;
    if (l_i < first_qtr)
    {
        bits_plus_follow_ppm(ofp, &byte_out, &bits_in_byte_out, &bits_to_follow, 0);
    }
    else
    {
        bits_plus_follow_ppm(ofp, &byte_out, &bits_in_byte_out, &bits_to_follow, 1);
    }

    while ((bits_in_byte_out < BITS_IN_BYTE) && (bits_in_byte_out))
    {
        byte_out = byte_out << 1; // push 0
        ++bits_in_byte_out;
    }
    if (bits_in_byte_out == BITS_IN_BYTE)
    {
        fwrite(&byte_out, 1, sizeof(byte_out), ofp);
    }

    fclose(ifp);
    fclose(ofp);
}

void decompress_ppm(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");


    u_int32_t *tables[NUM_OF_SYM];
    for (int i = 0; i < NUM_OF_SYM; ++i)
    {
        tables[i] = (u_int32_t*)malloc((NUM_OF_SYM+1) * sizeof(u_int32_t));
        for(u_int32_t j = 0; j <= NUM_OF_SYM; ++j)
        {
            tables[i][j] = j;
        }
    }


    size_t l_i = 0;
    size_t l_i_prev = l_i;
    size_t h_i = BITNESS;
    size_t h_i_prev = h_i;
    size_t first_qtr = (h_i+1)/4;
    size_t half = first_qtr*2;
    size_t third_qtr = first_qtr*3;
    u_int8_t bits_left;
    unsigned char c;
    u_int8_t input_buff;
    u_int32_t pre_index = 0;
    u_int32_t j = 1;

    u_int32_t size;
    fread(&size, 1, sizeof(size), ifp);

    u_int32_t value;
    fread(&input_buff, 1, sizeof(input_buff), ifp);
    value = input_buff;
    value = value << 8;
    fread(&input_buff, 1, sizeof(input_buff), ifp);
    value += input_buff;
    value = value << 8;
    fread(&input_buff, 1, sizeof(input_buff), ifp);
    value += input_buff;
    value = value << 8;
    fread(&input_buff, 1, sizeof(input_buff), ifp);
    value += input_buff;
    bits_left = 0;


    for(u_int32_t i = 0; i < size; ++i)
    {
        pre_index = j - 1;
        for (j = 1; (size_t)(tables[pre_index][j]*(h_i - l_i + 1)/tables[pre_index][NUM_OF_SYM]) + l_i <= value; ++j); // Находим его индекс
        l_i_prev = l_i;
        h_i_prev = h_i;
        l_i = l_i_prev + tables[pre_index][j-1]*(h_i_prev - l_i_prev + 1)/tables[pre_index][NUM_OF_SYM];
        h_i = l_i_prev + tables[pre_index][j]*(h_i_prev - l_i_prev + 1)/tables[pre_index][NUM_OF_SYM] - 1;
        update_frequency_ppm(tables[pre_index], j);
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

    fclose(ifp);
    fclose(ofp);
}
