/*
GPIO1 - PB4
SDN - PA7 (pull down to GND - active, when driven HIGH - shut down)
TCXOEN - PA8 (drive HIGH to power the TCXO)
NPCS0 - PA11 (automatically HIGH when SPI enabled)
MISO - PA12
MOSI - PA13
SPCK - PA14

Initialization
drive SDN HIGH for minimum of 10us (640 MCK cycles)
then drive SDN LOW again
Power on Reset (POR) requires 10ms to settle the circuit
POWER_UP command is then required to initialize the radio
*/

#ifndef ARM_SI4060_H
#define ARM_SI4060_H

#include "stdint.h"


#define SI4060_RTTY_INTERRUPT
#define SI4060_GFSK

//#define BIT_DEBUG


#define TCXO 32000000UL
#define FREQUENCY 144800000UL
#define FREQUENCY_RTTY 434287000UL
#define FREQUENCY_APRS 144800000UL
// MODEM_FREQ_DEV = (2^19 * outdiv * deviation_Hz) / (Npresc * TCXO_Hz)
#define TX_DEVIATION_RTTY 16 // 434MHz [outdiv 8] (39 - 1200Hz, 26 - 800Hz, 16 - 450Hz)
#define TX_DEVIATION_APRS 1179 // 144MHz [outdiv 24] (1179 - 12000Hz, 865 - 8800Hz, 432 - 4400Hz, 216 - 2200Hz, 118 - 1200Hz)

// RTTY NORMAL VERSION (ms)
#define TXDELAY 5

// RTTY INTERRUPT VERSION
#define TXBAUD 5000 // TC0 compare value with TIMER_CLOCK_4 (10000 - 50 baud, 5000 - 100 baud, 2500 - 200 baud, 1667 - 300 baud, 833 - 600 baud, 417 - 1200 baud)
#define TXTRANSPAUSE 0 // number of TC0 interrupts between transmissions
#define TXBITS 8 // 7 or 8 bits for ASCII characters
#define TXPAUSE 0
#define TXSTARTBIT 1
#define TXBYTE 2
#define TXSTOPBIT 3
#define TXRELOAD 4
#define TXWAIT 5
#define TX_BUFFER_SIZE 330

static volatile uint32_t SI4060_buffer[16]; // buffer for SI4060 response
extern volatile uint8_t TC0_gfsk_rtty; // switch for the TC0_Handler (GFSK = 1, RTTY = 0)

// RTTY INTERRUPT VERSION
static volatile uint8_t SI4060_interrupt_TXbuffer[TX_BUFFER_SIZE]; // interrupt buffer for transmission strings
static volatile uint8_t TXstate = 5;
static volatile uint8_t TXpause = 0;
static volatile uint8_t TXstopbits = 1; // 1 for 2 Stop Bits, 0 for 1 Stop Bit
static volatile uint8_t TXbyte = 0; // byte to TX
static volatile uint8_t TXbit = TXBITS; // number of bits per ASCII character
static volatile uint16_t TXreload; // flag for loading the next byte
extern uint8_t TXbuffer[TX_BUFFER_SIZE];
extern volatile uint32_t TXdone_get_data;
extern volatile uint8_t TXdata_ready; // flag for filling new data to SI4060_interrupt_TXbuffer
extern uint32_t TXmessageLEN;

// GFSK
static volatile uint8_t SineWaveFast = 0;
static volatile uint8_t SineWaveSlow = 0;
extern uint8_t APRSpacket[330];
extern uint16_t APRS_packet_size;
extern volatile uint8_t APRSbit;
static uint32_t interruptStatus = 0;

#ifdef SI4060_GFSK
	static uint8_t GFSKsineSpeed = 0; // 0 - 1200Hz, 1 - 2200Hz
	static uint8_t GFSKbaud = 0;
#endif // SI4060_GFSK


// Functions
void SI4060_CTS_check_and_read(uint8_t response_length);
void SI4060_init(void);
void SI4060_setup_pins(uint8_t gpio0, uint8_t gpio1, uint8_t gpio2, uint8_t gpio3, uint8_t nirq, uint8_t sdo);
void SI4060_frequency(uint32_t freq);
void SI4060_frequency_deviation(uint32_t deviation);
void SI4060_frequency_offset(uint8_t offset1, uint8_t offset2);
void SI4060_channel_step_size(uint8_t step1, uint8_t step2);
void SI4060_modulation(uint8_t mod, uint8_t syncasync);
void SI4060_change_state(uint8_t state);
void SI4060_start_TX(uint8_t channel);
void SI4060_power_level(uint8_t val);
void SI4060_info(void);

void TC0_Handler(void);
void TC0_stop(void);

void SI4060_OOK_blips(uint32_t count, uint32_t duration, uint32_t delay);
void SI4060_RTTY_TX_string(uint8_t *string);

#ifdef SI4060_RTTY_INTERRUPT
	void TC0_init_RTTY(void);
#endif // SI4060_RTTY_INTERRUPT

#ifdef SI4060_GFSK
	void SI4060_filter_coeffs(void);
	void SI4060_data_rate(uint8_t data_rate1, uint8_t data_rate2, uint8_t data_rate3);
	void SI4060_setup_GFSK(void);
	//void PIOA_Handler(void);
	void TC0_init_GFSK(void);
	void SI4060_setup_GFSK_APRS(void);
#endif // SI4060_GFSK

#endif // ARM_SI4060_H_