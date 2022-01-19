#include "bsp.h"
#include "i2c.h"
#include "adxl343/adxl343.h"

/***************************************************************************
 * IMU BSP
 ***************************************************************************/

bool bsp_adxl343_tx_rx(uint8_t *tx, uint16_t tx_len, uint8_t *rx, uint16_t rx_len)
{
	if (tx != NULL && tx_len > 0)
	{
		if (HAL_I2C_Master_Transmit(&hi2c1, ADXL343_ADDRESS << 1, tx, tx_len, 1000) != HAL_OK)
		{
			return false;
		}
	}
	if (rx != NULL && rx_len > 0)
	{
		if (HAL_I2C_Master_Receive(&hi2c1, ADXL343_ADDRESS << 1, rx, rx_len, 1000) != HAL_OK)
		{
			return false;
		}
	}
	return true;
}
