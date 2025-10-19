/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file greth_lwip.c
 * @brief Ethernet LwIP interface functions for GRETH driver.
 *
 * This file provides the implementation of LwIP network interface
 * initialization, DHCP information retrieval, MAC address handling,
 * and IP address conversion utilities for the GRETH Ethernet driver.
 *
 * The main functionalities include:
 * - Starting and initializing network interfaces with static IP or DHCP.
 * - Retrieving DHCP-assigned IP address, netmask, and gateway.
 * - Displaying network interface statistics.
 * - Setting and converting MAC addresses to string format.
 * - Converting IP addresses to dotted-decimal string representation.
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

#include "lwip/tcpip.h" /* includes - lwip/opt.h, lwip/api_msg.h, 
                        lwip/netifapi.h, lwip/pbuf.h, lwip/api.h, lwip/sys.h, 
                        lwip/timers.h, lwip/netif.h */
#include "lwip/stats.h"
#include "lwip/dhcp.h"
#include "lwip/netifapi.h"
#include "netif/etharp.h" /* includes - lwip/ip.h, lwip/netif.h, lwip/ip_addr.h, 
                          lwip/pbuf.h */
#include "eth_lwip_defaults.h"
#include "eth_lwip.h"

#include <arpa/inet.h>

#include "greth_netif.h"
#include "greth_emac.h"

#include <stdio.h>
#include <inttypes.h>

#ifndef MAX_EMAC_INSTANCE
#define MAX_EMAC_INSTANCE           1
#endif /*MAX_EMAC_INSTANCE*/

#define SUCCESS ERR_OK
#define FAILURE ERR_IF

static struct netif eth_lwip_netifs[MAX_EMAC_INSTANCE]; /* */
static void eth_lwip_conv_IP_decimal_Str(ip_addr_t ip, uint8_t *ipStr);

/**
 * @brief Print the DHCP-assigned IP information for the default netif.
 *
 * This function retrieves the first LwIP network interface (instance 0)
 * and checks if a DHCP address has been assigned. If a DHCP lease exists,
 * it prints the IP address, netmask, and gateway in decimal notation.
 * Otherwise, it prints "dhcp not bound".
 * 
 * @param void
 * @returns void
 */
void eth_lwip_get_dhcp_info(void)
{
  struct netif *netif = eth_lwip_get_netif(0);  

  if (dhcp_supplied_address(netif)) { 
    uint8_t ipString[16]; 
    eth_lwip_conv_IP_decimal_Str(netif->ip_addr, ipString);
    printf("Address: %s\n", ipString);
    eth_lwip_conv_IP_decimal_Str(netif->netmask, ipString);
    printf("Netmask: %s\n", ipString);
    eth_lwip_conv_IP_decimal_Str(netif->gw, ipString);
    printf("Gateway: %s\n", ipString);
  } else {
    printf("dhcp not bound\n");
  }
}

/**
 * @brief Initialize and start the networking interface with given parameters.
 *
 * This function sets up a network interface using LwIP. It configures the
 * MAC address, initializes the TCP/IP stack, sets IP addresses, netmask,
 * and gateway, and adds the network interface to LwIP. It also sets the
 * interface as default and brings it up.
 *
 * @param net_interface Pointer to the LwIP network interface structure.
 * @param ipaddr        Pointer to the desired IP address (ip_addr_t).
 * @param netmask       Pointer to the desired network mask (ip_addr_t).
 * @param gateway       Pointer to the desired gateway address (ip_addr_t).
 * @param mac_addr      Pointer to 6-byte MAC address. If NULL, a default
 *                      MAC address defined by `ETH_MAC_ADDR` is used.
 *
 * @return int Status code
 * @retval SUCCESS       Networking started successfully
 * @retval NETIF_ADD_ERR Failed to add the network interface
 */
int start_networking(
  struct netif  *net_interface,
  ip_addr_t     *ipaddr,
  ip_addr_t     *netmask,
  ip_addr_t     *gateway,
  uint8_t *mac_addr
)
{
  int8_t retVal = SUCCESS;

  struct netif *netif = net_interface;
  struct netif *netif_tmp;
  struct greth_netif_state *greth_state = 
                                      malloc(sizeof(struct greth_netif_state));
  u8_t default_mac[MAC_ADDR_LEN] = ETH_MAC_ADDR;

  if (mac_addr == NULL)
    mac_addr = default_mac;

  eth_lwip_set_hwaddr(netif, mac_addr);
  tcpip_init(NULL, NULL);

  ip4_addr_t ip_addr, net_mask, gw_addr;
  
  ip_addr.addr = ipaddr->u_addr.ip4.addr;
  net_mask.addr = netmask->u_addr.ip4.addr;
  gw_addr.addr = gateway->u_addr.ip4.addr;

  netif_tmp = netif_add(netif, &ip_addr, &net_mask, &gw_addr,
                      greth_state, greth_init_dev_and_lwip_netif, tcpip_input);

  if (netif_tmp == NULL)
    return NETIF_ADD_ERR;

  netif_set_default(netif);
  
#if LWIP_NETIF_API
  netifapi_netif_set_up(netif);
#endif

  return retVal;
}

