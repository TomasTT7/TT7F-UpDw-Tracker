/*
 * ARM_SI4060.c
 *
 * Created: 20.6.2016 15:37:46
 *  Author: Tomy2
 */ 

#include "sam.h"
#include "ARM_SI4060.h"
#include "ARM_SPI.h"
#include "ARM_DELAY.h"

void SI4060_CTS_check_and_read(uint8_t response_length)
{
	uint16_t data;
	uint8_t dt = 0;
	
	while(dt != 0xFF)
	{
		SPI->SPI_CR = SPI_CR_SPIEN; // enable SPI if disabled
		SPI_write(0x44, 0); // READ_CMD_BUFF
		data = SPI_write(0x00, 0);
		dt = (uint8_t)data;
		if(dt == 0xFF) break;
		SPI->SPI_CR = SPI_CR_SPIDIS; // workaround to deassert chip select
	}
	
	for(uint8_t i = 0; i < (response_length - 1); i++)
	{
		SI4060_buffer[i] = SPI_write(0x00, 0);
	}
	SI4060_buffer[response_length - 1] = SPI_write(0x00, 1);
}

void SI4060_init(void)
{
	// enable TCXO - PA8
	PMC->PMC_PCER0 |= (1 << ID_PIOA); // enable clock to the peripheral
	PIOA->PIO_OER |= PIO_PA8; // enable Output on PA8
	PIOA->PIO_PER |= PIO_PA8; // enable PIO control on the pin (disable peripheral control)
	PIOA->PIO_PUDR |= PIO_PA8; // disable pull-up
	PIOA->PIO_SODR |= PIO_PA8; // Set Output Data Register
	
	// setup GPIO1 - PB4
	MATRIX->CCFG_SYSIO |= (1 << 4); // System I/O lines must be configured in this register as well (PB4, PB5, PB6, PB7, PB10, PB11, PB12)
	PMC->PMC_PCER0 |= (1 << ID_PIOB); // enable clock to the peripheral
	PIOB->PIO_OER |= PIO_PB4; // enable Output on PB4
	PIOB->PIO_PER |= PIO_PB4; // enable PIO control on the pin (disable peripheral control)
	PIOB->PIO_PUDR |= PIO_PB4; // disable pull-up
	
	// setup SDN - PA7
	PIOA->PIO_OER |= PIO_PA7; // enable Output on PA7
	PIOA->PIO_PER |= PIO_PA7; // enable PIO control on the pin (disable peripheral control)
	PIOA->PIO_PUDR |= PIO_PA7; // disable pull-up
	PIOA->PIO_SODR |= PIO_PA7; // Set Output Data Register
	SysTick_delay_ms(1);
	PIOA->PIO_CODR |= PIO_PA7; // Clear Output Data Register
	SysTick_delay_ms(10);
	
	// divide the XTAL/TCXO frequency into 4 bytes
	uint32_t tcxo1 = TCXO / 16777216;
	uint32_t tcxo2 = (TCXO - tcxo1 * 16777216) / 65536;
	uint32_t tcxo3 = (TCXO - tcxo1 * 16777216 - tcxo2 * 65536) / 256;
	uint32_t tcxo4 = (TCXO - tcxo1 * 16777216 - tcxo2 * 65536 - tcxo3 * 256);
	
	SPI_write(0x02, 0); // POWER_UP - 15ms
	SPI_write(0x01, 0);
	SPI_write(0x01, 0); // 0x00 - XTAL, 0x01 - TCXO
	SPI_write(tcxo1, 0);
	SPI_write(tcxo2, 0);
	SPI_write(tcxo3, 0);
	SPI_write(tcxo4, 1);
	SI4060_CTS_check_and_read(0); // monitor CTS
	
	SPI_write(0x20, 0); // GET_INT_STATUS
	SPI_write(0x00, 0);
	SPI_write(0x00, 0);
	SPI_write(0x00, 1);
	SI4060_CTS_check_and_read(9); // monitor CTS
}

