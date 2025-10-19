/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file greth_phy.h
 * @brief GRETH PHY driver definitions and functions
 *
 * This header file contains MII register definitions, PHY capabilities,
 * and function prototypes for initializing and autonegotiating PHY devices
 * connected to the GRETH Ethernet controller.
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

#ifndef __DRV_PHY_H
#define __DRV_PHY_H

#include<stdbool.h>

#include "greth_emac.h"
#include "greth_mdio.h"
#include "greth_phy.h"

struct greth_netif_state;
struct phy_device_info;

#define GRETH_AUTONEGO_TIMEOUT_MS 4000


extern const struct timespec greth_tan;

#ifdef __cplusplus
extern "C" {
#endif

/* MII registers */
#define GRETH_MII_EXTADV_1000FD 0x00000200 /**< 1000 Mbps FD adv capability */
#define GRETH_MII_EXTADV_1000HD 0x00000100 /**< 1000 Mbps HD adv capability */
#define GRETH_MII_EXTPRT_1000FD 0x00000800 /**< 1000 Mbps FD partner ability */
#define GRETH_MII_EXTPRT_1000HD 0x00000400 /**< 1000 Mbps HD partner ability */

#define GRETH_MII_100T4         0x00000200 /**< 100BASE-T4 capability */
#define GRETH_MII_100TXFD       0x00000100 /**< 100BASE-TX FD capability */
#define GRETH_MII_100TXHD       0x00000080 /**< 100BASE-TX HD capability */
#define GRETH_MII_10FD          0x00000040 /**< 10 Mbps FD capability */
#define GRETH_MII_10HD          0x00000020 /**< 10 Mbps HD capability */


/**
 * Resets PHY
 *
 * @param greth_dev    Pointer to GRETH Device struct
 * @param phy_dev      Pointer to PHY Device struct
 *
 * @note Calling this function is blocking until PHY indicates the reset process 
 * is complete.
 */
void PHY_reset(struct greth_netif_state *greth_dev);

/**
 * This function starts autonegotiaon with the other PHY device connected
 * to the GRETH PHY.
 *
 * @param greth_dev    Pointer to GRETH Device struct
 * @param phy_dev      Pointer to PHY Device struct
 *
 *  @return TRUE if autonegotiation succesful, FALSE if autonegotiation failed
 */
bool PHY_start_auto_negotiate(struct greth_netif_state *greth_dev, 
                                            struct phy_device_info *phy_dev);

/**
 * This function does Autonegotiates with the PHYC device connected
 * to the PHY of GRETH. It will wait till the autonegotiation completes.
 *
 * @param greth_dev    Pointer to GRETH Device struct
 * @param phy_dev      Pointer to PHY Device struct
 *
 * @return TRUE if autonegotiation succesful, FALSE if autonegotiation failed
 *
 */
bool PHY_auto_negotiate(struct greth_netif_state *greth_dev, 
                                            struct phy_device_info *phy_dev);

/**
 * This function examines, whether autonegotiation is done and if yes, then 
 * completes things post it. Must be called after PHY_start_auto_negotiate.
 *
 * @param greth_dev    Pointer to GRETH Device struct
 * @param phy_dev      Pointer to PHY Device struct
 *
 * @return TRUE if post-autonegotiation procedure succesful and done, FALSE if 
 * post-autonegotiation procedure failed.
 */
bool PHY_post_auto_negotiate(struct greth_netif_state *greth_dev, 
                                            struct phy_device_info *phy_dev);

#ifdef __cplusplus
}
#endif
#endif /* __DRV_PHY_H */