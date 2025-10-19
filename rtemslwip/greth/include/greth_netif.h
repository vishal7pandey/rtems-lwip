/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file greth_netif.h
 * @brief GRETH network interface initialization and debug functions
 *
 * This header provides initialization functions for the GRETH Ethernet device
 * and the lwIP stack. It also provides debug utilities
 * when GRETH_DEBUG is enabled.
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

#ifndef __GRETH_NETIF_H
#define __GRETH_NETIF_H

#include "netif/etharp.h"       /* includes - lwip/ip.h, lwip/netif.h, 
                                lwip/ip_addr.h, lwip/pbuf.h */

#define GRETH_DEBUG 1 
/*#define GRETH_GBIT_TEST 1 */
#define GRETH_AUTONEGO_PRINT_TIME 0

#if GRETH_DEBUG
#define greth_debug_printf(...) iprintf(__VA_ARGS__)
#else
#define greth_debug_printf(...) 
#endif

err_t greth_init_dev_and_lwip_netif(struct netif *netif);
struct greth_netif_state *greth_init_state();

#if GRETH_DEBUG
void greth_debug_print_info(struct netif *netif);
#endif /* GRETH_NETIF_DEBUG */

#endif /* __GRETH_NETIF_H */
