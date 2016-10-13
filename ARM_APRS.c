/*
 * ARM_APRS.c
 *
 * Created: 21.6.2016 11:41:32
 *  Author: Tomy2
 */ 

#include "sam.h"
#include "math.h"
#include "ARM_APRS.h"

#define lo8(x) ((x)&0xff)
#define hi8(x) ((x)>>8)

uint16_t crc_ccitt_update(uint32_t crc, uint32_t data)
{
	data ^= crc & 0xff;
	data ^= data << 4;
	return ((((uint32_t)data << 8) | ((crc >> 8) & 0xff)) ^	(uint32_t)(data >> 4) ^ ((uint32_t)data << 3));
}

uint16_t crc_ccitt_update_ORIG(uint16_t crc, uint8_t data)
{
	data ^= lo8 (crc);
	data ^= data << 4;
	
	return ((((uint16_t)data << 8) | hi8 (crc)) ^ (uint8_t)(data >> 4) ^ ((uint16_t)data << 3));
}

void Base91_u16_2DEC_encode(uint16_t number, uint8_t *buffer)
{
	if(number > 8280) // maximum acceptable value
	{
		 buffer[num++] = '!'; // decoded as 0
		 buffer[num++] = '!'; // decoded as 0
	}else{
		buffer[num++] = (number / 91) + '0';
		buffer[num++] = (number % 91) + '0';
	}
}

void Base91_u32_encode(uint32_t number, uint8_t *buffer)
{
	uint32_t temp = number;
	uint8_t part1 = 0;
	uint8_t part2 = 0;
	uint8_t part3 = 0;
	uint8_t part4 = 0;
	uint8_t part5 = 0;
	
	part1 = temp / 68574961;
	temp = temp % 68574961;
	part2 = temp / 753571;
	temp = temp % 753571;
	part3 = temp / 8281;
	temp = temp % 8281;
	part4 = temp / 91;
	part5 = temp % 91;
	
	if(number >= 68574961) buffer[num++] = part1 + '0';
	if(number >= 753571) buffer[num++] = part2 + '0';
	if(number >= 8281) buffer[num++] = part3 + '0';
	if(number >= 91) buffer[num++] = part4 + '0';
	buffer[num++] = part5 + '0';
}

void ASCII_8bitfield_transmit_APRS(uint8_t number, uint8_t *buffer)
{
	buffer[num++] = ((number >> 7) & 0x01) + '0'; // bit 7
	buffer[num++] = ((number >> 6) & 0x01) + '0'; // bit 6
	buffer[num++] = ((number >> 5) & 0x01) + '0'; // bit 5
	buffer[num++] = ((number >> 4) & 0x01) + '0'; // bit 4
	buffer[num++] = ((number >> 3) & 0x01) + '0'; // bit 3
	buffer[num++] = ((number >> 2) & 0x01) + '0'; // bit 2
	buffer[num++] = ((number >> 1) & 0x01) + '0'; // bit 1
	buffer[num++] = ((number >> 0) & 0x01) + '0'; // bit 0
}

void ASCII_16bit_transmit_APRS(uint16_t number, uint8_t *buffer, uint8_t ints)
{
	uint8_t ascii1 = 0; // 16bit numbers - 5 decimal places max
	uint8_t ascii2 = 0;
	uint8_t ascii3 = 0;
	uint8_t ascii4 = 0;
	uint8_t ascii5 = 0;
	uint16_t numb = number;
	ascii1 = numb / 10000;
	numb = number;
	ascii2 = (numb - (ascii1 * 10000)) / 1000;
	numb = number;
	ascii3 = (numb - (ascii1 * 10000) - (ascii2 * 1000)) / 100;
	numb = number;
	ascii4 = (numb - (ascii1 * 10000) - (ascii2 * 1000) - (ascii3 * 100)) / 10;
	numb = number;
	ascii5 = (numb - (ascii1 * 10000) - (ascii2 * 1000) - (ascii3 * 100)) % 10;
	ascii1 += '0';
	ascii2 += '0';
	ascii3 += '0';
	ascii4 += '0';
	ascii5 += '0';
	if(ints == 5) buffer[num++] = ascii1;
	if(ints >= 4) buffer[num++] = ascii2;
	if(ints >= 3) buffer[num++] = ascii3;
	if(ints >= 2) buffer[num++] = ascii4;
	if(ints >= 1) buffer[num++] = ascii5;
	else buffer[num++] = 0;
}

