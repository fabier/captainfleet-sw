#include <xc.h>
#include "hardware.h"
#include "init.h"
#include "globals.h"
#include "serial.h"
#include "modem.h"
#include "gps.h"
#include "interrupts.h"
#include "adc.h"
# include "string.h"
#include <stdio.h>
#include <stdlib.h>

// CONFIG1
#pragma config FOSC = INTOSC    // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE = SWDTEN    // Watchdog Timer Enable (WDT controlled by the SWDTEN bit in the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Disabled
#pragma config MCLRE = ON       // MCLR Pin Function Select (MCLR/VPP pin function is MCLR)
#pragma config CP = OFF          // Flash Program Memory Code Protection (Program memory code protection is enabled)
#pragma config CPD = ON         // Data Memory Code Protection (Data memory code protection is enabled)
#pragma config BOREN = OFF      // Brown-out Reset Enable (Brown-out Reset disabled)
#pragma config CLKOUTEN = OFF   // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO = ON        // Internal/External Switchover (Internal/External Switchover mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT = OFF        // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN = OFF      // PLL Enable (4x PLL disabled)
#pragma config STVREN = ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV = LO        // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP = ON         // Low-Voltage Programming Enable (Low-voltage programming enabled)

#define SW_VERSION 9 //(0x0 à 0xF MAX)
//Attention version de HW!!

//Seuils hauts
#define SEUIL_PROTECTION_SUPERCAP 2600 //2,6V
#define SEUIL_SUPERCAP_SECURE 2500 //2,5V
#define SEUIL_SUPERCAP_TRAMES_SUPP 2550 //2,55V

//Seuils bas
#define SEUIL_CRITIQUE_SUPERCAP 900 //0,9V
#define SEUIL_GPS_AUTORISE 1500 //1,5V

//Seuil Demarrage
#define SEUIL_DEMARRAGE_SUPERCAP 1150 //1,15V

//Seuil Detection
#define SEUIL_DETECTION_JOUR 1000  //1V


#define PERIODE_EMISSIONS_REGULIERES_JOUR 213 // environs 60minutes
#define PERIODE_EMISSIONS_SUPP_JOUR 71 // environs 20minutes : doit toujours être un multiple de PERIODE_EMISSIONS_REGULIERES_JOUR

#define PERIODE_EMISSIONS_REGULIERES_NUIT 1350 //4 heurees : environs 900fois 16secondes

#define PERIODE_EMISSIONS_REGULIERES_SERVICE 5100 //Environs 24H

#if  PERIODE_EMISSIONS_REGULIERES_SERVICE <= 10
    #define PERIODE_REVEIL_MESURE_TEMPERATURE 1
#elif PERIODE_EMISSIONS_REGULIERES_SERVICE == 5100
    #define PERIODE_REVEIL_MESURE_TEMPERATURE 102 //Environs mesure toutes les 29minutes
//Attention : (PERIODE_EMISSIONS_REGULIERES_SERVICE/PERIODE_REVEIL_MESURE_TEMPERATURE) doit être < 256 sinon le calcul de moyenne excèdera la valeur Max
#elif PERIODE_EMISSIONS_REGULIERES_SERVICE == 2000
    #define PERIODE_REVEIL_MESURE_TEMPERATURE 100
#elif PERIODE_EMISSIONS_REGULIERES_SERVICE == 330
    #define PERIODE_REVEIL_MESURE_TEMPERATURE 33
#endif

#define NB_ELEMENT_TEMP (PERIODE_EMISSIONS_REGULIERES_SERVICE/PERIODE_REVEIL_MESURE_TEMPERATURE)
#define ARRONDI_SUP (0.5*NB_ELEMENT_TEMP)
//Ne pas modifier
#define JOUR 0b1
#define NUIT 0b0

#define TIMEOUT_GPS 716 //~153 Secondes : 2,5 minutes attention ne pas modifier : Stats envoyés par message en comptant la confirmation de position

#define NB_CONFIRMATION 2
#define ACTIV_TRAMES_SERVICE


#define POWER_ON_MODULE T3V_ON; Delay_100ms(2); 
#define POWER_OFF_MODULE T3V_OFF;

//#define TEST_GPS
//#define TEST_CW 1 //Mise en mode CW pour Tests
//#define TEST_FAB 1 //Mise en mode envoi régulier + Tests Fab
//#define TEST_FRAMES //Mise en mode envoi régulier pour tests
//#define TEST_COURANT_VBACKUP_GPS
//#define TEST_SERVICE_FRAMES //Mise en mode envoi régulier pour tests


