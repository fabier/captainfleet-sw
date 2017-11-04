#include "globals.h"
#include "modem.h"
#include "serial.h"
#include <xc.h>
#include "init.h"
#include "hardware.h"
#include <string.h>

static unsigned char compteur_trames;
static unsigned char nb_ko_modem;

void init_compteur_trames(void){
    compteur_trames=0;
    nb_ko_modem=0;
}

void hex_to_ascii(unsigned char hex){
    unsigned char a;
    unsigned char b;
    a=hex/16;
    if(a<10) UART_Transmit_one_char(a+48);
    else UART_Transmit_one_char(a-10+65);
    b=hex-a*16;
    if(b<10) UART_Transmit_one_char(b+48);
    else UART_Transmit_one_char(b-10+65);
}

void Envoi_message(unsigned long Longitude,unsigned long Latitude,
        unsigned char dop,unsigned short Vsolar, unsigned short Vbackup, unsigned char day_status,
        unsigned char timeout, unsigned char vitesse, unsigned char cap) {

    unsigned char i,i2;
    UART_RX_Buffers_Init();

    ENABLE_UART_MODEM;
    
    UART_Transmit_string("AT$SF=");

    //Lat: 4 bytes
    hex_to_ascii((Latitude >> 24) & 0xFF);
    hex_to_ascii((Latitude >> 16) & 0xFF);
    hex_to_ascii((Latitude >> 8) & 0xFF);
    hex_to_ascii((Latitude) & 0xFF);
    //Long: 4 bytes
    hex_to_ascii((Longitude >> 24) & 0xFF);
    hex_to_ascii((Longitude >> 16) & 0xFF);
    hex_to_ascii((Longitude >> 8) & 0xFF);
    hex_to_ascii((Longitude) & 0xFF);

    //Dop+ NbSat (déja remplis par le décodage GPS) + J/N + Compteur: 1 byte
    i = dop | ((day_status << 3)& 0x08);
    i = i | ((compteur_trames) &0x07);
    if (compteur_trames <7) compteur_trames++;
    else compteur_trames=0;
    hex_to_ascii(i);

    i= (timeout<<4);
    i= i | (vitesse >>1);
    hex_to_ascii(i);

    i= (vitesse & 0x01) << 7; //ultime bit de vitesse
    i= i | ((cap & 0b01111100)  <<2);
    if(Vsolar>=(31*(unsigned short)RESOLUTION_VSOLAR_MV)) i2=0x1F; //31
    else  i2=  (unsigned char)(((float)Vsolar / (float)RESOLUTION_VSOLAR_MV)+0.5);
    i = i | (i2 >> 3); //on fait rentrer les 2 MSB de la tension Solar
    hex_to_ascii(i);

    i= i2 << 5; //3LSB de Vsolar
    if(Vbackup>=(31*(unsigned short)RESOLUTION_VBACKUP_MV)) i2=0x1F; //31
    else  i2=  (unsigned char)(((float)Vbackup / (float)RESOLUTION_VBACKUP_MV)+0.5);
    i = i | i2 ;
    hex_to_ascii(i);

    //Validation
    UART_Transmit_one_char(END_OF_TRANSMISSION_MODEM); //Space
    UART_WAIT_FOR_END_OF_TRANSMISSION;
    Delay_100ms(10);
    //Wait for Modem Answer!
    //Ajouter la gestion de la non réponse modem: Reset + incrémentation du compteur

    Delay_100ms(90);

    DISABLE_UART_MODEM;
}


