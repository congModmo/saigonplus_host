/******************************************************************************
KXTJ3-1057.cpp
KXTJ3-1057 Arduino
Leonardo Bispo
May, 2020
https://github.com/ldab/KXTJ3-1057
Resources:
Uses Wire.h for i2c operation

Distributed as-is; no warranty is given.
******************************************************************************/

#include <kxtj3/kxtj3.h>
#include "stdint.h"
#include "bsp.h"


kxtj3_status_t kxtj3_begin( float SampleRate, uint8_t accRange )
{

	debug("Configuring IMU");

	kxtj3_status_t returnError = IMU_SUCCESS;

  //Wire.begin();
  
	// Start-up time, Figure 1: Typical StartUp Time - DataSheet

	delay(2);

	//Check the ID register to determine if the operation was a success.
	uint8_t _whoAmI;
	
	kxtj3_readRegister(&_whoAmI, KXTJ3_WHO_AM_I);

	if( _whoAmI != 0x35 )
	{
		returnError = IMU_HW_ERROR;
	}

	accelSampleRate = SampleRate;
	accelRange 			= accRange;

	debug("Apply settings");
	kxtj3_applySettings();

	return returnError;
}

//****************************************************************************//
//  ReadRegisterRegion
//
//  Parameters:
//    *outputPointer -- Pass &variable (base address of) to save read data to
//    offset -- register to read
//    length -- number of bytes to read
//****************************************************************************//
kxtj3_status_t kxtj3_readRegisterRegion(uint8_t *outputPointer , uint8_t offset, uint8_t length)
{
	if(!bsp_kxtj3_tx_rx(&offset, 1, outputPointer, length)){
		return IMU_HW_ERROR;
	}
	return IMU_SUCCESS;
}

//****************************************************************************//
//  ReadRegister
//
//  Parameters:
//    *outputPointer -- Pass &variable (address of) to save read data to
//    offset -- register to read
//****************************************************************************//
kxtj3_status_t kxtj3_readRegister(uint8_t* outputPointer, uint8_t offset) {
	if(!bsp_kxtj3_tx_rx(&offset, 1, outputPointer, 1)){
		return IMU_HW_ERROR;
	}
	return IMU_SUCCESS;
}

//****************************************************************************//
//  readRegisterInt16
//
//  Parameters:
//    *outputPointer -- Pass &variable (base address of) to save read data to
//    offset -- register to read
//****************************************************************************//
kxtj3_status_t kxtj3_readRegisterInt16( int16_t* outputPointer, uint8_t offset )
{
	//offset |= 0x80; //turn auto-increment bit on
	uint8_t myBuffer[2];
	kxtj3_status_t returnError = kxtj3_readRegisterRegion(myBuffer, offset, 2);  //Does memory transfer
	int16_t output = (int16_t)myBuffer[0] | ((int16_t)myBuffer[1]) << 8;

	*outputPointer = output;
	return returnError;
}

//****************************************************************************//
//  writeRegister
//
//  Parameters:
//    offset -- register to write
//    dataToWrite -- 8 bit data to write to register
//****************************************************************************//
kxtj3_status_t kxtj3_writeRegister(uint8_t offset, uint8_t dataToWrite)
{
	uint8_t buff[2]={offset, dataToWrite};
	if(!bsp_kxtj3_tx_rx(buff, 2, NULL, 0)){
		return IMU_HW_ERROR;
	}

	return IMU_SUCCESS;
}

// Read axis acceleration as Float
float kxtj3_axisAccel( axis_t _axis)
{
	int16_t outRAW;
	uint8_t regToRead = 0;
	switch (_axis)
	{
		case KXTJ3_X:
			// X axis
			regToRead = KXTJ3_OUT_X_L;
			break;
		case KXTJ3_Y:
			// Y axis
			regToRead = KXTJ3_OUT_Y_L;
			break;
		case KXTJ3_Z:
			// Z axis
			regToRead = KXTJ3_OUT_Z_L;
			break;
	
		default:
			// Not valid axis return NAN
			return 0;
			break;
	}

	kxtj3_readRegisterInt16( &outRAW, regToRead );

	float outFloat;

	switch( accelRange )
	{
		case 2:
		outFloat = (float)outRAW / 15987;
		break;
		case 4:
		outFloat = (float)outRAW / 7840;
		break;
		case 8:
		outFloat = (float)outRAW / 3883;
		break;
		case 16:
		outFloat = (float)outRAW / 1280;
		break;
		default:
		outFloat = 0;
		break;
	}

	return outFloat;

}

