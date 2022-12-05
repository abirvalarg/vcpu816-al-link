#include <stdio.h>
#include <stdlib.h>
#include "errors.h"

__attribute__((noreturn))
void out_of_memory()
{
    puts("Out of memory!");
    exit(1);
}