void SI4060_setup_pins(uint8_t gpio0, uint8_t gpio1, uint8_t gpio2, uint8_t gpio3, uint8_t nirq, uint8_t sdo)
{
	SPI_write(0x13, 0); // GPIO_PIN_CFG
	SPI_write(gpio0, 0); // GPIO0 - 0x02
	SPI_write(gpio1, 0); // GPIO1 - 0x04 - pin is configured as a CMOS input
	SPI_write(gpio2, 0); // GPIO2 - 0x02
	SPI_write(gpio3, 0); // GPIO3 - 0x02
	SPI_write(nirq, 0); // NIRQ - 0x00
	SPI_write(sdo, 0); // SDO - 0x00 - possible to use as TX_DATA_CLK output for GFSK
	SPI_write(0x00, 1); // GPIOs configured as outputs will have the highest drive strength
	SI4060_CTS_check_and_read(8); // monitor CTS
}

void SI4060_frequency(uint32_t freq)
{
	// divider for PLL Synthesizer running at 3.6GHz
	uint8_t outdiv = 4;
	uint8_t band = 0;
	if (freq < 705000000UL) { outdiv = 6;  band = 1;};
	if (freq < 525000000UL) { outdiv = 8;  band = 2;};
	if (freq < 353000000UL) { outdiv = 12; band = 3;};
	if (freq < 239000000UL) { outdiv = 16; band = 4;};
	if (freq < 177000000UL) { outdiv = 24; band = 5;};
	
	uint32_t f_pfd = 2 * TCXO / outdiv;
	uint32_t n = ((uint32_t)(freq / f_pfd)) - 1;
	float ratio = (float)freq / (float)f_pfd;
	float rest = ratio - (float)n;
	uint32_t m = (uint32_t)(rest * 524288UL);
	uint32_t m2 = m / 0x10000;
	uint32_t m1 = (m - m2 * 0x10000) / 0x100;
	uint32_t m0 = (m - m2 * 0x10000 - m1 * 0x100);
	
	SPI_write(0x11, 0); // SET_PROPERTY (cmd)
	SPI_write(0x20, 0); // MODEM (group)
	SPI_write(0x01, 0); // 1 (num_props)
	SPI_write(0x51, 0); // MODEM_CLKGEN_BAND (start_prop)
	SPI_write(0b1000 + band, 1); // (data)
	SI4060_CTS_check_and_read(0);
	
	SPI_write(0x11, 0); // SET_PROPERTY (cmd)
	SPI_write(0x40, 0); // FREQ_CONTROL (group)
	SPI_write(0x04, 0); // 4 (num_props)
	SPI_write(0x00, 0); // FREQ_CONTROL_INTE (start_prop)
	SPI_write(n, 0); // (data) - FREQ_CONTROL_INTE
	SPI_write(m2, 0); // (data) - FREQ_CONTROL_FRAC
	SPI_write(m1, 0); // (data) - FREQ_CONTROL_FRAC
	SPI_write(m0, 1); // (data) - FREQ_CONTROL_FRAC
	SI4060_CTS_check_and_read(0);
}

void SI4060_frequency_deviation(uint32_t deviation)
{	
	SPI_write(0x11, 0); // SET_PROPERTY (cmd)
	SPI_write(0x20, 0); // MODEM (group)
	SPI_write(0x03, 0); // 3 (num_props)
	SPI_write(0x0A, 0); // MODEM_FREQ_DEV (start_prop)
	SPI_write((deviation >> 16) & 0xFF, 0); // (data) - MODEM_FREQ_DEV - usually 0x00
	SPI_write((deviation >> 8) & 0xFF, 0); // (data) - MODEM_FREQ_DEV - usually 0x00
	SPI_write(deviation & 0xFF, 1); // (data) - MODEM_FREQ_DEV - good for up to 2500Hz (at 144MHz) or 7700Hz (at 434MHz)
	SI4060_CTS_check_and_read(0);
}