void ASCII_16bit_transmit_APRS_auto(uint16_t number, uint8_t *buffer)
{
	uint8_t ascii1 = 0; // 16bit numbers - 5 decimal places max
	uint8_t ascii2 = 0;
	uint8_t ascii3 = 0;
	uint8_t ascii4 = 0;
	uint8_t ascii5 = 0;
	uint16_t numb = number;
	ascii1 = numb / 10000;
	numb = number;
	ascii2 = (numb - (ascii1 * 10000)) / 1000;
	numb = number;
	ascii3 = (numb - (ascii1 * 10000) - (ascii2 * 1000)) / 100;
	numb = number;
	ascii4 = (numb - (ascii1 * 10000) - (ascii2 * 1000) - (ascii3 * 100)) / 10;
	numb = number;
	ascii5 = (numb - (ascii1 * 10000) - (ascii2 * 1000) - (ascii3 * 100)) % 10;
	ascii1 += '0';
	ascii2 += '0';
	ascii3 += '0';
	ascii4 += '0';
	ascii5 += '0';
	if(number >= 10000) buffer[num++] = ascii1;
	if(number >= 1000) buffer[num++] = ascii2;
	if(number >= 100) buffer[num++] = ascii3;
	if(number >= 10) buffer[num++] = ascii4;
	buffer[num++] = ascii5;
}

void ASCII_32bit_LATLON_transmit_APRS(uint32_t number, uint8_t *buffer, uint8_t altitude) // 0 - for lat_dec/lon_dec, 1 - for alt
{
	uint8_t ascii1 = 0; // 32bit numbers - 10 decimal places max
	uint8_t ascii2 = 0;
	uint8_t ascii3 = 0;
	uint8_t ascii4 = 0;
	uint8_t ascii5 = 0;
	uint8_t ascii6 = 0;
	uint8_t ascii7 = 0;
	uint8_t ascii8 = 0;
	uint8_t ascii9 = 0;
	uint8_t ascii10 = 0;
	uint16_t num1 = number % 1000;
	uint16_t num2 = ((number % 1000000) - num1) / 1000;
	uint16_t num3 = ((number % 1000000000) - num1 - num2) / 1000000;
	uint16_t num4 = number / 1000000000;
	ascii1 = num4;
	ascii2 = num3 / 100;
	ascii3 = (num3 - (ascii2 * 100)) / 10;
	ascii4 = (num3 - (ascii2 * 100)) % 10;
	ascii5 = num2 / 100;
	ascii6 = (num2 - (ascii5 * 100)) / 10;
	ascii7 = (num2 - (ascii5 * 100)) % 10;
	ascii8 = num1 / 100;
	ascii9 = (num1 - (ascii8 * 100)) / 10;
	ascii10 = (num1 - (ascii8 * 100)) % 10;
	ascii1 += '0';
	ascii2 += '0';
	ascii3 += '0';
	ascii4 += '0';
	ascii5 += '0';
	ascii6 += '0';
	ascii7 += '0';
	ascii8 += '0';
	ascii9 += '0';
	ascii10 += '0';
	//buffer[order++] = ascii4;
	if(altitude)
	{
		buffer[num++] = ascii5;
		buffer[num++] = ascii6;
		buffer[num++] = ascii7;
		buffer[num++] = ascii8;
		buffer[num++] = ascii9;
		buffer[num++] = ascii10;
	}else{
		buffer[num++] = ascii6;
		buffer[num++] = ascii7;
	}
}

