#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    uint64 k;

    int result = dump2(3, 2, &k);
    printf("This process PID: %d \n", getpid());
    printf("Result: %d\n", result);
    printf("%p \n", k);
    exit(0);
}