void SI4060_frequency_offset(uint8_t offset1, uint8_t offset2)
{
	SPI_write(0x11, 0); // SET_PROPERTY (cmd)
	SPI_write(0x20, 0); // MODEM (group)
	SPI_write(0x02, 0); // 2 (num_props)
	SPI_write(0x0D, 0); // MODEM_FREQ_OFFSET (start_prop)
	SPI_write(offset1, 0); // (data) - MODEM_FREQ_OFFSET
	SPI_write(offset2, 1); // (data) - MODEM_FREQ_OFFSET
	SI4060_CTS_check_and_read(0);
}

void SI4060_channel_step_size(uint8_t step1, uint8_t step2)
{
	SPI_write(0x11, 0); // SET_PROPERTY (cmd)
	SPI_write(0x40, 0); // FREQ_CONTROL (group)
	SPI_write(0x02, 0); // 2 (num_props)
	SPI_write(0x04, 0); // FREQ_CONTROL_CHANNEL_STEP_SIZE (start_prop)
	SPI_write(step1, 0); // (data) - FREQ_CONTROL_CHANNEL_STEP_SIZE
	SPI_write(step2, 1); // (data) - FREQ_CONTROL_CHANNEL_STEP_SIZE
	SI4060_CTS_check_and_read(0);
}

void SI4060_modulation(uint8_t mod, uint8_t syncasync) // synchronous mode - 0, asynchronous mode - 1
{
	SPI_write(0x11, 0); // SET_PROPERTY (cmd)
	SPI_write(0x20, 0); // MODEM (group)
	SPI_write(0x01, 0); // 1 (num_props)
	SPI_write(0x00, 0); // MODEM_MOD_TYPE (start_prop)
	SPI_write(((syncasync << 7) | (1 << 5) | (1 << 3) | mod), 1); // (data) GPIO1 as the source of data (INPUT), TX Direct Mode
	SI4060_CTS_check_and_read(0);
}

void SI4060_change_state(uint8_t state) // 0x00 No Change, 0x01 SLEEP/STANBY, 0x02 SPI_ACTIVE, 0x03 READY, 0x05 TX_TUNE, 0x07 TX
{
	SPI_write(0x34, 0); // CHANGE_STATE
	SPI_write(state, 1);
	SI4060_CTS_check_and_read(0);
}

void SI4060_start_TX(uint8_t channel)
{
	SPI_write(0x31, 0); // START_TX
	SPI_write(channel, 0); // CHANNEL
	SPI_write((1 << 4) | 0x00, 0); // CONDITION - 0x01 or 0x00 ??, (7 for TX_STATE, 1 for SLEEP_STATE)
	SPI_write(0x00, 0); // TX_LEN
	SPI_write(0x00, 1); // TX_DELAY
	SI4060_CTS_check_and_read(0);
}

void SI4060_power_level(uint8_t val) // default 0x7F to 0x3C - 127 to 60
{
	SPI_write(0x11, 0); // SET_PROPERTY (cmd)
	SPI_write(0x22, 0); // PA (group)
	SPI_write(0x01, 0); // 1 (num_props)
	SPI_write(0x01, 0); // PA_PWR_LVL (start_prop)
	SPI_write(val, 1); // (data)
	SI4060_CTS_check_and_read(0);
}

void SI4060_info(void)
{
	SPI_write(0x01, 1); // PART_INFO
	SI4060_CTS_check_and_read(9);
}


