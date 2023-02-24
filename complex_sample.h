
#ifndef complex_sample_H
#define complex_sample_H

// short complex number
typedef struct complex_sample
{
    short data_re;
    short data_im;
} complex_sample_t;

// convert complex data to a string
void complex_sample_get_string(char* c, complex_sample_t data);

#endif // complex_sample_H
