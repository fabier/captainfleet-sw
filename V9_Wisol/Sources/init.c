//#include <xc.h>
//#include "hardware.h"
//#include "init.h"
//#include "globals.h"
//#include "serial.h"

//void wait_sleep_mode(unsigned char ms)
//{
//	// check end transmission : empty the tx buffer
//	while( TXSTAbits.TRMT == 0);
//
//	// uart is not in reception : idle
//	while(RCIDL == 0);
//
//    //reprogram wdog to have ms
//    WDTCONbits.SWDTEN = 0b0; //Switch OFF the Watchdog
//	WDTCONbits.WDTPS=ms&0x1f; // ms miliseconds timeout
//    WDTCONbits.SWDTEN = 0b1; //Switch ON the Watchdog
//	asm("SLEEP");
//	asm("nop");
//	//back to normal wdog : 16s
//    WDTCONbits.SWDTEN = 0b0; //Switch OFF the Watchdog
//	WDTCONbits.WDTPS= WD_PRESCALER_16S;
//    WDTCONbits.SWDTEN = 0b1; //Switch ON the Watchdog
//}