// Interrupt & TC0 -------------------------------------------------------------------------
/*
TC0 - 26400Hz (26400 interrupts per second)
Baud Rate - 1200 (every 22 interrupts)
Slow Sine Wave - 1200Hz (1 full wave length per Baud) -> every 11 interrupts change of HIGH/LOW on PB4 to create a full wave
Fast Sine Wave - 2200Hz (1.83 wave length per Baud) -> every 6 interrupt change of HIGH/LOW on PB4 to create a full wave
Slow Data Rate (SI4060) - 3 x 1200 = 3600
Fast Data Rate (SI4060) - 3 x 2200 = 6600
*/
void TC0_Handler(void)
{
	interruptStatus = TC0->TC_CHANNEL[0].TC_SR; // read the Status Register and clear the interrupt
	
	if(TC0_gfsk_rtty) // GFSK Handler
	{
		GFSKbaud++; // counter for 1200 baud rate
	
		// serves to create the 1200Hz and 2200Hz sine waves -------------
		if(GFSKsineSpeed)
		{
			if(GFSKbaud == 6 || GFSKbaud == 12 || GFSKbaud == 18 || GFSKbaud == 22) // 2200Hz
			{
				if(PIOB->PIO_ODSR && PIO_PB4) PIOB->PIO_CODR |= PIO_PB4; // Clear Output Data Register
				else PIOB->PIO_SODR |= PIO_PB4; // Set Output Data Register
			}
		}else{
			if(GFSKbaud == 11 || GFSKbaud == 22) // 1200Hz
			{
				if(PIOB->PIO_ODSR && PIO_PB4) PIOB->PIO_CODR |= PIO_PB4; // Clear Output Data Register
				else PIOB->PIO_SODR |= PIO_PB4; // Set Output Data Register
			}
		}
	
		// operates the 1200 baud rate -----------------------------------
		if(GFSKbaud >= 22)
		{
			GFSKbaud = 0;
			APRSbit = 1;
		}
	}
	else // RTTY Handler
	{
		switch(TXstate)
		{
			case TXPAUSE:
			TXdone_get_data = 0;
			if(TXpause) TXpause--;
			else{
				TXstate = TXSTARTBIT;
				TXbyte = SI4060_interrupt_TXbuffer[0];
				TXreload = 1;
			}
			break;
			
			case TXSTARTBIT:
			PIOB->PIO_CODR |= PIO_PB4;
			TXstopbits = 1;
			TXstate = TXBYTE;
			TXbit = TXBITS;
			break;
			
			case TXBYTE:
			if(TXbyte & 0x01) PIOB->PIO_SODR |= PIO_PB4;
			else PIOB->PIO_CODR |= PIO_PB4;
			TXbyte >>= 1;
			TXbit--;
			if(!TXbit) TXstate = TXSTOPBIT;
			break;
			
			case TXSTOPBIT:
			if(TXstopbits) {PIOB->PIO_SODR |= PIO_PB4; TXstopbits = 0;}
			else{
				PIOB->PIO_SODR |= PIO_PB4;
				if(TXreload) TXstate = TXRELOAD;
				else TXstate = TXWAIT;
			}
			break;
			
			case TXRELOAD:
			TXbyte = SI4060_interrupt_TXbuffer[TXreload];
			if((TXreload + 1) >= TXmessageLEN || (TXreload + 1) >= TX_BUFFER_SIZE) TXreload = 0;
			else TXreload++;
			TXstate = TXSTARTBIT;
			break;
			
			case TXWAIT:
			TXdone_get_data = 1;
			if(TXdata_ready)
			{
				for(uint32_t i = 0; i < TX_BUFFER_SIZE; i++)
				{
					SI4060_interrupt_TXbuffer[i] = TXbuffer[i];
				}
				TXdata_ready = 0;
				if(TXPAUSE == 0) TXstate = TXPAUSE;
				else {TXstate = TXPAUSE; TXpause = TXPAUSE - 1;}
			}
			break;
			
			default:
			break;
		}
	}
}

void TC0_stop(void)
{
	TC0->TC_CHANNEL[0].TC_IDR = (1 << 4); // disable RC Compare interrupt
	TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKDIS; // disable the clock
}