kxtj3_status_t	kxtj3_standby( bool _en )
{
	uint8_t _ctrl;

	// "Backup" KXTJ3_CTRL_REG1
	kxtj3_readRegister(&_ctrl, KXTJ3_CTRL_REG1);
	
	if( _en )
		_ctrl &= 0x7E;
	else
		_ctrl |= (0x01 << 7);	// disable standby-mode -> Bit7 = 1 = operating mode

	return kxtj3_writeRegister(KXTJ3_CTRL_REG1, _ctrl);
}

//****************************************************************************//
//
//  Apply settings passed to .begin();
//
//****************************************************************************//
void kxtj3_applySettings( void )
{
	uint8_t dataToWrite = 0;  //Temporary variable

	kxtj3_standby( true );
	
	//Build DATA_CTRL_REG

	//  Convert ODR
	if(accelSampleRate < 1)					dataToWrite |= 0x08;	// 0.781Hz
	else if(accelSampleRate < 2)		dataToWrite |= 0x09;	// 1.563Hz
	else if(accelSampleRate < 4)		dataToWrite |= 0x0A;	// 3.125Hz
	else if(accelSampleRate < 8)		dataToWrite |= 0x0B;	// 6.25Hz
	else if(accelSampleRate < 16)		dataToWrite |= 0x00;	// 12.5Hz
	else if(accelSampleRate < 30)		dataToWrite |= 0x01;	// 25Hz
	else if(accelSampleRate < 60)		dataToWrite |= 0x02;	// 50Hz
	else if(accelSampleRate < 150)	dataToWrite |= 0x03;	// 100Hz
	else if(accelSampleRate < 250)	dataToWrite |= 0x04;	// 200Hz
	else if(accelSampleRate < 450)	dataToWrite |= 0x05;	// 400Hz
	else if(accelSampleRate < 850)	dataToWrite |= 0x06;	// 800Hz
	else														dataToWrite	|= 0x07;	// 1600Hz

	//Now, write the patched together data
	debug("KXTJ3_DATA_CTRL_REG: 0x%x\n", dataToWrite);
	kxtj3_writeRegister(KXTJ3_DATA_CTRL_REG, dataToWrite);

	//Build CTRL_REG1
	
	// LOW power, 8-bit mode
	dataToWrite = 0x80;

	#ifdef HIGH_RESOLUTION
		dataToWrite = 0xC0;
	#endif

	//  Convert scaling
	switch(accelRange)
	{	
		default:
		case 2:
		dataToWrite |= (0x00 << 2);
		break;
		case 4:
		dataToWrite |= (0x02 << 2);
		break;
		case 8:
		dataToWrite |= (0x04 << 2);
		break;
		case 16:
		dataToWrite |= (0x01 << 2);
		break;
	}

	//Now, write the patched together data
	debug("KXTJ3_CTRL_REG1: 0x%x\n", dataToWrite);
	kxtj3_writeRegister(KXTJ3_CTRL_REG1, dataToWrite);

}