void Envoi_message_Erreur(unsigned short Vsolar, unsigned short Vbackup, unsigned char day_status,unsigned char raison) {

    unsigned char i,i2;
    UART_RX_Buffers_Init();

    ENABLE_UART_MODEM;

    UART_Transmit_string("AT$SF=");


    hex_to_ascii(0xE0);
    hex_to_ascii(raison);

    
    i = ((day_status << 3)& 0x08);
    i = i | ((compteur_trames) &0x07);
    if (compteur_trames <7) compteur_trames++;
    else compteur_trames=0;
    hex_to_ascii(i);

    i=(unsigned char)((float)Vsolar/(float)20.0);
    hex_to_ascii(i); //Tension Backup Min par pas de 20mV
   
    i=(unsigned char)((float)Vbackup/20.0);
    hex_to_ascii(i); //Tension Backup Min par pas de 20mV


    //Validation
    UART_Transmit_one_char(END_OF_TRANSMISSION_MODEM); //Space
    UART_WAIT_FOR_END_OF_TRANSMISSION;
    Delay_100ms(10);
    //Wait for Modem Answer!
    //Ajouter la gestion de la non réponse modem: Reset + incrémentation du compteur

    Delay_100ms(90);
    DISABLE_UART_MODEM;
}


void Envoi_message_service(unsigned short *nb_slot_protec,signed char actual_temp,signed char *temp_min,signed char *temp_max,signed char temp_moy,unsigned char sw_version,float* VBackup_min, float* VBackup_max){
    unsigned char i,i2;
    UART_RX_Buffers_Init();
    ENABLE_UART_MODEM;
    UART_Transmit_string("AT$SF=");

    i=0x60;//Type trame de Service
    i = i | ((compteur_trames) &0x07);
    if (compteur_trames <7) compteur_trames++;
    else compteur_trames=0;
    
    hex_to_ascii(i);
    
    hex_to_ascii(actual_temp);//Temp actuelle
    hex_to_ascii(temp_moy);//Temp Moyenne
    hex_to_ascii(*temp_min);//Temp Min
    *temp_min=actual_temp;
    hex_to_ascii(*temp_max);//Temp Max
    *temp_max=actual_temp;

    *nb_slot_protec = *nb_slot_protec / 20; //Résolution = 1/20eme -> 16s*20=320 secondes, Résolution = environs  5,3minutes
    if(*nb_slot_protec > 63)  *nb_slot_protec=63; //Clamping
    i = (char) (*nb_slot_protec << 2);
    *nb_slot_protec=0;

    if(nb_ko_modem<4) i2=nb_ko_modem;
    else i2=3;
    i= i | i2;
    hex_to_ascii(i); //Nb KO modem + Nb protect

    i= sw_version << 4;
    i = i & 0xF0;
    hex_to_ascii(i); //N° Version sur le 4 bits de MSB

    i=(unsigned char)((float)*VBackup_min/(float)20.0);
    hex_to_ascii(i); //Tension Backup Min par pas de 20mV
    i=(unsigned char)((float)*VBackup_max/(float)20.0);
    hex_to_ascii(i); //Tension Backup Min par pas de 20mV
    *VBackup_min=9999; //Valeur supérieure à toute mesure
    *VBackup_max=-9999; //Valeur inférieure à toute mesure
    //Validation
    UART_Transmit_one_char(END_OF_TRANSMISSION_MODEM); //Space
    UART_WAIT_FOR_END_OF_TRANSMISSION;
    Delay_100ms(10);
    //Wait for Modem Answer!
    //Ajouter la gestion de la non réponse modem: Reset + incrémentation du compteur

    Delay_100ms(90);
    DISABLE_UART_MODEM;
}

unsigned char decode_trame_modem(void)
{
    unsigned char flag;
    if(strncmp(chaine, "OK", 2) == 0) flag=1;
    else flag=0;

    chaine_Init();

    return flag;
}




void Envoyer_SIGFOX(const char * s) {

    unsigned char a;
    a=0;
    ENABLE_UART_MODEM;
    UART_Transmit_one_char('A');
    UART_Transmit_one_char('T');
    UART_Transmit_one_char('$');
    UART_Transmit_one_char('S');
    UART_Transmit_one_char('F');
    UART_Transmit_one_char('=');

    while((s[a])!='\0') {
        hex_to_ascii(s[a]);
        a++;
    }
    //Validation
    UART_Transmit_one_char(END_OF_TRANSMISSION_MODEM); //Space
    UART_WAIT_FOR_END_OF_TRANSMISSION;

    //Wait for Modem Answer!
    //Ajouter la gestion de la non réponse modem: Reset + incrémentation du compteur

    DISABLE_UART_MODEM;
}