void APRS_base91_SSDV_encode(uint8_t *BASE91buffer, uint8_t *SSDVpacket)
{
	uint32_t numb = 0;
	uint32_t z = 0;
	
	for(uint8_t i = 0; i < 64; i++)
	{
		numb = 0;
		numb = (SSDVpacket[i*4] << 24) | (SSDVpacket[i*4+1] << 16) | (SSDVpacket[i*4+2] << 8) | SSDVpacket[i*4+3];
		BASE91buffer[z++] = (uint8_t)(numb / 68574961) + 33;
		BASE91buffer[z++] = (uint8_t)((numb % 68574961) / 753571) + 33;
		BASE91buffer[z++] = (uint8_t)(((numb % 68574961) % 753571) / 8281) + 33;
		BASE91buffer[z++] = (uint8_t)((((numb % 68574961) % 753571) % 8281) / 91) + 33;
		BASE91buffer[z++] = (uint8_t)((((numb % 68574961) % 753571) % 8281) % 91) + 33;
	}
}

void APRS_SSDV_packet(uint8_t *buffer, uint8_t *packet)
{
	// SSDV packet in the Comment Field
	
	buffer[num++] = '{'; // User-Defined APRS packet format
	//buffer[num++] = '{'; // Experimental (DL7AD)
	//buffer[num++] = 'I'; // Image (DL7AD)
	
	for(uint16_t i = 0; i < 50; i++) buffer[num++] = packet[i];
	//for(uint16_t i = 33; i < 62; i++) buffer[num++] = i;
	//for(uint16_t i = 63; i < 95; i++) buffer[num++] = i;
	//for(uint16_t i = 96; i < 124; i++) buffer[num++] = i;
}

void APRS_time_short(uint8_t *buffer, uint8_t hour, uint8_t minute, uint8_t second)
{
	// Format: "HHMMSSh"
	
	buffer[num++] = (hour / 10) + '0';
	buffer[num++] = (hour % 10) + '0';
	buffer[num++] = (minute / 10) + '0';
	buffer[num++] = (minute % 10) + '0';
	buffer[num++] = (second / 10) + '0';
	buffer[num++] = (second % 10) + '0';
	buffer[num++] = 'h'; // 24h GMT
}

void APRS_time_mid(uint8_t *buffer, uint8_t hour, uint8_t minute, uint8_t day)
{
	// Format: "DDHHMMz"
	
	buffer[num++] = (day / 10) + '0';
	buffer[num++] = (day % 10) + '0';
	buffer[num++] = (hour / 10) + '0';
	buffer[num++] = (hour % 10) + '0';
	buffer[num++] = (minute / 10) + '0';
	buffer[num++] = (minute % 10) + '0';
	buffer[num++] = 'z'; // 24h ZULU (UTC)
}

void APRS_time_long(uint8_t *buffer, uint8_t hour, uint8_t minute, uint8_t month, uint8_t day)
{
	// Format: "MMDDhhmm"
	
	buffer[num++] = (month / 10) + '0';
	buffer[num++] = (month % 10) + '0';
	buffer[num++] = (day / 10) + '0';
	buffer[num++] = (day % 10) + '0';
	buffer[num++] = (hour / 10) + '0';
	buffer[num++] = (hour % 10) + '0';
	buffer[num++] = (minute / 10) + '0';
	buffer[num++] = (minute % 10) + '0';
}

void APRS_position_uncompressed(uint8_t *buffer, uint16_t lat_int, uint16_t lon_int, uint32_t lat_dec, uint32_t lon_dec, uint8_t NS, uint8_t EW, uint32_t alt)
{
	// Uncompressed format: "YYYY.YYy/XXXXX.XXxO/A=FFFFFF"
		
	ASCII_16bit_transmit_APRS(lat_int, buffer, 4); // Latitude
	buffer[num++] = '.';
	ASCII_32bit_LATLON_transmit_APRS(lat_dec, buffer, 0);
	if(NS == 0) buffer[num++] = 'S'; // South / North
	else if(NS == 1) buffer[num++] = 'N';
	else buffer[num++] = '?';
	
	buffer[num++] = '/'; // Symbol Table Identifier
	
	ASCII_16bit_transmit_APRS(lon_int, buffer, 5); // Longitude
	buffer[num++] = '.';
	ASCII_32bit_LATLON_transmit_APRS(lon_dec, buffer, 0);
	if(EW == 0) buffer[num++] = 'W'; // East / West
	else if(EW == 1) buffer[num++] = 'E';
	else buffer[num++] = '?';
	
	buffer[num++] = 'O'; // Symbol Code for Balloon
	
	// Altitude - has to be in the Comment Text and in feets
	uint32_t altitude = alt * 328;
	altitude /= 100;
	
	buffer[num++] = '/';
	buffer[num++] = 'A';
	buffer[num++] = '=';
	ASCII_32bit_LATLON_transmit_APRS(altitude, buffer, 1); // doesn't do negative altitude (don't know the APRS format for that)
}

