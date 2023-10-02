
#define MSTRING_IMPLEMENTATION
#include "MString.h"

#include <stdio.h>

int main()
{
    IString something = {};
    something = "Hello, world!";
    printf("%s\n", something.Ptr());
    return 0;
}