// OOK -------------------------------------------------------------------------------------
void SI4060_OOK_blips(uint32_t count, uint32_t duration, uint32_t delay) // function using SysTick_delay
{
	SI4060_modulation(1, 1); // OOK, Asynchronous
	SI4060_change_state(7); // TX
	for(uint32_t i = 0; i < count; i++)
	{
		PIOB->PIO_SODR |= PIO_PB4; // Set Output Data Register
		SysTick_delay_ms(duration);
		PIOB->PIO_CODR |= PIO_PB4; // Clear Output Data Register
		SysTick_delay_ms(delay);
	}
	SI4060_change_state(1); // Sleep/Standby
}


// RTTY NORMAL VERSION ---------------------------------------------------------------------
void SI4060_RTTY_TX_string(uint8_t *string)
{
	uint8_t c;
	c = *string++;
	
	while(c != '\0')
	{
		PIOB->PIO_CODR |= PIO_PB4; // clear GPIO1
		SysTick_delay_ms(TXDELAY);
		for(uint8_t i = 0; i < TXBITS; i++)
		{
			if(c & 0x01)
			{
				PIOB->PIO_SODR |= PIO_PB4; // set GPIO1
				}else{
				PIOB->PIO_CODR |= PIO_PB4; // clear GPIO1
			}
			SysTick_delay_ms(TXDELAY);
			c = c >> 1;
		}
		PIOB->PIO_SODR |= PIO_PB4; // set GPIO1
		SysTick_delay_ms(TXDELAY);
		PIOB->PIO_SODR |= PIO_PB4; // set GPIO1
		SysTick_delay_ms(TXDELAY);
		c = *string++;
	}
}


// RTTY INTERRUPT VERSION ------------------------------------------------------------------
#ifdef SI4060_RTTY_INTERRUPT

void TC0_init_RTTY(void)
{
	// disable and clear any pending interrupt input before configuring it
	NVIC_DisableIRQ(TC0_IRQn);
	NVIC_ClearPendingIRQ(TC0_IRQn);
	NVIC_SetPriority(TC0_IRQn, 0);
	
	PMC->PMC_PCER0 |= (1 << ID_TC0); // enable clock to the peripheral
	TC0->TC_CHANNEL[0].TC_CMR = 3 | (1 << 14); // TIMER_CLOCK4, CPCTRG
	TC0->TC_CHANNEL[0].TC_RC = TXBAUD; // compare value
	TC0->TC_CHANNEL[0].TC_IER = (1 << 4); // CPCS enable RC Compare interrupt
	TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG; // enable the clock
	
	// enable interrupt input (system exceptions/interrupts don't use NVIC)
	NVIC_EnableIRQ(TC0_IRQn);
}

#endif // SI4060_RTTY_INTERRUPT


// GFSK ------------------------------------------------------------------------------------
#ifdef SI4060_GFSK

