/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file greth_mdio.h
 * @brief GRETH MDIO interface functions
 *
 * This header provides function prototypes for reading and writing PHY
 * registers over the MDIO interface for GRETH Ethernet device.
 *
 * @ingroup GRETH
 */

/*
 * Copyright (C) 2025 Prithvi Tambewagh

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
*/

#include "greth_emac.h"


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>


#ifdef __cplusplus
extern "C" {
#endif

struct greth_netif_state;

/**
 * @brief   Reads a PHY register using MDIO.
 *
 * @param   baseAddr      Base Address of the MDIO Module Registers.
 * @param   phyAddr       PHY Adress.
 * @param   regNum        Register Number to be read.
 * @param   dataPtr       Pointer where the read value shall be written.
 *
 * @return  status of the read \n
 * @retval TRUE read is successful.
 * @retval FALSE read is not acknowledged properly.
 *
 **/
int MDIOPhyRegRead(struct greth_netif_state *greth_device, uint32_t phyAddr, 
                                uint32_t regAddr, volatile uint32_t *dataPtr);

/**
 * @brief   Writes a PHY register using MDIO.
 *
 * @param   baseAddr      Base Address of the MDIO Module Registers.
 * @param   phyAddr       PHY Adress.
 * @param   regNum        Register Number to be read.
 * @param   RegVal        Value to be written.
 *
 * @return  void
 *
 **/
void MDIOPhyRegWrite(struct greth_netif_state *greth_device, uint32_t phyAddr, 
                                            uint32_t regAddr, uint32_t Value);

#ifdef __cplusplus
}
#endif