void Demander_ID() {

   
    ENABLE_UART_MODEM;
    UART_Transmit_string("ATI7");
    UART_Transmit_one_char(END_OF_TRANSMISSION_MODEM);
    UART_WAIT_FOR_END_OF_TRANSMISSION;

    DISABLE_UART_MODEM;
}
  
void Sleep_Modem() {
    ENABLE_UART_MODEM;
    UART_Transmit_string("AT$P=1");
    UART_Transmit_one_char(END_OF_TRANSMISSION_MODEM);
    UART_WAIT_FOR_END_OF_TRANSMISSION;
    Delay_100ms(4);
    DISABLE_UART_MODEM;
}
void Wake_UP_Modem() {
    ENABLE_UART_MODEM;
    UART_Transmit_one_char('\0');
    Delay_100ms(2);
    UART_Transmit_one_char(END_OF_TRANSMISSION_MODEM);
    UART_WAIT_FOR_END_OF_TRANSMISSION;
    Delay_100ms(12);
    UART_RX_Buffers_Init();//in order to flush the AT pArse error while returning from sleep
}
signed char Recuperer_Temp() {
    unsigned char cpt_timeout;
    signed char temp;
#ifndef TD120X

    Wake_UP_Modem();
    chaine_Init(); //RAZ de la chaine
    UART_Transmit_string("AT$T?");
    UART_Transmit_one_char(END_OF_TRANSMISSION_MODEM); //Space
    UART_WAIT_FOR_END_OF_TRANSMISSION;

     cpt_timeout=0;
    while(cpt_timeout<10){ // 2 secondes... ( le temps de recevoir le OK du modem)
        if (UART_Frame_Received()!=_NO) {
            temp= 100*(chaine[0]-0x30)+10*(chaine[1]-0x30)+(chaine[2]-0x30);
            //temp= (unsigned char)((float)temp + 0,5 + (((float)(chaine[3]-0x30)) /10) );
            break;
        }
        Delay_100ms(2); //Attente 100 mili seconde
        //Ne pas trop attendre pour qu'il ait le temps de vider la fifo...
        cpt_timeout++;
    }
    chaine_Init();
    Sleep_Modem();
#else
    ENABLE_UART_MODEM;
    UART_RX_Buffers_Init();
    chaine_Init(); //RAZ de la chaine
    UART_Transmit_string("ATI26");
    UART_Transmit_one_char(END_OF_TRANSMISSION_MODEM); //Space
    UART_WAIT_FOR_END_OF_TRANSMISSION;

    cpt_timeout=0;
    while(cpt_timeout<10){ // 2 secondes... ( le temps de recevoir le OK du modem)
        if (UART_Frame_Received()!=_NO) { ///r/n avant la température...
            if (UART_Frame_Received()!=_NO) {
                if(chaine[0]==0x30)temp=0; // retourne temp <=0°C....
                else if(chaine[1]==0x0D)temp=chaine[0]-0x30;// retourne temp entre 0°C et 9°C....
                else temp= 10*(chaine[0]-0x30)+(chaine[1]-0x30); //temp Classique à 2 digits
            break;
            }
        }
        Delay_100ms(2); //Attente 100 mili seconde
        //Ne pas trop attendre pour qu'il ait le temps de vider la fifo...
        cpt_timeout++;
    }
    chaine_Init();
    DISABLE_UART_MODEM;
#endif
    
    if(cpt_timeout<10)return temp;
    else return 0xFF;
}


void Activer_Mode_CW() {

    UART_RX_Buffers_Init();

    ENABLE_UART_MODEM;

    UART_Transmit_string("AT$CW=868130000,1,15");

    //Validation
    UART_Transmit_one_char(END_OF_TRANSMISSION_MODEM); //CR
    UART_WAIT_FOR_END_OF_TRANSMISSION;
    Delay_100ms(10);
    //Wait for Modem Answer!
    //Ajouter la gestion de la non réponse modem: Reset + incrémentation du compteur

    DISABLE_UART_MODEM;

}