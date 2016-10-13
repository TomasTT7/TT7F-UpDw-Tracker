/*
ADC_init() - initializes SAM3S8's pins

for multiple ADC measurements:
	ADC_start()
	ADC_sample(1)
	ADC_sample(2)
	ADC_sample(3)
	ADC_sample_temperature()
	ADC_stop()
*/

#ifndef ARM_ADC_H
#define ARM_ADC_H

#include "stdint.h"

// Functions
void ADC_init(void);
void ADC_start(void);
uint16_t ADC_sample(uint8_t channel);
uint16_t ADC_sample_temperature(void);
void ADC_stop(void);
uint16_t ADC_full_sample(uint8_t channel);
uint16_t ADC_full_sample_temperature(void);

#endif // ARM_ADC_H_