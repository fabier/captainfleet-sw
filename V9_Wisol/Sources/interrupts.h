#ifndef INTERRUPTS
#define INTERRUPTS

#define INT_ENABLE \
    PIE1=0; \
    PIE2=0; \
    (void)RCREG; \
    (void)RCREG; \
    PIE1bits.RCIE=0b1; \
    INTCONbits.PEIE=0b1; \
    INTCONbits.GIE=0b1


static void interrupt isr(void);

#endif