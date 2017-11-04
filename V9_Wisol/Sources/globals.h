#ifndef GLOBALS
#define GLOBALS

#define S_WAIT_FOR_ENERGY 0x11
#define S_JOUR 0x22
#define S_NUIT 0x33
#define S_WAIT_CONF_NUIT 0x44
#define S_WAIT_CONF_JOUR 0x55

#define SS_ACQ_GPS 0x66

#define SS_SERVICE 0x77


#define WD_PRESCALER_16S 0b01110; // 16s
#define WD_PRESCALER_32S 0b01111; // 32s
#define WD_PRESCALER_256S 0b10010;//256s

#define UART_BUFFER_RX_SIZE 100  //Max trame GGA: 74
//UART_TX_WR_ptr=(UART_TX_WR_ptr+1) & 0x07;
#define TX_NULL '0'
#define END_CHAR '\n' // For GPS and for Modem


//Do not change _NO and _YES!!
#define _NO   0b0
#define _YES  0b1

#define RESOLUTION_VSOLAR_MV 100.0 //Max=31*100=3100mV
#define RESOLUTION_VBACKUP_MV 90.0 //Max=31*90=2790mV

void Delay_100ms(unsigned short duree);

#endif//GLOBALS