void APRS_position_base91(uint8_t *buffer, float lat, float lon, float alt, uint8_t showAlt) // 21.313287, -157.853138, 12756
{
	// Compressed Format: "/YYYYXXXX$csT"
	// base91: the result is ASCII characters between 33-124 (! through |)
	
	lat = (90.0f - lat) * 380926.0f;
	lon = (180.0f + lon) * 190463.0f;
	alt = log((float)alt * 3.28f) / log(1.002f);
	alt = (uint16_t)alt % 8281;
		
	buffer[num++] = '/'; // the Symbol Table Identifier
	
	buffer[num++] = (lat / 753571) + 33; // Latitude
	lat = (uint32_t)lat % 753571;
	buffer[num++] = (lat / 8281) + 33;
	lat = (uint32_t)lat % 8281;
	buffer[num++] = (lat / 91) + 33;
	buffer[num++] = ((uint32_t)lat % 91) + 33;
	
	buffer[num++] = (lon / 753571) + 33; // Longitude
	lon = (uint32_t)lon % 753571;
	buffer[num++] = (lon / 8281) + 33;
	lon = (uint32_t)lon % 8281;
	buffer[num++] = (lon / 91) + 33;
	buffer[num++] = ((uint32_t)lon % 91) + 33;
	
	buffer[num++] = 'O'; // the Symbol Code (Balloon)
	
	if(showAlt)
	{
		buffer[num++] = (alt / 91) + 33; // Altitude
		buffer[num++] = ((uint16_t)alt % 91) + 33;
		buffer[num++] = 0x57; // the Compression Type Identifier - 00110110 -> 54 + 33 -> 0x57 'W'
	}else{
		buffer[num++] = ' '; // if 'c' byte is space ' ', csT bytes are ignored
		buffer[num++] = ' ';
		buffer[num++] = ' ';
	}
}

void APRS_telemetry_uncompressed(uint8_t *buffer, uint16_t sequence, int16_t value1, int16_t value2, int16_t value3, int16_t value4, int16_t value5, uint8_t bitfield)
{
	// Uncompressed Format: "T#xxx,aaa,aaa,aaa,aaa,aaa,bbbbbbbb"
	
	buffer[num++] = 'T'; // APRS Data Type Identifier (Telemetry Data)
	buffer[num++] = '#';
	
	ASCII_16bit_transmit_APRS_auto(sequence, buffer); // "xxx" - sequence
	buffer[num++] = ',';
	
	if(value1 < 0)
	{
		buffer[num++] = '-';
		value1 = 65535 - value1 + 1;
	}
	ASCII_16bit_transmit_APRS_auto((uint16_t)value1, buffer); // "aaa" - value 1
	buffer[num++] = ',';
	
	if(value2 < 0)
	{
		buffer[num++] = '-';
		value2 = 65535 - value2 + 1;
	}
	ASCII_16bit_transmit_APRS_auto((uint16_t)value2, buffer); // "aaa" - value 2
	buffer[num++] = ',';
	
	if(value3 < 0)
	{
		buffer[num++] = '-';
		value3 = 65535 - value3 + 1;
	}
	ASCII_16bit_transmit_APRS_auto((uint16_t)value3, buffer); // "aaa" - value 3
	buffer[num++] = ',';
	
	if(value4 < 0)
	{
		buffer[num++] = '-';
		value4 = 65535 - value4 + 1;
	}
	ASCII_16bit_transmit_APRS_auto((uint16_t)value4, buffer); // "aaa" - value 4
	//buffer[num++] = ',';
	
	//if(value5 < 0)
	//{
		//buffer[num++] = '-';
		//value5 = 65535 - value5 + 1;
	//}
	//ASCII_16bit_transmit_APRS_auto((uint16_t)value5, buffer); // "aaa" - value 5
	//buffer[num++] = ',';
	
	//ASCII_8bitfield_transmit_APRS(bitfield, buffer); // "bbbbbbbb" - 8 bitfield
}

