/* rgbTileProc.cpp
*  implementation of TILE COMPRESSION
*/
#include <stdio.h>
#include <math.h>
#include <iostream>
#include <memory.h>
#include <assert.h>
#include "rgbTileProc.h"


#define TILE_SIZE 8
#define LOG2_TILE_SIZE 3

static int g_nTileWidth = 0;
static int g_nTileHeight = 0;

int my_abs(int x)
{
    if(x >= 0)
    {
        return x;
    }
    else
    {
        return -x;
    }

}

int my_ceil(int num, int den)
{
    int result;

    if((num % den) == 0) {
        result = num / den;
    }
    else
    {
        result = num / den + 1;
    }

    return result;
}

// 在列的方向上作差
void diff_col(const unsigned char *block_data, int *diff_data, int *diff_data_abs)
{
    int row, col;

    for(row = 0;row<TILE_SIZE;row++)
    {
        for(col = 0;col<TILE_SIZE;col++)
        {
            if(row == 0 && col == 0)
            {
                diff_data[row*TILE_SIZE+col] = block_data[row*TILE_SIZE+col];
            }
            else if(row == 0 && col != 0)
            {
                diff_data[row*TILE_SIZE+col] = block_data[row*TILE_SIZE+col] - block_data[row*TILE_SIZE+col-1];
            }
            else
            {
                diff_data[row * TILE_SIZE + col] = block_data[row * TILE_SIZE + col] - block_data[(row - 1) * TILE_SIZE + col];
            }

            diff_data_abs[row * TILE_SIZE + col] = my_abs(diff_data[row * TILE_SIZE + col]);
        }
    }
}
// 在行的方向上作差
void diff_row(const unsigned char *block_data, int *diff_data, int *diff_data_abs)
{
    int row, col;

    for(row = 0;row<TILE_SIZE;row++)
    {
        for(col = 0;col<TILE_SIZE;col++)
        {
            if(row == 0 && col == 0)
            {
                diff_data[row*TILE_SIZE + col] = block_data[row*TILE_SIZE + col];
            }
            else if(row != 0 && col == 0)
            {
                diff_data[row*TILE_SIZE + col] = block_data[row*TILE_SIZE + col] - block_data[(row - 1)*TILE_SIZE + col];
            }
            else
            {
                diff_data[row*TILE_SIZE + col] = block_data[row*TILE_SIZE + col] - block_data[row*TILE_SIZE+col-1];
            }

            diff_data_abs[row * TILE_SIZE + col] = my_abs(diff_data[row * TILE_SIZE + col]);
        }
    }
}

void encode_flag(int* flag_data, int *total_bit, const int* diff)
{

    int row = 0;
    int col = 0;
    *total_bit = 8;

    for (row = 0; row < TILE_SIZE; row++) {
        for (col = 0; col < TILE_SIZE; col++) {

            if(row == 0 && col == 0) {
                flag_data[row*TILE_SIZE+col] = 0;
                *total_bit += 0;
            } else if (diff[row*TILE_SIZE+col] == 0) {
                flag_data[row*TILE_SIZE+col] = 0;
                *total_bit += 2;
            } else if (diff[row*TILE_SIZE+col] >= 1 && diff[row*TILE_SIZE+col] <= 3) {
                flag_data[row*TILE_SIZE+col] = 1;
                *total_bit += 4;
            } else if (diff[row*TILE_SIZE+col] >= -3 && diff[row*TILE_SIZE+col] <= -1) {
                flag_data[row*TILE_SIZE+col] = 2;
                *total_bit += 4;
            } else if (diff[row*TILE_SIZE+col] >= 4 && diff[row*TILE_SIZE+col] <= 15) {
                flag_data[row*TILE_SIZE+col] = 3;
                *total_bit += 7;
            } else if (diff[row*TILE_SIZE+col] >= -15 && diff[row*TILE_SIZE+col] <= -4) {
                flag_data[row*TILE_SIZE+col] = 4;
                *total_bit += 8;
            } else if (diff[row*TILE_SIZE+col] >= 16) {
                flag_data[row*TILE_SIZE+col] = 5;
                *total_bit += 13;
            } else if (diff[row*TILE_SIZE+col] <= -16) {
                flag_data[row*TILE_SIZE+col] = 6;
                *total_bit += 13;
            } else {
                std::cout << "flag_data assign error" << std::endl;
            }

        }
    }
}

int judge_rgbflag(const int* flag_1, const int* flag_2, int* addr, int* content, int* judge_equ_count){
    int judge_equ = 0;
    int n = 0;
    int addr_cnt = 0;
    int use_less;

    *judge_equ_count = 0;
    for(n = 1; n < TILE_SIZE * TILE_SIZE;n++){

        if(flag_1[n] == flag_2[n]){
            ;
        } else if(*judge_equ_count < 7 && flag_1[n] != flag_2[n]){
            addr[addr_cnt] = n;
            content[addr_cnt] = flag_2[n];
            addr_cnt++;
            *judge_equ_count = *judge_equ_count + 1;
        }else if(*judge_equ_count >= 7 && flag_1[n] != flag_2[n]){
            *judge_equ_count = *judge_equ_count + 1;
            break;
        }

        if(flag_1[0] != 0)
        {
            use_less = 4;
        }

    }
    if(*judge_equ_count <= 7){
        judge_equ = 1;
    }
    return judge_equ;
}

//重新确定bit数
int data_bit_counter(const int* data_diff_abs, const int* flag_data){
    int totalbit = 8;
    int n = 1;
    int data_bit_count[7] = {0, 2, 2, 4, 4, 8, 8};
    for(n = 1;n < TILE_SIZE * TILE_SIZE;n++){
        totalbit += data_bit_count[flag_data[n]];
    }
    return totalbit;
}

int bit_count_again_col(const int* flag_data) {
    int total_bit = 0;
    int judge = 0;
    int equ = 0;
    int equ_count = 0;
    int rc = 0;
    int col = 0;
    int shift_data[7] = {2, 2, 2, 3, 4, 5, 5};
    for (rc = 0; rc < TILE_SIZE; rc++) {
        equ_count = 0;

        //按行判断
        //先判断首尾
        //第一个为0，不参与判断
        if (rc == 0) {
            judge_equ_count_col(&equ_count, &judge, rc, flag_data);
            if (equ_count == TILE_SIZE - 1) {
                total_bit += 1;//判断位 0 1bit
                total_bit += shift_data[flag_data[1]];
            }
            else if (equ_count == TILE_SIZE - 3) {
                total_bit += 2 + LOG2_TILE_SIZE;//判断位 10 2bit 位置位 3bit
                if (judge == 1) {
                    total_bit += shift_data[flag_data[2]] + shift_data[flag_data[1]];
                } else {
                    total_bit += shift_data[flag_data[1]] + shift_data[flag_data[judge]];
                }
            } else {
                total_bit += 2;//判断位 11
                for(col = 1; col < TILE_SIZE; col++){
                    total_bit += shift_data[flag_data[col]];
                }

            }
        } else {
            judge_equ_count_col(&equ_count, &judge, rc, flag_data);
            if (equ_count == TILE_SIZE) {
                total_bit += 1;//判断位 0 1bit
                total_bit += shift_data[flag_data[rc * TILE_SIZE]];
            } else if (equ_count == TILE_SIZE - 2) {
                total_bit += 2 + LOG2_TILE_SIZE;//判断位 10 2bit 位置位 3bit
                if (judge == 0) {
                    total_bit += shift_data[flag_data[rc * TILE_SIZE]] + shift_data[flag_data[rc * TILE_SIZE + 1]];
                } else {
                    total_bit += shift_data[flag_data[rc * TILE_SIZE]] + shift_data[flag_data[rc * TILE_SIZE + judge]];
                }
            } else {
                total_bit += 2;//判断位 11
                for(col = 0; col < TILE_SIZE; col++){
                    total_bit += shift_data[flag_data[rc * TILE_SIZE + col]];
                }
            }

        }

    }
    return  total_bit;
}
void judge_equ_count_col(int* equ_count, int* judge, int rc, const int* flag_data){
    int valid = 0;
    int equ = 0;
    int equ1 = 0;
    int diff_1 = 0,diff_2 = 0;
    int diff_1_cnt =0, diff_2_cnt = 0;


    if(rc == 0) {
        *equ_count = 0;
        *judge = 0;
        valid = 0;
        for (equ = 1; equ < TILE_SIZE - 1; equ++) {
            if (flag_data[equ] == flag_data[equ + 1]) {
                *equ_count += 1;
            } else {
                if (!valid) {
                    diff_1_cnt = 0;
                    diff_2_cnt = 0;
                    diff_1 = flag_data[equ];
                    diff_2 = flag_data[equ + 1];
                    for (equ1 = 1; equ1 < TILE_SIZE; equ1++) {
                        if (diff_1 == flag_data[equ1]) {
                            diff_1_cnt++;
                        } else if (diff_2 == flag_data[equ1]) {
                            diff_2_cnt++;
                        }
                    }
                    if (diff_1_cnt >= 2 && diff_2_cnt >= 2) {
                        *equ_count = 0;
                        break;
                    }
                    valid = 1;
                }
                *judge = equ;
            }
        }
        if (flag_data[1] == flag_data[TILE_SIZE - 1]) {
            *equ_count += 1;
        } else {
            if (*judge == TILE_SIZE - 2) {
                *judge = TILE_SIZE - 1;
            }
        }
    }
    else{
        *equ_count = 0;
        *judge = 0;
        valid = 0;
        for (equ = 0; equ < TILE_SIZE - 1; equ++) {
            if (flag_data[rc * TILE_SIZE + equ] == flag_data[rc * TILE_SIZE + equ + 1]) {
                *equ_count += 1;
            } else {
                if (!valid) {
                    diff_1_cnt = 0;
                    diff_2_cnt = 0;
                    diff_1 = flag_data[rc * TILE_SIZE + equ];
                    diff_2 = flag_data[rc * TILE_SIZE + equ + 1];
                    for (equ1 = 0; equ1 < TILE_SIZE; equ1++) {
                        if (diff_1 == flag_data[rc * TILE_SIZE + equ1]) {
                            diff_1_cnt++;
                        } else if (diff_2 == flag_data[rc * TILE_SIZE + equ1]) {
                            diff_2_cnt++;
                        }
                    }
                    if (diff_1_cnt >= 2 && diff_2_cnt >= 2) {
                        *equ_count = 0;
                        valid = 1;
                        break;
                    }
                }
                *judge = equ;
            }
        }
        if (flag_data[rc * TILE_SIZE] == flag_data[rc * TILE_SIZE + TILE_SIZE - 1]) {
            *equ_count += 1;
        } else {
            if (*judge == TILE_SIZE - 2) {
                *judge = TILE_SIZE - 1;
            }
        }
    }
}
int bit_count_again_row(const int* flag_data) {
    int total_bit = 0;
    int judge = 0;
    int equ = 0;
    int equ_count = 0;
    int rc = 0;
    int row = 0;
    int shift_data[7] = {2, 2, 2, 3, 4, 5, 5};
    for (rc = 0; rc < TILE_SIZE; rc++) {
        equ_count = 0;

        //按行判断
        //判断首尾
        //第一个为0，不参与判断
        if (rc == 0) {
            judge_equ_count_row(&equ_count, &judge, rc, flag_data);

            if (equ_count == TILE_SIZE - 1) {
                total_bit += 1;//判断位 0 1bit
                total_bit += shift_data[flag_data[TILE_SIZE]];
            }
            else if (equ_count == TILE_SIZE -3) {
                total_bit += 2 + LOG2_TILE_SIZE;//判断位 10 2bit 位置位 3bit
                if (judge == 1) {
                    total_bit += shift_data[flag_data[TILE_SIZE * 2]] + shift_data[flag_data[TILE_SIZE]];
                } else {
                    total_bit += shift_data[flag_data[TILE_SIZE * judge]] +shift_data[flag_data[TILE_SIZE]];
                }
            } else {
                total_bit += 2;//判断位 11
                for(row = 1; row<TILE_SIZE;row++){
                    total_bit += shift_data[flag_data[row * TILE_SIZE]];
                }
            }
        } else {
            judge_equ_count_row(&equ_count, &judge, rc, flag_data);
            if (equ_count == TILE_SIZE) {
                total_bit += 1;//判断位 0 1bit
                total_bit += shift_data[flag_data[rc]];
            } else if (equ_count == TILE_SIZE - 2) {
                total_bit += 2 + LOG2_TILE_SIZE;//判断位 10 2bit 位置位 3bit
                if (judge == 0) {
                    total_bit += shift_data[flag_data[rc]] +shift_data[flag_data[rc + TILE_SIZE]];
                } else {
                    total_bit += shift_data[flag_data[rc]] +shift_data[flag_data[rc + judge * TILE_SIZE]];
                }
            } else {
                total_bit += 2;//判断位 11
                for(row = 0; row<TILE_SIZE;row++){
                    total_bit += shift_data[flag_data[rc + row * TILE_SIZE]];
                }
            }

        }

    }
    return  total_bit;
}

