#ifndef ADC_H
#define ADC_H

#define CHANNEL_TEMP 0b01110101
#define CHANNEL_VDD 0b01111101

#define CHANNEL_BACKUP 0b00011001
#define CHANNEL_SOLAR 0b00011101

#define ADC_AVERAGE 4  // do not average on more than 16

unsigned short ADC_Read(char channel);

#endif