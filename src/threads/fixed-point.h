#ifndef FIXED_POINT_H
#define FIXED_POINT_H

typedef int FP;
static const int Correction = 16384; // 1 << 14

static inline FP int_to_FP(int x){
    return x * Correction;
}

static inline FP FP_to_int(int x){
    return x / Correction;
}

static inline FP FP_to_round(FP x){
    if(x >= 0) 
        return (x + (Correction / 2)) / Correction;
    return (x - (Correction / 2)) / Correction;
}

static inline FP FP_add(FP x, FP y){
    return x + y;
}

static inline FP FP_add_int(FP x, int y){
    return x + y * Correction;
}

static inline FP FP_sub(FP x, FP y){
    return x - y;
}

static inline FP FP_sub_int(FP x, int y){
    return x - y * Correction;
}

static inline FP FP_mult(FP x, FP y){
    return (((int64_t) x) * y) / Correction;
}

static inline FP FP_mult_int(FP x, int y){
    return x * y;
}

static inline FP FP_div(FP x, FP y){
    return (((int64_t) x) * Correction) / y;
}

static inline FP FP_div_int(FP x, int y){
    return x / y;
}

#endif