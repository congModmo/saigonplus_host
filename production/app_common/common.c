#include <app_common/common.h>
#include <string.h>
#include <stdio.h>

void STM32_ParseStr(char str[], char delimit[], char *ptr[])
{
	char *p = NULL;
	int i = 0;
	uint16_t timeOut = 3000;

	p = strtok(str, delimit);
	while (p)
	{
		if (timeOut-- == 0)
		{
			break;
		}
		ptr[i++] = p;
		p = strtok(NULL, delimit);
	}
}

uint8_t STM32_CompareByte(char source_buffer[], char dest_buffer[], uint16_t len)
{
	uint16_t count = 0;

	for (uint16_t i = 0; i < len; i++)
	{
		if (dest_buffer[i] == source_buffer[i])
		{
			count++;
		}
	}
	if (count == len)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void STM32_UpperToLower(char upper[], char lower[])
{
	for (uint8_t i = 0; i <= strlen(upper); i++)
	{
		if ((upper[i] >= 65) && (upper[i] <= 90))
		{
			upper[i] = upper[i] + 32;
		}
	}

	memcpy(lower, upper, strlen(upper));
}

uint16_t min(uint16_t a, uint16_t b)
{
	if (a > b)
	{
		return b;
	}

	if (a < b)
	{
		return a;
	}

	return 0;
}

uint8_t ascii_2_hex(uint8_t val)
{
	if (val >= '0' && val <= '9')
	{
		return (uint8_t)(val - '0');
	}

	else if (val >= 'A' && val <= 'F')
	{
		return (uint8_t)(val - 'A' + 10);
	}

	else if (val >= 'a' && val <= 'f')
	{
		return (uint8_t)(val - 'a' + 10);
	}

	return 0;
}

uint8_t STM32_CompareWord(int32_t source_buffer[], int32_t dest_buffer[], uint16_t len)
{
	uint16_t count = 0;

	for (uint16_t i = 0; i < len; i++)
	{
		if (dest_buffer[i] == source_buffer[i])
		{
			count++;
		}
	}

	if (count == len)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

uint8_t STM32_CRC8Cal(uint8_t data[], uint8_t len)
{
	uint8_t crc = 0;

	//calculates 8-Bit checksum with given polynomial
	for (uint8_t i = 0; i < len; i++)
	{
		crc ^= (data[i]);
		for (uint8_t j = 8; j > 0; j--)
		{
			if (crc & 0x80)
			{
				crc = (crc << 1) ^ 0x07;
			}
			else
			{
				crc = (crc << 1);
			}
		}
	}

	return crc;
}
bool timeout_check(uint32_t *from, uint32_t interval)
{
	if (millis() - (*from) >= interval)
	{
		*from = millis();
		return true;
	}
	return false;
}

void binaryToHexString(uint8_t *hex, int len, char *string)
{
	for (int i = 0; i < len; i++)
	{
		uint8_t value = hex[i] >> 4;
		if (value > 9)
		{
			string[2 * i] = value + 'A' - 10;
		}
		else
		{
			string[2 * i] = value + '0';
		}
		value = hex[i] & 0x0f;
		if (value > 9)
		{
			string[2 * i + 1] = value + 'A' - 10;
		}
		else
		{
			string[2 * i + 1] = value + '0';
		}
	}
	string[2 * len] = '\0';
}

int hexStringToBinary(char *string, uint8_t *buf)
{
	int str_len = strlen(string);
	if (str_len % 2 != 0)
	{
		return 0;
	}
	for (int i = 0; i < str_len / 2; i++)
	{
		if (string[2 * i] >= 'A' && string[2 * i] <= 'F')
		{
			buf[i] = (string[2 * i] - 'A' + 10) << 4;
		}
		else if (string[2 * i] >= '0' && string[2 * i] <= '9')
		{
			buf[i] = (string[2 * i] - '0') << 4;
		}
		else
			return 0;

		if (string[2 * i + 1] >= 'A' && string[2 * i + 1] <= 'F')
		{
			buf[i] |= (string[2 * i + 1] - 'A' + 10) & 0x0f;
		}
		else if (string[2 * i + 1] >= '0' && string[2 * i + 1] <= '9')
		{
			buf[i] |= (string[2 * i + 1] - '0');
		}
		else
			return 0;
	}
	return str_len / 2;
}

uint8_t dec_to_bcd(uint8_t n)
{
	return n + 6 * (n / 10);
}

uint8_t bcd_to_dec(uint8_t n)
{
	return n - 6 * (n >> 4);
}
