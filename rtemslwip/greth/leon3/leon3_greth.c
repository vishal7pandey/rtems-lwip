/* SPDX-License-Identifier: BSD-2-Clause */

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

#include <bsp.h>
#include<greth.h>
#include <grlib/ambapp.h>
#include <stdio.h>
#include "greth_emac.h"

#define RDA_COUNT     32
#define TDA_COUNT     32

greth_configuration_t leon_greth_configuration;

int rtems_lwip_leon3_greth_driver_attach(struct greth_netif_state *greth_chip)
{
  unsigned int base_addr = 0; 
  unsigned int eth_irq = 0;   
  struct ambapp_dev *adev;
  struct ambapp_apb_info *apb;

  adev = (void *)ambapp_for_each(ambapp_plb(), (OPTIONS_ALL|OPTIONS_APB_SLVS),
                                 VENDOR_GAISLER, GAISLER_ETHMAC,
                                 ambapp_find_by_idx, NULL);
  if (adev) {
    apb = DEV_TO_APB(adev);
    base_addr = apb->start;
    eth_irq = apb->common.irq;

    *(volatile int *) base_addr = 0;
    *(volatile int *) base_addr = GRETH_CTRL_RST;
    *(volatile int *) base_addr = 0;
    leon_greth_configuration.base_address = (void *)base_addr;
    printf("(DEBUG) GRETH Base Addr. : %p\n", leon_greth_configuration.base_address);
    leon_greth_configuration.vector = eth_irq; 
    leon_greth_configuration.txd_count = TDA_COUNT;
    leon_greth_configuration.rxd_count = RDA_COUNT;
  }
  greth_chip->regs = leon_greth_configuration.base_address;
  greth_chip->vec = leon_greth_configuration.vector;
  printf("(DEBUG) Vector Number obtained : %d\n", eth_irq);
  printf("(DEBUG) Vector Number set : %d\n", greth_chip->vec);
  greth_chip->num_tx_bd = leon_greth_configuration.txd_count;
  greth_chip->num_rx_bd = leon_greth_configuration.rxd_count;
  greth_chip->regs->ctrl |= GRETH_CTRL_TXEN;
  printf("GRETH CTRL Reg. : %d\n", greth_chip->regs->ctrl);

  return 0;
}
