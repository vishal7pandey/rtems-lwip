/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file greth_mdio.c
 * @brief MDIO (Management Data Input/Output) interface functions for GRETH.
 *
 * This file implements functions to read and write PHY registers
 * via the MDIO interface for the GRETH Ethernet driver.
 * It provides blocking access to PHY registers and handles
 * basic error reporting on failed reads.
 *
 * Main functionalities:
 * - Reading a PHY register using `MDIOPhyRegRead()`.
 * - Writing a PHY register using `MDIOPhyRegWrite()`.
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
#include "greth_mdio.h"
#if 1
#ifndef __MDIO_H__
#define __MDIO_H__


#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

int MDIOPhyRegRead(struct greth_netif_state *greth_device, uint32_t phyAddr, 
                                  uint32_t regAddr, volatile uint32_t *dataPtr)
{
  while(greth_device->regs->mdio_ctrl & GRETH_MDIO_BUSY);
  greth_device->regs->mdio_ctrl = (phyAddr << 11) | (regAddr << 6) | 
                                                                GRETH_MDIO_READ;
  while(greth_device->regs->mdio_ctrl & GRETH_MDIO_BUSY);
  if(!(greth_device->regs->mdio_ctrl & GRETH_MDIO_LINKFAIL)){
    *dataPtr = ((greth_device->regs->mdio_ctrl >> 16) & 0xFFFF);
    return 1;
  }
  else{
    printf("greth: failed to read\n");
    return 0;
  }
}

void MDIOPhyRegWrite(struct greth_netif_state *greth_device, uint32_t phyAddr,
                uint32_t regAddr, uint32_t Value)
{
  while(greth_device->regs->mdio_ctrl & GRETH_MDIO_BUSY);   
  greth_device->regs->mdio_ctrl = (((Value & 0xFFFF) << 16) | (phyAddr << 11) | 
                                            (regAddr << 6) | GRETH_MDIO_WRITE);
  while(greth_device->regs->mdio_ctrl & GRETH_MDIO_BUSY);   
}

#ifdef __cplusplus
}
#endif
#endif /* __MDIO_H__ */
#endif /* 0 */
