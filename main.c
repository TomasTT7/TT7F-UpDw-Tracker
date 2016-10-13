/*
 * ARM_UPDOWN_1.c
 *
 * Created: 7.8.2016 20:30:13
 *  Author: Tomy2
 */ 

#include "sam.h"
#include "ARM_ADC.h"
#include "ARM_APRS.h"
#include "ARM_DELAY.h"
#include "ARM_LED.h"
#include "ARM_SI4060.h"
#include "ARM_SPI.h"
#include "ARM_UART.h"
#include "ARM_UBLOX.h"

// $$callsign,telemCount,GPShour:GPSminute:GPSsecond,GPSlat_int.GPSlat_dec,GPSlon_int.GPSlon_dec,GPSalt,GPSsats,GPSfix,AD9,temperatureF,GPSerror*crc
// $$TT7F,26,15:04:06,4928.12509,1809.05291,411,4,1,3768,24,`*869C

#define GPS_SETTINGS_INTERVAL 20 // number of telemetry strings between NAV mode checks
#define APRS_TELEMETRY_INTERVAL 8 // number of RTTY telemetry strings between APRS packets


// ADC -------------------------------------------------------------------------------------
uint16_t AD3data = 0;
uint16_t AD9data = 0;
uint16_t AD15data = 0;


// APRS ------------------------------------------------------------------------------------
uint8_t APRSHour = 0;
uint8_t APRSMinute = 0;
uint8_t APRSSecond = 0;
uint8_t APRSDay = 0;
uint8_t APRSMonth = 0;
uint16_t APRSYear = 0;
float APRSLatitude = 0.0;
float APRSLongitude = 0.0;
uint16_t APRSAltitude = 0;
uint32_t APRSSequence = 0;
uint16_t APRSValue1 = 0;
uint16_t APRSValue2 = 0;
uint16_t APRSValue3 = 0;
uint16_t APRSValue4 = 0;
uint16_t APRSValue5 = 0;
uint16_t APRSBitfield = 0;
uint16_t APRSlat_int = 0;
uint16_t APRSlon_int = 0;
uint32_t APRSlat_dec = 0;
uint32_t APRSlon_dec = 0;
uint8_t APRSlatNS = 0;
uint8_t APRSlonEW = 0;
uint8_t APRSpacket[330]; // max APRS packet size + flags (Destination 7, Source 7, Digipeater 0-56, Control 1, Protocol ID 1, Information 1-256, FCS 2)
uint8_t APRS_packet_mode = 0;
uint8_t APRS_send_path = 0; // 0 - don't send, 1 - send WIDE2-1
uint8_t APRS_show_alt_B91 = 0; // 0 - don't show, 1 - show
uint8_t *pointerSSDVpacket;
uint16_t APRS_packet_size = 0;
volatile uint8_t APRSbit;


// SI4060 ----------------------------------------------------------------------------------
volatile uint32_t TXdone_get_data = 1;
volatile uint8_t TXdata_ready = 0; // flag for filling new data to SI4060_interrupt_TXbuffer
volatile uint8_t TC0_gfsk_rtty = 0; // switch for the TC0_Handler (GFSK = 1, RTTY = 0)
uint32_t TXmessageLEN = 0;
uint8_t TXbuffer[TX_BUFFER_SIZE];


// UART ------------------------------------------------------------------------------------
volatile uint8_t UART0_RX_buffer[UART0_BUFFER_SIZE];
volatile uint8_t UART1_RX_buffer[UART1_BUFFER_SIZE];
volatile uint32_t UART0_buffer_pointer;
volatile uint32_t UART1_buffer_pointer;
volatile uint32_t UART0_temp;
volatile uint32_t UART1_temp;


// UBLOX -----------------------------------------------------------------------------------
uint8_t GPShour = 0;
uint8_t GPSminute = 0;
uint8_t GPSsecond = 0;
uint8_t GPSday = 0;
uint8_t GPSmonth = 0;
uint16_t GPSyear = 0;
uint8_t GPSsats = 0;
uint8_t GPSfix = 0;
int16_t GPSlat_int = 0;
int16_t GPSlon_int = 0;
int16_t GPSlat_int_L = 0;
int16_t GPSlon_int_L = 0;
int32_t GPSlat_dec = 0;
int32_t GPSlon_dec = 0;
int32_t GPSlat_dec_L = 0;
int32_t GPSlon_dec_L = 0;
int32_t GPSalt = 0;
int32_t GPSalt_L = 0;
int32_t GPSlat = 0;
int32_t GPSlon = 0;
int32_t GPSlat_L = 0;
int32_t GPSlon_L = 0;
uint8_t GPSlonEW = 0;
uint8_t GPSlatNS = 0;
uint8_t GPSnavigation = 0;
uint8_t GPSpowermode = 0;
uint8_t GPSerror = 0b00100000;
uint32_t telemCount = 0;
uint32_t telemetry_len = 0;


