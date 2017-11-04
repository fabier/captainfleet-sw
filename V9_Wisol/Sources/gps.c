#include "globals.h"
#include "serial.h"
#include "gps.h"
#include <xc.h>
#include "hardware.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define GPGGA_COL  12 //MAX 255
#define GPGGA_LINES 16 //MAX 255

#define GPVTG_COL 8 //MAX 255
#define GPVTG_LINES 12 //MAX 255


const char GGL[]="$PUBX,40,GLL,0,0,0,0*5C\r\n";
const char GSA[]="$PUBX,40,GSA,0,0,0,0*4E\r\n";
const char GSV[]="$PUBX,40,GSV,0,0,0,0*59\r\n";
const char RMC[]="$PUBX,40,RMC,0,0,0,0*47\r\n";
const char GGA_OFF[]="$PUBX,40,GGA,0,0,0,0*5A\r\n";
const char GGA_ON[]="$PUBX,40,GGA,0,1,0,0*5B\r\n";
const char VTG_OFF[]="$PUBX,40,VTG,0,0,0,0*5E\r\n"; //Stop
const char VTG_ON[]="$PUBX,40,VTG,0,1,0,0*5F\r\n"; //Start Every Seconds on USART1

const char AOP_ON[]={0xB5,0x62,0x06,0x23,0x28,0x00
,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00
,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
,0x92,0x06}; //Assist Autonomous ON

extern char chaine[UART_BUFFER_RX_SIZE];
unsigned char champsTrameGPSGGA[GPGGA_LINES][GPGGA_COL] = {0};
unsigned char champsTrameGPSVTG[GPVTG_LINES][GPVTG_COL] = {0};

void Reinit_trame_GPS(void){
    unsigned char i,j;
    i=0;
    j=0;
    while(i<GPGGA_LINES){
        while(j<GPGGA_COL){
            champsTrameGPSGGA[i][j]= 0; //Mettre \0 dans les cases
            j++;
        }
        i++;
    }
    i=0;
    j=0;
    while(i<GPGGA_LINES){
        while(j<GPGGA_COL){
            champsTrameGPSGGA[i][j]= 0; //Mettre \0 dans les cases
            j++;
        }
        i++;
    }
    chaine_Init();
//    champsTrameGPSGGA[6][0]='0'; //Indique qu'il n'y a pas de GPS Fix...
//    champsTrameGPSVTG[1][0]='\0'; //Cap indisponible
//    champsTrameGPSVTG[7][0]='\0';// Vitesse indisponible
}

//Return 1 si Trame GGA
//Return 2 si trame VTG
//Return 0 si pas de trame
unsigned char decode_trame_GPS(void)
{
    unsigned char flagGPS;
    unsigned char caract = 0;
    unsigned char i, j, k;
    if(strncmp(chaine, "$GPGGA", 6) == 0){
        if(verifierCRC(chaine)) {
            //$GPGGA,053740.000,2503.6319,N,12136.0099,E,1,08,1.1,63.8,M,15.2,M,,0000*64
            for(i = 0, j = 0, k = 0; caract != '\n'; i++)
            {
                caract = chaine[i];
                if(caract == ',')
                {
                    champsTrameGPSGGA[j][k] = '\0';
                    j++;
                    k = 0;
                }
                else
                {
                    champsTrameGPSGGA[j][k] = caract;
                    k++;
                }
            }
            if(champsTrameGPSGGA[6][0] == '1' || champsTrameGPSGGA[6][0] == '2') {
                flagGPS = 1;

            }
            else flagGPS = 0;
        }
    }
    else if(strncmp(chaine, "$GPVTG", 6) == 0){
        if(verifierCRC(chaine)) {
            //$GPVTG,,T,,M,0.244,N,0.452,K,A*22
            for(i = 0, j = 0, k = 0; caract != '\n'; i++)
            {
                caract = chaine[i];
                if(caract == ',')
                {
                    champsTrameGPSVTG[j][k] = '\0';
                    j++;
                    k = 0;
                }
                else
                {
                    champsTrameGPSVTG[j][k] = caract;
                    k++;
                }
            }
            flagGPS = 2;
        }
    }
    chaine_Init();

    return flagGPS;
}

char verifierCRC(unsigned char trameGPS[])
{
    if(trameGPS[0] == '$' && trameGPS[1] != '\0')
    {
        unsigned char i;
        unsigned char result;

        for(i = 2, result = trameGPS[1]; trameGPS[i] != '*' && trameGPS[i] != '\0'; i++)  result ^= trameGPS[i];
        unsigned char crc[3];
        i++;
        if(trameGPS[i] != '\r' && trameGPS[i] != '\0') crc[0] = trameGPS[i];
        i++;
        if(trameGPS[i] != '\r' && trameGPS[i] != '\0')crc[1] = trameGPS[i];
        crc[2] = '\0';

        if(result == (unsigned char)xtoi(crc)) return 1;
    }
    return 0;
}

