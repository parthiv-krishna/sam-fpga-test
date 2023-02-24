#ifndef DMA_ACCEL_H
#define DMA_ACCEL_H

// return flags
#define DMA_ACCEL_SUCCESS           0
#define DMA_ACCEL_DMA_INIT_FAIL    -1
#define DMA_ACCEL_INTC_INIT_FAIL   -2
#define DMA_ACCEL_TRANSFER_FAIL    -3

typedef struct dma_accel dma_accel_t;

dma_accel_t* dma_accel_create(int dma_device_id, int intc_device_id, int s2mm_intr_id,
                              int mm2s_intr_id, int sample_size_bytes);

void dma_accel_free(dma_accel_t* p_dma_accel_inst);

void dma_accel_set_input_buf(dma_accel_t* p_dma_accel_inst, void* p_input_buf);

void* dma_accel_get_input_buf(dma_accel_t* p_dma_accel_inst);

void dma_accel_set_output_buf(dma_accel_t* p_dma_accel_inst, void* p_output_buf);

void* dma_accel_get_output_buf(dma_accel_t* p_dma_accel_inst);

void dma_accel_set_buf_length(dma_accel_t* p_dma_accel_inst, int buf_length);

int dma_accel_get_buf_length(dma_accel_t* p_dma_accel_inst);

void dma_accel_set_sample_size_bytes(dma_accel_t* p_dma_accel_inst, int sample_size_bytes);

int dma_accel_get_sample_size_bytes(dma_accel_t* p_dma_accel_inst);

int dma_accel_transfer(dma_accel_t* p_dma_accel_inst);

#endif // DMA_ACCEL_H

