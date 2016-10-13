/*
BAUD RATE
VALUE = 64000000 / (BAUD RATE * 16)
9600 - 417
19200 - 208
38400 - 104
57600 - 69
115200 - 35
250000 - 16
500000 - 8
1000000 - 4
*/

#ifndef ARM_UART_H
#define ARM_UART_H

#include "stdint.h"

#define UART0_BAUD 16
#define UART1_BAUD 417
#define UART0_BUFFER_SIZE 100
#define UART1_BUFFER_SIZE 100

extern volatile uint8_t UART0_RX_buffer[UART0_BUFFER_SIZE];
extern volatile uint8_t UART1_RX_buffer[UART1_BUFFER_SIZE];
extern volatile uint32_t UART0_buffer_pointer;
extern volatile uint32_t UART1_buffer_pointer;
extern volatile uint32_t UART0_temp;
extern volatile uint32_t UART1_temp;

// Functions
void UART0_init(void);
void UART1_init(void);
void UART0_TX(uint8_t data);
void UART1_TX(uint8_t data);
uint8_t UART0_RX(void);
uint8_t UART1_RX(void);
//void UART0_Handler(void);
void UART1_Handler(void);

#endif // ARM_UART_H_