void configure_GPS(void) {
    Delay_100ms(5);
    UART_Transmit_string(GGL);
    UART_Transmit_string(GSA);
    UART_Transmit_string(GSV);
    UART_Transmit_string(RMC);
    UART_Transmit_string(GGA_ON); 
    UART_Transmit_string(VTG_OFF);
    UART_WAIT_FOR_END_OF_TRANSMISSION;
}

void configure_GPS_VTG(void) {
    //Delay_100ms(5);
    UART_Transmit_string(GGA_OFF);
    UART_WAIT_FOR_END_OF_TRANSMISSION;
    Delay_100ms(2);
    UART_RX_Buffers_Init();
    UART_Transmit_string(VTG_ON);
    UART_WAIT_FOR_END_OF_TRANSMISSION;
}

void decodage_VTG(float * cap, float * vitesse){
    if(champsTrameGPSVTG[1][0] == '\0') *cap=0.0; //pas d'info
    else *cap=atof(champsTrameGPSVTG[1]);
    if(champsTrameGPSVTG[7][0] == '\0') *vitesse=0.0; //pas d'info
    else *vitesse=atof(champsTrameGPSVTG[7]);
}






void decodage_Lat_Long(unsigned long * Lat,unsigned long * Long,unsigned char *dopBis){
    unsigned char Char_temp;
    unsigned long Long_temp;
    float dop_f;

    //Décodage Latitude: structure: 4 avant le . et 4 après: codé sur long
    //Latitude: conversion en entier sur 4 octets des caractères ascii reçus + Signe positif si Nord, Négatif si Sud

    Char_temp=(unsigned char)champsTrameGPSGGA[2][0]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)10000000;
    *Lat= Long_temp;

    Char_temp=(unsigned char)champsTrameGPSGGA[2][1]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)1000000;
    *Lat= *Lat + Long_temp;

    Char_temp=(unsigned char)champsTrameGPSGGA[2][2]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)100000;
    *Lat= *Lat + Long_temp;

    Char_temp=(unsigned char)champsTrameGPSGGA[2][3]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)10000;
    *Lat= *Lat + Long_temp;
    //4 est le point
    Char_temp=(unsigned char)champsTrameGPSGGA[2][5]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)1000;
    *Lat= *Lat + Long_temp;

    Char_temp=(unsigned char)champsTrameGPSGGA[2][6]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)100;
    *Lat= *Lat + Long_temp;

    Char_temp=(unsigned char)champsTrameGPSGGA[2][7]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)10;
    *Lat= *Lat + Long_temp;

    Char_temp=(unsigned char)champsTrameGPSGGA[2][8]-0x30;
    Long_temp=(unsigned long)Char_temp;
    *Lat= *Lat + Long_temp;

    if(champsTrameGPSGGA[3][0] == 'S') *Lat=*Lat |  0x80000000;

    //Décodage Longitude: structure: 5 avant le . et 4 après: codé sur long
    //Longitude: conversion en entier sur 4 octets des caractères ascii reçus + Signe positif si Est, Négatif si Ouest

    Char_temp=(unsigned char)champsTrameGPSGGA[4][0]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)100000000;
    *Long= Long_temp;

    Char_temp=(unsigned char)champsTrameGPSGGA[4][1]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)10000000;
    *Long= *Long + Long_temp;

    Char_temp=(unsigned char)champsTrameGPSGGA[4][2]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)1000000;
    *Long= *Long + Long_temp;

    Char_temp=(unsigned char)champsTrameGPSGGA[4][3]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)100000;
    *Long= *Long + Long_temp;

    Char_temp=(unsigned char)champsTrameGPSGGA[4][4]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)10000;
    *Long= *Long + Long_temp;
    //5 est le point...
    Char_temp=(unsigned char)champsTrameGPSGGA[4][6]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)1000;
    *Long= *Long + Long_temp;

    Char_temp=(unsigned char)champsTrameGPSGGA[4][7]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)100;
    *Long= *Long + Long_temp;

    Char_temp=(unsigned char)champsTrameGPSGGA[4][8]-0x30;
    Long_temp=(unsigned long)Char_temp*(unsigned long)10;
    *Long= *Long + Long_temp;

    Char_temp=(unsigned char)champsTrameGPSGGA[4][9]-0x30;
    Long_temp=(unsigned long)Char_temp;
    *Long= *Long + Long_temp;

    if(champsTrameGPSGGA[5][0] == 'W') *Long=*Long | 0x80000000;
    
    dop_f=atof(champsTrameGPSGGA[8]);
    if(dop_f < 1.0) *dopBis = 0;
    else if(dop_f < 2.0) *dopBis = 1;
    else if(dop_f < 5.0) *dopBis = 2;
    else *dopBis = 3;

    *dopBis=(*dopBis <<6) & 0xC0; //positionné en 2 bits de poids fort

    Char_temp= atoi(champsTrameGPSGGA[7]);
    if(Char_temp < 4); //3 sat Max
    else if(Char_temp < 7) *dopBis = *dopBis | (0b00010000); //4 à 6 sats
    else if(Char_temp < 9) *dopBis = *dopBis | (0b00100000); //7à8 sats
    else *dopBis = *dopBis | (0b00110000); //>8 sats
    
}