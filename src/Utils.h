#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define ERROR_EXIT(...) { fprintf(stderr, "ERORR: "); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "!\n"); exit(1); }

namespace utils
{

}
