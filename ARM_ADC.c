/*
 * ARM_ADC.c
 *
 * Created: 20.6.2016 15:13:57
 *  Author: Tomy2
	
	uint32_t AD3 = ((uint32_t)AD3data * 3300) / 4096;
	uint32_t AD9 = ((uint32_t)AD9data * 6600) / 4096;
	float temperatureF = (float)AD15data * 0.30402 - 274.887;
*/

#include "sam.h"
#include "ARM_ADC.h"
#include "ARM_DELAY.h"

void ADC_init(void)
{
	PMC->PMC_PCER0 |= (1 << ID_PIOA); // enable clock to the peripheral
	PIOA->PIO_PDR |= (1 << 20); // disable PIOC control on the pin (disable peripheral control)
	PIOA->PIO_PDR |= (1 << 22); // disable PIOC control on the pin (disable peripheral control)
	PIOA->PIO_PUDR |= (1 << 20); // disable pull-up
	PIOA->PIO_PUDR |= (1 << 22); // disable pull-up
}

void ADC_start(void)
{
	PMC->PMC_PCER0 |= (1 << ID_ADC); // enable clock for ADC
	ADC->ADC_CR = ADC_CR_SWRST; // Reset the controller.
	ADC->ADC_MR = 0; // Reset Mode Register.
	ADC->ADC_MR |= (0x1u << 28) | (0x00u << 24) | (0x03u << 20) | (0xFFu << 8) | (0x08u << 0); // 125kHz (minimum) ADC clock, disable External trigger, settling 17 periods	
}

uint16_t ADC_sample(uint8_t channel)
{
	ADC->ADC_CHER = (1 << channel); // enable channel
	ADC->ADC_CR = ADC_CR_START; // begin AD conversion
	uint32_t status = ADC->ADC_ISR;
	while((status & (0x1u << channel)) != (0x1u << channel)) status = ADC->ADC_ISR;
	uint16_t data = (uint16_t)ADC->ADC_CDR[channel];
	ADC->ADC_CHDR = (1 << channel); // channel disable
	
	return data;
}

uint16_t ADC_sample_temperature(void)
{
	ADC->ADC_ACR = (1 << 4); // Temperature sensor on.
	SysTick_delay_ms(1); // 40us start-up time
	ADC->ADC_CHER = (1 << 15); // enable channel
	ADC->ADC_CR = ADC_CR_START; // begin AD conversion
	uint32_t status = ADC->ADC_ISR;
	while((status & (0x1u << 15)) != (0x1u << 15)) status = ADC->ADC_ISR;
	uint16_t data = (uint16_t)ADC->ADC_CDR[15];
	ADC->ADC_ACR = 0; // Temperature sensor off.
	ADC->ADC_CHDR = (1 << 15); // channel disable
	
	return data;
}

void ADC_stop(void)
{
	PMC->PMC_PCER0 &= ~(1 << ID_ADC); // disable clock
}

uint16_t ADC_full_sample(uint8_t channel)
{
	PMC->PMC_PCER0 |= (1 << ID_ADC); // enable clock for ADC
	ADC->ADC_CR = ADC_CR_SWRST; // Reset the controller.
	ADC->ADC_MR = 0; // Reset Mode Register.
	ADC->ADC_MR |= (0x1u << 28) | (0x00u << 24) | (0x03u << 20) | (0xFFu << 8) | (0x08u << 0); // 125kHz (minimum) ADC clock, disable External trigger, settling 17 periods
	ADC->ADC_CHER = (1 << channel); // enable channel
	ADC->ADC_CR = ADC_CR_START; // begin AD conversion
	uint32_t status = ADC->ADC_ISR;
	while((status & (0x1u << channel)) != (0x1u << channel)) status = ADC->ADC_ISR; // wait for
	uint16_t data = (uint16_t)ADC->ADC_CDR[channel];
	
	ADC->ADC_CHDR = (1 << channel); // channel disable
	PMC->PMC_PCER0 &= ~(1 << ID_ADC); // disable clock
	
	return data;
}

uint16_t ADC_full_sample_temperature(void) // AD15
{
	PMC->PMC_PCER0 |= (1 << ID_ADC); // enable clock for ADC
	ADC->ADC_ACR = (1 << 4); // Temperature sensor on.
	SysTick_delay_ms(1); // 40us start-up time
	ADC->ADC_CR = ADC_CR_SWRST; // Reset the controller.
	ADC->ADC_MR = 0; // Reset Mode Register.
	ADC->ADC_MR |= (0x1u << 28) | (0x00u << 24) | (0x03u << 20) | (0xFFu << 8) | (0x08u << 0); // 125kHz (minimum) ADC clock, disable External trigger, settling 17 periods
	ADC->ADC_CHER = (1 << 15); // enable channel
	ADC->ADC_CR = ADC_CR_START; // begin AD conversion
	
	uint32_t status = ADC->ADC_ISR;
	while((status & (0x1u << 15)) != (0x1u << 15)) status = ADC->ADC_ISR; // wait for End of Conversion
	uint16_t data = (uint16_t)ADC->ADC_CDR[15];
	
	ADC->ADC_ACR = 0; // Temperature sensor off.
	ADC->ADC_CHDR = (1 << 15); // channel disable
	PMC->PMC_PCER0 &= ~(1 << ID_ADC); // disable clock
	
	return data;
}