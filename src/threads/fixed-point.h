static int Correction = 16384; // 1 << 14

typedef int FP;

FP int_to_FP(int x);
int FP_to_int(FP x);
FP FP_to_round(FP x);
FP FP_add(FP x, FP y);
FP FP_add_int(FP x, int y);
FP FP_sub(FP x, FP y);
FP FP_sub_int(FP x, int y);
FP FP_mult(FP x, FP y);
FP FP_mult_int(FP x, int y);
FP FP_div(FP x, FP y);
FP FP_div_int(FP x, int y);

FP int_to_FP(int x){
    return x * Correction;
}

FP FP_to_zero(FP x){
    return x / Correction;
}

FP FP_to_round(FP x){
    if(x >= 0) 
        return (x + (Correction / 2)) / Correction;
    return (x - (Correction / 2)) / Correction;
}

FP FP_add(FP x, FP y){
    return x + y;
}

FP FP_add_int(FP x, int y){
    return x + y * Correction;
}

FP FP_sub(FP x, FP y){
    x - y;
}

FP FP_sub_int(FP x, int y){
    x - y * Correction;
}

FP FP_mult(FP x, FP y){
    return (((int64_t) x) * y) / Correction;
}

FP FP_mult_int(FP x, int y){
    return x * y;
}

FP FP_div(FP x, FP y){
    return (((int64_t) x) * Correction) / y;
}

FP FP_div_int(FP x, int y){
    return x / y;
}