void SI4060_filter_coeffs(void)
{
	//uint8_t filter[9] = {0x01,0x03,0x08,0x11,0x21,0x36,0x4D,0x60,0x67}; // default
	//uint8_t filter[9] = {0x81,0x9F,0xC4,0xEE,0x18,0x3E,0x5C,0x70,0x76}; // UTRAK - 6dB@1200 Hz, 4400 Hz
	//uint8_t filter[9] = {0x1d,0xe5,0xb8,0xaa,0xc0,0xf5,0x36,0x6b,0x7f}; // UTRAK - 6dB@1200 Hz, 2400 Hz
	//uint8_t filter[9] = {0x07,0xde,0xbf,0xb9,0xd4,0x05,0x40,0x6d,0x7f}; // UTRAK - 3db@1200 Hz, 2400 Hz
	//uint8_t filter[9] = {0xfa,0xe5,0xd8,0xde,0xf8,0x21,0x4f,0x71,0x7f}; // UTRAK - LP only, 2400 Hz
	//uint8_t filter[9] = {0xd9,0xf1,0x0c,0x29,0x44,0x5d,0x70,0x7c,0x7f}; // UTRAK - LP only, 4800 Hz
	//uint8_t filter[9] = {0xd5,0xe9,0x03,0x20,0x3d,0x58,0x6d,0x7a,0x7f}; // UTRAK - LP only, 4400 Hz
	//uint8_t filter[9] = {0x19,0x21,0x07,0xC8,0x8E,0x9A,0xFB,0x75,0xAD}; // UBSEDS FIR python
	//uint8_t filter[9] = {7, 10, 2, 238, 218, 222, 255, 40, 59}; // MY FILTER (FIR python) - no pre-emphasis
	uint8_t filter[9] = {6, 10, 6, 244, 224, 224, 251, 32, 50}; // MY FILTER (FIR python) - 6.14dB ripple
	
	SPI_write(0x11, 0); // SET_PROPERTY (cmd)
	SPI_write(0x20, 0); // MODEM (group)
	SPI_write(0x09, 0); // 9 (num_props)
	SPI_write(0x0F, 0); // MODEM_TX_FILTER_COEFF (start_prop)
	SPI_write(filter[8], 0); // (data) - MODEM_TX_FILTER_COEFF_8
	SPI_write(filter[7], 0); // (data) - MODEM_TX_FILTER_COEFF_7
	SPI_write(filter[6], 0); // (data) - MODEM_TX_FILTER_COEFF_6
	SPI_write(filter[5], 0); // (data) - MODEM_TX_FILTER_COEFF_5
	SPI_write(filter[4], 0); // (data) - MODEM_TX_FILTER_COEFF_4
	SPI_write(filter[3], 0); // (data) - MODEM_TX_FILTER_COEFF_3
	SPI_write(filter[2], 0); // (data) - MODEM_TX_FILTER_COEFF_2
	SPI_write(filter[1], 0); // (data) - MODEM_TX_FILTER_COEFF_1
	SPI_write(filter[0], 1); // (data) - MODEM_TX_FILTER_COEFF_0
	SI4060_CTS_check_and_read(0);
}

void SI4060_data_rate(uint8_t data_rate1, uint8_t data_rate2, uint8_t data_rate3)
{
	SPI_write(0x11, 0); // SET_PROPERTY (cmd)
	SPI_write(0x20, 0); // MODEM (group)
	SPI_write(0x04, 0); // 4 (num_props)
	SPI_write(0x06, 0); // MODEM_TX_NCO_MODE (start_prop), Gaussian filter oversampling ratio derived from TCXO's frequency
	SPI_write(0x01, 0); // (data) - MODEM_TX_NCO_MODE
	SPI_write(0xE8, 0); // (data) - MODEM_TX_NCO_MODE
	SPI_write(0x48, 0); // (data) - MODEM_TX_NCO_MODE
	SPI_write(0x00, 1); // (data) - MODEM_TX_NCO_MODE
	SI4060_CTS_check_and_read(0);
	
	// TX_DATA_RATE = (MODEM_DATA_RATE * FXTAL_HZ) / MODEM_TX_NCO_MODE / TXOSR, 1200 = (12000 * 32000000) / 32000000 / 10
	SPI_write(0x11, 0); // SET_PROPERTY (cmd)
	SPI_write(0x20, 0); // MODEM (group)
	SPI_write(0x03, 0); // 3 (num_props)
	SPI_write(0x03, 0); // MODEM_DATA_RATE (start_prop)
	SPI_write(data_rate1, 0); // (data) - MODEM_DATA_RATE
	SPI_write(data_rate2, 0); // (data) - MODEM_DATA_RATE
	SPI_write(data_rate3, 1); // (data) - MODEM_DATA_RATE
	SI4060_CTS_check_and_read(0);
}

