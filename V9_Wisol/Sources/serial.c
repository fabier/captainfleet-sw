#include "globals.h"
#include "serial.h"
#include <xc.h>
#include "hardware.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

unsigned char volatile UART_buffer_RX[UART_BUFFER_RX_SIZE];
unsigned char volatile UART_RX_WR_ptr;
unsigned char UART_RX_RD_ptr;

char chaine[UART_BUFFER_RX_SIZE];

void chaine_Init(void){
    unsigned char i;
    for(i=0;i<UART_BUFFER_RX_SIZE;i++)chaine[i]='\0';
}

/*! \fn  void UART_RX_Buffers_Init(void)
 *  \brief Initialize pointers for rx buffer management.
 *  \param .
 *  
 *  \return .
 */
void UART_RX_Buffers_Init(void){
    unsigned char i;
    UART_RX_WR_ptr=0;
    UART_RX_RD_ptr=0;
    for(i=0;i<UART_BUFFER_RX_SIZE;i++)UART_buffer_RX[i]=TX_NULL;
}

void UART_Transmit_one_char(unsigned char tx)
{
    while( TXSTAbits.TRMT == 0); //Send if the buffer is free
    TXREG = tx;  // send data
}

void UART_Transmit_string(const unsigned char tx[])
{
    unsigned char a;
    a=0;
    while(tx[a] != '\0'){
        while( TXSTAbits.TRMT == 0); //Send if the buffer is free
        TXREG = tx[a++];  // send data
    }
}

char UART_Get_Next_Char(void){
	char temp;
	temp=UART_buffer_RX[UART_RX_RD_ptr];
	UART_buffer_RX[UART_RX_RD_ptr]=TX_NULL;
	UART_RX_RD_ptr++;
	if (UART_RX_RD_ptr == UART_BUFFER_RX_SIZE)
        UART_RX_RD_ptr = 0; //Fin du buffer
	return temp;
}

unsigned char UART_Frame_Received(void) // detection function of received frame ended by END_CHAR
{
    unsigned char flag;
    unsigned char rd_ptr_temp;
    unsigned char c;
    char temp;
    rd_ptr_temp=UART_RX_RD_ptr;
    flag=_NO;
    c=1;

    while(rd_ptr_temp != UART_RX_WR_ptr)
    {
        if (UART_buffer_RX[rd_ptr_temp]==END_CHAR)
        {
            flag=_YES;
            break;
        }
        c++;
        rd_ptr_temp++;
        if (rd_ptr_temp == UART_BUFFER_RX_SIZE)
        {
            rd_ptr_temp = 0; //Fin du buffer
        }
    }
    c=0;
    if(flag==_YES){ //Si trouvé, copie de fIFO
        do{
            temp=UART_Get_Next_Char();
            chaine[c++]=temp;
        }
        while (temp!= END_CHAR);

    }
    return flag;
}