// MAIN ------------------------------------------------------------------------------------
int main(void)
{
    SystemInit();
	WDT->WDT_MR = WDT_MR_WDDIS; // disable WatchDog (WatchDog peripheral enabled by default)
	SysTick_delay_init(); // configure the delay timer
	
	UART1_init(); // communication to UBLOX
	SPI_init(); // communication to SI4060
	ADC_init(); // prepare the ADC pins
	SI4060_init(); // setup the radio, TCXO and PIO lines
	
	// prepare the LEDs
	LED_PA0_init();
	LED_PB5_init();
	LED_PA0_disable();
	LED_PB5_disable();
	
	SysTick_delay_ms(500); // give some time to the GPS module to come alive

    while (1) 
    {
		// once in a while re-initialize the GPS settings
		if(telemCount % GPS_SETTINGS_INTERVAL == 0)
		{
			// -----------------------
			UBLOX_request_UBX(request0624, 8, 44, UBLOX_parse_0624);
			// -----------------------
			UBLOX_send_message(setNMEAoff, 28);
			SysTick_delay_ms(100);
			UBLOX_send_message(setNAVmode, 44);
			SysTick_delay_ms(100);
		}
		
		// update GPS and ADC data
		UBLOX_send_message(requestGNGGA, 15); // GPGGA or GNGGA depending on the previous GPS settings
		UBLOX_fill_buffer_NMEA(GPSbuffer);
		UBLOX_process_GGA(GPSbuffer);
		
		ADC_start();
		//AD3data = ADC_sample(3);
		AD9data = ADC_sample(9);
		AD15data = ADC_sample_temperature();
		ADC_stop();
		
		// decide if it is time for the next TX
		if(TXdone_get_data)
		{
			if(telemCount % APRS_TELEMETRY_INTERVAL == 0) // TX APRS packet
			{
				// Initialize Si4060 for APRS
				TC0_stop();
				SI4060_change_state(0x01); // SPI READY state
				TC0_gfsk_rtty = 1; // GFSK
				
				SI4060_init(); // setup the radio, TCXO and PIO lines
				SI4060_setup_pins(0x02, 0x04, 0x02, 0x02, 0x00, 0x00);
				SI4060_frequency_offset(0x00, 0x00);
				SI4060_frequency(FREQUENCY_APRS);
				
				// Update the APRS data
				APRSSequence++;
				APRSHour = GPShour;
				APRSMinute = GPSminute;
				APRSSecond = GPSsecond;
				APRSDay = GPSday;
				APRSMonth = GPSmonth;
				APRSYear = GPSyear;
				APRSAltitude = GPSalt;
				APRSlat_int = GPSlat_int;
				APRSlon_int = GPSlon_int;
				APRSlat_dec = GPSlat_dec;
				APRSlon_dec = GPSlon_dec;
				APRSlatNS = GPSlatNS;
				APRSlonEW = GPSlonEW;
				
				// not sending additional APRS telemetry messages -> convert right away
				//uint32_t AD3 = ((uint32_t)AD3data * 3300) / 4096; // convert ADC reading to mV
				uint32_t AD9 = ((uint32_t)AD9data * 6600) / 4096; // convert ADC reading to mV
				float temperatureF = (float)AD15data * 0.30402 - 274.887; // convert ADC reading to °C
				APRSValue1 = 0; // no solar this time
				APRSValue2 = AD9;
				APRSValue3 = (int16_t)temperatureF;
				APRSValue4 = GPSsats;
				APRSValue5 = GPSfix;
				APRSBitfield = 0;
				
				// convert the Latitude and Longitude values when using NMEA
				Coords_DEGtoDEC(APRSlat_int, APRSlat_dec, APRSlon_int, APRSlon_dec, APRSlatNS, APRSlonEW, APRSLatitude, APRSLongitude);
				
				// construct the APRS packet
				APRS_packet_mode = 3;
				APRS_send_path = 1;
				APRS_show_alt_B91 = 0;
				APRS_packet_construct(APRSpacket);
				
				LED_PA0_blink(5); // blink the lower LED
				
				SI4060_setup_GFSK_APRS();
				
				// Initialize Si4060 for RTTY
				TC0_gfsk_rtty = 0; // RTTY
				
				SI4060_init(); // setup the radio, TCXO and PIO lines
				SI4060_setup_pins(0x02, 0x04, 0x02, 0x02, 0x00, 0x00);
				SI4060_frequency_offset(0x00, 0x00);
				SI4060_frequency(FREQUENCY_RTTY);
				SI4060_modulation(2, 1); // FSK, asynchronous
				SI4060_frequency_deviation(TX_DEVIATION_RTTY);
				SI4060_power_level(0x7F); // 0x7F max
				SI4060_change_state(0x07); // TX state
				TC0_init_RTTY();
				SysTick_delay_ms(100);
			}
			
			// TX RTTY packet
			telemCount++;
			TX_BUFFER_fill_NMEA(TXbuffer);
			TXmessageLEN = telemetry_len;
			
			LED_PB5_blink(5); // blink the upper LED
			
			TXdata_ready = 1;
		}
    }
}
