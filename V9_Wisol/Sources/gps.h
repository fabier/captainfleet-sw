#ifndef GPS_H
#define GPS_H

#include "hardware.h"
//#include <xc.h>

//Macro which Configure GPS UART
#define GPS_CONFIG chaine_Init();

#define ENABLE_UART_GPS \
    TX_PIN_DIRECTION=OUTPUT; \
    APFCON0bits.TXCKSEL=1; /*RA0*/ \
    APFCON0bits.RXDTSEL=1; /*RA1*/\
    TXSTAbits.TXEN=0b1; \
    RCSTAbits.CREN=0b1

#define DISABLE_UART_GPS \
    TXSTAbits.TXEN=0b0; \
    RCSTAbits.CREN=0b0;  /*Rx Disable*/ \
    TX_PIN_DIRECTION=INPUT; \
    UART_RX_Buffers_Init()//RAZ des buffers

unsigned char decode_trame_GPS(void);
char verifierCRC(unsigned char trameGPS[]);
void envoyerTrameGPS(unsigned char lat[], unsigned char N_S[], unsigned char lon[], unsigned char E_W[], unsigned char dop[], unsigned char nbSat[], unsigned char vitesse[], unsigned short seqNumber);
void configure_GPS(void);
void configure_GPS_VTG(void);
void decodage_Lat_Long(unsigned long * Lat,unsigned long * Long,unsigned char *dopBis);
void decodage_VTG(float * cap, float * vitesse);
void Reinit_trame_GPS(void);

extern unsigned char champsTrameGPSGGA[16][12];
extern unsigned char champsTrameGPSVTG[12][8];

#endif