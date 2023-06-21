#ifndef _CONFIG_H__
#define _CONFIG_H__

#include "pico/stdlib.h"
#include "hardware/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

	
#define INPUT_1_PIN 6
#define INPUT_2_PIN 5

#define DEBUG_TX_PIN 8
#define DEBUG_RX_PIN 9
#define DEBUG_DEV uart1

#define LED_PIN 10

#define ADDRESS_B0_PIN 16
#define ADDRESS_B1_PIN 17
#define ADDRESS_B2_PIN 18
#define ADDRESS_B3_PIN 19
#define ADDRESS_B4_PIN 20
#define ADDRESS_B5_PIN 21
#define ADDRESS_B6_PIN 22
#define ADDRESS_B7_PIN 23

#define OUTPUT_1_PIN 26
#define OUTPUT_2_PIN 27

#define VSENSE_5V_PIN 28
#define VSENSE_12V_PIN 29

#ifdef __cplusplus
}
#endif

#endif // _CONFIG_H__
