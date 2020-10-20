#include <stdlib.h>
#include <stdio.h>

#include "ppm.h"

u_int8_t UpdateOneTable(struct OneTable *table, u_int32_t index)
{
    u_int8_t has_symbol = 0;
    u_int32_t added_size = 0;
    int32_t i;
    for (i = 0; i < table->length; ++i)
    {
        if (index == table->symbols[i].index)
        {
            has_symbol = 1;
            ++table->symbols[i].additional_size;
            ++table->total_size;
            break;
        }
    }
    if (!has_symbol)
    {
        if (table->length == table->memory)
        {
            table->memory = table->memory * 2;
            table->symbols = (struct Symbol*)realloc(table->symbols, table->memory * sizeof(struct Symbol));
        }
        ++table->esc_size;
        if (table->length == 0)
        {
            table->symbols[table->length].index = index;
            table->symbols[table->length].additional_size = 1;
        }
        else
        {
            for (i = table->length - 1; i >= 0; --i)
            {
                if (table->symbols[i].index > index)
                {
                    table->symbols[i+1] = table->symbols[i];
                    if (i == 0)
                    {
                        table->symbols[0].index = index;
                        table->symbols[0].additional_size = 1;
                    }
                }
                else
                {
                    table->symbols[i+1].index = index;
                    table->symbols[i+1].additional_size = 1;
                    break;
                }
            }
        }
        ++table->length;
        ++table->total_size;
    }
    if (table->total_size == MAX_TABLE_SIZE)
    {
        added_size = 0;
        for (i = 0; i < table->length; ++i)
        {
            table->symbols[i].additional_size = (u_int32_t)((table->symbols[i].additional_size + 1)/2);
            added_size = added_size + table->symbols[i].additional_size;
        }
        table->total_size = NUM_OF_SYM + added_size;
    }
    return has_symbol;
}

void UpdateFrequencyPPM(struct ALlTables *tables, const u_int16_t pre_index[PPM_LEVEL], u_int32_t index)
{
    u_int8_t right_table = 0;
    for (u_int32_t i = 0; i < tables->length; ++i)
    {
        right_table = 1;
        for (u_int8_t j = 0; j < PPM_LEVEL; ++j)
        {
            if (pre_index[j] != tables->all_tables[i].previous_symbols[j])
            {
                right_table = 0;
                break;
            }
        }
        if (right_table)
        {
            if (!UpdateOneTable(&tables->all_tables[i], index)) // esc_sym was used
            {
                UpdateOneTable(&tables->all_tables[0], index);
            }
            break;
        }
    }
    if (!right_table)
    {
        UpdateOneTable(&tables->all_tables[0], index);
        if (tables->length == tables->memory)
        {
            tables->memory = tables->memory * 2;
            tables->all_tables = (struct OneTable*)realloc(tables->all_tables, tables->memory * sizeof(struct OneTable));
        }
        for (u_int8_t i = 0; i < PPM_LEVEL; ++i)
        {
            tables->all_tables[tables->length].previous_symbols[i] = pre_index[i];
        }
        tables->all_tables[tables->length].memory = 10;
        tables->all_tables[tables->length].symbols = (struct Symbol*)malloc(tables->all_tables[tables->length].memory * sizeof(struct Symbol));
        tables->all_tables[tables->length].length = 0;
        tables->all_tables[tables->length].total_size = 0;
        UpdateOneTable(&tables->all_tables[tables->length], index);
        ++tables->length;
    }
}

void BitsPlusFollowPPM(FILE *ofp, struct OutputBuffer *buffer, u_int8_t bit)
{
    if (buffer->bits_in_byte_out == BITS_IN_BYTE) // if the byte full push it to output buffer
    {
        fwrite(&buffer->byte_out, 1, sizeof(buffer->byte_out), ofp);
        buffer->bits_in_byte_out = 0;
    };
    buffer->byte_out = (buffer->byte_out << 1) | bit; // push bit
    ++(buffer->bits_in_byte_out);
    bit = (~bit) & 0x01; // inverse pushed bit before bits_to_follow
    while (buffer->bits_to_follow)
    {
        if (buffer->bits_in_byte_out == BITS_IN_BYTE)
        {
            fwrite(&buffer->byte_out, 1, sizeof(buffer->byte_out), ofp);
            buffer->bits_in_byte_out = 0;
        }
        buffer->byte_out = (buffer->byte_out << 1) | bit; // push bit
        ++(buffer->bits_in_byte_out);
        --(buffer->bits_to_follow);
    }
}

