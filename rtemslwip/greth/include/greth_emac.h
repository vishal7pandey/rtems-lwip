/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file greth_emac.h
 * @brief GRETH Ethernet MAC (EMAC) structures and utility functions.
 *
 * This header defines the GRETH EMAC buffer descriptor structure (emac_bd),
 * hardware register mapping (greth_regs), PHY device information 
 * (phy_device_info), and the GRETH device runtime state (greth_netif_state)
 * 
 * It also provides inline utility functions such as EMACMACSrcAddrSet to
 * configure the MAC hardware address.
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

#pragma once

#include "greth_netif.h"
#include "greth.h"
#include "greth_phy.h"
#include "lwip/sys.h"
#include<stdbool.h>

/**
 * @struct emac_bd
 * @brief Ethernet MAC Buffer Descriptor structure.
 *
 * This structure represents a buffer descriptor used by the GRETH Ethernet
 * hardware. Each descriptor contains a control field and a pointer/address
 * to the data buffer. The structure is aligned to 8 bytes for proper
 * hardware access.
 */
struct emac_bd{
  volatile uint32_t ctrl;  /**< Control field of Register */
  uint32_t addr;           /**< Address of payload field of pbuf assigned to BD*/
} __attribute((aligned(8)));

/** 
 * @struct greth_regs
 * @brief GRETH EMAC Configuratin Registers 
 */
typedef struct _greth_regs {
   volatile uint32_t ctrl;          /**< Control Register */
   volatile uint32_t status;        /**< Status Register */
   volatile uint32_t mac_addr_msb;  /**< MSB i.e. Bit 47-32 of MAC address */
   volatile uint32_t mac_addr_lsb;  /**< LSB i.e. Bit 31-0 of MAC address */
   volatile uint32_t mdio_ctrl;     /**< MDIO control and status register */
   volatile uint32_t txdesc;        /**< First Transmit descriptor pointer */
   volatile uint32_t rxdesc;        /**< First Receive descriptor pointer */
} greth_regs;

/** 
 * @struct phy_device_info
 * @brief PHY device information and capabilities
 */
struct phy_device_info {
   uint32_t vendor;      /**< Vendor identifier */
   uint32_t device;      /**< Device identifier */
   uint32_t rev;         /**< Revision number */

   uint32_t adv;         /**< Advertised capabilities */
   uint32_t part;        /**< Part number */

   uint32_t extadv;      /**< Extended advertised capabilities */
   uint32_t extpart;     /**< Extended part number */

   uint32_t phyAddr;     /**< PHY address on the MDIO bus */
   uint32_t phyStatus;   /**< Current PHY status register */
   uint32_t phyCtrl;     /**< PHY control register */
   bool autoneg;         /**< Autonegotiation enabled/disabled */
};

/** 
 * @struct greth_netif_state
 * @brief GRETH device runtime state and configuration
 */
struct greth_netif_state
{
    u32_t inst_num;                    /**< Device instance number */
    unsigned int vec;                  /**< Interrupt vector number */

    uint32_t num_tx_bd;                /**< Number of TX descriptors */
    uint32_t num_rx_bd;                /**< Number of RX descriptors */

    u32_t phy_addr;                    /**< PHY address */
    uint32_t waitTimeForPHYAnegSec;    /**< PHY autonegotiation wait time */

    greth_regs *regs;                  /**< GRETH MAC registers */
    sys_sem_t intPend_sem;             /**< Interrupt semaphore */

    volatile u32_t *emac_base;         /**< EMAC base address */
    volatile u32_t *emac_ctrl_base;    /**< EMAC control base */
    volatile u32_t emac_ctrl_ram;      /**< EMAC RAM mirror */
    volatile u32_t *mdio_base;         /**< MDIO base */

    /** 
    * @struct tx_comm_chn
    * @brief Transmit channel state
    */
    struct tx_comm_chn {
        volatile struct emac_bd *desc_array; /**< TX descriptors */
        unsigned int head;                   /**< Next TX descriptor to queue */
        unsigned int tail;                   /**< Next TX descriptor to check */
        struct pbuf **tx_pbuf_ref;           /**< TX pbuf pointers */
    } tx_channel __attribute((aligned(8)));

    /** 
    * @struct rx_comm_chn
    * @brief Receive channel state
    */
    struct rx_comm_chn {
        volatile struct emac_bd *desc_array; /**< RX descriptors */
        unsigned int head;                   /**< Next RX descriptor to store */
        unsigned int tail;                   /**< Next RX descriptor to check */
        u32_t freed_pbuf_len;                /**< Freed pbuf length */
        struct pbuf **rx_pbuf_ref;           /**< RX pbuf pointers */
    } rx_channel __attribute((aligned(8)));

    int acceptBroadcast;                 /**< Accept broadcast pktif != 0 */
    rtems_id daemonTid;                  /**< Background task ID */

    unsigned int tx_dptr;                /**< Next TX descriptor to check */
    unsigned int curr_bd_in_use;         /**< TX descriptors in use */
    unsigned int rx_ptr;                 /**< Next RX descriptor to store */
    unsigned int txbufs;                 /**< TX ring size */
    unsigned int rxbufs;                 /**< RX ring size */
    struct emac_bd *txdesc;              /**< TX descriptor array */
    struct emac_bd *rxdesc;              /**< RX descriptor array */
    rtems_vector_number vector;          /**< Interrupt vector */

    int tx_int_gen;                       /**< TX interrupt interval */
    int tx_int_gen_cur;                   /**< Current TX interrupt countdown */
    struct pbuf *next_tx_pbuf;           /**< Pending TX packet */
    int max_fragsize_ever;                /**< Max fragments in any packet */

    struct phy_device_info phy_dev;      /**< PHY info */
    uint32_t fd;                         /**< Full duplex enabled */
    uint32_t sp;                         /**< Speed: 100=1, 10=0 */
    uint32_t gb;                         /**< Gigabit mode available */
    uint32_t gbit_mac;                   /**< MAC supports Gigabit */
    uint32_t auto_neg;                   /**< Autonegotiation enabled */
    struct timespec auto_neg_time;       /**< Autonegotiation time */

    /** @brief Statistics */
    unsigned long rxInterrupts;          
    unsigned long rxPackets;             
    unsigned long rxLengthError;         
    unsigned long rxNonOctet;            
    unsigned long rxBadCRC;              
    unsigned long rxOverrun;             

    unsigned long txInterrupts;          
    unsigned long txDeferrred;           
    unsigned long txHeartbeat;           
    unsigned long txLateCollision;       
    unsigned long txRetryLimit;          
    unsigned long txUnderrun;            
};



/***************************EMAC Specific fns******************** */
/**
 * @brief Set the GRETH MAC hardware address
 * 
 * This function programs the GRETH MAC registers with the provided 6-byte MAC 
 * address.
 * 
 * @param greth_dev Pointer to the GRETH device state structure
 * @param macAddr   Pointer to a 6-byte MAC address array
 * 
 * @returns void
 */
static inline void EMACMACSrcAddrSet(struct greth_netif_state *greth_dev, 
                                                               uint8_t *macAddr)
{
  greth_regs *regs = greth_dev->regs;
  regs->mac_addr_msb = (macAddr[0] << 8) | macAddr[1];               
  regs->mac_addr_lsb = (macAddr[2] << 24) |(macAddr[3] << 16) | 
                                                (macAddr[4] << 8) | macAddr[5];
  greth_debug_printf("GRETH MAC MSB : %x\n", regs->mac_addr_msb);
  greth_debug_printf("GRETH MAC LSB : %x\n", regs->mac_addr_lsb);
}
