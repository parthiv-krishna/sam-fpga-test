#ifndef FFT_H
#define FFT_H

#include "complex_sample.h"
#include "dma_accel.h"

#define FFT_ARCH_PIPELINED   0
#define FFT_ARCH_RADIX4      1
#define FFT_ARCH_RADIX2      2
#define FFT_ARCH_RADIX2_LITE 3

#define FFT_MAX_NUM_PTS      8192
#define FFT_NUM_PTS_MASK     0x0000001F // Bits [4:0]
#define FFT_NUM_PTS_SHIFT    0
#define FFT_FWD_INV_MASK     0x00000100 // Bit 8
#define FFT_FWD_INV_SHIFT    8
#define FFT_SCALE_SCH_MASK   0x007FFE00 // Bits [22:9]
#define FFT_SCALE_SCH_SHIFT  9

#define FFT_SUCCESS          0
#define FFT_GPIO_INIT_FAIL  -1
#define FFT_ILLEGAL_NUM_PTS -2
#define FFT_DMA_FAIL        -3

typedef enum
{
    FFT_INVERSE = 0,
    FFT_FORWARD = 1
} fft_fwd_inv_t;

typedef struct fft fft_t;

fft_t* fft_create(int gpio_device_id, int dma_device_id, int intc_device_id, int s2mm_intr_id, int mm2s_intr_id);

void fft_destroy(fft_t* p_fft_inst);

void fft_set_fwd_inv(fft_t* p_fft_inst, fft_fwd_inv_t fwd_inv);

fft_fwd_inv_t fft_get_fwd_inv(fft_t* p_fft_inst);

int fft_set_num_pts(fft_t* p_fft_inst, int num_pts);

int fft_get_num_pts(fft_t* p_fft_inst);

void fft_set_scale_sch(fft_t* p_fft_inst, int scale_sch);

int fft_get_scale_sch(fft_t* p_fft_inst);

int fft(fft_t* p_fft_inst, complex_sample_t* din, complex_sample_t* dout);

complex_sample_t* fft_get_input_buf(fft_t* p_fft_inst);

complex_sample_t* fft_get_output_buf(fft_t* p_fft_inst);

void fft_print_params(fft_t* p_fft_inst);

void fft_print_input_buf(fft_t* p_fft_inst);

void fft_print_output_buf(fft_t* p_fft_inst);

#endif // FFT_H

