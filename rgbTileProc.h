/* rgbTileProc.h
*  implementation of TILE COMPRESSION
*/
#ifndef _RGBTILEPROC_H_
#define _RGBTILEPROC_H_

void tileSetSize(int nTileWidth, int nTileHeight);

/* compress ARGB data to tile
*  param:
*    pClrBlk      -- IN, pixel's ARGB data
*    pTile        -- OUT, tile data
*    pTileSize    -- OUT, tile's bytes
*  return:
*    0  -- succeed
*   -1  -- failed
*/
int argb2tile(const unsigned char *pClrBlk, unsigned char *pTile, int *pTileSize);

/* decompress tile data to ARGB
*  param:
*    pTile        -- IN, tile data
*    pTileSize    -- IN, tile's bytes
*    pClrBlk      -- OUT, pixel's ARGB data
*  return:
*    0  -- succeed
*   -1  -- failed
*/
int tile2argb(const unsigned char* pTile, int nTileSize, unsigned char* pClrBlk);

int my_abs(int x);
int my_ceil(int num, int den);
void encode_flag(int* flag_data, int *total_bit, const int* diff);
int compress_core(unsigned char block_data_1_1,const int* diff_data_abs,const int* flag_data,int over_size_flag,int bit_count,unsigned char* pTile1, int col_row_flag);
int decompress_core(const unsigned char * pTile,int bit_count, int byte_count, int total_byte, unsigned char* block_data, int col_row_flag, int* flag1);
void diff_col(const unsigned char *block_data, int *diff_data, int *diff_data_abs);
void diff_row(const unsigned char *block_data, int *diff_data, int *diff_data_abs);
void judge_equ_count_row(int* equ_count, int* judge, int rc, const int* flag_data);
void judge_equ_count_col(int* equ_count, int* judge, int rc, const int* flag_data);
int data_bit_counter(const int* data_diff_abs, const int* flag_data);
int bit_count_again_col(const int* flag_data);
int bit_count_again_row(const int* flag_data);
int judge_rgbflag(const int* flag_1, const int* flag_2, int* addr, int* content, int* judge_equ_count);
int rgb_byte_judgement(const int* content, int butongzongshu);
void compress_rgbequ(const int* diff_abs, const int* addr, const int* content, int butongzongshu, unsigned char* pTile1, unsigned char block_data_1_1, const int* flag_data);
int decompress_rgbequ(const unsigned char* pTile, int byte_count,unsigned char* block_g, int* flag1, int col_row_flag);
#endif