void judge_equ_count_row(int* equ_count, int* judge, int rc, const int* flag_data){
    int valid = 0;
    int equ = 0;
    int equ1 = 0;
    int diff_1 = 0,diff_2 = 0;
    int diff_1_cnt =0, diff_2_cnt = 0;


    if(rc == 0) {
        *equ_count = 0;
        *judge = 0;
        valid = 0;
        for (equ = 1; equ < TILE_SIZE - 1; equ++) {
            if (flag_data[equ * TILE_SIZE] == flag_data[(equ + 1) * TILE_SIZE]) {
                *equ_count += 1;
            } else {
                if (!valid) {
                    diff_1_cnt = 0;
                    diff_2_cnt = 0;
                    diff_1 = flag_data[equ * TILE_SIZE];
                    diff_2 = flag_data[(equ + 1)* TILE_SIZE];
                    for (equ1 = 1; equ1 < TILE_SIZE; equ1++) {
                        if (diff_1 == flag_data[equ1 * TILE_SIZE]) {
                            diff_1_cnt++;
                        } else if (diff_2 == flag_data[equ1 * TILE_SIZE]) {
                            diff_2_cnt++;
                        }
                    }
                    if (diff_1_cnt >= 2 && diff_2_cnt >= 2) {
                        *equ_count = 0;
                        valid = 1;
                        break;
                    }
                }
                *judge = equ;
            }
        }
        if (flag_data[TILE_SIZE] == flag_data[TILE_SIZE * (TILE_SIZE - 1)]) {
            *equ_count += 1;
        } else {
            if (*judge == TILE_SIZE - 2) {
                *judge = TILE_SIZE - 1;
            }
        }
    }
    else{
        *equ_count = 0;
        *judge = 0;
        valid = 0;
        for (equ = 0; equ < TILE_SIZE - 1; equ++) {
            if (flag_data[rc + equ * TILE_SIZE] == flag_data[rc + (equ + 1) * TILE_SIZE]) {
                *equ_count += 1;
            } else {
                if (!valid) {
                    diff_1_cnt = 0;
                    diff_2_cnt = 0;
                    diff_1 = flag_data[rc + equ * TILE_SIZE];
                    diff_2 = flag_data[rc + (equ + 1) * TILE_SIZE];
                    for (equ1 = 0; equ1 < TILE_SIZE; equ1++) {
                        if (diff_1 == flag_data[rc + equ1 * TILE_SIZE]) {
                            diff_1_cnt++;
                        } else if (diff_2 == flag_data[rc + equ1 * TILE_SIZE]) {
                            diff_2_cnt++;
                        }
                    }
                    if (diff_1_cnt >= 2 && diff_2_cnt >= 2) {
                        *equ_count = 0;
                        valid = 1;
                        break;
                    }
                }
                *judge = equ;
            }
        }
        if (flag_data[rc] == flag_data[rc + TILE_SIZE * (TILE_SIZE - 1)]) {
            *equ_count += 1;
        } else {
            if (*judge == TILE_SIZE - 2) {
                *judge = TILE_SIZE - 1;
            }
        }
    }


}


//计算通道标志位差不多时的总byte
int rgb_byte_judgement(const int* content, int butongzongshu) {
    int bit = 3;
    int byte = 0;
    int r;
    int shift_data[7] = {2, 2, 2, 3, 4, 5, 5};

    for (r = 0; r < butongzongshu; r++) {
        bit += shift_data[content[r]];//不同内容的偏移
    }
    if (butongzongshu == 0) {
        bit = 8;
    } else {
        bit += 6 * butongzongshu;//每个不同6bit地址位
    }
    byte = my_ceil(bit, 8);
    return byte;
}

