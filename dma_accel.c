
#include <stdlib.h>
#include "xaxidma.h"
#include "xscugic.h"
#include "dma_accel.h"
#define RESET_TIMEOUT_COUNTER 10000

static int g_s2mm_done = 0;
static int g_mm2s_done = 0;
static int g_dma_err   = 0;

typedef struct dma_accel_periphs {
    XAxiDma dma_inst;
    XScuGic intc_inst;
} dma_accel_periphs_t;

typedef struct dma_accel {
    dma_accel_periphs_t periphs;
    void*               p_input_buf;
    void*               p_output_buf;
    int                 buf_length;
    int                 sample_size_bytes;
} dma_accel_t;

// interrupt service routine for stream to memory-mapped
static void s2mm_isr(void* CallbackRef) {
    XAxiDma* p_dma_inst = (XAxiDma*)CallbackRef;

    // turn off interrupts so we don't get re-interrupted
    XAxiDma_IntrDisable(p_dma_inst, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
    XAxiDma_IntrDisable(p_dma_inst, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

    // read irq status
    int irq_status = XAxiDma_IntrGetIrq(p_dma_inst, XAXIDMA_DEVICE_TO_DMA);

    // acknowledge any pending interrupts
    XAxiDma_IntrAckIrq(p_dma_inst, irq_status, XAXIDMA_DEVICE_TO_DMA);

    // if there are no asserted interrupts, there's nothing to do
    if (!(irq_status & XAXIDMA_IRQ_ALL_MASK)) {
        // re-enable interrupts
        XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DMA_TO_DEVICE);
        XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DEVICE_TO_DMA);
        return;
    }

    // error interrupt
    if ((irq_status & XAXIDMA_IRQ_ERROR_MASK)) {

        // error flag
        g_dma_err = 1;

        // try to reset dma
        XAxiDma_Reset(p_dma_inst);
        for (int i = 0; i < RESET_TIMEOUT_COUNTER; i++) {
            if (XAxiDma_ResetIsDone(p_dma_inst)) {
                break;
            }
        }

        // re-enable interrupts
        XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DMA_TO_DEVICE);
        XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DEVICE_TO_DMA);

        return;
    }

    // completed interrupt
    if (irq_status & XAXIDMA_IRQ_IOC_MASK) {
        // flag that s2mm is completed
        g_s2mm_done = 1;
    }
    
    // re-enable interrupts
    XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DMA_TO_DEVICE);
    XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DEVICE_TO_DMA);

}

// interrupt service routine for memory-mapped to stream
static void mm2s_isr(void* CallbackRef) {
    XAxiDma* p_dma_inst = (XAxiDma*)CallbackRef;

  // turn off interrupts so we don't get re-interrupted
    XAxiDma_IntrDisable(p_dma_inst, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DMA_TO_DEVICE);
    XAxiDma_IntrDisable(p_dma_inst, XAXIDMA_IRQ_ALL_MASK, XAXIDMA_DEVICE_TO_DMA);

    // read irq status
    int irq_status = XAxiDma_IntrGetIrq(p_dma_inst, XAXIDMA_DMA_TO_DEVICE);

    // acknowledge any pending interrupts
    XAxiDma_IntrAckIrq(p_dma_inst, irq_status, XAXIDMA_DMA_TO_DEVICE);

    // if there are no asserted interrupts, there's nothing to do
    if (!(irq_status & XAXIDMA_IRQ_ALL_MASK)) {
        // re-enable interrupts
        XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DMA_TO_DEVICE);
        XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DEVICE_TO_DMA);
        return;
    }

    // error interrupt
    if (irq_status & XAXIDMA_IRQ_ERROR_MASK) {

        // error flag
        g_dma_err = 1;

        // try to reset dma
        XAxiDma_Reset(p_dma_inst);
        for (int i = 0; i < RESET_TIMEOUT_COUNTER; i++) {
            if (XAxiDma_ResetIsDone(p_dma_inst)) {
                break;
            }
        }

        // re-enable interrupts
        XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DMA_TO_DEVICE);
        XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DEVICE_TO_DMA);

        return;
    }

    // completed interrupt
    if (irq_status & XAXIDMA_IRQ_IOC_MASK) {
        // flag that mm2s is done
        g_mm2s_done = 1;
    }

    // re-enable interrupts
    XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DMA_TO_DEVICE);
    XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DEVICE_TO_DMA);

}

