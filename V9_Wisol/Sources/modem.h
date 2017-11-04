#ifndef MODEM_H
#define	MODEM_H
#include "hardware.h"

#ifndef TD120X
#define END_OF_TRANSMISSION_MODEM 0x0DA //LF
#else
#define END_OF_TRANSMISSION_MODEM 0x0D //CR
#endif


#define ENABLE_UART_MODEM \
    TX_MICRO_TO_MODEM_PIN_DIRECTION=OUTPUT; \
    APFCON0bits.TXCKSEL=0; /*RC4*/\
    APFCON0bits.RXDTSEL=0; /*RC5*/\
    TXSTAbits.TXEN=0b1; \
    RCSTAbits.CREN=0b1

#define DISABLE_UART_MODEM \
    TXSTAbits.TXEN=0b0; \
    RCSTAbits.CREN=0b0;  /*Rx Disable*/ \
    TX_MICRO_TO_MODEM_PIN_DIRECTION=INPUT; \
    UART_RX_Buffers_Init()//RAZ des buffers


void init_compteur_trames(void);
void configure_modem(void);
void UART_Transmit_one_char_to_modem(unsigned char tx);
void hex_to_ascii(unsigned char hex);
void Envoi_message(unsigned long Longitude,unsigned long Latitude,
        unsigned char dop,unsigned short Vsolar, unsigned short Vbackup,
        unsigned char day_status,unsigned char timeout,unsigned char vitesse, unsigned char cap);
void Envoi_message_service(unsigned short *nb_slot_protec,signed char actual_temp,signed char *temp_min,signed char *temp_max,signed char temp_moy,unsigned char sw_version,float* VBackup_min, float* VBackup_max);
void Envoi_message_Erreur(unsigned short Vsolar, unsigned short Vbackup, unsigned char day_status,unsigned char raison);


unsigned char UART_MODEM_Frame_Received(void);
unsigned char decode_trame_modem(void);
void Envoyer_SIGFOX(const char * s);
void Demander_ID();
void Demander_PAC();
void Sleep_Modem();
void Wake_UP_Modem();
signed char Recuperer_Temp();
void Activer_Mode_CW();
#endif	/* MODEM_H */

