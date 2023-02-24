#include <stdio.h>
#include <stdlib.h>
#include "platform.h"
#include "xuartps_hw.h"
#include "fft.h"
#include "complex_sample.h"
#include "input_samples.h"

// input_samples.h
extern int sig_two_sine_waves[FFT_MAX_NUM_PTS];

void which_fft_param(fft_t* p_fft_inst);

int main() {

    // init platform, uart, etc
    init_platform();
    xil_printf("\fHello World!\n\r");

    // fft object
    fft_t* p_fft_inst = fft_create(
        XPAR_GPIO_0_DEVICE_ID,
        XPAR_AXIDMA_0_DEVICE_ID,
        XPAR_PS7_SCUGIC_0_DEVICE_ID,
        XPAR_FABRIC_CTRL_AXI_DMA_0_S2MM_INTROUT_INTR,
        XPAR_FABRIC_CTRL_AXI_DMA_0_MM2S_INTROUT_INTR
    );

    if (p_fft_inst == NULL) {
        xil_printf("ERROR! Failed to create FFT instance.\n\r");
        return -1;
    }

    // data buffers
    complex_sample_t* input_buf = (complex_sample_t*) malloc(sizeof(complex_sample_t)*FFT_MAX_NUM_PTS);
    if (input_buf == NULL) {
        xil_printf("ERROR! Failed to allocate memory for the inputulus buffer.\n\r");
        return -1;
    }

    complex_sample_t* output_buf = (complex_sample_t*) malloc(sizeof(complex_sample_t)*FFT_MAX_NUM_PTS);
    if (output_buf == NULL) {
        xil_printf("ERROR! Failed to allocate memory for the output buffer.\n\r");
        return -1;
    }

    // fill input buffer with some signal
    memcpy(input_buf, sig_two_sine_waves, sizeof(complex_sample_t)*FFT_MAX_NUM_PTS);

    while (1) {
        // terrible code below. beware.
        xil_printf("What would you like to do?\n\r");
        xil_printf("0: Print current FFT parameters\n\r");
        xil_printf("1: Change FFT parameters\n\r");
        xil_printf("2: Perform FFT using current parameters\n\r");
        xil_printf("3: Print current inputulus to be used for the FFT operation\n\r");
        xil_printf("4: Print outputs of previous FFT operation\n\r");
        xil_printf("5: Quit\n\r");
        char c = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);

        if (c == '0') {
            xil_printf("---------------------------------------------\n\r");
            fft_print_params(p_fft_inst);
            xil_printf("---------------------------------------------\n\r");
        } else if (c == '1') {
            which_fft_param(p_fft_inst);
        } else if (c == '2') {
            // clear buffer before it's populatad
            memset(output_buf, 0, sizeof(complex_sample_t)*FFT_MAX_NUM_PTS);

            int status = fft(p_fft_inst, (complex_sample_t*)input_buf, (complex_sample_t*)output_buf);
            if (status != FFT_SUCCESS)
            {
                xil_printf("ERROR! FFT failed.\n\r");
                return -1;
            }

            xil_printf("FFT complete!\n\r");
        } else if (c == '3') {
            fft_print_input_buf(p_fft_inst);
        } else if (c == '4') {
            fft_print_output_buf(p_fft_inst);
        } else if (c == '5') {
            xil_printf("Okay, exiting...\n\r");
            break;
        } else {
            xil_printf("Invalid character. Please try again.\n\r");
        }

    }

    free(input_buf);
    free(output_buf);
    fft_destroy(p_fft_inst);

    return 0;

}

void which_fft_param(fft_t* p_fft_inst) {
    xil_printf("Okay, which parameter would you like to change?\n\r");
    xil_printf("0: Point length\n\r");
    xil_printf("1: Direction\n\r");
    xil_printf("2: Exit\n\r");
    while (1) {
        char c = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);

        if (c == '0') {
            xil_printf("What would you like to set the FFT point length to? Type:\n\r");
            xil_printf("0: FFT point length = 16\n\r");
            xil_printf("1: FFT point length = 32\n\r");
            xil_printf("2: FFT point length = 64\n\r");
            xil_printf("3: FFT point length = 128\n\r");
            xil_printf("4: FFT point length = 256\n\r");
            xil_printf("5: FFT point length = 512\n\r");
            xil_printf("6: FFT point length = 1024\n\r");
            xil_printf("7: FFT point length = 2048\n\r");
            xil_printf("8: FFT point length = 4096\n\r");
            xil_printf("9: FFT point length = 8192\n\r");

            c = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
            // I should have just done c - '0' and then exponentiated that. Oh well.
            while (1) {
                if (c == '0') {
                    xil_printf("Okay, setting the core to perform a 16-point FFT.\n\r");
                    fft_set_num_pts(p_fft_inst, 16);
                    break;
                } else if (c == '1') {
                    xil_printf("Okay, setting the core to perform a 32-point FFT.\n\r");
                    fft_set_num_pts(p_fft_inst, 32);
                    break;
                } else if (c == '2') {
                    xil_printf("Okay, setting the core to perform a 64-point FFT.\n\r");
                    fft_set_num_pts(p_fft_inst, 64);
                    break;
                } else if (c == '3') {
                    xil_printf("Okay, setting the core to perform a 128-point FFT.\n\r");
                    fft_set_num_pts(p_fft_inst, 128);
                    break;
                } else if (c == '4') {
                    xil_printf("Okay, setting the core to perform a 256-point FFT.\n\r");
                    fft_set_num_pts(p_fft_inst, 256);
                    break;
                } else if (c == '5') {
                    xil_printf("Okay, setting the core to perform a 512-point FFT.\n\r");
                    fft_set_num_pts(p_fft_inst, 512);
                    break;
                } else if (c == '6') {
                    xil_printf("Okay, setting the core to perform a 1024-point FFT.\n\r");
                    fft_set_num_pts(p_fft_inst, 1024);
                    break;
                } else if (c == '7') {
                    xil_printf("Okay, setting the core to perform a 2048-point FFT.\n\r");
                    fft_set_num_pts(p_fft_inst, 2048);
                    break;
                } else if (c == '8') {
                    xil_printf("Okay, setting the core to perform a 4096-point FFT.\n\r");
                    fft_set_num_pts(p_fft_inst, 4096);
                    break;
                } else if (c == '9') {
                    xil_printf("Okay, setting the core to perform a 8192-point FFT.\n\r");
                    fft_set_num_pts(p_fft_inst, 8192);
                    break;
                } else {
                    xil_printf("Invalid character. Please try again.\n\r");
                }
            }
            break;
        } else if (c == '1') {
            
            xil_printf("What would you like to set the FFT direction to? Type:\n\r");
            xil_printf("0: Inverse\n\r");
            xil_printf("1: Forward\n\r");

            c = XUartPs_RecvByte(XPAR_PS7_UART_1_BASEADDR);
            while (1) {
                if (c == '0') {
                    xil_printf("Okay, setting the core to perform an inverse FFT.\n\r");
                    fft_set_fwd_inv(p_fft_inst, FFT_INVERSE);
                    break;
                } else if (c == '1') {
                    xil_printf("Okay, setting the core to perform a forward FFT.\n\r");
                    fft_set_fwd_inv(p_fft_inst, FFT_FORWARD);
                    break;
                } else
                    xil_printf("Invalid character. Please try again.\n\r");
            }
            break;
        } else if (c == '2') {
            return;
        } else {
            xil_printf("Invalid character. Please try again.\n\r");
        }
    }
}