static int init_intc(XScuGic* p_intc_inst, int intc_device_id, XAxiDma* p_dma_inst, int s2mm_intr_id, int mm2s_intr_id) {

    // lookup hardware configuration 
    XScuGic_Config* cfg_ptr = XScuGic_LookupConfig(intc_device_id);
    if (!cfg_ptr) {
        xil_printf("ERROR! No hardware configuration found for Interrupt Controller with device id %d.\r\n", intc_device_id);
        return DMA_ACCEL_INTC_INIT_FAIL;
    }

    // init driver
    int status = XScuGic_CfgInitialize(p_intc_inst, cfg_ptr, cfg_ptr->CpuBaseAddress);
    if (status != XST_SUCCESS)
    {
        xil_printf("ERROR! Initialization of Interrupt Controller failed with %d.\r\n", status);
        return DMA_ACCEL_INTC_INIT_FAIL;
    }

    // set interrupt priorities and trigger type
    XScuGic_SetPriorityTriggerType(p_intc_inst, s2mm_intr_id, 0xA0, 0x3);
    XScuGic_SetPriorityTriggerType(p_intc_inst, mm2s_intr_id, 0xA8, 0x3);

    // setup interrupt handlers
    status = XScuGic_Connect(p_intc_inst, s2mm_intr_id, (Xil_InterruptHandler)s2mm_isr, p_dma_inst);
    if (status != XST_SUCCESS)
    {
        xil_printf("ERROR! Failed to connect s2mm_isr to the interrupt controller.\r\n", status);
        return DMA_ACCEL_INTC_INIT_FAIL;
    }
    status = XScuGic_Connect(p_intc_inst, mm2s_intr_id, (Xil_InterruptHandler)mm2s_isr, p_dma_inst);
    if (status != XST_SUCCESS)
    {
        xil_printf("ERROR! Failed to connect mm2s_isr to the interrupt controller.\r\n", status);
        return DMA_ACCEL_INTC_INIT_FAIL;
    }

    // enable interrupts
    XScuGic_Enable(p_intc_inst, s2mm_intr_id);
    XScuGic_Enable(p_intc_inst, mm2s_intr_id);

    // initialize exception table and register handler
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, p_intc_inst);

    // enable noncritical exceptions
    Xil_ExceptionEnable();

    return DMA_ACCEL_SUCCESS;

}

static int init_dma(XAxiDma* p_dma_inst, int dma_device_id) {


    // lookup hardware configuration
    XAxiDma_Config* cfg_ptr = XAxiDma_LookupConfig(dma_device_id);
    if (!cfg_ptr) {
        xil_printf("ERROR! No hardware configuration found for AXI DMA with device id %d.\r\n", dma_device_id);
        return DMA_ACCEL_DMA_INIT_FAIL;
    }

    // init driver
    int status = XAxiDma_CfgInitialize(p_dma_inst, cfg_ptr);
    if (status != XST_SUCCESS)
    {
        xil_printf("ERROR! Initialization of AXI DMA failed with %d\r\n", status);
        return DMA_ACCEL_DMA_INIT_FAIL;
    }

    // confirm dma has scatter/gather
    if (XAxiDma_HasSg(p_dma_inst))
    {
        xil_printf("ERROR! Device configured as SG mode.\r\n");
        return DMA_ACCEL_DMA_INIT_FAIL;
    }

    // reset
    XAxiDma_Reset(p_dma_inst);
    while (!XAxiDma_ResetIsDone(p_dma_inst)) {
        // empty
    }

    // enable dma interrupts
    XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DMA_TO_DEVICE);
    XAxiDma_IntrEnable(p_dma_inst, (XAXIDMA_IRQ_IOC_MASK | XAXIDMA_IRQ_ERROR_MASK), XAXIDMA_DEVICE_TO_DMA);

    return DMA_ACCEL_SUCCESS;

}