void Processing_Temperature(signed char * actual_temp,signed char * temp_min,signed char * temp_max,signed short * temp_moy,signed char * tab_temp,unsigned char *index_temp_old){
    signed char temp;
    temp=*actual_temp;
    *actual_temp=Recuperer_Temp();
    if((unsigned char)(*actual_temp) != 0xFF) { //Il est arrivé 1 fois sur des dizaines de milliers de mesures que le TD ne réponde pas et que FF se retrouve en temp min
        if(*actual_temp<*temp_min) *temp_min=*actual_temp;
        if(*actual_temp>*temp_max) *temp_max=*actual_temp;

        //calcul Moyenne
        *temp_moy = *temp_moy + *actual_temp - tab_temp[*index_temp_old];

        //cette plus ancienne valeur n'est plus utile, on y stocke la plus récente
        tab_temp[*index_temp_old] = *actual_temp;
        *index_temp_old = *index_temp_old + 1;
        if(*index_temp_old==NB_ELEMENT_TEMP) { //gestion du buffer circulaire
            *index_temp_old=0;
        }
    }
    else *actual_temp=temp; //Si l'on arrive pas a récupérer une valeur correcte, on reste sur l'ancienne mesure
}

void Measure_Vdd_VSolar_VBackup(float * Vdd,float * VSolar, float * VBackup, float* VBackup_min, float* VBackup_max){
    unsigned short ADC_data,ADC_data1,ADC_data2 ;

    //Simplification de ce qu'il y a au dessus
    ADC_data = ADC_Read(CHANNEL_VDD); //Mesure de la ref pour détermination du VDD

    //Mesure directe, pas de switch
    ADC_data2 = ADC_Read(CHANNEL_SOLAR); //Mesure de la valeur de backup
    *VSolar= ((float)ADC_data2 * 4.096) /(float)ADC_data; //2* because of HW Resistor divider

    // Power On measure
    EN_MEAS_VBACKUP_PIN_DIRECTION=OUTPUT; //OUTPUT at 0....
    __delay_ms(2); //2ms avec grosse marge car testé fonctionnel @50us

    ADC_data1 = ADC_Read(CHANNEL_BACKUP); //Mesure de la valeur de backup
    *VBackup= ((float)ADC_data1 * 4.096) /(float)ADC_data;//2* because of HW Resistor divider
    EN_MEAS_VBACKUP_PIN_DIRECTION=INPUT; //HZ...

    *Vdd =2095.104 / (float)ADC_data;
    *Vdd=(*Vdd)*1000;
    *VSolar=(*VSolar)*1000;
    *VBackup=(*VBackup)*1000;

    if(*VBackup<*VBackup_min) *VBackup_min=*VBackup;
    if(*VBackup>*VBackup_max) *VBackup_max=*VBackup;
    
}