kxtj3_status_t kxtj3_configThreshold(uint16_t threshold)
{
	kxtj3_standby( true );
	uint8_t dataToWrite;
	dataToWrite = (uint8_t)(threshold >> 4);
	debug("KXTJ3_WAKEUP_THRD_H: 0x%x\n", dataToWrite);
	kxtj3_writeRegister(KXTJ3_WAKEUP_THRD_H, dataToWrite);
	dataToWrite = (uint8_t)(threshold << 4);
	debug("KXTJ3_WAKEUP_THRD_L: 0x%x\n", dataToWrite);
	kxtj3_writeRegister(KXTJ3_WAKEUP_THRD_L, dataToWrite);
	return kxtj3_standby( false );
}
//****************************************************************************//
//  Configure interrupt, stop or move, threshold and duration
//	Durationsteps and maximum values depend on the ODR chosen.
//****************************************************************************//
kxtj3_status_t kxtj3_intConf(uint16_t threshold, uint8_t moveDur, uint8_t naDur, bool polarity )
{
	// Note that to properly change the value of this register, the PC1 bit in CTRL_REG1must first be set to “0”.
	kxtj3_standby( true );

	kxtj3_status_t returnError = IMU_SUCCESS;

	// Build INT_CTRL_REG1

	uint8_t dataToWrite = 0b00100010;
	
	if( polarity == HIGH )
		dataToWrite |= (0x01 << 4);		// Active HIGH

	debug("KXTJ3_INT_CTRL_REG1: 0x%x\n", dataToWrite);
	returnError = kxtj3_writeRegister(KXTJ3_INT_CTRL_REG1, dataToWrite);
	
	// WUFE – enables the Wake-Up (motion detect) function.

	uint8_t _reg1;

	returnError = kxtj3_readRegister(&_reg1, KXTJ3_CTRL_REG1);
	
	_reg1 |= (0x01 << 1);

	returnError = kxtj3_writeRegister(KXTJ3_CTRL_REG1, _reg1);

	uint8_t _reg2;

	returnError = kxtj3_readRegister(&_reg2, KXTJ3_CTRL_REG2);

	_reg2 |= 0b0101; //25 hz

	returnError = kxtj3_writeRegister(KXTJ3_CTRL_REG2, _reg2);
// Build INT_CTRL_REG2

	dataToWrite = 0x3C;  // enable interrupt on X Y

	debug("KXTJ3_INT_CTRL_REG1: 0x%x\n", dataToWrite);
	returnError = kxtj3_writeRegister(KXTJ3_INT_CTRL_REG2, dataToWrite);

	// Set WAKE-UP (motion detect) Threshold

	dataToWrite = (uint8_t)(threshold >> 4);

	debug("KXTJ3_WAKEUP_THRD_H: 0x%x\n", dataToWrite);
	returnError = kxtj3_writeRegister(KXTJ3_WAKEUP_THRD_H, dataToWrite);

	dataToWrite = (uint8_t)(threshold << 4);

	debug("KXTJ3_WAKEUP_THRD_L: 0x%x\n", dataToWrite);
	returnError = kxtj3_writeRegister(KXTJ3_WAKEUP_THRD_L, dataToWrite);

	// WAKEUP_COUNTER -> Sets the time motion must be present before a wake-up interrupt is set
	// WAKEUP_COUNTER (counts) = Wake-Up Delay Time (sec) x Wake-Up Function ODR(Hz)

	dataToWrite = moveDur;

	debug("KXTJ3_WAKEUP_COUNTER: 0x%x\n", dataToWrite);
	returnError = kxtj3_writeRegister(KXTJ3_WAKEUP_COUNTER, dataToWrite);

	// Non-Activity register sets the non-activity time required before another wake-up interrupt will be reported.
	// NA_COUNTER (counts) = Non-ActivityTime (sec) x Wake-Up Function ODR(Hz)

	dataToWrite = naDur;

	debug("KXTJ3_NA_COUNTER: 0x%x\n", dataToWrite);
	returnError = kxtj3_writeRegister(KXTJ3_NA_COUNTER, dataToWrite);
	// Set IMU to Operational mode
	returnError = kxtj3_standby( false );

	return returnError;
}

kxtj3_status_t kxtj3_selfTest(bool start)
{
	kxtj3_standby( true );
	uint8_t dataToWrite = (start)?0xca:0x00;
	kxtj3_writeRegister(KXTJ3_SELF_TEST, dataToWrite);
	return kxtj3_standby( false );
}
