#include <stdio.h>
#include "complex_sample.h"

void complex_sample_get_string(char* c, complex_sample_t data)
{
    sprintf(c, "%d + %dj", data.data_re, data.data_im);
}
