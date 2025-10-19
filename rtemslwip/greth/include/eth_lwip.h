/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file eth_lwip.h
 * @brief LwIP Ethernet interface and initialization functions
 *
 * This header provides function prototypes and error codes for initializing
 * and managing Ethernet interfaces with LwIP. It includes functions to
 * configure MAC addresses, retrieve DHCP information, and check netif status.
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

#ifndef __ETH_LWIP_H
#define __ETH_LWIP_H

#include <stdio.h>
#include <stdbool.h>
#include "lwip/netif.h"


/**
 * While scanning phy addresses no alive phy was found.
 * Return value of rpp_eth_hw_init() function.
 */
#define NO_PHY_ALIVE             -1
/**
 * Scanning default phy address, it was found it's not alive.
 * Return value of rpp_eth_hw_init() function.
 */
#define DFLT_PHY_NOT_ALIVE       -1
/**
 * When setting autonegotiation parameters to EMAC module, there was found 
 * impossible mode (usually on timeout of autonegotiation).
 * Return value of rpp_eth_hw_init_postInit() function.
 */
#define UNKN_DUPLEX_MODE         -2 /* this could mean that autonegotiation was 
                                    not completed yet */
/**
 * Phy is down error.
 * Return value of rpp_eth_init_postInit() function.
 */
#define PHY_LINK_DOWN            -3

/**
 * LwIP netif couldn't be added, it is likely that there was an error during 
 * initialization of the hardware.
 */
#define NETIF_ADD_ERR            -10 /* could be one of previous, except 
                                    PHY_LINK_DOWN - currently */
/**
 * Memory requirements couldn't be satisfied.
 */
#define DHCP_MEM_ERR             -11

/**
 * configures whether rpp_eth_get_macAddrStr() creates string with big or 
 * small latin letters
 */
#define MAC_BIG_LETTERS           1


int8_t eth_lwip_init(uint8_t *mac_addr);
void eth_lwip_get_dhcp_info(void);
int eth_lwip_get_netif_status_cmd(void);
void eth_lwip_set_hwaddr(struct netif *netif, uint8_t *mac_addr);
void eth_lwip_get_hwaddr_str(struct netif *netif, uint8_t *macStr);
struct netif *eth_lwip_get_netif(uint32_t instance_number);






#endif /* __ETH_LWIP_H */
