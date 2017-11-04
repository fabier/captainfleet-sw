#include "globals.h"
#include "init.h"
#include <xc.h>

void Delay_100ms(unsigned short duree){ //__delay_ms is too limitated
    unsigned short i;
    for(i=0;i<duree;i++) {
        __delay_ms(25);
        __delay_ms(25);
        __delay_ms(25);
        __delay_ms(25);
    }
}