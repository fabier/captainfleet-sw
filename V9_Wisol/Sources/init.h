#ifndef INIT
#define INIT

#include "globals.h"


//#define HW_STANDARD
#define HW_V5_TD_LOW_CONSUMPTION


#define _XTAL_FREQ 16000000

#define ULTRA_LOW_SPEED_CLK 	0x00   	//31khz LF
#define LOW_SPEED_CLK 		0x07   	//500khz MF
#define MEDIUM_SPEED_CLK 	0x0D	//4Mhz HF
#define HIGH_SPEED_CLK 		0x0F	//16Mhz HF

//Macro which set the clock to 16Mhz
#define CLK_SET_16M_SPEED  \
    OSCCONbits.SCS=3; \
    OSCCONbits.SPLLEN=0; \
    OSCCONbits.IRCF=HIGH_SPEED_CLK; \
    while(OSCSTATbits.HFIOFR==0)

//Macro which set the clock to 16Mhz
#define CLK_SET_31K_SPEED  \
    OSCCONbits.SCS=3; \
    OSCCONbits.SPLLEN=0; \
    OSCCONbits.IRCF=ULTRA_LOW_SPEED_CLK; \
    while(OSCSTATbits.LFIOFR==0)

// Macro which make All hw initializations.
#define HW_INIT  \
    EN_MEAS_VBACKUP_PIN_DIRECTION = INPUT;/*HZ*/ \
    CMD_GPS_ON_PIN_DIRECTION = INPUT; /*HZ*/ \
    T3V_OFF; \
    EN_3V_PIN_DIRECTION=OUTPUT;  \
    RX_MICRO_TO_MODEM=NICO_ON; /* default value */ \
    RX_MICRO_TO_MODEM_PIN_DIRECTION=INPUT; \
    TX_MICRO_TO_MODEM=NICO_ON; \
    TX_MICRO_TO_MODEM_PIN_DIRECTION=INPUT; /* ZERO LEAKAGE WHILE MODEM POWEED OFF*/ \
    VBACKUP=NICO_OFF; /*Not powered*/ \
    VBACKUP_PIN_DIRECTION=OUTPUT; \
    EN_MEAS_VBACKUP=NICO_OFF; \
    CMD_GPS_ON=NICO_OFF; \
    CMD_PROTEC=NICO_OFF; /*always at 0*/ \
    CMD_PROTEC_PIN_DIRECTION = INPUT; /*disabled*/\
    RX_PIN_DIRECTION = INPUT; \
    TX_PIN=NICO_ON; \
    TX_PIN_DIRECTION = INPUT; /* ZERO LEAKAGE WHILE GPS POWEED OFF*/\
    MEASURE_SOLAR_PIN_DIRECTION = INPUT; \
    MEASURE_BACKUP_PIN_DIRECTION = INPUT; \
    ANSELA=0x00; \
    ANSELC=0b00001100
//    APFCON0bits.RXDTSEL=0b1; \
//    APFCON0bits.TXCKSEL=0b1;


//Macro which make the Sw init
#define SW_INIT \
    WDTCONbits.WDTPS= WD_PRESCALER_16S // 16seconds timeout
/*    WDTCONbits.SWDTEN = 0b1;*/

//Macro which make enters in DSM
#define ENTER_SLEEP_MODE \
    WDTCONbits.SWDTEN = 0b0; \
    WDTCONbits.SWDTEN = 0b1; \
    asm("SLEEP"); \
    asm("nop"); \
    WDTCONbits.SWDTEN = 0b0 /*pas de watchdog!!!*/

//void wait_sleep_mode(unsigned char ms);

#endif