u_int32_t* GetSymbolWeight(struct OneTable *table, u_int16_t index)
{
    u_int8_t new_symbol = 1;
    u_int32_t followed_size = 0;
    u_int32_t *result = (u_int32_t*)malloc(3 * sizeof(u_int32_t)); // weight of previous symbol, of current symbol and divider
    result[2] = table->total_size + table->esc_size;
    for (u_int32_t j = 0; j < table->length; ++j)
    {
        if (table->symbols[j].index == index)
        {
            new_symbol = 0;
            result[0] = followed_size;
            result[1] = followed_size + table->symbols[j].additional_size;
            break;
        }
        else if (table->symbols[j].index < index)
        {
            followed_size += table->symbols[j].additional_size;
        }
    }
    if (new_symbol)
    {
        result[0] = table->total_size;
        result[1] = table->total_size + table->esc_size;
    }
    return result;
}

u_int16_t IndexSearch(struct OneTable *table, u_int32_t value, size_t l_i, size_t h_i)
{
    u_int32_t i;
    u_int32_t symbol_weight = 0;
    u_int32_t added_weight = 0;
    for (i = 0; i < table->length; ++i)
    {
        symbol_weight = added_weight + table->symbols[i].additional_size;
        if (((size_t)((symbol_weight*(h_i-l_i+1))/(table->total_size+table->esc_size)) + l_i) > value)
        {
            return table->symbols[i].index;
        }
        else
        {
            added_weight += table->symbols[i].additional_size;
        }
    }
    return 0; // there is no sym in the table so we use escape
}

void EncodePPM(FILE *ofp, struct ALlTables *tables, struct EncodeContext *context, u_int16_t index)
{
    u_int32_t i;
    u_int8_t new_table;
    for (i = 0; i < tables->length; ++i)
    {
        new_table = 0;
        for (u_int32_t j = 0; j < PPM_LEVEL; ++j)
        {
            if (tables->all_tables[i].previous_symbols[j] != context->pre_index[j])
            {
                new_table = 1;
                break;
            }
        }
        if (!new_table)
        {
            break;
        }
    }
    if (!new_table)
    {
        if (EncodeSym(ofp, &tables->all_tables[i], context, index)) // true, if encoded esc_sym
        {
            EncodeSym(ofp, &tables->all_tables[0], context, index);
        }
    }
    else
    {
        EncodeSym(ofp, &tables->all_tables[0], context, index);
    }
}

u_int8_t EncodeSym(FILE *ofp, struct OneTable *table, struct EncodeContext *context, u_int16_t index)
{
    u_int8_t esc_sym = 0;
    u_int32_t *weights_and_divider; // weight of previous symbol, of current symbol and divider
    size_t l_i_prev = context->l_i;
    size_t h_i_prev = context->h_i;

    weights_and_divider = GetSymbolWeight(table, index);
    if (weights_and_divider[1] == table->total_size + table->esc_size)
    {
        esc_sym = 1;
    }

    context->l_i = l_i_prev + (size_t)(weights_and_divider[0]*(h_i_prev - l_i_prev + 1))/(size_t)(weights_and_divider[2]);
    context->h_i = l_i_prev + (size_t)(weights_and_divider[1]*(h_i_prev - l_i_prev + 1))/(size_t)(weights_and_divider[2]) - 1;

    for (;;)
    { // Обрабатываем варианты
        if (context->h_i < HALF)// переполнения
        {
            BitsPlusFollowPPM(ofp, &(context->buffer), 0);
        }
        else if (context->l_i >= HALF)
        {
            BitsPlusFollowPPM(ofp, &(context->buffer), 1);
            context->l_i -= HALF;
            context->h_i -= HALF;
        }
        else if ((context->l_i >= FIRST_QTR) && (context->h_i < THIRD_QTR))
        {
            context->buffer.bits_to_follow++;
            context->l_i -= FIRST_QTR;
            context->h_i -= FIRST_QTR;
        }
        else break;
        context->l_i += context->l_i;
        context->h_i += context->h_i + 1;
    }
    return esc_sym;
}

