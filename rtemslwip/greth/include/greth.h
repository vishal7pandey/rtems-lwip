/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file greth.h
 * @brief GRETH (Gigabit Ethernet) driver definitions
 *
 * This header file contains the register definitions, bit masks,
 * and data structures required for the GRETH Ethernet driver.
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

#ifndef _GR_ETH_
#define _GR_ETH_

#include "greth_emac.h"
#include "lwip/def.h"

struct emac_bd;

/**< Configuration Information */

typedef struct {
  void                 *base_address;
  rtems_vector_number   vector;
  uint32_t              txd_count;
  uint32_t              rxd_count;
} greth_configuration_t;


#define GRETH_TOTAL_BD           128
#define GRETH_MAXBUF_LEN         1520

/**< Tx BD */
#define GRETH_TXD_ENABLE      0x0800 /**< Tx BD Enable */
#define GRETH_TXD_WRAP        0x1000 /**< Tx BD Wrap (last BD) */
#define GRETH_TXD_IRQ         0x2000 /**< Tx BD IRQ Enable */
#define GRETH_TXD_MORE        0x20000 /**< Tx BD More (more descs for packet) */
#define GRETH_TXD_IPCS        0x40000 /**< Tx BD insert ip chksum */
#define GRETH_TXD_TCPCS       0x80000 /**< Tx BD insert tcp chksum */
#define GRETH_TXD_UDPCS       0x100000 /**< Tx BD insert udp chksum */

#define GRETH_TXD_UNDERRUN    0x4000 /**< Tx BD Underrun Status */
#define GRETH_TXD_RETLIM      0x8000 /**< Tx BD Retransmission Limit Status */
#define GRETH_TXD_LATECOL     0x10000 /**< Tx BD Late Collision */

#define GRETH_TXD_STATS       (GRETH_TXD_UNDERRUN            | \
                               GRETH_TXD_RETLIM              | \
                               GRETH_TXD_LATECOL)

#define GRETH_TXD_CS          (GRETH_TXD_IPCS            | \
                               GRETH_TXD_TCPCS           | \
                               GRETH_TXD_UDPCS)

/**< Rx BD */
#define GRETH_RXD_ENABLE      0x0800 /**< Rx BD Enable */
#define GRETH_RXD_WRAP        0x1000 /**< Rx BD Wrap (last BD) */
#define GRETH_RXD_IRQ         0x2000 /**< Rx BD IRQ Enable */

#define GRETH_RXD_DRIBBLE     0x4000 /**< Rx BD Dribble Nibble Status */
#define GRETH_RXD_TOOLONG     0x8000 /**< Rx BD Too Long Status */
#define GRETH_RXD_CRCERR      0x10000 /**< Rx BD CRC Error Status */
#define GRETH_RXD_OVERRUN     0x20000 /**< Rx BD Overrun Status */
#define GRETH_RXD_LENERR      0x40000 /**< Rx BD Length Error */
#define GRETH_RXD_ID          0x40000 /**< Rx BD IP Detected */
#define GRETH_RXD_IR          0x40000 /**< Rx BD IP Chksum Error */
#define GRETH_RXD_UD          0x40000 /**< Rx BD UDP Detected*/
#define GRETH_RXD_UR          0x40000 /**< Rx BD UDP Chksum Error */
#define GRETH_RXD_TD          0x40000 /**< Rx BD TCP Detected */
#define GRETH_RXD_TR          0x40000 /**< Rx BD TCP Chksum Error */


#define GRETH_RXD_STATS       (GRETH_RXD_OVERRUN             | \
                               GRETH_RXD_DRIBBLE             | \
                               GRETH_RXD_TOOLONG             | \
                               GRETH_RXD_CRCERR)

/**< CTRL Register */
#define GRETH_CTRL_TXEN         0x00000001 /**< Transmit Enable */
#define GRETH_CTRL_RXEN         0x00000002 /**< Receive Enable  */
#define GRETH_CTRL_TXIRQ        0x00000004 /**< Transmit Enable */
#define GRETH_CTRL_RXIRQ        0x00000008 /**< Receive Enable  */
#define GRETH_CTRL_FULLD        0x00000010 /**< Full Duplex */
#define GRETH_CTRL_PRO          0x00000020 /**< Promiscuous (receive all) */
#define GRETH_CTRL_RST          0x00000040 /**< Reset MAC */

/**< Status Register */
#define GRETH_STATUS_RXERR      0x00000001 /**< Receive Error */
#define GRETH_STATUS_TXERR      0x00000002 /**< Transmit Error IRQ */
#define GRETH_STATUS_RXIRQ      0x00000004 /**< Receive Frame IRQ */
#define GRETH_STATUS_TXIRQ      0x00000008 /**< Transmit Error IRQ */
#define GRETH_STATUS_RXAHBERR   0x00000010 /**< Receiver AHB Error */
#define GRETH_STATUS_TXAHBERR   0x00000020 /**< Transmitter AHB Error */

/**< MDIO Control  */
#define GRETH_MDIO_WRITE        0x00000001 /**< MDIO Write */
#define GRETH_MDIO_READ         0x00000002 /**< MDIO Read */
#define GRETH_MDIO_LINKFAIL     0x00000004 /**< MDIO Link failed */
#define GRETH_MDIO_BUSY         0x00000008 /**< MDIO Link Busy */
#define GRETH_MDIO_REGADR       0x000007C0 /**< Register Address */
#define GRETH_MDIO_PHYADR       0x0000F800 /**< PHY address */
#define GRETH_MDIO_DATA         0xFFFF0000 /**< MDIO DATA */

#endif

