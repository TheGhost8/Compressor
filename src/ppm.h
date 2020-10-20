#pragma once

#define FILE_BUFFERS_SIZE 8192 // input and output buffers size
#define BITS_IN_BYTE 8
#define NUM_OF_SYM 256 // 256 symbols
#define MAX_TABLE_SIZE 0x0000FFFF
#define TOP_BOUNDARY 0xFFFFFFFF
#define FIRST_QTR (((size_t)TOP_BOUNDARY+1)/4)
#define HALF (FIRST_QTR*2)
#define THIRD_QTR (FIRST_QTR*3)
#define PPM_LEVEL 5


struct Symbol
{
    u_int16_t index;
    u_int32_t additional_size;
};

struct OneTable
{
    u_int16_t previous_symbols[PPM_LEVEL];
    struct Symbol *symbols;
    u_int32_t length, memory, total_size, esc_size;
};

struct ALlTables
{
    struct OneTable* all_tables;
    u_int32_t length, memory;
};

struct OutputBuffer
{
    unsigned char byte_out;
    u_int8_t bits_in_byte_out;
    u_int32_t bits_to_follow;
};

struct EncodeContext
{
    size_t l_i, h_i;
    u_int16_t pre_index[PPM_LEVEL];
    struct OutputBuffer buffer;
};

struct InputBuffer
{
    u_int8_t bits_left;
    u_int8_t input_buff;
};

struct DecodeContext
{
    size_t l_i, h_i;
    u_int16_t pre_index[PPM_LEVEL];
    u_int32_t value;
    struct InputBuffer buffer;
};

u_int8_t UpdateOneTable(struct OneTable *table, u_int32_t index);
void UpdateFrequencyPPM(struct ALlTables *tables, const u_int16_t pre_index[PPM_LEVEL], u_int32_t index);
void BitsPlusFollowPPM(FILE *ofp, struct OutputBuffer *buffer, u_int8_t bit);
u_int32_t* GetSymbolWeight(struct OneTable *table, u_int16_t index);
u_int16_t IndexSearch(struct OneTable *tables, u_int32_t value, size_t l_i, size_t h_i);
void EncodePPM(FILE *ofp, struct ALlTables *tables, struct EncodeContext *context, u_int16_t index);
u_int8_t EncodeSym(FILE *ofp, struct OneTable *tables, struct EncodeContext *context, u_int16_t index);
void DoneEncoding(FILE *ofp, struct EncodeContext *context);
void DecodePPM(FILE *ifp, struct ALlTables *tables, struct DecodeContext *context, u_int16_t *index);
void DecodeSym(FILE *ifp, struct OneTable *table, struct DecodeContext *context, u_int16_t index);
void compress_ppm(char *ifile, char *ofile);
void decompress_ppm(char *ifile, char *ofile);