void DoneEncoding(FILE *ofp, struct EncodeContext *context)
{
    ++(context->buffer.bits_to_follow);
    if (context->l_i < FIRST_QTR)
    {
        BitsPlusFollowPPM(ofp, &(context->buffer), 0);
    }
    else
    {
        BitsPlusFollowPPM(ofp, &(context->buffer), 1);
    }

    while ((context->buffer.bits_in_byte_out < BITS_IN_BYTE) && (context->buffer.bits_in_byte_out))
    {
        context->buffer.byte_out = context->buffer.byte_out << 1; // push 0
        ++(context->buffer.bits_in_byte_out);
    }
    if (context->buffer.bits_in_byte_out == BITS_IN_BYTE)
    {
        fwrite(&(context->buffer.byte_out), 1, sizeof(context->buffer.byte_out), ofp);
    }
}

void DecodePPM(FILE *ifp, struct ALlTables *tables, struct DecodeContext *context, u_int16_t *index)
{
    u_int8_t has_table;
    u_int32_t table_index;
    for (table_index = 0; table_index < tables->length; ++table_index)
    {
        has_table = 1;
        for (u_int8_t j = 0; j < PPM_LEVEL; ++j)
        {
            if (context->pre_index[j] != tables->all_tables[table_index].previous_symbols[j])
            {
                has_table = 0;
                break;
            }
        }
        if (has_table)
        {
            break;
        }
    }

    if (has_table)
    {
        *index = IndexSearch(&tables->all_tables[table_index], context->value, context->l_i, context->h_i);
        if (*index == 0) // no such symbol
        {
            if (tables->all_tables[table_index].total_size + tables->all_tables[table_index].esc_size)
            {
                DecodeSym(ifp, &tables->all_tables[table_index], context, *index);
            }
            table_index = 0;
            *index = IndexSearch(&tables->all_tables[table_index], context->value, context->l_i, context->h_i);
        }
    }
    else
    {
        table_index = 0;
        *index = IndexSearch(&tables->all_tables[table_index], context->value, context->l_i, context->h_i);
    }

    DecodeSym(ifp, &tables->all_tables[table_index], context, *index);

    UpdateFrequencyPPM(tables, context->pre_index, *index);
}

void DecodeSym(FILE *ifp, struct OneTable *table, struct DecodeContext *context, u_int16_t index)
{
    u_int32_t *weights_and_divider;
    size_t l_i_prev = context->l_i;
    size_t h_i_prev = context->h_i;

    weights_and_divider = GetSymbolWeight(table, index);

    context->l_i = l_i_prev + (size_t)(weights_and_divider[0]*(h_i_prev - l_i_prev + 1))/(size_t)(weights_and_divider[2]);
    context->h_i = l_i_prev + (size_t)(weights_and_divider[1]*(h_i_prev - l_i_prev + 1))/(size_t)(weights_and_divider[2]) - 1;

    for(;;)
    {
        if(context->h_i < HALF)
        {}
        else if(context->l_i >= HALF)
        {
            context->value -= HALF;
            context->l_i -= HALF;
            context->h_i -= HALF;
        }
        else if((context->l_i >= FIRST_QTR) && (context->h_i < THIRD_QTR))
        {
            context->value -= FIRST_QTR;
            context->l_i -= FIRST_QTR;
            context->h_i -= FIRST_QTR;
        } else break;
        context->l_i += context->l_i;
        context->h_i += context->h_i + 1;
        if (!context->buffer.bits_left)
        {
            fread(&(context->buffer.input_buff), 1, sizeof(context->buffer.input_buff), ifp);
            context->buffer.bits_left = 8;
        }
        context->value += context->value + (context->buffer.input_buff >> 7); // Добавляем бит из файла
        context->buffer.input_buff = context->buffer.input_buff << 1;
        --context->buffer.bits_left;
    }
}