int compress_core(unsigned char block_data_1_1, const int *diff_data_abs, const int *flag_data, int over_size_flag,
                  int bit_count, unsigned char *pTile1, int col_row_flag)
{
    int shift_data[7] = {2, 2, 2, 3, 4, 5, 5};
    int add_data[7] = {1, 0, 2, 3, 7, 15, 31};
    int data_bit_count[7] = {0, 2, 2, 4, 4, 8, 8};
    int last_data = 0;
    int this_data = 0;
    int cur_bit = 0;
    int last_bit = 0;
    int n = 1;
    int byte_count = 0;
    int eff = 0;
    int clear_cnt = 0;
    int equ_count = 0;
    int equ = 0;
    int judge = 0;
    int give = 0;
    int shuju = 0;

    if (over_size_flag) {
        for (clear_cnt = 0; clear_cnt < TILE_SIZE * TILE_SIZE; clear_cnt++) {
            pTile1[clear_cnt] = 0;
        }
        return TILE_SIZE * TILE_SIZE;
    }
        //具体的字节数据
    else {
        //curbit代表thisdata长度
        //按列压缩，先将标志位存储
        if (col_row_flag) {
            for (eff = 0; eff < TILE_SIZE; eff++) {
                equ_count = 0;
                //判断首尾
                //先判断首列
                if(eff == 0){
                    judge_equ_count_row(&equ_count, &judge, eff, flag_data);
                    if (equ_count == TILE_SIZE - 1) {
                        this_data =
                                last_data + (add_data[flag_data[TILE_SIZE]] << (1 + last_bit)) +
                                (0 << last_bit);
                        cur_bit = last_bit + shift_data[flag_data[TILE_SIZE]] + 1;
                        //未凑满一字节
                        if (cur_bit < 8) {
                            last_data = this_data;//进行下一循环
                            last_bit = cur_bit;
                            byte_count += 0;
                        }//凑满一字节
                        else {
                            pTile1[byte_count] = this_data % 256;
                            last_data = this_data / (256);
                            cur_bit -= 8;
                            last_bit = cur_bit;
                            byte_count += 1;
                        }

                    }
                        //一列中一个不一样, 判断位10 位置位3bit 不一样的标志位xbit 一样的标志位ybit
                    else if (equ_count == TILE_SIZE - 3) {
                        //列中第一个就不一样
                        if (judge == 1) {
                            this_data =
                                    last_data
                                    + (1 << last_bit) + (judge << (2 + last_bit))
                                    + (add_data[flag_data[TILE_SIZE]] << (2 + last_bit + LOG2_TILE_SIZE))
                                    + (add_data[flag_data[2 * TILE_SIZE]] << (2 + last_bit + LOG2_TILE_SIZE +
                                                                              shift_data[flag_data[TILE_SIZE]])); //剩的数据加位置位和判断位和不一样的标志位和一样的标志位

                            cur_bit = last_bit + shift_data[flag_data[TILE_SIZE]] +
                                      shift_data[flag_data[2 * TILE_SIZE]] +
                                      2 + LOG2_TILE_SIZE;
                        }//不是第一个
                        else {
                            this_data =
                                    last_data
                                    + (1 << last_bit) + (judge << (2 + last_bit))
                                    + (add_data[flag_data[judge * TILE_SIZE]] << (2 + last_bit + LOG2_TILE_SIZE))
                                    + (add_data[flag_data[TILE_SIZE]]
                                            << (2 + last_bit + LOG2_TILE_SIZE + shift_data[flag_data[judge * TILE_SIZE]]));
                            cur_bit = last_bit + shift_data[flag_data[TILE_SIZE]] +
                                      shift_data[flag_data[judge * TILE_SIZE]] +
                                      2 + LOG2_TILE_SIZE;
                        }
                        //在此种情况下，判断位2bit加不一样位置位3bit，不一样内容x1bit，一样内容x2bit，9<2+3+x1+x2<22
                        //1 byte
                        if (cur_bit < 16) {
                            pTile1[byte_count] = this_data % 256;
                            last_data = this_data / 256;
                            cur_bit -= 8;
                            last_bit = cur_bit;
                            byte_count += 1;

                        }
                            //2 byte
                        else if (cur_bit < 24 && cur_bit >= 16) {
                            pTile1[byte_count] = this_data % 256;
                            this_data = this_data / 256;
                            cur_bit -= 8;
                            pTile1[byte_count + 1] = this_data % 256;
                            last_data = this_data / 256;
                            cur_bit -= 8;
                            last_bit = cur_bit;
                            byte_count += 2;
                        }
                            //error
                        else {
                            std::cout << "error bytes1" << std::endl;
                        }

                    }
                        //其他，判断位11
                    else {
                        this_data = last_data + (3 << (last_bit));
                        cur_bit = last_bit + 2;
                        last_data = this_data;
                        last_bit = cur_bit;

                        for (give = 1; give < TILE_SIZE; give++) {
                            this_data = last_data +
                                        (add_data[flag_data[give * TILE_SIZE]] << last_bit);
                            cur_bit = last_bit + shift_data[flag_data[give * TILE_SIZE]];
                            if (cur_bit < 8) {
                                last_data = this_data;
                                last_bit = cur_bit;
                            } else {
                                pTile1[byte_count] = this_data % 256;
                                last_data = this_data / 256;
                                cur_bit -= 8;
                                last_bit = cur_bit;
                                byte_count += 1;
                            }

                        }

                    }
                } else {
                    judge_equ_count_row(&equ_count, &judge, eff, flag_data);

                    //一列标志位全相等 判断位0
                    if (equ_count == TILE_SIZE) {
                        this_data =
                                last_data + (add_data[flag_data[eff]] << (1 + last_bit)) +
                                (0 << last_bit);
                        cur_bit = last_bit + shift_data[flag_data[eff]] + 1;
                        //未凑满一字节
                        if (cur_bit < 8) {
                            last_data = this_data;//进行下一循环
                            last_bit = cur_bit;
                            byte_count += 0;
                        }//凑满一字节
                        else {
                            pTile1[byte_count] = this_data % 256;
                            last_data = this_data / 256;
                            cur_bit -= 8;
                            last_bit = cur_bit;
                            byte_count += 1;
                        }

                    }
                        //一列中一个不一样, 判断位10 位置位3bit 不一样的标志位xbit 一样的标志位ybit
                    else if (equ_count == TILE_SIZE - 2) {
                        //列中第一个就不一样
                        if (judge == 0) {
                            this_data =
                                    last_data
                                    + (1 << last_bit) + (judge << (2 + last_bit))
                                    + (add_data[flag_data[eff]] << (2 + last_bit + LOG2_TILE_SIZE))
                                    + (add_data[flag_data[eff + TILE_SIZE]] << (2 + last_bit + LOG2_TILE_SIZE +
                                                                                shift_data[flag_data[eff]])); //剩的数据加位置位和判断位和不一样的标志位和一样的标志位

                            cur_bit = last_bit + shift_data[flag_data[eff]] +
                                      shift_data[flag_data[eff + TILE_SIZE]] +
                                      2 + LOG2_TILE_SIZE;
                        }//不是第一个
                        else {
                            this_data =
                                    last_data
                                    + (1 << last_bit) + (judge << (2 + last_bit))
                                    + (add_data[flag_data[eff + judge * TILE_SIZE]] << (2 + last_bit + LOG2_TILE_SIZE))
                                    + (add_data[flag_data[eff]]
                                            << (2 + last_bit + LOG2_TILE_SIZE + shift_data[flag_data[eff + judge * TILE_SIZE]]));
                            cur_bit = last_bit + shift_data[flag_data[eff]] +
                                      shift_data[flag_data[eff + judge * TILE_SIZE]] +
                                      2 + LOG2_TILE_SIZE;
                        }
                        //在此种情况下，判断位2bit加不一样位置位3bit，不一样内容x1bit，一样内容x2bit，9<2+3+x1+x2<22
                        //1 byte
                        if (cur_bit < 16) {
                            pTile1[byte_count] = this_data % 256;
                            last_data = this_data / 256;
                            cur_bit -= 8;
                            last_bit = cur_bit;
                            byte_count += 1;

                        }
                            //2 byte
                        else if (cur_bit < 24 && cur_bit >= 16) {
                            pTile1[byte_count] = this_data % 256;
                            this_data = this_data / 256;
                            cur_bit -= 8;
                            pTile1[byte_count + 1] = this_data % 256;
                            last_data = this_data / 256;
                            cur_bit -= 8;
                            last_bit = cur_bit;
                            byte_count += 2;
                        }
                            //error
                        else {
                            std::cout << "error bytes2" << std::endl;
                        }

                    }
                        //其他，判断位11
                    else {
                        this_data = last_data + (3 << (last_bit));
                        cur_bit = last_bit + 2;
                        last_data = this_data;
                        last_bit = cur_bit;

                        for (give = 0; give < TILE_SIZE; give++) {
                            this_data = last_data +
                                        (add_data[flag_data[eff + give * TILE_SIZE]] << last_bit);
                            cur_bit = last_bit + shift_data[flag_data[eff + give * TILE_SIZE]];
                            if (cur_bit < 8) {
                                last_data = this_data;
                                last_bit = cur_bit;
                            } else {
                                pTile1[byte_count] = this_data % 256;
                                last_data = this_data / 256;
                                cur_bit -= 8;
                                last_bit = cur_bit;
                                byte_count += 1;
                            }

                        }

                    }
                }
                this_data = 0;//压完一行或者一列清零
                cur_bit = 0;
            }
        }
            //按行压缩
        else {
            for (eff = 0; eff < TILE_SIZE; eff++) {
                equ_count = 0;
                if(eff == 0){
                    judge_equ_count_col(&equ_count, &judge, eff, flag_data);

                    if (equ_count == TILE_SIZE - 1) {
                        this_data =
                                last_data + (add_data[flag_data[1]] << (1 + last_bit)) +
                                (0 << last_bit);
                        cur_bit = last_bit + shift_data[flag_data[1]] + 1;
                        //未凑满一字节
                        if (cur_bit < 8) {
                            last_data = this_data;//进行下一循环
                            last_bit = cur_bit;
                            byte_count += 0;
                        }//凑满一字节
                        else {
                            pTile1[byte_count] = this_data % 256;
                            last_data = this_data / (256);
                            cur_bit -= 8;
                            last_bit = cur_bit;
                            byte_count += 1;
                        }

                    }
                        //一列中一个不一样, 判断位10 位置位3bit 不一样的标志位xbit 一样的标志位ybit
                    else if (equ_count == TILE_SIZE - 3) {
                        //列中第一个就不一样
                        if (judge == 1) {
                            this_data =
                                    last_data
                                    + (1 << last_bit) + (judge << (2 + last_bit))
                                    + (add_data[flag_data[1]] << (2 + last_bit + LOG2_TILE_SIZE))
                                    + (add_data[flag_data[2]] << (2 + last_bit + LOG2_TILE_SIZE +
                                                                  shift_data[flag_data[1]])); //剩的数据加位置位和判断位和不一样的标志位和一样的标志位

                            cur_bit = last_bit + shift_data[flag_data[1]] +
                                      shift_data[flag_data[2]] +
                                      2 + LOG2_TILE_SIZE;
                        }//不是第一个
                        else {
                            this_data =
                                    last_data
                                    + (1 << last_bit) + (judge << (2 + last_bit))
                                    + (add_data[flag_data[judge]] << (2 + last_bit + LOG2_TILE_SIZE))
                                    + (add_data[flag_data[1]]
                                            << (2 + last_bit + LOG2_TILE_SIZE + shift_data[flag_data[judge]]));
                            cur_bit = last_bit + shift_data[flag_data[judge]] +
                                      shift_data[flag_data[1]] +
                                      2 + LOG2_TILE_SIZE;
                        }
                        //在此种情况下，判断位2bit加不一样位置位3bit，不一样内容x1bit，一样内容x2bit，9<2+3+x1+x2<22
                        //1 byte
                        if (cur_bit < 16) {
                            pTile1[byte_count] = this_data % 256;
                            last_data = this_data / 256;
                            cur_bit -= 8;
                            last_bit = cur_bit;
                            byte_count += 1;

                        }
                            //2 byte
                        else if (cur_bit < 24 && cur_bit >= 16) {
                            pTile1[byte_count] = this_data % 256;
                            this_data = this_data / 256;
                            cur_bit -= 8;
                            pTile1[byte_count + 1] = this_data % 256;
                            last_data = this_data / 256;
                            cur_bit -= 8;
                            last_bit = cur_bit;
                            byte_count += 2;
                        }
                            //error
                        else {
                            std::cout << "error bytes3" << std::endl;
                        }

                    }
                        //其他，判断位11
                    else {
                        this_data = last_data + (3 << (last_bit));
                        cur_bit = last_bit + 2;
                        last_data = this_data;
                        last_bit = cur_bit;

                        for (give = 1; give < TILE_SIZE; give++) {
                            this_data = last_data +
                                        (add_data[flag_data[give]] << last_bit);
                            cur_bit = last_bit + shift_data[flag_data[give]];
                            if (cur_bit < 8) {
                                last_data = this_data;
                                last_bit = cur_bit;
                            } else {
                                pTile1[byte_count] = this_data % 256;
                                last_data = this_data / 256;
                                cur_bit -= 8;
                                last_bit = cur_bit;
                                byte_count += 1;
                            }

                        }

                    }
                } else {
                    judge_equ_count_col(&equ_count, &judge, eff, flag_data);
                    //一列标志位全相等 判断位0
                    if (equ_count == TILE_SIZE) {
                        this_data =
                                last_data  + (add_data[flag_data[eff * TILE_SIZE]] << (1 + last_bit)) +
                                (0 << last_bit);
                        cur_bit = last_bit + shift_data[flag_data[eff * TILE_SIZE]] + 1;
                        //未凑满一字节
                        if (cur_bit < 8) {
                            last_data = this_data;//进行下一循环
                            last_bit = cur_bit;
                            byte_count += 0;
                        }//凑满一字节
                        else {
                            pTile1[byte_count] = this_data % 256;
                            last_data = this_data / 256;
                            cur_bit -= 8;
                            last_bit = cur_bit;
                            byte_count += 1;
                        }

                    }
                        //一行中一个不一样, 判断位10 位置位3bit 不一样的标志位xbit 一样的标志位ybit
                    else if (equ_count == TILE_SIZE - 2) {
                        //行中第一个就不一样
                        if (judge == 0) {
                            this_data =
                                    last_data
                                    + (1 << last_bit) + (judge << (2 + last_bit))
                                    + (add_data[flag_data[eff * TILE_SIZE]] << (2 + last_bit + LOG2_TILE_SIZE))
                                    + (add_data[flag_data[eff * TILE_SIZE + 1]] << (2 + last_bit + LOG2_TILE_SIZE +
                                                                                    shift_data[flag_data[eff * TILE_SIZE]])); //剩的数据加位置位和判断位和不一样的标志位和一样的标志位

                            cur_bit =
                                    last_bit + shift_data[flag_data[eff * TILE_SIZE]]
                                    + shift_data[flag_data[eff * TILE_SIZE + 1]]
                                    + 2 + LOG2_TILE_SIZE;
                        }//不是第一个
                        else {
                            this_data =
                                    last_data
                                    + (1 << last_bit) + (judge << (2 + last_bit))
                                    + (add_data[flag_data[eff * TILE_SIZE + judge]] << (2 + last_bit + LOG2_TILE_SIZE))
                                    + (add_data[flag_data[eff * TILE_SIZE]]
                                            << (2 + last_bit + LOG2_TILE_SIZE + shift_data[flag_data[eff * TILE_SIZE + judge]]));
                            cur_bit =
                                    last_bit + shift_data[flag_data[eff * TILE_SIZE]] + shift_data[flag_data[eff * TILE_SIZE + judge]] + 2 + 3;
                        }
                        //在此种情况下，判断位2bit加不一样位置位3bit，不一样内容x1bit，一样内容x2bit，9<2+3+x1+x2<22
                        //1 byte
                        if (cur_bit < 16) {
                            pTile1[byte_count] = this_data % 256;
                            last_data = this_data / 256;
                            cur_bit -= 8;
                            last_bit = cur_bit;
                            byte_count += 1;

                        }
                            //2 byte
                        else if (cur_bit < 24 && cur_bit >= 16) {
                            pTile1[byte_count] = this_data % 256;
                            this_data = this_data / 256;
                            cur_bit -= 8;
                            pTile1[byte_count + 1] = this_data % 256;
                            last_data = this_data / 256;
                            cur_bit -= 8;
                            last_bit = cur_bit;
                            byte_count += 2;
                        }
                            //error
                        else {
                            std::cout << "error bytes4" << std::endl;
                        }

                    }
                        //其他，判断位11
                    else {
                        this_data = last_data + (3 << (last_bit));
                        cur_bit = last_bit + 2;
                        last_data = this_data;
                        last_bit = cur_bit;
                        for (give = 0; give < TILE_SIZE; give++) {
                            this_data = last_data +
                                        (add_data[flag_data[eff * TILE_SIZE + give]] << last_bit);
                            cur_bit = last_bit + shift_data[flag_data[eff * TILE_SIZE + give]];
                            if (cur_bit < 8) {
                                last_data = this_data;
                                last_bit = cur_bit;
                            } else {
                                pTile1[byte_count] = this_data % 256;
                                last_data = this_data / 256;
                                cur_bit -= 8;
                                last_bit = cur_bit;
                                byte_count += 1;
                            }

                        }

                    }
                }
                this_data = 0;//压完一行或者一列清零
                cur_bit = 0;
            }
        }
        //剩下的标志位填成一个字节
        if (last_bit == 0) {
            last_data = 0;
        } else {
            pTile1[byte_count] = last_data;
            last_bit = 0;
            byte_count += 1;
            last_data = 0;
        }
        //压缩数据位
        //首位
        pTile1[byte_count] = block_data_1_1;
        byte_count += 1;
        //差值位
        for (shuju = 1; shuju < 64; shuju++) {
            this_data = last_data + (diff_data_abs[shuju] << last_bit);
            cur_bit = last_bit + data_bit_count[flag_data[shuju]];
            if (cur_bit < 8) {
                last_data = this_data;
                last_bit = cur_bit;
            } else {
                pTile1[byte_count] = this_data % 256;
                last_data = this_data / 256;
                cur_bit -= 8;
                last_bit = cur_bit;
                byte_count += 1;
            }
            this_data = 0;
            cur_bit = 0;
        } //最后没凑满的数据补全一字节
        if (last_bit != 0) {
            pTile1[byte_count] = last_data;
            byte_count += 1;
        }

        return byte_count;
    }
}
void tileSetSize(int nTileWidth, int nTileHeight) {
    g_nTileWidth = nTileWidth;
    g_nTileHeight = nTileHeight;
}

