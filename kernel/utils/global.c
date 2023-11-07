#include "../param.h"
#include "../types.h"
#include "../riscv.h"
#include "../defs.h"


void printfb(uint64 u) {
    printf("0b");
    while (u != 0) {
        printf("%d", (u % 2));
        u /= 2;
    }
    printf("; ");
} 