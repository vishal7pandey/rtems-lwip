/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file eth_lwip_defaults.h
 * @brief Default configuration for LwIP Ethernet interface
 *
 * This header defines default MAC address, IP address, netmask, and gateway
 * for the Ethernet interface when using LwIP. Static IP configuration can
 * be enabled via `STATIC_IP_ADDRESS`.
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

#ifndef __ETH_LWIP_DEFAULT_H
#define __ETH_LWIP_DEFAULT_H

/* #define STATIC_IP_ADDRESS 1 */
#define MAC_ADDR_LEN              ETHARP_HWADDR_LEN

#ifndef ETH_MAC_ADDR
#define ETH_MAC_ADDR             { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC }
#endif

#if STATIC_IP_ADDRESS

/**
 * When static IP is configured in lwipopts.h, 
 * this IP addr is used for interface.
 */
#ifndef ETH_IP_ADDR
#define ETH_IP_ADDR               0xC0A8F701 /* 192.168.247.1  */
#endif

/**
 * When static IP is configured in lwipopts.h, this NETMASK address is 
 * used for interface.
 */
#ifndef ETH_NETMASK
#define ETH_NETMASK               0xFFFFFF00 /* 255.255.255.0  */
#endif

/**
 * When static IP is configured in lwipopts.h, this Gateway address is 
 * used for interface.
 */
#ifndef ETH_GW
#define ETH_GW                    0xC0A8F7FE /* 192.168.247.254*/
#endif

#endif /* STATIC_IP_ADDRESS */

#endif /* __ETH_LWIP_DEFAULT_H */