void APRS_telemetry_base91(uint8_t *buffer, uint32_t sequence, uint16_t value1, uint16_t value2, uint16_t value3, uint16_t value4, uint16_t value5, uint8_t bitfield)
{
	// Compressed Format: "|ssaaaaaaaaaadd|"
	// Max VALUE = 8280
	uint16_t seq = sequence & 0x1FFF;
	
	buffer[num++] = '|';
	Base91_u16_2DEC_encode(seq, buffer); // "ss" - sequence
	Base91_u16_2DEC_encode(value1, buffer); // "aa" - value 1
	Base91_u16_2DEC_encode(value2, buffer); // "aa" - value 2
	Base91_u16_2DEC_encode(value3, buffer); // "aa" - value 3
	Base91_u16_2DEC_encode(value4, buffer); // "aa" - value 4
	Base91_u16_2DEC_encode(value5, buffer); // "aa" - value 5
	//Base91_u16_2DEC_encode(bitfield, buffer); // "dd" - 8 bitfield
	buffer[num++] = '|';
}

void APRS_telemetry_PARM(uint8_t *buffer, char *string, char *addressee)
{
	// Format: ":call-ssid:PARM."
	
	buffer[num++] = ':';
	
	// Addressee
	for(uint8_t i = 0; i < 9; i++)
	{
		buffer[num++] = *addressee++;
	}
		
	buffer[num++] = ':';
	buffer[num++] = 'P';
	buffer[num++] = 'A';
	buffer[num++] = 'R';
	buffer[num++] = 'M';
	buffer[num++] = '.';
	
	while(*string)
	{
		buffer[num++] = *string++;
	}
}

void APRS_telemetry_UNIT(uint8_t *buffer, char *string, char *addressee)
{
	// Format: ":call-ssid:UNIT."
	
	buffer[num++] = ':';
	
	// Addressee
	for(uint8_t i = 0; i < 9; i++)
	{
		buffer[num++] = *addressee++;
	}
	
	buffer[num++] = ':';
	buffer[num++] = 'U';
	buffer[num++] = 'N';
	buffer[num++] = 'I';
	buffer[num++] = 'T';
	buffer[num++] = '.';
	
	while(*string)
	{
		buffer[num++] = *string++;
	}
}

void APRS_telemetry_EQNS(uint8_t *buffer, char *string, char *addressee)
{
	// Format: ":call-ssid:EQNS."
	
	buffer[num++] = ':';
	
	// Addressee
	for(uint8_t i = 0; i < 9; i++)
	{
		buffer[num++] = *addressee++;
	}
	
	buffer[num++] = ':';
	buffer[num++] = 'E';
	buffer[num++] = 'Q';
	buffer[num++] = 'N';
	buffer[num++] = 'S';
	buffer[num++] = '.';
	
	while(*string)
	{
		buffer[num++] = *string++;
	}
}

void APRS_telemetry_BITS(uint8_t *buffer, char *string, char *addressee)
{
	// Format: ":call-ssid:BITS."
	
	buffer[num++] = ':';
	
	// Addressee
	for(uint8_t i = 0; i < 9; i++)
	{
		buffer[num++] = *addressee++;
	}
	
	buffer[num++] = ':';
	buffer[num++] = 'B';
	buffer[num++] = 'I';
	buffer[num++] = 'T';
	buffer[num++] = 'S';
	buffer[num++] = '.';
	
	while(*string)
	{
		buffer[num++] = *string++;
	}
}

void APRS_comment(uint8_t *buffer, char *string)
{
	while(*string)
	{
		buffer[num++] = *string++;
	}
}