void SI4060_setup_GFSK(void)
{
	SI4060_modulation(3, 0); // GFSK, synchronous
	SI4060_frequency_deviation(TX_DEVIATION_APRS); // 0x06E9 18kHz, 0x0625 16kHz
	SI4060_data_rate(0x00, 0x2E, 0xE0); // 13200Hz needed for creating 1200Hz and 2200Hz signal (0203A0 - 13200Hz, 00ABE0 - 4400Hz)
	SI4060_filter_coeffs();
	SI4060_power_level(0x7F); // 0x7F max
	SI4060_change_state(0x07); // TX state
	SI4060_setup_pins(0x02, 0x04, 0x02, 0x02, 0x00, 0x10); // enable TX_DATA_CLK output on SDO
	PMC->PMC_PCER0 |= (1 << ID_PIOA); // enable clock for PIOA
	PIOA->PIO_IER |= (1 << 12); // enable Pin Change interrupt
	PIOA->PIO_AIMER |= (1 << 12); // additional interrupt modes enabled
	PIOA->PIO_ESR |= (1 << 12); // edge interrupt selection
	PIOA->PIO_REHLSR |= (1 << 12); // rising edge interrupt selection
	NVIC_EnableIRQ(PIOA_IRQn);
}
/*
void PIOA_Handler(void) // GFSK - TX_DATA_CLK output on SDO, detects rising edge
{
	if(PIOA->PIO_ISR & (1 << 12))
	{
		// TX bit
		if(PIOB->PIO_ODSR && PIO_PB4) PIOB->PIO_CODR |= PIO_PB4; // Clear Output Data Register
		else PIOB->PIO_SODR |= PIO_PB4; // Set Output Data Register
	}
}
*/
void TC0_init_GFSK(void)
{
	// disable and clear any pending interrupt input before configuring it
	NVIC_DisableIRQ(TC0_IRQn);
	NVIC_ClearPendingIRQ(TC0_IRQn);
	NVIC_SetPriority(TC0_IRQn, 0);
	
	PMC->PMC_PCER0 |= (1 << ID_TC0); // enable clock to the peripheral
	TC0->TC_CHANNEL[0].TC_CMR = 1 | (1 << 14); // TIMER_CLOCK2 (0x01), CPCTRG
	TC0->TC_CHANNEL[0].TC_RC = 303; // compare value: 303 - 26400Hz
	TC0->TC_CHANNEL[0].TC_IER = (1 << 4); // CPCS enable RC Compare interrupt
	TC0->TC_CHANNEL[0].TC_CCR = TC_CCR_CLKEN | TC_CCR_SWTRG; // enable the clock
	
	// enable interrupt input (system exceptions/interrupts don't use NVIC)
	NVIC_EnableIRQ(TC0_IRQn);
}

