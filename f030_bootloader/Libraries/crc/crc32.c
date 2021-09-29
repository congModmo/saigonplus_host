/**
 * Copyright (c) 2013 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "crc32.h"

#include <stdlib.h>

uint32_t crc32_compute(uint8_t const * p_data, uint32_t size, uint32_t const * p_crc)
{
    const uint32_t polynomial = 0x04C11DB7; /* divisor is 32bit */
    uint32_t crc = (p_crc!=NULL)?*p_crc:0xFFFFFFFF;

    for(int i=0; i<size; i++)
    {
        crc ^= (uint32_t)(p_data[i] << 24); /* move byte into MSB of 32bit CRC */

        for (int i = 0; i < 8; i++)
        {
            if ((crc & 0x80000000) != 0) /* test for MSB = bit 31 */
            {
                crc = (uint32_t)((crc << 1) ^ polynomial);
            }
            else
            {
                crc <<= 1;
            }
        }
    }
    return crc;
}

bool crc32_verify(uint8_t const * p_data, uint32_t size, uint32_t expected_crc){
	uint32_t crc=crc32_compute(p_data, size, NULL);
	if(crc!=expected_crc){
		return false;
	}
	return true;
}
