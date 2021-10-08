#include <stdint.h>

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