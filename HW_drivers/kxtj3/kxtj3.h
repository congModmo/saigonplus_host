/******************************************************************************
KXTJ3-1057.h
KXTJ3-1057 Arduino
Leonardo Bispo
May, 2020
https://github.com/ldab/KXTJ3-1057
Resources:
Uses Wire.h for i2c operation

Distributed as-is; no warranty is given.
******************************************************************************/

#ifndef __KXTJ3_IMU_H__
#define __KXTJ3_IMU_H__

#include <stdint.h>
#include <stdbool.h>
#if defined LOW_POWER && defined HIGH_RESOLUTION
	#error Please choose between the 2 resolution types
#endif

// Return values 
typedef enum
{
	IMU_SUCCESS,
	IMU_HW_ERROR,
	IMU_NOT_SUPPORTED,
	IMU_GENERIC_ERROR,
	IMU_OUT_OF_BOUNDS,
	IMU_ALL_ONES_WARNING,
	//...
} kxtj3_status_t;

typedef enum
{
	KXTJ3_X = 0,
	KXTJ3_Y,
	KXTJ3_Z,
} axis_t;

/*
Accelerometer range = 2, 4, 8, 16g
Sample Rate - 0.781, 1.563, 3.125, 6.25, 12.5, 25, 50, 100, 200, 400, 800, 1600Hz
Output Data Rates ≥400Hz will force device into High Resolution mode
*/
kxtj3_status_t kxtj3_begin( float SampleRate,	uint8_t accRange );

// readRegister reads one 8-bit register
kxtj3_status_t kxtj3_readRegister(uint8_t* outputPointer, uint8_t offset);

// Reads two 8-bit regs, LSByte then MSByte order, and concatenates them.
// Acts as a 16-bit read operation
kxtj3_status_t kxtj3_readRegisterInt16(int16_t*, uint8_t offset );

// Writes an 8-bit byte;
kxtj3_status_t kxtj3_writeRegister(uint8_t, uint8_t);

// Configure Interrupts
// @Threshold from 1 to 4095 counts
// @moveDur   from 1 to 255 counts
// @naDur			from 1 to 255 counts
// Threshold (g) = threshold (counts) / 256(counts/g)
// timeDur (sec) = WAKEUP_COUNTER (counts) / Wake-Up Function ODR(Hz)
// Non-ActivityTime (sec) = NA_COUNTER (counts) / Wake-Up Function ODR(Hz)
kxtj3_status_t kxtj3_intConf( uint16_t threshold, uint8_t moveDur, uint8_t naDur, bool polarity );

// Read axis acceleration as Float
float kxtj3_axisAccel( axis_t _axis);

// Set IMU to Standby ~0.9uA, also Enable configuration -> PC1 bit in CTRL_REG1 must first be set to “0”
kxtj3_status_t kxtj3_standby( bool _en);

float 	accelSampleRate; 	// Sample Rate - 0.781, 1.563, 3.125, 6.25, 12.5, 25, 50, 100, 200, 400, 800, 1600Hz
uint8_t accelRange; 			// Accelerometer range = 2, 4, 8, 16g

//Apply settings at .begin()
void kxtj3_applySettings( void );

//ReadRegisterRegion takes a uint8 array address as input and reads
//  a chunk of memory into that array.
kxtj3_status_t kxtj3_readRegisterRegion(uint8_t*, uint8_t, uint8_t );


//implement in bsp

bool bsp_kxtj3_tx_rx(uint8_t *tx, uint16_t tx_len, uint8_t *rx, uint16_t rx_len);

//Device Registers
#define KXTJ3_WHO_AM_I               0x0F
#define KXTJ3_DCST_RESP							 0x0C	// used to verify proper integrated circuit functionality.It always has a byte value of 0x55

#define KXTJ3_OUT_X_L                0x06
#define KXTJ3_OUT_X_H                0x07
#define KXTJ3_OUT_Y_L                0x08
#define KXTJ3_OUT_Y_H                0x09
#define KXTJ3_OUT_Z_L                0x0A
#define KXTJ3_OUT_Z_H                0x0B

#define KXTJ3_STATUS				         0x18
#define KXTJ3_INT_SOURCE1            0x16
#define KXTJ3_INT_SOURCE2            0x17
#define KXTJ3_INT_REL            0x1A

#define KXTJ3_CTRL_REG1              0x1B // *
#define KXTJ3_CTRL_REG2              0x1D // *

#define KXTJ3_INT_CTRL_REG1          0x1E	// *
#define KXTJ3_INT_CTRL_REG2          0x1F	// *

#define KXTJ3_DATA_CTRL_REG					 0x21	// *
#define KXTJ3_WAKEUP_COUNTER				 0x29	// *
#define KXTJ3_NA_COUNTER						 0x2A	// *

#define KXTJ3_WAKEUP_THRD_H				 	 0x6A	// *
#define KXTJ3_WAKEUP_THRD_L					 0x6B	// *
#define KXTJ3_SELF_TEST						 0x3A	// *
// * Note that to properly change the value of this register, the PC1 bit in CTRL_REG1 must first be set to “0”.


#endif // End of __KXTJ3_IMU_H__ definition check
