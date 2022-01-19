#define __DEBUG__ 4

#include "adxl343.h"
#include "bsp.h"
/**************************************************************************/
/*!
    @brief  Writes 8-bits to the specified destination register

    @param reg The register to write to
    @param value The value to write to the register
*/
/**************************************************************************/

static void writeRegister(uint8_t reg, uint8_t value)
{
	uint8_t buff[2]={reg, value};
	bsp_adxl343_tx_rx(buff, 2, NULL, 0);
}

/**************************************************************************/
/*!
    @brief  Reads 8-bits from the specified register

    @param reg register to read

    @return The results of the register read request
*/
/**************************************************************************/
static uint8_t readRegister(uint8_t reg)
{
	uint8_t output;
	bsp_adxl343_tx_rx(&reg, 1, &output, 1);
	return output;
}

/**************************************************************************/
/*!
    @brief  Reads 16-bits from the specified register

    @param reg The register to read two bytes from

    @return The 16-bit value read from the reg starting address
*/
/**************************************************************************/

static int16_t read16(uint8_t reg)
{
	int16_t output;
	bsp_adxl343_tx_rx(&reg, 1, (uint8_t *)&output, 2);
	return output;
}

/**************************************************************************/
/*!
    @brief  Read the device ID (can be used to check connection)

    @return The 8-bit device ID
*/
/**************************************************************************/

uint8_t adxl343_check_interrupts(void)
{
	return readRegister(ADXL3XX_REG_INT_SOURCE);
}

bool adxl343_map_interrupts(int_config_t cfg)
{
	writeRegister(ADXL3XX_REG_INT_ENABLE, cfg.value);
	return true;
}

bool adxl343_enable_interrupts(int_config_t cfg)
{
	writeRegister(ADXL3XX_REG_INT_ENABLE, cfg.value);
	return true;
}

uint8_t adxl343_get_device_id(void)
{
	return readRegister(ADXL3XX_REG_DEVID);
}

void adxl343_set_range(adxl34x_range_t range)
{
	uint8_t data_format=readRegister(ADXL3XX_REG_DATA_FORMAT);
	data_format &= 0b11111100;
	data_format |=range;
	data_format |= (1<<4); //full res
	data_format &= ~(1<<5); // no invert
	writeRegister(ADXL3XX_REG_DATA_FORMAT, data_format);
}

void adxl343_set_data_rate(adxl3xx_dataRate_t dataRate)
{
	uint8_t bw_rate=readRegister(ADXL3XX_REG_BW_RATE);
	bw_rate &= 0b11110000;
	bw_rate |= dataRate;
	writeRegister(ADXL3XX_REG_BW_RATE, bw_rate);
}

void adxl343_start_measure(void)
{
	uint8_t power_control = (1<<3);
	writeRegister(ADXL3XX_REG_POWER_CTL ,power_control);
	power_control=readRegister(ADXL3XX_REG_POWER_CTL);
}

int16_t adxl343_get_x(void)
{
	return read16(ADXL3XX_REG_DATAX0);
}

int16_t adxl343_get_y(void)
{
	return read16(ADXL3XX_REG_DATAY0);
}

int16_t adxl343_get_z(void)
{
	return read16(ADXL3XX_REG_DATAZ0);
}

bool adxl343_init(adxl34x_range_t range, adxl3xx_dataRate_t dataRate)
{
	if(adxl343_get_device_id()!=ADXL343_ID)
	{
		return false;
	}
	writeRegister(ADXL3XX_REG_INT_ENABLE, 0);
	adxl343_set_range(range);
	adxl343_set_data_rate(dataRate);
	adxl343_start_measure();
	return true;
}

bool init_active_interrupt(uint8_t threshold, uint8_t count, uint8_t ad_dc)
{
	writeRegister(ADXL3XX_REG_INT_ENABLE, 0);
	writeRegister(ADXL3XX_REG_THRESH_ACT, threshold);
	int_config_t int_map_config={ADXL343_INT2};
	int_map_config.bits.activity=ADXL343_INT1;
	writeRegister(ADXL3XX_REG_INT_MAP, int_map_config.value);
	uint8_t act_inact_ctrl=(ad_dc << 7) | 0b01110000;
	writeRegister(ADXL3XX_REG_ACT_INACT_CTL, act_inact_ctrl);
	int_config_t int_config={0};
	int_config.bits.activity=1;
	writeRegister(ADXL3XX_REG_INT_ENABLE, int_config.value);
	return true;
}

/**************************************************************************
 *  UNIT test
 *************************************************************************/

static const char *done="ADXL343 TEST DONE\n";
static const char *fail="ADXL343 TEST FAIL\n";

bool TEST_adxl343_device_id()
{
	ASSERT_RET(adxl343_get_device_id()==ADXL343_ID, false, "Device id invalid");
	return true;
}

const char * adxl343_unit_test()
{
	if(!TEST_adxl343_device_id())
	{
		return fail;
	}
	return done;
}