void SI4060_setup_GFSK_APRS(void)
{
	SI4060_modulation(3, 0); // GFSK, synchronous
	SI4060_frequency_deviation(TX_DEVIATION_APRS); // 0x06E9 18kHz, 0x0625 16kHz, 0x03E8 10.2kHz - UTRAK, 0x189 4kHz, 0x76 1.2kHz, 0x07AE 20kHz
	SI4060_data_rate(0x00, 0x2E, 0xE0); // 12000 for 1200Hz sine wave and 22000 for 2200Hz sine wave
	SI4060_filter_coeffs();
	SI4060_power_level(0x7F); // 0x7F max
	SI4060_change_state(0x07); // TX state
	TC0_init_GFSK(); // setup the GFSK timer running at 26400Hz
	
	// prepare TX
	uint32_t APRSbyte = 0;
	APRSbit = 0;
	uint8_t bitStuffCounter = 0;
	uint8_t APRSpacketReady = 1;
	uint8_t bit = 0;
	uint8_t dataByte = APRSpacket[APRSbyte];
	uint8_t APRSstate = 0;
	
	#ifdef BIT_DEBUG
		uint32_t fff = 0;
		uint8_t bits[1000];
	#endif // BIT_DEBUG
	
	if(APRSpacket[APRSbyte] == 0x7E) APRSstate = 1;
	else APRSstate = 0;
	
	while(APRSpacketReady) // handles switching between 1200Hz and 2200Hz sine waves while TXing APRS packet
	{
		if(SineWaveFast)
		{
			SPI_write(0x11, 0); // SET_PROPERTY (cmd)
			SPI_write(0x20, 0); // MODEM (group)
			SPI_write(0x03, 0); // 3 (num_props)
			SPI_write(0x03, 0); // MODEM_DATA_RATE (start_prop)
			SPI_write(0x00, 0); // (data) - MODEM_DATA_RATE
			SPI_write(0x55, 0); // (data) - MODEM_DATA_RATE 0x55 (with custom FIR)
			SPI_write(0xF0, 1); // (data) - MODEM_DATA_RATE 0xF0 (with custom FIR)
			SI4060_CTS_check_and_read(0);
			
			SineWaveFast = 0;
		}
		
		if(SineWaveSlow)
		{
			SPI_write(0x11, 0); // SET_PROPERTY (cmd)
			SPI_write(0x20, 0); // MODEM (group)
			SPI_write(0x03, 0); // 3 (num_props)
			SPI_write(0x03, 0); // MODEM_DATA_RATE (start_prop)
			SPI_write(0x00, 0); // (data) - MODEM_DATA_RATE
			SPI_write(0x2E, 0); // (data) - MODEM_DATA_RATE 0x2E (with custom FIR)
			SPI_write(0xE0, 1); // (data) - MODEM_DATA_RATE 0xE0 (with custom FIR)
			SI4060_CTS_check_and_read(0);
			
			SineWaveSlow = 0;
		}
		
		if(APRSbit)
		{
			if(dataByte & 0x01) // transmit LSB first
			{
				// in NRZI encoding '1' leaves the SINE WAVE's speed the same
				bitStuffCounter++;
				
				if(bitStuffCounter >= 6) // if there is a sequence of 6 '1' stuff in a '0'
				{
					// in NRZI encoding '0' represents a transition (change in SINE WAVE speed)
					if(APRSstate == 1) // no '0' bit stuffing in 0x7E Flag
					{
						bit++;
						dataByte >>= 1;
						
						#ifdef BIT_DEBUG
							bits[fff++] = 1;
						#endif // BIT_DEBUG
						
					}else if(APRSstate == 0)
					{
						if(GFSKsineSpeed == 0) // 2200Hz
						{
							SineWaveFast = 1;
							GFSKsineSpeed = 1;
						}else{ // 1200Hz
							SineWaveSlow = 1;
							GFSKsineSpeed = 0;
						}
						
						#ifdef BIT_DEBUG
							bits[fff++] = 0;
						#endif // BIT_DEBUG
					}
					bitStuffCounter = 0;
				}else{
					bit++;
					dataByte >>= 1;
					
					#ifdef BIT_DEBUG
						bits[fff++] = 1;
					#endif // BIT_DEBUG
				}
			}else{
				// in NRZI encoding '0' represents a transition (change in SINE WAVE's speed)
				if(GFSKsineSpeed == 0) // 2200Hz
				{
					SineWaveFast = 1;
					GFSKsineSpeed = 1;
				}else{ // 1200Hz
					SineWaveSlow = 1;
					GFSKsineSpeed = 0;
				}
				bitStuffCounter = 0;
				bit++;
				dataByte >>= 1;
				
				#ifdef BIT_DEBUG
					bits[fff++] = 0;
				#endif // BIT_DEBUG
			}

			if(bit >= 8)
			{
				bit = 0;
				APRSbyte++;
				dataByte = APRSpacket[APRSbyte];
				
				if(APRSpacket[APRSbyte] == 0x7E) APRSstate = 1; // flag
				else APRSstate = 0; // normal byte
			}
			
			APRSbit = 0;
		}
		
		if(APRSbyte >= APRS_packet_size) APRSpacketReady = 0; // change (APRS_packet_size - 1)
	}

	TC0_stop();
	SI4060_change_state(0x01); // SPI READY state
	
	#ifdef BIT_DEBUG
		for(uint32_t g = 0; g < fff; g++) udi_cdc_putc(bits[g]);
	#endif // BIT_DEBUG
}

#endif // SI4060_GFSK