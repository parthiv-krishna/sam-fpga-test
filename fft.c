#include <stdlib.h>
#include "fft.h"
#include "xgpio.h"

typedef struct fft_periphs {
    dma_accel_t* p_dma_accel_inst;
    XGpio        gpio_inst;
} fft_periphs_t;

typedef struct fft {
    fft_periphs_t periphs;
    fft_fwd_inv_t fwd_inv;
    int           num_pts;
    int           scale_sch;
} fft_t;

static int is_power_of_2(int x) {
    while ((x % 2 == 0) && (x > 1)) {
        x /= 2;
    }

    return (x == 1);
}

// floor(log2(x ))
static int floor_log2(int x) {    
    int log2_N = 0;
    x--;
    for (log2_N = 0;  x > 0; log2_N++) {
        x >>= 1;
    }
    return log2_N;
}

static int init_gpio(XGpio* p_gpio_inst, int gpio_device_id) {
    // init driver
    int status = XGpio_Initialize(p_gpio_inst, gpio_device_id);
    if (status != XST_SUCCESS)
    {
        xil_printf("ERROR! Initialization of AXI GPIO instance failed.\n\r");
        return FFT_GPIO_INIT_FAIL;
    }

    return FFT_SUCCESS;

}

static void fft_commit_params(fft_t* p_fft_inst) {

    int reg  = (p_fft_inst->scale_sch         << FFT_SCALE_SCH_SHIFT) & FFT_SCALE_SCH_MASK;
    reg |= (p_fft_inst->fwd_inv           << FFT_FWD_INV_SHIFT)   & FFT_FWD_INV_MASK;
    reg |= (floor_log2(p_fft_inst->num_pts) << FFT_NUM_PTS_SHIFT)   & FFT_NUM_PTS_MASK;

    XGpio_DiscreteWrite(&p_fft_inst->periphs.gpio_inst, 1, reg);

}

// Public functions
fft_t* fft_create(int gpio_device_id, int dma_device_id, int intc_device_id, int s2mm_intr_id, int mm2s_intr_id) {

    // Allocate memory for FFT object
    fft_t* p_obj = (fft_t*) malloc(sizeof(fft_t));
    if (p_obj == NULL) {
        xil_printf("ERROR! Failed to allocate memory for FFT object.\n\r");
        return NULL;
    }

    // create dma accelerator that will be used to compute fft
    p_obj->periphs.p_dma_accel_inst = dma_accel_create(dma_device_id, intc_device_id, s2mm_intr_id, mm2s_intr_id, sizeof(complex_sample_t));

    if (p_obj->periphs.p_dma_accel_inst == NULL) {
        xil_printf("ERROR! Failed to create DMA Accelerator object for use by the FFT engine.\n\r");
        free(p_obj);
        return NULL;
    }

    // register and init peripherals
    int status = init_gpio(&p_obj->periphs.gpio_inst, gpio_device_id);
    if (status != FFT_SUCCESS) {
        xil_printf("ERROR! Failed to initialize GPIO.\n\r");
        fft_destroy(p_obj);
        return NULL;
    }

    // init fft parameters
    fft_set_fwd_inv(p_obj, FFT_FORWARD);
    status = fft_set_num_pts(p_obj, 1024);
    if (status != FFT_SUCCESS) {
        xil_printf("ERROR! Failed to initialize the number of points in the FFT.\n\r");
        fft_destroy(p_obj);
        return NULL;
    }
    fft_set_scale_sch(p_obj, 0x2AB);

    return p_obj;

}

void fft_destroy(fft_t* p_fft_inst) {
    dma_accel_free(p_fft_inst->periphs.p_dma_accel_inst);
    free(p_fft_inst);
}

void fft_set_fwd_inv(fft_t* p_fft_inst, fft_fwd_inv_t fwd_inv) {
    p_fft_inst->fwd_inv = fwd_inv;
}

fft_fwd_inv_t fft_get_fwd_inv(fft_t* p_fft_inst) {
    return (p_fft_inst->fwd_inv);
}

int fft_set_num_pts(fft_t* p_fft_inst, int num_pts) {
    if (num_pts > FFT_MAX_NUM_PTS) {
        xil_printf("ERROR! Attempted to set too large number of points in the FFT.\n\r");
        return FFT_ILLEGAL_NUM_PTS;
    } else if (!is_power_of_2(num_pts)) {
        xil_printf("ERROR! Attempted to set a non-power-of-2 value for the number of points in the FFT.\n\r");
        return FFT_ILLEGAL_NUM_PTS;
    } else {
        p_fft_inst->num_pts = num_pts;
        dma_accel_set_buf_length(p_fft_inst->periphs.p_dma_accel_inst, p_fft_inst->num_pts);
        return FFT_SUCCESS;
    }
}

int fft_get_num_pts(fft_t* p_fft_inst) {
    return (p_fft_inst->num_pts);
}

void fft_set_scale_sch(fft_t* p_fft_inst, int scale_sch) {
    p_fft_inst->scale_sch = scale_sch;
}

int fft_get_scale_sch(fft_t* p_fft_inst) {
    return (p_fft_inst->scale_sch);
}

int fft(fft_t* p_fft_inst, complex_sample_t* din, complex_sample_t* dout) {

    // commit struct parameters to hardware
    fft_commit_params(p_fft_inst);

    // setup dma buffers
    dma_accel_set_input_buf(p_fft_inst->periphs.p_dma_accel_inst, (void*)din);
    dma_accel_set_output_buf(p_fft_inst->periphs.p_dma_accel_inst, (void*)dout);

    // dma transfer
    status = dma_accel_transfer(p_fft_inst->periphs.p_dma_accel_inst);
    if (status != DMA_ACCEL_SUCCESS) {
        xil_printf("ERROR! DMA transfer failed.\n\r");
        return FFT_DMA_FAIL;
    }

    return FFT_SUCCESS;
}

complex_sample_t* fft_get_input_buf(fft_t* p_fft_inst) {
    return (complex_sample_t*)dma_accel_get_input_buf(p_fft_inst->periphs.p_dma_accel_inst);
}

complex_sample_t* fft_get_output_buf(fft_t* p_fft_inst) {
    return (complex_sample_t*)dma_accel_get_output_buf(p_fft_inst->periphs.p_dma_accel_inst);
}

void fft_print_params(fft_t* p_fft_inst) {
    xil_printf("fwd_inv   = %s\n\r", (p_fft_inst->fwd_inv == FFT_FORWARD ? "forward" : "inverse"));
    xil_printf("num_pts   = %d\n\r", p_fft_inst->num_pts);
    xil_printf("scale_sch = 0x%X\n\r", p_fft_inst->scale_sch);
}

void fft_print_input_buf(fft_t* p_fft_inst) {

    char         str[32]; // 2 ints and some spare
    complex_sample_t* tmp = (complex_sample_t*)dma_accel_get_input_buf(p_fft_inst->periphs.p_dma_accel_inst);

    for (int i = 0; i < p_fft_inst->num_pts; i++)
    {
        complex_sample_get_string(str, tmp[i]);
        xil_printf("xn(%d) = %s\n\r", i, str);
    }

}

void fft_print_output_buf(fft_t* p_fft_inst) {

    char         str[32]; // 2 ints and some spare
    complex_sample_t* tmp = (complex_sample_t*)dma_accel_get_output_buf(p_fft_inst->periphs.p_dma_accel_inst);


    for (i = 0; i < p_fft_inst->num_pts; i++)
    {
        complex_sample_get_string(str, tmp[i]);
        xil_printf("Xk(%d) = %s\n\r", i, str);
    }

}

