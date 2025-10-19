/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file lwipbspopts.h
 * @brief GRETH/LwIP BSP configuration options
 *
 * This header defines LwIP buffer pool sizes, ARP configuration, and
 * related Ethernet BSP options for GRETH-based network interfaces.
 * It allows selecting dynamic or static ARP entries, ARP timer intervals,
 * and pbuf pool sizes.
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

#include <legacy_lwipopts.h>

#define PBUF_POOL_SIZE      48434          /**< Number of pbufs in the pool */
#define PBUF_POOL_BUFSIZE   1512           /**< Size of each pbuf buffer */
#define PBUF_LEN_MAX        PBUF_POOL_SIZE /**< Maximum pbuf length */

/*#define GRETH_DYN_ARP       1*/            /**< Enable dynamic ARP entries */
#define GRETH_STATIC_ARP    1              /**< Enable static ARP entries */

#if GRETH_DYN_ARP
    #define GRETH_STATIC_ARP    0      /**< Disable static ARP */
    #define ARP_TMR_INTERVAL    1000   /**< ARP timer interval in ms */
    #define ARP_TABLE_SIZE      10     /**< Number of dynamic ARP table entries */
#endif

#if GRETH_STATIC_ARP
    #define GRETH_DYN_ARP                   0 /**< Disable dynamic ARP */
    #define ETHARP_SUPPORT_STATIC_ENTRIES   1 /**< Enable LwIP support 
                                                for static ARP entries */
#endif