// Public functions
dma_accel_t* dma_accel_create(int dma_device_id, int intc_device_id, int s2mm_intr_id, int mm2s_intr_id, int sample_size_bytes) {

    // allocate memory for dma accelerator object
    dma_accel_t* p_obj = (dma_accel_t*) malloc(sizeof(dma_accel_t));
    if (p_obj == NULL) {
        xil_printf("ERROR! Failed to allocate memory for DMA Accelerator object.\n\r");
        return NULL;
    }

    // register and initialize peripherals
    int status = init_dma(&p_obj->periphs.dma_inst, dma_device_id);
    if (status != DMA_ACCEL_SUCCESS) {
        xil_printf("ERROR! Failed to initialize AXI DMA.\n\r");
        dma_accel_free(p_obj);
        return NULL;
    }

    status = init_intc(&p_obj->periphs.intc_inst, intc_device_id, &p_obj->periphs.dma_inst, s2mm_intr_id, mm2s_intr_id);
    
    if (status != DMA_ACCEL_SUCCESS) {
        xil_printf("ERROR! Failed to initialize Interrupt controller.\n\r");
        dma_accel_free(p_obj);
        return NULL;
    }

    // init buffer pointers
    dma_accel_set_input_buf(p_obj, NULL);
    dma_accel_set_output_buf(p_obj, NULL);

    // init buffer length
    dma_accel_set_buf_length(p_obj, 1024);

    // init sample size
    dma_accel_set_sample_size_bytes(p_obj, sample_size_bytes);

    return p_obj;

}

void dma_accel_free(dma_accel_t* p_dma_accel_inst) {
    free(p_dma_accel_inst);
}

void dma_accel_set_input_buf(dma_accel_t* p_dma_accel_inst, void* p_input_buf) {
    p_dma_accel_inst->p_input_buf = p_input_buf;
}

void* dma_accel_get_input_buf(dma_accel_t* p_dma_accel_inst) {
    return (p_dma_accel_inst->p_input_buf);
}

void dma_accel_set_output_buf(dma_accel_t* p_dma_accel_inst, void* p_output_buf) {
    p_dma_accel_inst->p_output_buf = p_output_buf;
}

void* dma_accel_get_output_buf(dma_accel_t* p_dma_accel_inst) {
    return (p_dma_accel_inst->p_output_buf);
}

void dma_accel_set_buf_length(dma_accel_t* p_dma_accel_inst, int buf_length) {
    p_dma_accel_inst->buf_length = buf_length;
}

int dma_accel_get_buf_length(dma_accel_t* p_dma_accel_inst) {
    return (p_dma_accel_inst->buf_length);
}

void dma_accel_set_sample_size_bytes(dma_accel_t* p_dma_accel_inst, int sample_size_bytes) {
    p_dma_accel_inst->sample_size_bytes = sample_size_bytes;
}

int dma_accel_get_sample_size_bytes(dma_accel_t* p_dma_accel_inst) {
    return (p_dma_accel_inst->sample_size_bytes);
}

int dma_accel_transfer(dma_accel_t* p_dma_accel_inst) {

    const int num_bytes = p_dma_accel_inst->buf_length*p_dma_accel_inst->sample_size_bytes;

    // flush cache
    Xil_DCacheFlushRange((int)p_dma_accel_inst->p_input_buf, num_bytes);
    Xil_DCacheFlushRange((int)p_dma_accel_inst->p_output_buf, num_bytes);

    // initialize control flags which get set by ISRs. Should be a better way to do this...
    g_s2mm_done = 0;
    g_mm2s_done = 0;
    g_dma_err   = 0;

    // MM2S transfer
    int status = XAxiDma_SimpleTransfer(&p_dma_accel_inst->periphs.dma_inst, (int)p_dma_accel_inst->p_input_buf, num_bytes, XAXIDMA_DMA_TO_DEVICE);
    
    if (status != DMA_ACCEL_SUCCESS) {
        xil_printf("ERROR! Failed to kick off MM2S transfer!\n\r");
        return DMA_ACCEL_TRANSFER_FAIL;
    }

    // S2MM transfer
    status = XAxiDma_SimpleTransfer( &p_dma_accel_inst->periphs.dma_inst, (int)p_dma_accel_inst->p_output_buf, num_bytes, XAXIDMA_DEVICE_TO_DMA);

    if (status != DMA_ACCEL_SUCCESS) {
        xil_printf("ERROR! Failed to kick off MM2S transfer!\n\r");
        return DMA_ACCEL_TRANSFER_FAIL;
    }

    // wait for transfer to complete
    while (!(g_s2mm_done && g_mm2s_done) && !g_dma_err) {
        // dumb busy waiting. With better code you could do smth useful here
    }

    // verify no dma error
    if (g_dma_err) {
        xil_printf("ERROR! AXI DMA returned an error during the MM2S transfer.\n\r");
        return DMA_ACCEL_TRANSFER_FAIL;
    }

    return DMA_ACCEL_SUCCESS;

}

