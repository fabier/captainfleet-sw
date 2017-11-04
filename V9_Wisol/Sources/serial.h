#ifndef SERIAL_H
#define SERIAL_H

//Macro which Configure UART
#define UART_CONFIG \
    SPBRGH=0x01; \
    SPBRGL=0xA0; \
    BAUDCONbits.BRG16=0b1; \
    TXSTAbits.BRGH=0b1; \
    TXSTAbits.SYNC=0b0; \
    RCSTAbits.SPEN=0b1; \
    TXSTAbits.TXEN=0b0; \
    UART_RX_Buffers_Init()
  
//Macro which disable UART_RX side
#define UART_RX_DISABLE  RCSTAbits.CREN=0b0;  // receiver disabled
//MACRO which Enable UART Rx
#define UART_RX_ENABLE RCSTAbits.CREN=0b1;   // receiver enabled

#define UART_WAIT_FOR_END_OF_TRANSMISSION while( TXSTAbits.TRMT == 0); //Send if the buffer is free

void chaine_Init(void);
void UART_RX_Buffers_Init(void);
void UART_Transmit_one_char(unsigned char tx);
void UART_Transmit_string(const unsigned char tx[]);
unsigned char UART_Frame_Received(void); // detection function of received frame ended by END_CHAR

extern unsigned char volatile UART_RX_WR_ptr;
extern unsigned char UART_RX_RD_ptr;

extern char chaine[UART_BUFFER_RX_SIZE];

extern volatile char UART_buffer_RX[UART_BUFFER_RX_SIZE];

#endif