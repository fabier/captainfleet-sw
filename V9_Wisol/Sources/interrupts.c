#include "globals.h"
#include <xc.h>
#include "interrupts.h"
#include "serial.h"
#include "init.h"

static void interrupt isr(void){

    while(RCIF){// 2 chars possible in FIFO
        while(BAUDCONbits.WUE==1);
        if(RCSTAbits.FERR!=1){//No framing error
            UART_buffer_RX[UART_RX_WR_ptr]=RCREG;
            UART_RX_WR_ptr++;
            if (UART_RX_WR_ptr == UART_BUFFER_RX_SIZE) UART_RX_WR_ptr = 0; //end of buffer
        }
        else (void)RCREG;
    }
}