void APRS_comment_altitude(uint8_t *buffer, uint32_t alt)
{
	// Altitude will be expressed in feets (input is in meters)
	uint32_t altitude = alt * 328;
	altitude /= 100;
	
	buffer[num++] = '/';
	buffer[num++] = 'A';
	buffer[num++] = '=';
	ASCII_32bit_LATLON_transmit_APRS(altitude, buffer, 1); // doesn't do negative altitude (don't know the APRS format for that)
}

void APRS_packet_construct(uint8_t *buffer)
{
	num = 0;
	
	// Flags
	for(uint8_t i = 0 ; i < APRSFLAGS; i++)
	{
		buffer[num++] = 0x7E;
	}
	
	// Destination Address
	for(uint8_t i = 0; i < 6; i++)
	{
		buffer[num++] = APRS_destination[i];
	}
	
	// SSID of the destination
	buffer[num++] = 0b00110000 | DSSID; // 0b0CRRSSID (C - command/response bit '1', RR - reserved '11', SSID - 0-15)
	
	// Source Address
	for(uint8_t i = 0; i < 6; i++)
	{
		buffer[num++] = APRS_callsign[i];
	}
	
	// SSID to specify an APRS symbol (11 - balloon)
	buffer[num++] = 0b01110000 | SSID; // 0b0CRRSSID (C - command/response bit '1', RR - reserved '11', SSID - 0-15)
	
	// Path
	if(APRS_send_path)
	{
		buffer[num++] = 'W';
		buffer[num++] = 'I';
		buffer[num++] = 'D';
		buffer[num++] = 'E';
		buffer[num++] = '2'; // "do not use WIDE1-1"
		buffer[num++] = ' ';
	
		// SSID
		buffer[num++] = 0b00110001; // 0b0HRRSSID (H - 'has been repeated' bit, RR - reserved '11', SSID - 0-15)
	}
	
	// Left Shifting the Address Bytes
	for(uint8_t i = APRSFLAGS; i < num; i++)
	{
		buffer[i] <<= 1;
		if(i == (num - 1)) buffer[i] |= 0x01; // LSB '1' to indicate the end of the address fields
	}
	
	// Control Field
	buffer[num++] = 0x03;
	
	// Protocol ID
	buffer[num++] = 0xF0;
	
	// Information Field
	/*
	APRS Data Type Identifier (1 byte)
	APRS Data (n bytes)
	APRS Data Extension (7 bytes)
	Comment (n bytes)
	
	Possible Formates:
		"/HHMMSShYYYY.YYy/XXXXX.XXxO/A=FFFFFFcomment"				(timestamp, uncompressed position, altitude, comment)
		"/HHMMSSh/YYYYXXXXOAAWcomment"								(timestamp, base91 position, comment)
		"/DDHHMMzYYYY.YYy/XXXXX.XXxO/A=FFFFFF"						(timestampD, uncompressed position, altitude)
		"/DDHHMMz/YYYYXXXXOAAW"										(timestampD, base91 position)
		":call-ssid:PARM."											(telemetry parameters)
		":call-ssid:UNIT."											(telemetry units)
		":call-ssid:EQNS."											(telemetry equations)
		":call-ssid:BITS."											(telemetry bits and message)
		"T#sss,val1,val2,val3,val4"									(uncompressed telemetry)
		"/HHMMSSh/YYYYXXXXO   /A=FFFFFF"							(timestamp, base91 position, altitude in comment)
		"/HHMMSSh/YYYYXXXXO   /A=FFFFFFT#sss,val1,val2,val3,val4"	(timestamp, base91 position, altitude in comment, telemetry in comment - unparsed)
		
	Doesn't work:
		"!YYYY.YYy/XXXXX.XXxO/A=FFFFFF|ssaaaaaaaaaadd|"					(without timestamp, uncompressed position, altitude, base91 telemetry)
		"!/YYYYXXXXOAAW|ssaaaaaaaaaadd|"								(without timestamp, base91 position, base91 telemetry)
		"!YYYYXXXXOAAW"													(without timestamp, base91 position without Symbol Table Identifier)
		"!/YYYYXXXXOAAW"												(without timestamp, base91 position)
		"!YYYY.YYy/XXXXX.XXxO/A=FFFFFF"									(without timestamp, uncompressed position)
		"!/YYYYXXXXOAAW/|ssaaaaaaaaaadd|"								(without timestamp, base91 position, added '/', base91 telemetry)
		"/HHMMSSh/YYYYXXXXOAAW|ssaaaaaaaaaadd|"							(timestamp, base91 position, base91 telemetry)
		"/HHMMSShYYYY.YYy/XXXXX.XXxO/A=FFFFFF|ssaaaaaaaaaadd|"			(timestamp, uncompressed position, altitude, base91 telemetry)
		"/MMDDhhmmYYYY.YYy/XXXXX.XXxO/A=FFFFFF"							(timestampMD, uncompressed position, altitude)
		"/MMDDhhmm/YYYYXXXXOAAW"										(timestampMD, base91 position)
		"/HHMMSSh/YYYYXXXXOAAWcomment|ssaaaaaaaaaadd|"					(timestamp, base91 position, comment, base91 telemetry)
		"/HHMMSShYYYY.YYy/XXXXX.XXxO/A=FFFFFFcomment|ssaaaaaaaaaadd|"	(timestamp, uncompressed position, altitude, comment, base91 telemetry)
		"/HHMMSSh/YYYYXXXXOAAW/comment|ssaaaaaaaaaadd|"					(timestamp, base91 position, /comment, base91 telemetry)
		"/HHMMSShYYYY.YYy/XXXXX.XXxO/A=FFFFFF/comment|ssaaaaaaaaaadd|"	(timestamp, uncompressed position, altitude, /comment, base91 telemetry)
	*/
	if(APRS_packet_mode == 1)
	{
		/*
		Telemetry parsed. Will send direct ADC sample values. Conversion to °C after RX (for negative values).
		*/
		APRS_telemetry_uncompressed(buffer, APRSSequence, APRSValue1, APRSValue2, APRSValue3, APRSValue4, APRSValue5, APRSBitfield);
	}
	else if(APRS_packet_mode == 2)
	{
		APRS_SSDV_packet(buffer, pointerSSDVpacket);
	}
	else if(APRS_packet_mode == 3)
	{
		/*
		Telemetry not parsed after RX.
		*/
		buffer[num++] = '/'; // Data Type Identifier (Report with Timestamp and without APRS Messaging)
		APRS_time_short(buffer, APRSHour, APRSMinute, APRSSecond);
		APRS_position_base91(buffer, APRSLatitude, APRSLongitude, APRSAltitude, APRS_show_alt_B91);
		APRS_comment_altitude(buffer, APRSAltitude);
		APRS_telemetry_uncompressed(buffer, APRSSequence, APRSValue1, APRSValue2, APRSValue3, APRSValue4, APRSValue5, APRSBitfield);
		//APRS_position_uncompressed(buffer, APRSlat_int, APRSlon_int, APRSlat_dec, APRSlon_dec, APRSlatNS, APRSlonEW, APRSAltitude);
		//APRS_telemetry_base91(buffer, APRSSequence, APRSValue1, APRSValue2, APRSValue3, APRSValue4, APRSValue5, APRSBitfield);
	}
	else if(APRS_packet_mode == 4)
	{
		buffer[num++] = '/'; // Data Type Identifier (Report with Timestamp and without APRS Messaging)
		APRS_time_short(buffer, APRSHour, APRSMinute, APRSSecond);
		APRS_position_base91(buffer, APRSLatitude, APRSLongitude, APRSAltitude, APRS_show_alt_B91);
		//APRS_comment_altitude(buffer, APRSAltitude);
		//APRS_telemetry_uncompressed(buffer, APRSSequence, APRSValue1, APRSValue2, APRSValue3, APRSValue4, APRSValue5, APRSBitfield);
		//APRS_time_mid(buffer, APRSHour, APRSMinute, APRSDay);
		APRS_comment(buffer, "/Test comment|!!!!|");
		//APRS_telemetry_base91(buffer, APRSSequence, APRSValue1, APRSValue2, APRSValue3, APRSValue4, APRSValue5, APRSBitfield);
	}
	else if(APRS_packet_mode == 5)
	{
		buffer[num++] = '/'; // Data Type Identifier (Report with Timestamp and without APRS Messaging)
		APRS_time_short(buffer, APRSHour, APRSMinute, APRSSecond);
		APRS_position_base91(buffer, 49.472537, 18.033199, 330, APRS_show_alt_B91);
		APRS_comment_altitude(buffer, 330);
		buffer[num++] = ' ';
		ASCII_16bit_transmit_APRS(APRSDay, buffer, 2);
		buffer[num++] = '/';
		ASCII_16bit_transmit_APRS(APRSMonth, buffer, 2);
		buffer[num++] = '/';
		ASCII_16bit_transmit_APRS(APRSYear, buffer, 4);
		//APRS_time_mid(buffer, APRSHour, APRSMinute, APRSDay);
		//APRS_position_uncompressed(buffer, 4933, 1748, 28500, 54300, 1, 1, 361);
		//APRS_comment(buffer, "/Test comment|!!!!|");
		//APRS_telemetry_base91(buffer, APRSSequence, APRSValue1, APRSValue2, APRSValue3, APRSValue4, APRSValue5, APRSBitfield);
	}
	else if(APRS_packet_mode == 6)
	{
		APRS_telemetry_PARM(buffer, "Vsolar,Vbatt,Tcpu,Sats,-", "OK7DMT-11");
	}
	else if(APRS_packet_mode == 7)
	{
		APRS_telemetry_UNIT(buffer, "V,V,C,n,-", "OK7DMT-11");
	}
	else if(APRS_packet_mode == 8)
	{
		APRS_telemetry_EQNS(buffer, "0,0.0008,0,0,0.0016,0,0,0.304,-273.15,0,1,0,0,1,0", "OK7DMT-11");
	}
	else if(APRS_packet_mode == 9)
	{
		APRS_telemetry_BITS(buffer, "11111111,High Altitude Balloon", "OK7DMT-11");
	}
	else if(APRS_packet_mode == 10)
	{
		APRS_comment(buffer, "!/5IO6R{WjO   ");
	}
	else if(APRS_packet_mode == 11)
	{
		APRS_comment(buffer, "=/)tqfQe8`O   ");
	}
	else if(APRS_packet_mode == 12)
	{
		APRS_comment(buffer, "@/)tqfQe8`O   ");
	}
	else if(APRS_packet_mode == 13)
	{
		APRS_comment(buffer, "//)tqfQe8`O   ");
	}
	else if(APRS_packet_mode == 14)
	{
		APRS_comment(buffer, "=/)tqfQe8`O   |jJLkS+=z@h!/|");
	}
	else // APRS_packet_mode == 0
	{
		/*
		"/HHMMSShYYYY.YYy/XXXXX.XXxO/A=FFFFFFcomment"
		Altitude is already a part of the Comment section.
		*/
		buffer[num++] = '/'; // Data Type Identifier (Report with Timestamp and without APRS Messaging)
		APRS_time_short(buffer, APRSHour, APRSMinute, APRSSecond);
		APRS_position_uncompressed(buffer, 4933, 1748, 28500, 54300, 1, 1, 361);
		APRS_comment(buffer, "Test Comment"); // Comment - max 43 characters after Symbol Code
	}
	
	// Frame Check Sequence - CRC-16-CCITT (0xFFFF)
	uint16_t crc = 0xFFFF;
	for(uint16_t i = 0; i < (num - APRSFLAGS); i++) crc = crc_ccitt_update_ORIG(crc, buffer[APRSFLAGS+i]);
	crc = ~crc;
	buffer[num++] = crc & 0xFF; // FCS is sent low-byte first
	buffer[num++] = (crc >> 8) & 0xFF; // and with the bits flipped
	
	// the end Flags
	buffer[num++] = 0x7E;
	buffer[num++] = 0x7E;
	buffer[num++] = 0x7E;
	
	APRS_packet_size = num; // packet length
}