/* compress ARGB data to tile
*  param:
*    pClrBlk      -- IN, pixel's ARGB data
*    pTile        -- OUT, tile data
*    pTileSize    -- OUT, tile's bytes
*  return:
*    0  -- succeed
*   -1  -- failed
*/
void compress_rgbequ(const int* diff_abs, const int* addr, const int* content, int butongzongshu, unsigned char* pTile1, unsigned char block_data_1_1, const int* flag_data) {
    int byte_count = 0;
    int biaozhi = 0;
    int shuju = 0;
    int this_data = 0;
    int cur_bit = 0;
    int last_data = 0;
    int last_bit = 0;
    int data_bit_count[7] = {0, 2, 2, 4, 4, 8, 8};
    int shift_data[7] = {2, 2, 2, 3, 4, 5, 5};
    int add_data[7] = {1, 0, 2, 3, 7, 15, 31};

    last_bit = 3;//必须要有不同总数的位置
    for (biaozhi = 0; biaozhi < butongzongshu; biaozhi++) {
        if(biaozhi == 0){
            last_data = butongzongshu;
            last_bit = 3;
        }
        this_data = last_data + (addr[biaozhi] << last_bit) + (add_data[content[biaozhi]] << (last_bit + 2 * LOG2_TILE_SIZE));
        cur_bit = 2 * LOG2_TILE_SIZE + shift_data[content[biaozhi]] + last_bit;
        pTile1[byte_count] = this_data % 256;
        byte_count += 1;
        cur_bit -= 8;
        this_data = this_data / 256;
        if(cur_bit >= 8){
            pTile1[byte_count] = this_data % 256;
            cur_bit -= 8;
            byte_count += 1;
            this_data = this_data / 256;
        }
        last_data = this_data;
        last_bit = cur_bit;
        this_data = 0;
        cur_bit = 0;
    }
    if(last_bit != 0){
        pTile1[byte_count] = last_data;
        last_bit = 0;
        last_data = 0;
        byte_count += 1;
    }

    pTile1[byte_count] = block_data_1_1;
    byte_count += 1;

    for(shuju = 1; shuju < TILE_SIZE * TILE_SIZE;shuju++){
        this_data = last_data + (diff_abs[shuju] << last_bit);
        cur_bit = last_bit + data_bit_count[flag_data[shuju]];
        if (cur_bit < 8) {
            last_data = this_data;
            last_bit = cur_bit;
        } else {
            pTile1[byte_count] = this_data % 256;
            last_data = this_data / 256;
            cur_bit -= 8;
            last_bit = cur_bit;
            byte_count += 1;
        }
        this_data = 0;
        cur_bit = 0;
    } //最后没凑满的数据补全一字节
    if (last_bit != 0) {
        pTile1[byte_count] = last_data;
        byte_count += 1;
    }
}