void compress_ppm(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");


    struct ALlTables tables;
    tables.memory = 256;
    tables.all_tables = (struct OneTable*)malloc(tables.memory * sizeof(struct OneTable));
    tables.length = 1; // optimize this
    tables.all_tables[0].memory = NUM_OF_SYM;
    tables.all_tables[0].symbols = (struct Symbol*)malloc(tables.all_tables[0].memory * sizeof(struct Symbol));
    tables.all_tables[0].length = NUM_OF_SYM;
    for (u_int32_t i = 0; i < tables.all_tables[0].length; ++i)
    {
        tables.all_tables[0].symbols[i].index = i+1;
        tables.all_tables[0].symbols[i].additional_size = 1;
    }
    tables.all_tables[0].total_size = NUM_OF_SYM;
    tables.all_tables[0].esc_size = 0;
    for (u_int8_t i = 0; i < PPM_LEVEL; ++i)
    {
        tables.all_tables[0].previous_symbols[i] = 0;
    }


    struct EncodeContext context;
    context.l_i = 0;
    context.h_i = TOP_BOUNDARY;
    for (u_int8_t i = 0; i < PPM_LEVEL; ++i)
    {
        context.pre_index[i] = 1;
    }
    context.buffer.bits_in_byte_out = 0;
    context.buffer.byte_out = 0;
    context.buffer.bits_to_follow = 0;

    size_t bytes_read;
    size_t index = 1;
    unsigned char buff[FILE_BUFFERS_SIZE];

    fseek(ifp, 0, SEEK_END);
    u_int32_t size = ftell(ifp);
    fwrite(&size, 1, sizeof(size), ofp); // write size of file
    rewind(ifp);

    do
    {
        bytes_read = fread(buff, 1, sizeof(buff), ifp);
        for (u_int32_t i = 0; i < bytes_read; ++i)
        {
            //printf("%u\n", i);
            for (int k = PPM_LEVEL-2; k >= 0; --k)
            {
                context.pre_index[k+1] = context.pre_index[k];
            }
            context.pre_index[0] = index;
            index = (int)buff[i] + 1;

            EncodePPM(ofp, &tables, &context, index);

            UpdateFrequencyPPM(&tables, context.pre_index, index);
        }

    } while (!feof(ifp));

    DoneEncoding(ofp, &context);

    fclose(ifp);
    fclose(ofp);
}

void decompress_ppm(char *ifile, char *ofile) {
    FILE *ifp = (FILE *)fopen(ifile, "rb");
    FILE *ofp = (FILE *)fopen(ofile, "wb");

    struct ALlTables tables;
    tables.memory = 256;
    tables.all_tables = (struct OneTable*)malloc(tables.memory * sizeof(struct OneTable));
    tables.length = 1;
    tables.all_tables[0].memory = NUM_OF_SYM;
    tables.all_tables[0].symbols = (struct Symbol*)malloc(tables.all_tables[0].memory * sizeof(struct Symbol));
    tables.all_tables[0].length = NUM_OF_SYM;
    for (u_int32_t i = 0; i < tables.all_tables[0].length; ++i)
    {
        tables.all_tables[0].symbols[i].index = i+1;
        tables.all_tables[0].symbols[i].additional_size = 1;
    }
    tables.all_tables[0].total_size = NUM_OF_SYM;
    tables.all_tables[0].esc_size = 0;
    for (u_int8_t i = 0; i < PPM_LEVEL; ++i)
    {
        tables.all_tables[0].previous_symbols[i] = 0;
    }


    struct DecodeContext context;
    context.l_i = 0;
    context.h_i = TOP_BOUNDARY;
    unsigned char c;
    for (u_int8_t i = 0; i < PPM_LEVEL; ++i)
    {
        context.pre_index[i] = 1;
    }

    u_int16_t index = 1;

    u_int32_t size;
    fread(&size, 1, sizeof(size), ifp);


    fread(&(context.buffer.input_buff), 1, sizeof(context.buffer.input_buff), ifp);
    context.value = context.buffer.input_buff;
    context.value = context.value << 8;
    fread(&(context.buffer.input_buff), 1, sizeof(context.buffer.input_buff), ifp);
    context.value += context.buffer.input_buff;
    context.value = context.value << 8;
    fread(&(context.buffer.input_buff), 1, sizeof(context.buffer.input_buff), ifp);
    context.value += context.buffer.input_buff;
    context.value = context.value << 8;
    fread(&(context.buffer.input_buff), 1, sizeof(context.buffer.input_buff), ifp);
    context.value += context.buffer.input_buff;
    context.buffer.bits_left = 0;


    for(u_int32_t i = 0; i < size; ++i)
    {
        //printf("%u\n", i);

        for (int k = PPM_LEVEL-2; k >= 0; --k)
        {
            context.pre_index[k+1] = context.pre_index[k];
        }
        context.pre_index[0] = index;

        DecodePPM(ifp, &tables, &context, &index);

        c = (char)(index-1);
        fwrite(&c, 1, sizeof(c), ofp); // сбрасываем символ в файл
    }

    fclose(ifp);
    fclose(ofp);
}