/**
 * @brief Display the current status of the LwIP network interface.
 *
 * This function prints network interface statistics using `stats_display()`.
 * It can be used as a command-line utility or called programmatically to
 * check the current state of the network interface.
 *
 * @param void
 *
 * @return int Always returns 0.
 */
int eth_lwip_get_netif_status_cmd(void)
{
  stats_display();
  return 0;
}

/**
 * @brief Retrieve a pointer to a LwIP network interface instance.
 *
 * This function returns the network interface structure corresponding to
 * the given instance number. It can be used to access a specific network
 * interface managed by the Ethernet/LwIP layer.
 *
 * @param instance_number Index of the network interface to retrieve.
 *
 * @return struct netif* Pointer to the network interface structure.
 * @retval NULL If the instance number is out of range.
 */
struct netif *
eth_lwip_get_netif(uint32_t instance_number)
{
  if (instance_number >= MAX_EMAC_INSTANCE)
    return NULL;
  return &eth_lwip_netifs[instance_number];
}

/**
 * @brief Convert an IP address to a dotted-decimal string.
 *
 * This function converts an `ip_addr_t` (IPv4) address to a human-readable
 * dotted-decimal string (e.g., "192.168.0.1") and stores it in the provided
 * buffer.
 *
 * @param ip     The IP address to convert.
 * @param ipStr  Pointer to a buffer of at least 16 bytes where the resulting
 *               string will be stored.
 *
 * @note The function currently handles only IPv4 addresses. 
 */
static void
eth_lwip_conv_IP_decimal_Str(ip_addr_t ip, uint8_t *ipStr)
{
  uint32_t addr;
 #if LWIP_IPV6
  addr = ip.u_addr.ip4.addr;
 #else
  addr = ip.addr; 
 #endif

  snprintf((char *)ipStr, 16, "%" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 \
  "", (addr >> 24), ((addr >> 16) & 0xff), ((addr >> 8) & 0xff), (addr & 0xff));
}

/**
 * @brief Set the hardware (MAC) address of a LwIP network interface.
 *
 * This function copies the given 6-byte MAC address into the network interface
 * structure and updates the hardware address length. Optionally, it prints
 * the MAC address if `DEBUG` is enabled.
 *
 * @param netif     Pointer to the LwIP network interface structure.
 * @param mac_addr  Pointer to a 6-byte array containing the MAC address.
 *
 * @note The MAC address must be 6 bytes long.
 * @note If `DEBUG` is defined, the function prints the set MAC address to
 *       the console using `eth_lwip_get_hwaddr_str()`.
 */
void
eth_lwip_set_hwaddr(struct netif *netif, uint8_t *mac_addr)
{
  int i;

  /* set MAC hardware address */
  for (i = 0; i < MAC_ADDR_LEN; i++) {
    netif->hwaddr[i] = mac_addr[i];
  }
  netif->hwaddr_len = MAC_ADDR_LEN;

#ifdef DEBUG
  uint8_t macStr[18];
  eth_lwip_get_hwaddr_str(netif, macStr);
  printf("Setting MAC... %s\r\n", macStr);
#endif
}

/**
 * @brief Convert the MAC address of a network interface to a string.
 *
 * This function converts the hardware (MAC) address of the given LwIP
 * network interface into a human-readable string in the format
 * "XX:XX:XX:XX:XX:XX" using uppercase hexadecimal digits.
 *
 * @param netif    Pointer to the LwIP network interface structure.
 * @param macStr   Pointer to a buffer where the resulting string will be stored
 *                 The buffer must be at least 18 bytes to hold the full MAC 
 *                 string plus the null terminator.
 */
void
eth_lwip_get_hwaddr_str(struct netif *netif, uint8_t *macStr)
{
  uint8_t index, outindex = 0;
  char ch;

  for (index = 0; index < netif->hwaddr_len; index++) {
    if (index)
      macStr[outindex++] = ':';
    ch = (netif->hwaddr[index] >> 4);
    macStr[outindex++] = (ch < 10) ? (ch + '0') : (ch - 10 + 'A');
    ch = (netif->hwaddr[index] & 0xf);
    macStr[outindex++] = (ch < 10) ? (ch + '0') : (ch - 10 + 'A');
  }
  macStr[outindex] = 0;
}