int argb2tile(const unsigned char *pClrBlk, unsigned char *pTile, int *pTILE_SIZE) {

    //assert(g_nTileWidth > 0 && g_nTileHeight > 0);
    //*pTILE_SIZE = g_nTileWidth * g_nTileHeight * 4;
    //memcpy(pTile, pClrBlk, *pTILE_SIZE);
    unsigned char pclr_a[TILE_SIZE * TILE_SIZE] = {0};
    unsigned char pclr_r[TILE_SIZE * TILE_SIZE] = {0};
    unsigned char pclr_g[TILE_SIZE * TILE_SIZE] = {0};
    unsigned char pclr_b[TILE_SIZE * TILE_SIZE] = {0};
    unsigned char pTile_a[TILE_SIZE * TILE_SIZE] = {0};
    unsigned char pTile_r[2 * TILE_SIZE * TILE_SIZE] = {0};
    unsigned char pTile_g[2 * TILE_SIZE * TILE_SIZE] = {0};
    unsigned char pTile_b[2 * TILE_SIZE * TILE_SIZE] = {0};
    int row = 0;
    int col = 0;

    int diff_a_col[TILE_SIZE * TILE_SIZE] = {0};
    int diff_r_col[TILE_SIZE * TILE_SIZE] = {0};
    int diff_g_col[TILE_SIZE * TILE_SIZE] = {0};
    int diff_b_col[TILE_SIZE * TILE_SIZE] = {0};
    int diff_a_abs_col[TILE_SIZE * TILE_SIZE] = {0};
    int diff_r_abs_col[TILE_SIZE * TILE_SIZE] = {0};
    int diff_g_abs_col[TILE_SIZE * TILE_SIZE] = {0};
    int diff_b_abs_col[TILE_SIZE * TILE_SIZE] = {0};

    int diff_a_row[TILE_SIZE * TILE_SIZE] = {0};
    int diff_r_row[TILE_SIZE * TILE_SIZE] = {0};
    int diff_g_row[TILE_SIZE * TILE_SIZE] = {0};
    int diff_b_row[TILE_SIZE * TILE_SIZE] = {0};
    int diff_a_abs_row[TILE_SIZE * TILE_SIZE] = {0};
    int diff_r_abs_row[TILE_SIZE * TILE_SIZE] = {0};
    int diff_g_abs_row[TILE_SIZE * TILE_SIZE] = {0};
    int diff_b_abs_row[TILE_SIZE * TILE_SIZE] = {0};

    int *diff_a;
    int *diff_r;
    int *diff_g;
    int *diff_b;
    int *diff_a_abs;
    int *diff_r_abs;
    int *diff_g_abs;
    int *diff_b_abs;

    int *flag_data_a;
    int *flag_data_r;
    int *flag_data_g;
    int *flag_data_b;

    int flag_data_a_col[TILE_SIZE * TILE_SIZE] = {0};
    int flag_data_r_col[TILE_SIZE * TILE_SIZE] = {0};
    int flag_data_g_col[TILE_SIZE * TILE_SIZE] = {0};
    int flag_data_b_col[TILE_SIZE * TILE_SIZE] = {0};

    int flag_data_a_row[TILE_SIZE * TILE_SIZE] = {0};
    int flag_data_r_row[TILE_SIZE * TILE_SIZE] = {0};
    int flag_data_g_row[TILE_SIZE * TILE_SIZE] = {0};
    int flag_data_b_row[TILE_SIZE * TILE_SIZE] = {0};

    int total_bit_a_col = 0;
    int total_bit_r_col = 0;
    int total_bit_g_col = 0;
    int total_bit_b_col = 0;

    int total_bit_a_row = 0;
    int total_bit_r_row = 0;
    int total_bit_g_row = 0;
    int total_bit_b_row = 0;

    int total_bit_a = 0;
    int total_bit_r = 0;
    int total_bit_g = 0;
    int total_bit_b = 0;

    int total_bit_a_col_flag = 0;
    int total_bit_r_col_flag = 0;
    int total_bit_g_col_flag = 0;
    int total_bit_b_col_flag = 0;

    int total_bit_a_row_flag = 0;
    int total_bit_r_row_flag = 0;
    int total_bit_g_row_flag = 0;
    int total_bit_b_row_flag = 0;

    int total_bit_a_row_data = 0;
    int total_bit_r_row_data = 0;
    int total_bit_g_row_data = 0;
    int total_bit_b_row_data = 0;

    int total_bit_a_col_data = 0;
    int total_bit_r_col_data = 0;
    int total_bit_g_col_data = 0;
    int total_bit_b_col_data = 0;

    int total_byte_a_col_flag = 0;
    int total_byte_r_col_flag = 0;
    int total_byte_g_col_flag= 0;
    int total_byte_b_col_flag = 0;

    int total_byte_a_row_flag = 0;
    int total_byte_r_row_flag = 0;
    int total_byte_g_row_flag = 0;
    int total_byte_b_row_flag = 0;

    int total_byte_a_row_data = 0;
    int total_byte_r_row_data = 0;
    int total_byte_g_row_data = 0;
    int total_byte_b_row_data = 0;

    int total_byte_a_col_data = 0;
    int total_byte_r_col_data = 0;
    int total_byte_g_col_data = 0;
    int total_byte_b_col_data = 0;

    int total_byte_a_col = 0;
    int total_byte_r_col = 0;
    int total_byte_g_col = 0;
    int total_byte_b_col = 0;

    int total_byte_a_row = 0;
    int total_byte_r_row = 0;
    int total_byte_g_row = 0;
    int total_byte_b_row = 0;

    int total_bit_g_flag = 0;
    int total_bit_r_flag = 0;
    int total_byte_r_flag = 0;
    int total_byte_g_flag = 0;

    int total_bit_r_data = 0;
    int total_bit_g_data = 0;
    int total_byte_r_data = 0;
    int total_byte_g_data = 0;

    int total_byte_a = 0;
    int total_byte_r = 0;
    int total_byte_g = 0;
    int total_byte_b = 0;
    int over_size_flag[4] = {0};
    int judge_total_byte[4] = {0};
    int bit_count = 0;
    int byte_count = 0;
    int total_byte = 0;
    int trans = 0;
    int abs_count = 0;

    int total_byte_col;
    int total_byte_row;
    int col_row_flag;

    int addr_r[7] = {0};
    int addr_g[7] = {0};

    int content_r[7] = {0};
    int content_g[7] = {0};


    int butonggeshu_r = 0;
    int butonggeshu_g = 0;

    int judge_equ_r = 0;
    int judge_equ_g = 0;

    int b;
    int g;
    int r;
    int a;
    //分配bgra通道
    for (trans = 0; trans < TILE_SIZE * TILE_SIZE; trans++) {
        pclr_b[trans] = pClrBlk[0 + 4 * trans];
        pclr_g[trans] = pClrBlk[1 + 4 * trans];
        pclr_r[trans] = pClrBlk[2 + 4 * trans];
        pclr_a[trans] = pClrBlk[3 + 4 * trans];
    }

    //求出差值
    diff_col(pclr_b, diff_b_col, diff_b_abs_col);
    diff_col(pclr_g, diff_g_col, diff_g_abs_col);
    diff_col(pclr_r, diff_r_col, diff_r_abs_col);
    diff_col(pclr_a, diff_a_col, diff_a_abs_col);

    diff_row(pclr_b, diff_b_row, diff_b_abs_row);
    diff_row(pclr_g, diff_g_row, diff_g_abs_row);
    diff_row(pclr_r, diff_r_row, diff_r_abs_row);
    diff_row(pclr_a, diff_a_row, diff_a_abs_row);

    //确定标志位
    encode_flag(flag_data_a_col, &total_bit_a_col, diff_a_col);
    encode_flag(flag_data_r_col, &total_bit_r_col, diff_r_col);
    encode_flag(flag_data_g_col, &total_bit_g_col, diff_g_col);
    encode_flag(flag_data_b_col, &total_bit_b_col, diff_b_col);
    //确定减少字节数

    //确定标志位
    encode_flag(flag_data_a_row, &total_bit_a_row, diff_a_row);
    encode_flag(flag_data_r_row, &total_bit_r_row, diff_r_row);
    encode_flag(flag_data_g_row, &total_bit_g_row, diff_g_row);
    encode_flag(flag_data_b_row, &total_bit_b_row, diff_b_row);
    //确定减少字节数


    total_bit_a_col_flag = bit_count_again_col(flag_data_a_col);
    total_bit_b_col_flag = bit_count_again_col(flag_data_b_col);

    total_bit_a_row_flag = bit_count_again_row(flag_data_a_row);
    total_bit_b_row_flag = bit_count_again_row(flag_data_b_row);

    total_bit_a_col_data = data_bit_counter(diff_a_abs_col, flag_data_a_col);
    total_bit_b_col_data = data_bit_counter(diff_b_abs_col, flag_data_b_col);

    total_bit_a_row_data = data_bit_counter(diff_a_abs_row, flag_data_a_row);
    total_bit_b_row_data = data_bit_counter(diff_b_abs_row, flag_data_b_row);

    //确定总字节数
    total_byte_a_col_flag = my_ceil(total_bit_a_col_flag, 8);
    total_byte_b_col_flag = my_ceil(total_bit_b_col_flag, 8);

    //确定总字节数
    total_byte_a_row_flag = my_ceil(total_bit_a_row_flag, 8);
    total_byte_b_row_flag = my_ceil(total_bit_b_row_flag, 8);

    total_byte_a_col_data = my_ceil(total_bit_a_col_data, 8);
    total_byte_b_col_data = my_ceil(total_bit_b_col_data, 8);

    total_byte_a_row_data = my_ceil(total_bit_a_row_data, 8);
    total_byte_b_row_data = my_ceil(total_bit_b_row_data, 8);


    total_byte_col = total_byte_a_col_flag + total_byte_b_col_flag + total_byte_a_col_data + total_byte_b_col_data;
    total_byte_row = total_byte_a_row_flag + total_byte_b_row_flag + total_byte_a_row_data + total_byte_b_row_data;


    if (total_byte_col <= total_byte_row) {
        col_row_flag = 0;
    } else {
        col_row_flag = 1;
    }

    flag_data_a = col_row_flag ? flag_data_a_row : flag_data_a_col;
    flag_data_r = col_row_flag ? flag_data_r_row : flag_data_r_col;
    flag_data_g = col_row_flag ? flag_data_g_row : flag_data_g_col;
    flag_data_b = col_row_flag ? flag_data_b_row : flag_data_b_col;

    diff_a = col_row_flag ? diff_a_row : diff_a_col;
    diff_r = col_row_flag ? diff_r_row : diff_r_col;
    diff_g = col_row_flag ? diff_g_row : diff_g_col;
    diff_b = col_row_flag ? diff_b_row : diff_b_col;

    diff_a_abs = col_row_flag ? diff_a_abs_row : diff_a_abs_col;
    diff_r_abs = col_row_flag ? diff_r_abs_row : diff_r_abs_col;
    diff_g_abs = col_row_flag ? diff_g_abs_row : diff_g_abs_col;
    diff_b_abs = col_row_flag ? diff_b_abs_row : diff_b_abs_col;
//确定两通道是否相同
    judge_equ_r = judge_rgbflag(flag_data_b,flag_data_r, addr_r, content_r, &butonggeshu_r);
    judge_equ_g = judge_rgbflag(flag_data_b,flag_data_g, addr_g, content_g, &butonggeshu_g);

    if(judge_equ_g == 0){
        if(col_row_flag){
            total_bit_g_flag = bit_count_again_row(flag_data_g);
        } else {
            total_bit_g_flag = bit_count_again_col(flag_data_g);
        }

        total_byte_g_flag = my_ceil(total_bit_g_flag, 8);
    } else{
        total_byte_g_flag = rgb_byte_judgement(content_g, butonggeshu_g);
    }
    if(judge_equ_r == 0){
        if(col_row_flag){
            total_bit_r_flag = bit_count_again_row(flag_data_r);
        } else {
            total_bit_r_flag = bit_count_again_col(flag_data_r);
        }
        total_byte_r_flag = my_ceil(total_bit_r_flag, 8);
    }else{
        total_byte_r_flag = rgb_byte_judgement(content_r, butonggeshu_r);
    }
    total_bit_r_data = data_bit_counter(diff_r_abs, flag_data_r);
    total_bit_g_data = data_bit_counter(diff_g_abs, flag_data_g);

    total_byte_r_data = my_ceil(total_bit_r_data, 8);
    total_byte_g_data = my_ceil(total_bit_g_data, 8);

    total_byte_r = total_byte_r_flag + total_byte_r_data;
    total_byte_g = total_byte_g_flag + total_byte_g_data;
    total_byte_b = col_row_flag ? (total_byte_b_row_flag + total_byte_b_row_data) : (total_byte_b_col_flag + total_byte_b_col_data);
    total_byte_a = col_row_flag ? (total_byte_a_row_flag + total_byte_a_row_data) : (total_byte_a_col_flag + total_byte_a_col_data);


    //判断是否超出字节数
    over_size_flag[0] = (total_byte_b >= TILE_SIZE * TILE_SIZE) ? 1 : 0;
    over_size_flag[1] = (total_byte_g >= TILE_SIZE * TILE_SIZE) ? 1 : 0;
    over_size_flag[2] = (total_byte_r >= TILE_SIZE * TILE_SIZE) ? 1 : 0;
    over_size_flag[3] = (total_byte_a >= TILE_SIZE * TILE_SIZE) ? 1 : 0;

    judge_total_byte[0] = over_size_flag[0] ? TILE_SIZE * TILE_SIZE : total_byte_b;
    judge_total_byte[1] = over_size_flag[1] ? TILE_SIZE * TILE_SIZE : total_byte_g;
    judge_total_byte[2] = over_size_flag[2] ? TILE_SIZE * TILE_SIZE : total_byte_r;
    judge_total_byte[3] = over_size_flag[3] ? TILE_SIZE * TILE_SIZE : total_byte_a;


    if(judge_equ_r == 1 || judge_equ_g == 1){
        if(judge_equ_r == 0){
            if(over_size_flag[2]){
                total_byte_r = TILE_SIZE * TILE_SIZE;
            } else{
                total_byte_r = total_byte_r;
            }

        }
        if(judge_equ_g == 0){
            if(over_size_flag[1]){
                total_byte_g = TILE_SIZE * TILE_SIZE;
            } else{
                total_byte_g = total_byte_g;
            }

        }
        if((total_byte_b + total_byte_r + total_byte_g) >= (3 * TILE_SIZE * TILE_SIZE - 1) && total_byte_a >= 63){
            memcpy(pTile, pclr_b, TILE_SIZE * TILE_SIZE);
            memcpy(pTile + 1 * TILE_SIZE * TILE_SIZE, pclr_g, TILE_SIZE * TILE_SIZE);
            memcpy(pTile + 2 * TILE_SIZE * TILE_SIZE, pclr_r, TILE_SIZE * TILE_SIZE);
            memcpy(pTile + 3 * TILE_SIZE * TILE_SIZE, pclr_a, TILE_SIZE * TILE_SIZE);
            total_byte = 4 * TILE_SIZE * TILE_SIZE;
        }
        else if((total_byte_b + total_byte_r + total_byte_g) >= (3 * TILE_SIZE * TILE_SIZE - 1) && total_byte_a < 63){
            over_size_flag[0] = 1;
            over_size_flag[1] = 1;
            over_size_flag[2] = 1;
            pTile[0] = judge_equ_r + (judge_equ_g << 1) + (col_row_flag << 2) + (over_size_flag[0] << 3) + (over_size_flag[1] << 4) + (over_size_flag[2] << 5)
                       +(over_size_flag[3] << 6);
            byte_count += 1;
            memcpy(pTile + byte_count, pclr_b, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
            memcpy(pTile + byte_count, pclr_g, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
            memcpy(pTile + byte_count, pclr_r, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
            a = compress_core(pclr_a[0], diff_a_abs, flag_data_a, over_size_flag[3], bit_count, pTile_a, col_row_flag);
            memcpy(pTile + byte_count, pTile_a, total_byte_a);
            total_byte = 3 * TILE_SIZE * TILE_SIZE + total_byte_a + 1;

        } else{
            pTile[0] = judge_equ_r + (judge_equ_g << 1) + (col_row_flag << 2) + (over_size_flag[0] << 3) + (over_size_flag[1] << 4) + (over_size_flag[2] << 5)
                       +(over_size_flag[3] << 6);
            byte_count += 1;
            b = compress_core(pclr_b[0], diff_b_abs, flag_data_b, 0, 0, pTile_b, col_row_flag);
            memcpy(pTile + byte_count, pTile_b, total_byte_b);
            byte_count += total_byte_b;
            total_byte += total_byte_b + 1;
            if(judge_equ_g){
                compress_rgbequ(diff_g_abs,addr_g, content_g, butonggeshu_g, pTile_g, pclr_g[0], flag_data_g);
                memcpy(pTile + byte_count, pTile_g, total_byte_g);
                byte_count += total_byte_g;
                total_byte += total_byte_g;
            } else{
                if(over_size_flag[1]){
                    memcpy(pTile + byte_count, pclr_g, TILE_SIZE * TILE_SIZE);
                    byte_count += TILE_SIZE * TILE_SIZE;
                    total_byte += TILE_SIZE * TILE_SIZE;
                } else{
                    g = compress_core(pclr_g[0], diff_g_abs, flag_data_g, 0, 0, pTile_g, col_row_flag);
                    memcpy(pTile + byte_count, pTile_g, total_byte_g);
                    byte_count += total_byte_g;
                    total_byte += total_byte_g;
                }

            }
            if(judge_equ_r){
                compress_rgbequ(diff_r_abs,addr_r, content_r, butonggeshu_r, pTile_r, pclr_r[0], flag_data_r);
                memcpy(pTile + byte_count, pTile_r, total_byte_r);
                byte_count += total_byte_r;
                total_byte += total_byte_r;
            }else{
                if(over_size_flag[2]){
                    memcpy(pTile + byte_count, pclr_r, TILE_SIZE * TILE_SIZE);
                    byte_count += TILE_SIZE * TILE_SIZE;
                    total_byte += TILE_SIZE * TILE_SIZE;
                } else {
                    r = compress_core(pclr_r[0], diff_r_abs, flag_data_r, 0, 0, pTile_r, col_row_flag);
                    memcpy(pTile + byte_count, pTile_r, total_byte_r);
                    byte_count += total_byte_r;
                    total_byte += total_byte_r;
                }

            }
            if(over_size_flag[3]){
                memcpy(pTile + byte_count,pclr_a, TILE_SIZE * TILE_SIZE);
                total_byte += TILE_SIZE * TILE_SIZE;
            }else{
                a = compress_core(pclr_a[0], diff_a_abs, flag_data_a, over_size_flag[3], bit_count, pTile_a, col_row_flag);
                memcpy(pTile + byte_count, pTile_a, total_byte_a);
                total_byte += total_byte_a;
            }
        }
    } else {
        //若都没有超容量
        if (over_size_flag[0] == 0 && over_size_flag[1] == 0 && over_size_flag[2] == 0 && over_size_flag[3] == 0) {
            bit_count = 0;
            byte_count = 1;
            total_byte = total_byte_a + total_byte_b + total_byte_g + total_byte_r + 1;
            pTile[0] = col_row_flag << 2;
        }
            //四个通道都超了
        else if (over_size_flag[0] == 1 && over_size_flag[1] == 1 && over_size_flag[2] == 1 &&
                 over_size_flag[3] == 1) {
            byte_count = 0;
            bit_count = 1;
            total_byte = 256;
        }
            //如果有超但没有全部超
        else {
            //如果多一个字节达到256，不压，并把标志全取1，以便后面分辨压不压
            if ((judge_total_byte[0] + judge_total_byte[1] + judge_total_byte[2] + judge_total_byte[3] + 1) == 256) {
                bit_count = 1;
                byte_count = 0;
                total_byte = 256;
                over_size_flag[0] = 1;
                over_size_flag[1] = 1;
                over_size_flag[2] = 1;
                over_size_flag[3] = 1;
            }
                //多一个没到256字节，可以压
            else {
                pTile[0] = (over_size_flag[0] << 3) + (over_size_flag[1] << 4) + (over_size_flag[2] << 5) +
                           (over_size_flag[3] << 6) + (col_row_flag << 2);
                bit_count = 0;
                byte_count = 1;
                total_byte_b = over_size_flag[0] ? TILE_SIZE * TILE_SIZE : total_byte_b;
                total_byte_g = over_size_flag[1] ? TILE_SIZE * TILE_SIZE : total_byte_g;
                total_byte_r = over_size_flag[2] ? TILE_SIZE * TILE_SIZE : total_byte_r;
                total_byte_a = over_size_flag[3] ? TILE_SIZE * TILE_SIZE : total_byte_a;

                total_byte = total_byte_b + total_byte_g + total_byte_r + total_byte_a + 1;
            }
        }

        // B
        b = compress_core(pclr_b[0], diff_b_abs, flag_data_b, over_size_flag[0], bit_count, pTile_b, col_row_flag);
        //每压完一个通道更新bitcount和bytecount
        bit_count = 0;
        //超容量直接赋原值
        if (over_size_flag[0]) {
            memcpy(pTile + byte_count, pclr_b, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
        } else {
            memcpy(pTile + byte_count, pTile_b, total_byte_b);
            byte_count += total_byte_b;
        }
        // G
        g = compress_core(pclr_g[0], diff_g_abs, flag_data_g, over_size_flag[1], bit_count, pTile_g, col_row_flag);
        bit_count = 0;
        if (over_size_flag[1]) {
            memcpy(pTile + byte_count, pclr_g, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
        } else {
            memcpy(pTile + byte_count, pTile_g, total_byte_g);
            byte_count += total_byte_g;
        }
        // R
        r = compress_core(pclr_r[0], diff_r_abs, flag_data_r, over_size_flag[2], bit_count, pTile_r, col_row_flag);
        bit_count = 0;
        if (over_size_flag[2]) {
            memcpy(pTile + byte_count, pclr_r, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
        } else {
            memcpy(pTile + byte_count, pTile_r, total_byte_r);
            byte_count += total_byte_r;
        }
        // A
        a = compress_core(pclr_a[0], diff_a_abs, flag_data_a, over_size_flag[3], bit_count, pTile_a, col_row_flag);
        bit_count = 0;
        if (over_size_flag[3]) {
            memcpy(pTile + byte_count, pclr_a, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
        } else {
            memcpy(pTile + byte_count, pTile_a, total_byte_a);
            byte_count += total_byte_a;
        }
    }

    *pTILE_SIZE = total_byte;
    return 0;
}


/* decompress tile data to ARGB
*  param:
*    pTile        -- IN, tile data
*    pTileSize    -- IN, tile's bytes
*    pClrBlk      -- OUT, pixel's ARGB data
*  return:
*    0  -- succeed
*   -1  -- failed
*/
int decompress_core(const unsigned char *pTile, int bit_count, int byte_count, int total_byte,
                    unsigned char *block_data, int col_row_flag, int* flag1) {
    //int data_bit_count[7] = {2, 4, 4, 7, 8, 13, 13};
    int data_bit_count[7] = {0, 2, 2, 4, 4, 8, 8};
    int flag_bit_count[7] = {2, 2, 2, 3, 4, 5, 5};
    int sign_table[7] = {1, 1, -1, 1, -1, 1, -1};
    int flag_index_table[32] = {
            1, 0, 2, 3, 1, 0, 2, 4,
            1, 0, 2, 3, 1, 0, 2, 5,
            1, 0, 2, 3, 1, 0, 2, 4,
            1, 0, 2, 3, 1, 0, 2, 6
    };
    int i = 1;

    int first_data = 0;
    int last_data = 0;
    int this_data = 0;
    int cur_flag = 0;
    int diff = 0;
    int diff_flag = 0;
    int cur_pixel_bit = 0;
    int cur_eff_pixel_bit = 0;
    int cur_bit = 0;
    int last_bit = 0;
    int de_flag = 0;
    int flag[TILE_SIZE * TILE_SIZE] = {0};
    int row = 0;
    int col = 0;
    int pre_count = 0;
    int judgebit = 0;
    int decode = 0;
    int trans = 0;

    flag[0] = 0;//第一位标志位为0
//循环TILESIZE次，只要保证每一次循环开始首位都是判断位即可
    for (de_flag = 0; de_flag < TILE_SIZE; de_flag++) {
        if(last_bit < 7){
            this_data = (pTile[byte_count] << last_bit) + last_data;
            cur_bit = last_bit + 8;
            byte_count += 1;
        }
        else{
            this_data = last_data;
            cur_bit = last_bit;
        }
        judgebit = this_data % 4;//读取判断位
        if ((judgebit == 0) || (judgebit == 2)) {
            this_data = this_data >> 1;//去掉判断位
            cur_bit -= 1;
            if (col_row_flag) {
                if(de_flag == 0){
                    for (col = 1; col < TILE_SIZE; col++) {
                        flag[de_flag + col * TILE_SIZE] = flag_index_table[this_data % 32];
                    }
                } else {
                    for (col = 0; col < TILE_SIZE; col++) {
                        flag[de_flag + col * TILE_SIZE] = flag_index_table[this_data % 32];
                    }
                }
            } else {
                if (de_flag == 0) {
                    for (row = 1; row < TILE_SIZE; row++) {
                        flag[de_flag * TILE_SIZE + row] = flag_index_table[this_data % 32];
                    }
                } else {
                    for (row = 0; row < TILE_SIZE; row++) {
                        flag[de_flag * TILE_SIZE + row] = flag_index_table[this_data % 32];
                    }
                }
            }
            last_data = this_data >> flag_bit_count[flag_index_table[this_data % 32]];
            last_bit = cur_bit - flag_bit_count[flag_index_table[this_data % 32]];
            this_data = 0;
            cur_bit = 0;


        } else if (judgebit == 1) {
            this_data = this_data >> 2;
            diff = this_data % 8;//标志位不一样的位置,取前三位
            this_data = this_data >> LOG2_TILE_SIZE;
            cur_bit  = cur_bit - 2 - LOG2_TILE_SIZE;
            if(cur_bit < 7){
                this_data = (pTile[byte_count] << cur_bit) + this_data;
                cur_bit += 8;
                this_data = (pTile[byte_count + 1] << cur_bit) + this_data;//数据不够再都一个字节
                byte_count += 2;
                cur_bit += 8;
            }
            else if(cur_bit < 16 && cur_bit >= 7){
                this_data = (pTile[byte_count] << cur_bit) + this_data;//数据不够再都一个字节
                byte_count += 1;
                cur_bit += 8;
            } else{
                this_data = this_data;
            }
            diff_flag = flag_index_table[this_data % 32];//再取x位内容位
            this_data = this_data >> flag_bit_count[diff_flag];
            cur_bit -= flag_bit_count[diff_flag];
            if (col_row_flag) {
                if(de_flag == 0){
                    for (col = 1; col < TILE_SIZE; col++) {
                        if (col == diff) {
                            flag[de_flag + col * TILE_SIZE] = diff_flag;
                        } else {
                            flag[de_flag + col * TILE_SIZE] = flag_index_table[this_data % 32];
                        }
                    }
                } else {
                    for (col = 0; col < TILE_SIZE; col++) {
                        if (col == diff) {
                            flag[de_flag + col * TILE_SIZE] = diff_flag;
                        } else {
                            flag[de_flag + col * TILE_SIZE] = flag_index_table[this_data % 32];
                        }
                    }
                }
            } else {
                if(de_flag == 0){
                    for (row = 1; row < TILE_SIZE; row++) {
                        if (row == diff) {
                            flag[de_flag * TILE_SIZE + row] = diff_flag;
                        } else {
                            flag[de_flag * TILE_SIZE + row] = flag_index_table[this_data % 32];
                        }
                    }
                } else {
                    for (row = 0; row < TILE_SIZE; row++) {
                        if (row == diff) {
                            flag[de_flag * TILE_SIZE + row] = diff_flag;
                        } else {
                            flag[de_flag * TILE_SIZE + row] = flag_index_table[this_data % 32];
                        }
                    }
                }
            }
            last_data = this_data >> flag_bit_count[flag_index_table[this_data % 32]];
            last_bit = cur_bit - flag_bit_count[flag_index_table[this_data % 32]];
            this_data = 0;
            cur_bit = 0;
        } else if (judgebit == 3) {
            this_data = this_data >> 2;
            cur_bit -= 2;
            if (col_row_flag) {
                if(de_flag == 0){
                    for (col = 1; col < TILE_SIZE; col++) {
                        if (cur_bit < 5) {
                            //不够需要读取下一字节
                            this_data = this_data + (pTile[byte_count] << cur_bit);
                            byte_count += 1;
                            cur_bit += 8;
                        } else {//不需要读取下一字节
                            this_data = this_data;
                        }
                        flag[de_flag + col * TILE_SIZE] = flag_index_table[this_data % 32];
                        cur_bit -= flag_bit_count[flag_index_table[this_data % 32]];
                        this_data = this_data >> flag_bit_count[flag_index_table[this_data % 32]];

                    }
                } else {
                    for (col = 0; col < TILE_SIZE; col++) {
                        if (cur_bit < 5) {//不够需要读取下一字节
                            this_data = this_data + (pTile[byte_count] << cur_bit);
                            byte_count += 1;
                            cur_bit += 8;
                        } else {//不需要读取下一字节
                            this_data = this_data;
                        }
                        flag[de_flag + col * TILE_SIZE] = flag_index_table[this_data % 32];
                        cur_bit -= flag_bit_count[flag_index_table[this_data % 32]];
                        this_data = this_data >> flag_bit_count[flag_index_table[this_data % 32]];

                    }
                }
            } else {
                if (de_flag == 0) {
                    for (row = 1; row < TILE_SIZE; row++) {
                        if (cur_bit < 5) {//不够需要读取下一字节
                            this_data = this_data + (pTile[byte_count] << cur_bit);
                            byte_count += 1;
                            cur_bit += 8;
                        } else {//不需要读取下一字节
                            this_data = this_data;
                        }
                        flag[de_flag * TILE_SIZE + row] = flag_index_table[this_data % 32];
                        cur_bit -= flag_bit_count[flag_index_table[this_data % 32]];
                        this_data = this_data >> flag_bit_count[flag_index_table[this_data % 32]];
                    }
                } else {
                    for (row = 0; row < TILE_SIZE; row++) {
                        if (cur_bit < 5) {//不够需要读取下一字节
                            this_data = this_data + (pTile[byte_count] << cur_bit);
                            byte_count += 1;
                            cur_bit += 8;
                        } else {//不需要读取下一字节
                            this_data = this_data;
                        }
                        flag[de_flag * TILE_SIZE + row] = flag_index_table[this_data % 32];
                        cur_bit -= flag_bit_count[flag_index_table[this_data % 32]];
                        this_data = this_data >> flag_bit_count[flag_index_table[this_data % 32]];
                    }
                }
            }
            last_data = this_data;
            last_bit = cur_bit;
            this_data = 0;
            cur_bit = 0;
        } else {
            std::cout << "error judge" << std::endl;
        }
    }

    if(last_bit >= 16){
        byte_count -= 2;
    }
    else if(last_bit < 16 &&last_bit >= 8){
        byte_count -= 1;
    }

    for(trans = 0;trans<TILE_SIZE * TILE_SIZE;trans++){
        flag1[trans] = flag[trans];
    }
    first_data = pTile[byte_count];//基准数据
    byte_count += 1;
    block_data[0] = first_data;
    for (decode = 1; decode < TILE_SIZE * TILE_SIZE;decode++) {
        if (cur_bit < data_bit_count[flag[decode]]) {
            //数据不够解码需要读取下一字节
            this_data = (pTile[byte_count] << cur_bit) + this_data;
            cur_bit += 8;
            byte_count += 1;
        }

        if(col_row_flag){
            if(decode % TILE_SIZE == 0){
                block_data[decode] = block_data[decode - TILE_SIZE] + sign_table[flag[decode]] * (this_data % (1 << data_bit_count[flag[decode]]));
            }else{
                block_data[decode] = block_data[decode - 1] + sign_table[flag[decode]] * (this_data % (1 << data_bit_count[flag[decode]]));
            }
        }else{
            if(decode <= (TILE_SIZE - 1)){
                block_data[decode] = block_data[decode - 1] + sign_table[flag[decode]] * (this_data % (1 << data_bit_count[flag[decode]]));
            }else{
                block_data[decode] = block_data[decode - TILE_SIZE] + sign_table[flag[decode]] * (this_data % (1 << data_bit_count[flag[decode]]));
            }
        }
        this_data = this_data >> data_bit_count[flag[decode]];
        cur_bit -= data_bit_count[flag[decode]];
    }
    if(cur_bit >= 8){
        byte_count -= 1;
    }
    return byte_count;
}

int decompress_rgbequ(const unsigned char* pTile, int byte_count,unsigned char* block_data, int* flag1, int col_row_flag){
    int butongzongshu = 0;
    int addr[7] = {0};
    int content[7] = {0};
    int this_data = 0;
    int cur_bit = 0;
    int last_data = 0;
    int last_bit = 0;
    int flag[TILE_SIZE * TILE_SIZE] = {0};
    int decode = 0;
    int n = 0;
    int m = 1;

    int flag_bit_count[7] = {2, 2, 2, 3, 4, 5, 5};
    int data_bit_count[7] = {0, 2, 2, 4, 4, 8, 8};
    int sign_table[7] = {1, 1, -1, 1, -1, 1, -1};
    int flag_index_table[32] = {
            1, 0, 2, 3, 1, 0, 2, 4,
            1, 0, 2, 3, 1, 0, 2, 5,
            1, 0, 2, 3, 1, 0, 2, 4,
            1, 0, 2, 3, 1, 0, 2, 6
    };

    this_data = pTile[byte_count];
    byte_count += 1;
    butongzongshu = this_data % 8;
    last_data = this_data >> 3;//3位总数
    last_bit = 5;
    if(butongzongshu == 0){
        last_data = 0;
        last_bit -= 5;
    }
    for(n = 0;n<butongzongshu;n++){
        if(last_bit < 2 * LOG2_TILE_SIZE){
            this_data = last_data + (pTile[byte_count] << last_bit);
            byte_count += 1;
            cur_bit = last_bit + 8;
        }else
        {
            this_data = last_data;
            cur_bit = last_bit;
        }
        addr[n] = this_data % 64 ;
        this_data = this_data / 64;
        cur_bit -= 2 * LOG2_TILE_SIZE;
        if(cur_bit < 5){
            this_data = this_data + (pTile[byte_count] << cur_bit);
            cur_bit += 8;
            byte_count += 1;
        }
        content[n] = flag_index_table[this_data % 32];
        last_data = this_data >> flag_bit_count[content[n]];
        cur_bit -= flag_bit_count[content[n]];
        last_bit = cur_bit;
        this_data = 0;
        cur_bit = 0;
        flag[addr[n]] = content[n];
    }
    if(last_bit >= 8){
        byte_count -= 1;
        last_data = 0;
        last_bit = 0;
    }
    flag[0] = 0;
    n = 0;
    for(m = 1; m < TILE_SIZE * TILE_SIZE; m++){
        if(m == addr[n]){
            n += 1;
        } else {
            flag[m] = flag1[m];
        }
    }
    block_data[0] = pTile[byte_count];
    byte_count += 1;
    for (decode = 1; decode < TILE_SIZE * TILE_SIZE;decode++) {
        if (cur_bit < data_bit_count[flag[decode]]) {
            //数据不够解码需要读取下一字节
            this_data = (pTile[byte_count] << cur_bit) + this_data;
            cur_bit += 8;
            byte_count += 1;
        }
        if(col_row_flag){
            if(decode % TILE_SIZE == 0){
                block_data[decode] = block_data[decode - TILE_SIZE] + sign_table[flag[decode]] * (this_data % (1 << data_bit_count[flag[decode]]));
            }else{
                block_data[decode] = block_data[decode - 1] + sign_table[flag[decode]] * (this_data % (1 << data_bit_count[flag[decode]]));
            }
        }else{
            if(decode <= (TILE_SIZE - 1)){
                block_data[decode] = block_data[decode - 1] + sign_table[flag[decode]] * (this_data % (1 << data_bit_count[flag[decode]]));
            }else{
                block_data[decode] = block_data[decode - TILE_SIZE] + sign_table[flag[decode]] * (this_data % (1 << data_bit_count[flag[decode]]));
            }
        }
        this_data = this_data >> data_bit_count[flag[decode]];
        cur_bit -= data_bit_count[flag[decode]];
    }
    if(cur_bit >= 8){
        byte_count -= 1;
    }
    return byte_count;
}

int tile2argb(const unsigned char* pTile, int nTileSize, unsigned char* pClrBlk) {
    //memcpy(pClrBlk, pTile, nTileSize);
    int over_size_flag[4] = {0};
    int bit_count;
    int byte_count;
    int byte_count_new;
    int trans = 0;
    int col_row_flag;
    int flag1[TILE_SIZE * TILE_SIZE] = {0};
    int flag2[TILE_SIZE * TILE_SIZE] = {0};
    int judge_equ_r = 0;
    int judge_equ_g = 0;
    unsigned char block_b[TILE_SIZE * TILE_SIZE] = {0};
    unsigned char block_g[TILE_SIZE * TILE_SIZE] = {0};
    unsigned char block_r[TILE_SIZE * TILE_SIZE] = {0};
    unsigned char block_a[TILE_SIZE * TILE_SIZE] = {0};
    unsigned char block_data[TILE_SIZE * TILE_SIZE * 4] ={0};
    //四个都没压缩
    if (nTileSize == 4 * TILE_SIZE * TILE_SIZE) {
        over_size_flag[0] = 1;
        over_size_flag[1] = 1;
        over_size_flag[2] = 1;
        over_size_flag[3] = 1;
        bit_count = 0;
        byte_count = 0;
    } else {
        judge_equ_r = (pTile[0] % 2);
        judge_equ_g = (pTile[0] % 4) / 2;
        col_row_flag = (pTile[0] % 8) / 4;
        over_size_flag[0] = (pTile[0] % 16) / 8;
        over_size_flag[1] = (pTile[0] % 32) / 16;
        over_size_flag[2] = (pTile[0] % 64) / 32;
        over_size_flag[3] = (pTile[0] % 128) / 64;

        bit_count = 0;
        byte_count = 1;
    }


    if(judge_equ_g == 1 || judge_equ_r == 1){
        if(nTileSize == 4 * TILE_SIZE * TILE_SIZE){
            memcpy(block_b, pTile, TILE_SIZE * TILE_SIZE);
            memcpy(block_g, pTile + TILE_SIZE * TILE_SIZE * 1, TILE_SIZE * TILE_SIZE);
            memcpy(block_r, pTile + TILE_SIZE * TILE_SIZE * 2, TILE_SIZE * TILE_SIZE);
            memcpy(block_a, pTile + TILE_SIZE * TILE_SIZE * 3, TILE_SIZE * TILE_SIZE);
        } else if(over_size_flag[0] == 1 && over_size_flag[1] == 1 && over_size_flag[2] == 1){
            memcpy(block_b, pTile + byte_count, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
            memcpy(block_g, pTile + byte_count, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
            memcpy(block_r, pTile + byte_count, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
            if(over_size_flag[3]){
                memcpy(block_a, pTile + byte_count, TILE_SIZE * TILE_SIZE);
            } else{
                byte_count_new = decompress_core(pTile, bit_count, byte_count, nTileSize, block_a, col_row_flag, flag2);
                byte_count = byte_count_new;
            }
        } else{
            byte_count_new = decompress_core(pTile, bit_count, byte_count, nTileSize, block_b, col_row_flag, flag1);
            byte_count = byte_count_new;
            if(judge_equ_g == 1){
                byte_count_new = decompress_rgbequ(pTile, byte_count,block_g, flag1, col_row_flag);
                byte_count = byte_count_new;
            } else {
                if(over_size_flag[1]){
                    memcpy(block_g, pTile + byte_count, TILE_SIZE * TILE_SIZE);
                    byte_count += TILE_SIZE * TILE_SIZE;
                } else{
                    byte_count_new = decompress_core(pTile, bit_count, byte_count, nTileSize, block_g, col_row_flag, flag2);
                    byte_count = byte_count_new;
                }
            }
            if(judge_equ_r == 1){
                byte_count_new = decompress_rgbequ(pTile, byte_count,block_r, flag1, col_row_flag);
                byte_count = byte_count_new;
            } else {
                if(over_size_flag[2]){
                    memcpy(block_r, pTile + byte_count, TILE_SIZE * TILE_SIZE);
                    byte_count += TILE_SIZE * TILE_SIZE;
                } else{
                    byte_count_new = decompress_core(pTile, bit_count, byte_count, nTileSize, block_r, col_row_flag, flag2);
                    byte_count = byte_count_new;
                }
            }
            if(over_size_flag[3]){
                memcpy(block_a, pTile + byte_count, TILE_SIZE * TILE_SIZE);
            } else{
                byte_count_new = decompress_core(pTile, bit_count, byte_count, nTileSize, block_a, col_row_flag, flag2);
                byte_count = byte_count_new;
            }
        }

    } else {
        //B
        if (over_size_flag[0]) {
            memcpy(block_b, pTile + byte_count, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
        } else {
            byte_count_new = decompress_core(pTile, bit_count, byte_count, nTileSize, block_b, col_row_flag, flag1);
            byte_count = byte_count_new;
            bit_count = 0;
        }
        //G
        if (over_size_flag[1]) {
            memcpy(block_g, pTile + byte_count, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
        } else {
            byte_count_new = decompress_core(pTile, bit_count, byte_count, nTileSize, block_g, col_row_flag, flag2);
            byte_count = byte_count_new;
            bit_count = 0;
        }
        //R
        if (over_size_flag[2]) {
            memcpy(block_r, pTile + byte_count, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
        } else {
            byte_count_new = decompress_core(pTile, bit_count, byte_count, nTileSize, block_r, col_row_flag, flag2);
            byte_count = byte_count_new;
            bit_count = 0;
        }
        //A
        if (over_size_flag[3]) {
            memcpy(block_a, pTile + byte_count, TILE_SIZE * TILE_SIZE);
            byte_count += TILE_SIZE * TILE_SIZE;
        } else {
            byte_count_new = decompress_core(pTile, bit_count, byte_count, nTileSize, block_a, col_row_flag, flag2);
            byte_count = byte_count_new;
            bit_count = 0;
        }
    }
    for(trans = 0; trans < 64; trans++){
        pClrBlk[0 + 4*trans] = block_b[trans];
        pClrBlk[1 + 4*trans] = block_g[trans];
        pClrBlk[2 + 4*trans] = block_r[trans];
        pClrBlk[3 + 4*trans] = block_a[trans];
    }

    return 0;
}