void main(void) {
    
    float Vdd,VBackup,VSolar;
    unsigned char day_status;
    unsigned short cpt_reveil, cpt_reveil_service;
    unsigned char compteur_trames;
    unsigned char flag_protect;
    unsigned short nb_slot_protec;
    unsigned short cpt_timeout,cpt_timeout2,cpt_timeout3;
    unsigned char dopBis;
    unsigned long Lat,Long;
    float vitesse,cap;
    unsigned char state_s, old_state_s;
    unsigned char count_confirmation;
    unsigned short temp_short;
    signed char actual_temp,temp_min,temp_max;
    signed char tab_temp[NB_ELEMENT_TEMP];
    signed short temp_moy;
    unsigned char index_temp_old;
    float VBackup_min,VBackup_max;
    signed char temp_char;
    unsigned char temp_uchar;

    CLK_SET_31K_SPEED;
    //High speed clock setting
    //CLK_SET_16M_SPEED;
    HW_INIT;
    SW_INIT;
    
    UART_CONFIG;
    GPS_CONFIG;

    //cpt_reveil=65530;
    cpt_reveil=0;
    cpt_reveil_service=0;
    compteur_trames=0;
    nb_slot_protec=0;
    day_status=JOUR;
    INT_ENABLE;
    init_compteur_trames();
    state_s=S_WAIT_FOR_ENERGY;
    count_confirmation=0;
    vitesse=0.0;
    cap=0.0;
    temp_min=127;
    temp_max=-127;
    VBackup_min=9999; //Valeur supérieure à toute mesure
    VBackup_max=-9999; //Valeur inférieure à toute mesure
    flag_protect=0;

    //effacer les valeurs anciennes de temperatures
    for(index_temp_old=0;index_temp_old<NB_ELEMENT_TEMP;index_temp_old++) {
        tab_temp[index_temp_old] = 0;
    }
    //on commencera à stocker à cet offset
    index_temp_old = 0;
    temp_moy = 0;


    CLK_SET_16M_SPEED;

    #ifdef TEST_GPS
    T3V_ON;
    Delay_100ms(20);
    POWER_ON_GPS;
    POWER_ON_VBACKUP;
    while(1){
        ENTER_SLEEP_MODE; //16S
    }

#endif


#ifdef TEST_COURANT_VBACKUP_GPS
        unsigned char a;
    //ENTER_SLEEP_MODE; //16S
    T3V_ON;
    Delay_100ms(25);
    configure_modem_TD();


    ENABLE_UART_GPS;
    POWER_ON_GPS;
    POWER_ON_VBACKUP;
    Reinit_trame_GPS();
    configure_GPS();
    Delay_100ms(100); //Attente
POWER_OFF_GPS;
    DISABLE_UART_GPS;
    while(1){
        ENTER_SLEEP_MODE; //16S
    }

#endif
#ifdef TEST_CW

    T3V_ON;
    Delay_100ms(25);
    configure_modem_TD();
    Activer_Mode_CW();
    while(1){
        ENTER_SLEEP_MODE; //16S
    }

#endif

#ifdef TEST_SERVICE_FRAMES
    T3V_ON;
    Delay_100ms(25);
    configure_modem_TD();
    while(1){
        Measure_Vdd_VSolar_VBackup(&Vdd,&VSolar,&VBackup,&VBackup_min,&VBackup_max);
        Processing_Temperature(&actual_temp,&temp_min,&temp_max,&temp_moy,tab_temp,&index_temp_old);
        Envoi_message_service(&nb_slot_protec,actual_temp,&temp_min,&temp_max,temp_char,SW_VERSION,&VBackup_min,&VBackup_max);
                
        //Delay total: 2 minutes
        Delay_100ms(150);
        Processing_Temperature(&actual_temp,&temp_min,&temp_max,&temp_moy,tab_temp,&index_temp_old);
        Delay_100ms(150);
        Processing_Temperature(&actual_temp,&temp_min,&temp_max,&temp_moy,tab_temp,&index_temp_old);
        Delay_100ms(150);
        Processing_Temperature(&actual_temp,&temp_min,&temp_max,&temp_moy,tab_temp,&index_temp_old);
        Delay_100ms(150);
        Processing_Temperature(&actual_temp,&temp_min,&temp_max,&temp_moy,tab_temp,&index_temp_old);
        Delay_100ms(150);
        Processing_Temperature(&actual_temp,&temp_min,&temp_max,&temp_moy,tab_temp,&index_temp_old);
        Delay_100ms(150);
        Processing_Temperature(&actual_temp,&temp_min,&temp_max,&temp_moy,tab_temp,&index_temp_old);
        Delay_100ms(150);
        Processing_Temperature(&actual_temp,&temp_min,&temp_max,&temp_moy,tab_temp,&index_temp_old);
        Delay_100ms(150);
    }

#endif

#ifdef TEST_FRAMES
    T3V_ON;
    Delay_100ms(25);
    configure_modem_TD();
    while(1){
        Delay_100ms(50);
        Measure_Vdd_VSolar_VBackup(&Vdd,&VSolar,&VBackup,&VBackup_min,&VBackup_max);
        Envoi_message_Erreur(VSolar,VBackup,JOUR,6);
    }

#endif

#ifdef TEST_FAB
    unsigned char a;
   // ENTER_SLEEP_MODE; //16S
    T3V_ON;
    Delay_100ms(25);
    configure_modem_TD();


    while(1){
//    Delay_100ms(10);
//    Envoyer_SIGFOX("TEST");

    Delay_100ms(50);
    Measure_Vdd_VSolar_VBackup(&Vdd,&VSolar,&VBackup,&VBackup_min,&VBackup_max);
    Envoi_message_Erreur(VSolar,VBackup,JOUR,6);
    Delay_100ms(50);
    ENABLE_UART_GPS;
    POWER_ON_GPS;
    Reinit_trame_GPS();
    configure_GPS();
    Delay_100ms(100); //Attente
    POWER_OFF_GPS;
    DISABLE_UART_GPS;
    Delay_100ms(50); //Attente
    PROTECTION_SUPERCAP_ACTIVE;
    Delay_100ms(50); //Attente
    PROTECTION_SUPERCAP_INACTIVE;
    Delay_100ms(50); //Attente
    }
//---------------TEST NICO!!!!!
#else

/*
///////////////////TEST///////////////
T3V_ON;
Delay_100ms(25);
configure_modem_TD();
while(1){
Measure_Vdd_VSolar_VBackup(&Vdd,&VSolar,&VBackup,&VBackup_min,&VBackup_max);
Envoi_message_Erreur(VSolar,VBackup,JOUR,6);
Delay_100ms(50);
}
///////////////////TEST///////////////
*/
    for (;;) {
        ENTER_SLEEP_MODE; //16S
        cpt_reveil++;
        cpt_reveil_service++;
        if(flag_protect==1) {if(nb_slot_protec<65535) nb_slot_protec++;}

        switch(state_s){
            case S_WAIT_FOR_ENERGY:
                Measure_Vdd_VSolar_VBackup(&Vdd,&VSolar,&VBackup,&VBackup_min,&VBackup_max);
                //Attente avant de demarrer car pas assez d'energie
                if((VBackup < SEUIL_DEMARRAGE_SUPERCAP)); // ne fait rien, repars en Sleep Mode general
                else {
                    //POwer ON REGU et modem
                    //POWER_ON_MODULE;
                    //Activer_Mode_CW();
                    //for(;;);
                    cpt_reveil=0;
                    cpt_reveil_service=0;

                    if (VSolar > SEUIL_DETECTION_JOUR ){
                        state_s=S_JOUR; // detection jour
                        POWER_ON_MODULE;
                        Envoi_message_Erreur(VSolar,VBackup,JOUR,6);
                        POWER_OFF_MODULE;
                    }
                    else {
                        state_s=S_NUIT; // detection nuit
                        POWER_ON_MODULE;
                        Envoi_message_Erreur(VSolar,VBackup,NUIT,6);
                        POWER_OFF_MODULE;
                    }
                }
                break;
            case S_JOUR:
                day_status=JOUR;
                Measure_Vdd_VSolar_VBackup(&Vdd,&VSolar,&VBackup,&VBackup_min,&VBackup_max);
                if(VBackup < SEUIL_CRITIQUE_SUPERCAP){
                    PROTECTION_SUPERCAP_INACTIVE; //Desactivation Auto de la protection supercap lorsque l'on consomme
                    //Envoi de trame puis passage en mode attente energie
                    flag_protect=0;
                    POWER_ON_MODULE;
                    Envoi_message_Erreur(VSolar,VBackup,day_status,1);
                    POWER_OFF_MODULE;
                    state_s=S_WAIT_FOR_ENERGY;
                    break;
                }
                else { //Tension nominale
                    if(VBackup > SEUIL_PROTECTION_SUPERCAP){
                        PROTECTION_SUPERCAP_ACTIVE;
                        flag_protect=1;
                    }
                    else if(VBackup < SEUIL_SUPERCAP_SECURE) {
                        PROTECTION_SUPERCAP_INACTIVE;
                        flag_protect=0;
                    }

                    if (VSolar < SEUIL_DETECTION_JOUR ){
                        count_confirmation=0;
                        state_s=S_WAIT_CONF_NUIT;
                    }
                    else{//Tension nominale et Mode nominal: JOUR
                        if((cpt_reveil / PERIODE_EMISSIONS_REGULIERES_JOUR) >=1 ){ //si Moment d'envoi GPS
                            cpt_reveil=0;
                            if (VBackup > SEUIL_GPS_AUTORISE){
                                old_state_s=state_s;
                                state_s=SS_ACQ_GPS;
                                //Envoi_message_service(&nb_slot_protec,actual_temp,&temp_min,&temp_max,&temp_moy,SW_VERSION,&VBackup_min,&VBackup_max);

                            }
                            else{
                                PROTECTION_SUPERCAP_INACTIVE; //Desactivation Auto de la protection supercap lorsque l'on consomme
                                flag_protect=0;
                                POWER_ON_MODULE;
                                Envoi_message_Erreur(VSolar,VBackup,day_status,2);
                                POWER_OFF_MODULE;
                                //On reste dans le même état pour la prochaine fois
                            }
                            break;
                        } //si Moment d'envoi GPS
                        //Si opportunité d'envoyer une trame en plus car supercap bien chargée...
                        else if(((cpt_reveil % PERIODE_EMISSIONS_SUPP_JOUR) ==0 ) && (VBackup > SEUIL_SUPERCAP_TRAMES_SUPP)){
                            old_state_s=state_s;
                            state_s=SS_ACQ_GPS;
                        }
                        else; //Sinon on attend
#ifdef ACTIV_TRAMES_SERVICE
                        if((cpt_reveil_service % PERIODE_REVEIL_MESURE_TEMPERATURE)==0){ //si Moment de prise de Mesure Temperature
                            Processing_Temperature(&actual_temp,&temp_min,&temp_max,&temp_moy,tab_temp,&index_temp_old);
                        }

                        if(((cpt_reveil_service /PERIODE_EMISSIONS_REGULIERES_SERVICE)>=1) && (VSolar > VBackup)){ //si Moment d'envoi trame service
                            cpt_reveil_service=0;
                            temp_char=(signed char)(((signed short)temp_moy+(signed short)ARRONDI_SUP)/NB_ELEMENT_TEMP);
                            POWER_ON_MODULE;
                            Envoi_message_service(&nb_slot_protec,actual_temp,&temp_min,&temp_max,temp_char,SW_VERSION,&VBackup_min,&VBackup_max);
                            POWER_OFF_MODULE;
                        }
#endif
                   }//fin siTension nominale et Mode nominal: Nuit
                }
                break;

            case S_NUIT:
                day_status=NUIT;
                Measure_Vdd_VSolar_VBackup(&Vdd,&VSolar,&VBackup,&VBackup_min,&VBackup_max);
                if((VBackup < SEUIL_CRITIQUE_SUPERCAP)){
                    //Envoi de trame puis passage en mode attente energie
                     PROTECTION_SUPERCAP_INACTIVE; //Desactivation Auto de la protection supercap lorsque l'on consomme
                     flag_protect=0;
                     POWER_ON_MODULE;
                     Envoi_message_Erreur(VSolar,VBackup,day_status,3);
                     POWER_OFF_MODULE;
                     state_s=S_WAIT_FOR_ENERGY;
                     break;
                }
                else { //Tension nominale
                    if (VSolar > SEUIL_DETECTION_JOUR ){
                        count_confirmation=0;
                        state_s=S_WAIT_CONF_JOUR;
                    }
                    else{//Tension nominale et Mode nominal: Nuit
                        if((cpt_reveil / PERIODE_EMISSIONS_REGULIERES_NUIT) >=1 ){
                            cpt_reveil=0;
                            if (VBackup > SEUIL_GPS_AUTORISE){
                                old_state_s=state_s;
                                state_s=SS_ACQ_GPS;
                            }
                            else{
                                PROTECTION_SUPERCAP_INACTIVE; //Desactivation Auto de la protection supercap lorsque l'on consomme
                                flag_protect=0;
                                POWER_ON_MODULE;
                                Envoi_message_Erreur(VSolar,VBackup,day_status,4);
                                POWER_OFF_MODULE;
                                //On reste dans le même état pour la prochaine fois
                            }
                        } //si Moment d'envoi GPS
                        else; //Sinon on attend

                        if((cpt_reveil_service % PERIODE_REVEIL_MESURE_TEMPERATURE)==0){ //si Moment de prise de Mesure Temperature
                            Processing_Temperature(&actual_temp,&temp_min,&temp_max,&temp_moy,tab_temp,&index_temp_old);
                        }

                   }//fin siTension nominale et Mode nominal: Nuit
                }
                break;
                
            case S_WAIT_CONF_JOUR:
               Measure_Vdd_VSolar_VBackup(&Vdd,&VSolar,&VBackup,&VBackup_min,&VBackup_max);
               if (VSolar > SEUIL_DETECTION_JOUR) {count_confirmation++;}
               else {
                   state_s= S_NUIT;
                   break;
               }
               if (count_confirmation >= NB_CONFIRMATION){
                    state_s= S_JOUR;
                    break;
               }
               else; // ne rien faire attendre prochain tour de confirmation
               break;


           case S_WAIT_CONF_NUIT:
               Measure_Vdd_VSolar_VBackup(&Vdd,&VSolar,&VBackup,&VBackup_min,&VBackup_max);
               if (VSolar < SEUIL_DETECTION_JOUR )count_confirmation++;
               else {
                   state_s= S_JOUR;
                   break;
               }
               if (count_confirmation >= NB_CONFIRMATION){
                   //Pour etre sur de l'état a l'entrée en mode Nuit   
                    PROTECTION_SUPERCAP_INACTIVE;
                    flag_protect=0;
                    state_s= S_NUIT;
                    break;
               }
               else; // ne rien faire attendre prochain tour de confirmation
               break;

            case SS_ACQ_GPS:
                UART_RX_Buffers_Init();
                ENABLE_UART_GPS;
                PROTECTION_SUPERCAP_INACTIVE; //Desactivation Auto de la protection supercap lorsque l'on consomme
                flag_protect=0;
                POWER_ON_GPS;
                POWER_ON_VBACKUP;
                Reinit_trame_GPS();
                configure_GPS();
                cpt_timeout=0;
                while(cpt_timeout<TIMEOUT_GPS){ // xx secondes... ( le temps de recevoir une trame GPS)
                    //CLRWDT();
                    if (UART_Frame_Received()!=_NO) {
                        if(decode_trame_GPS()==1) break;
                    }
                    Delay_100ms(2); //Attente 100 mili seconde
                    //Ne pas trop attendre pour qu'il ait le temps de vider la fifo...
                    cpt_timeout++;
                }

                //Attente 8 secondes pour amélioration du DOP (MAJ du nb sat et position)
                cpt_timeout2=0;
                while(cpt_timeout2<40){ // 8 secondes...
                    if (UART_Frame_Received()!=_NO)  decode_trame_GPS();
                    Delay_100ms(2); //Attente 200 mili seconde
                    //Ne pas trop attendre pour qu'il ait le temps de vider la fifo...
                    cpt_timeout2++;
                }

                //Attente 2 secondes pour recup de la vitesse/Cap
                configure_GPS_VTG();
                cpt_timeout3=0;
                while(cpt_timeout3<10){ // 2 secondes...
                    if (UART_Frame_Received()!=_NO)  decode_trame_GPS();
                    Delay_100ms(2); //Attente 200 mili seconde
                    //Ne pas trop attendre pour qu'il ait le temps de vider la fifo...
                    cpt_timeout3++;
                }

                POWER_OFF_GPS;
                DISABLE_UART_GPS;

                //
                temp_short=(cpt_timeout+ cpt_timeout2 + cpt_timeout3)/25;//Conversion de résolution 0,2s en multiples de 5 secondes
                if (temp_short > 15)temp_short=15;//Doit avoir un maximum de 15 d'ou 75secondes

                if(cpt_timeout< TIMEOUT_GPS){ //Trame GPS décodée
                    decodage_Lat_Long(&Lat,&Long,&dopBis);
                    decodage_VTG(&cap,&vitesse);
                    cap=cap / 11,25; //pas de 11,25°
                    vitesse = vitesse / 5.0; //pas de 5km/h
                    //Verification si retour GPS OK.... pour éviter les trames qui commencent par 0x7xxxx
                    temp_uchar = (Lat >> 24);
                    temp_uchar = temp_uchar & 0b01111111;
                    POWER_ON_MODULE;
                    if(temp_uchar > 0b01011111) Envoi_message_Erreur(VSolar,VBackup,day_status,7);
                    else Envoi_message(Long,Lat,dopBis,VSolar,VBackup,day_status,temp_short,(unsigned char)vitesse,(unsigned char)cap);
                    POWER_OFF_MODULE;
                }
                else {
                    POWER_ON_MODULE;    
                    Envoi_message_Erreur(VSolar,VBackup,day_status,5); //Trame GPS non décodée
                    POWER_OFF_MODULE;
                }
                state_s=old_state_s;//on revient dans l'état ou l'on était.
                break;
                
            default:
                break;
        }//end switch
    }//End For

#endif
}

//Calculs  sans simplification
//                    ADC_data = ADC_Read(CHANNEL_VDD); //Mesure de la ref pour détermination du VDD
//                    Vdd =2,048 * 1023.0 / (float)ADC_data;
//
//                    ADC_data = ADC_Read(CHANNEL_BACKUP); //Mesure de la valeur de backup
//                    VBackup= 2* ADC_data*Vdd/1023;//2* because of HW Resistor divider
//
//                    ADC_data = ADC_Read(CHANNEL_SOLAR); //Mesure de la valeur de backup
//                    VSolar= 2 * ADC_data*Vdd/1023; //2* because of HW Resistor divider