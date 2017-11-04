#include <xc.h>
#include "globals.h"
#include "init.h"
#include "adc.h"

    /*! \fn  void ADC_Read(void)
     *  \brief Read the Value of the channel specified.
     *  \param . Channel to read
     *  
     *  \return ADC Voltage.
     */
unsigned short ADC_Read(unsigned char channel){
    unsigned short adc_value;
    int i;
    adc_value=0x0000;

    if (channel == CHANNEL_TEMP)
    {
        //FVRCON  | FVREN | FVRRDY | TSEN | TSRING | CDAFVR<1:0> | ADFVR<1:0> |
        //Fixed voltage Enabled, Temperature sensor enabled,ref @0
        FVRCON = 0b11100000;
    	//TSEN=0 bit5 TSRING=1 bit4 FVREN bit=1  adfvr=0b10 :2.048V
    }
    else{
        //FVRCON:  | FVREN | FVRRDY | TSEN | TSRING | CDAFVR<1:0> | ADFVR<1:0> |
        //FVRCON = 0b11000010; //TSEN=0 bit5 TSRING=1 bit4 FVREN bit=1  adfvr=0b10 :2.048V
	    //Fixed voltage Enabled, Temperature sensor disabled,ref @2,048V
        FVRCON = 0b11000010;
    }

    //ADCON0  | - | CHS<4:0>  | GO/ /DONE | ADON |
    ADCON0=channel;
   	//ADCON1: | ADFM | ADCS<2:0> | - | - | ADPREF<1:0> |
    ADCON1 = 0b11010000;
        
	//average loop
	for(i=0;i<ADC_AVERAGE;i++)
	{
	    __delay_us(300);
        // start the conversion
	    ADCON0bits.ADGO=1;      // start the conversion
	    //wait the en of conversion	    
	    // stop conversion before end of automatic conversion : workarround microchip
	    _delay(43); // 43 is defined in wa with fosc/16
	    ADCON0bits.ADGO=0;
	    //read the result of the conversion
	    adc_value += ((ADRESH&0x03)<<8) |ADRESL; 
	}
	adc_value = adc_value/ADC_AVERAGE;
	//disable FVR for low consumption
	// switch off ADC
	FVRCON=0x0;
	ADCON0=0x0;
	ADCON1=0x0;
    return (adc_value&0x3FF);
}