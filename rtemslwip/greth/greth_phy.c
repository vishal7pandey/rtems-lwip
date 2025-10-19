/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @file greth_phy.c
 * @brief GRETH PHY control and autonegotiation functions.
 *
 * This file provides functions to control and configure the PHY device
 * connected to the GRETH Ethernet MAC. It includes routines to reset the PHY,
 * start and monitor autonegotiation, determine link parameters (speed, duplex),
 * and perform post-autonegotiation configuration based on PHY and MAC 
 * capabilities.
 *
 * The following functions are included:
 * - PHY_reset(): Perform a hardware reset of the PHY.
 * - PHY_start_auto_negotiate(): Initiate autonegotiation and wait for 
 * completion.
 * - PHY_auto_negotiate(): Monitor autonegotiation, determine link speed, 
 * duplex mode, and update the GRETH device state.
 * - PHY_post_auto_negotiate(): Read PHY manufacturing information and finalize
 *   PHY configuration after autonegotiation.
 *
 * Constants:
 * - greth_tan: Timeout value for PHY autonegotiation derived from
 *   GRETH_AUTONEGO_TIMEOUT_MS.
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

#include "greth_phy.h"
#include "greth_emac.h"
#include "greth_netif.h"

#ifndef TRUE
/**
 * Boolean definition for TRUE
 */
#define TRUE 1
#endif


#ifndef FALSE
/**
 * Boolean definition for FALSE
 */
#define FALSE 0
#endif

/**
 * @brief Timeout value for GRETH PHY autonegotiation.
 *
 * This constant specifies the timeout for the GRETH PHY autonegotiation process
 * expressed as a `struct timespec`. The `tv_sec` field contains the timeout in 
 * seconds, and `tv_nsec` contains the remaining nanoseconds.
 *
 * @note The timeout value is derived from `GRETH_AUTONEGO_TIMEOUT_MS`.
 */
const struct timespec greth_tan = {
   GRETH_AUTONEGO_TIMEOUT_MS/1000,
   (GRETH_AUTONEGO_TIMEOUT_MS % 1000) *1000000
};

/**
 * @brief Reset the GRETH PHY device.
 *
 * This function performs a hardware reset of the PHY connected to the GRETH 
 * Ethernet MAC. It reads the PHY control register, asserts the reset bit, and 
 * waits for the reset to complete. After the reset, the PHY status register is 
 * read to update the internal state.
 *
 * @param greth_dev Pointer to the GRETH device state structure.
 *
 * @note This function is blocking until the PHY indicates that the reset 
 * process has completed.
 */
void PHY_reset(struct greth_netif_state *greth_dev)
{
  greth_dev->phy_dev.phyAddr = (greth_dev->regs->mdio_ctrl >> 11) & 0x1F;
  MDIOPhyRegRead(greth_dev, greth_dev->phy_dev.phyAddr, 0, 
                                                  &greth_dev->phy_dev.phyCtrl);
  while (greth_dev->phy_dev.phyCtrl & 0x8000); 
  MDIOPhyRegWrite(greth_dev, greth_dev->phy_dev.phyAddr, 0,  
                                          greth_dev->phy_dev.phyCtrl & 0x8000); 
  while (greth_dev->phy_dev.phyCtrl & 0x8000); 
  MDIOPhyRegRead(greth_dev, greth_dev->phy_dev.phyAddr, 1, 
                                                &greth_dev->phy_dev.phyStatus);
}

/**
 * @brief Start autonegotiation with the PHY device.
 *
 * This function performs a PHY reset, configures the PHY for autonegotiation 
 * (disabling gigabit advertisement if the MAC does not support it), and then 
 * initiates the autonegotiation process. It waits for autonegotiation to 
 * complete by calling `PHY_auto_negotiate`.
 *
 * @param greth_dev Pointer to the GRETH device state structure.
 * @param phy_dev   Pointer to the PHY device information structure.
 *
 * @return bool
 * @retval TRUE  Autonegotiation completed successfully.
 * @retval FALSE Autonegotiation failed or timed out.
 *
 * @note This function is blocking until autonegotiation completes.
 */
bool PHY_start_auto_negotiate(struct greth_netif_state *greth_dev, 
                                                struct phy_device_info *phy_dev)
{
  PHY_reset(greth_dev);
  MDIOPhyRegRead(greth_dev, phy_dev->phyAddr, 1, &phy_dev->phyStatus); 
  MDIOPhyRegRead(greth_dev, phy_dev->phyAddr, 0, &phy_dev->phyCtrl);
  if ((!greth_dev->gbit_mac) && (phy_dev->phyStatus & 0x100)){ 
    MDIOPhyRegWrite(greth_dev, phy_dev->phyAddr, 9, 0);
  }
  if(phy_dev->phyStatus & 0x08){ //if PHY suppport AutoNeg
    MDIOPhyRegWrite(greth_dev, phy_dev->phyAddr, 0, (phy_dev->phyCtrl |0x1200)); 
    MDIOPhyRegRead(greth_dev, phy_dev->phyAddr, 0, &phy_dev->phyCtrl);
  }
  bool autoneg_done = PHY_auto_negotiate(greth_dev, &greth_dev->phy_dev);
  if(!autoneg_done){
    return FALSE;
  }
  return TRUE;
}


/**
 * @brief Perform PHY autonegotiation and determine link parameters.
 *
 * This function performs the autonegotiation process between the GRETH MAC
 * and the connected PHY. It waits until autonegotiation completes or times out,
 * and sets the link speed, duplex mode, and autonegotiation status in the
 * @ref `greth_netif_state` structure.
 *
 * @param greth_dev Pointer to the GRETH device state structure.
 * @param phy_dev   Pointer to the PHY device information structure.
 *
 * @return bool
 * @retval TRUE  Autonegotiation completed successfully.
 * @retval FALSE Autonegotiation failed or timed out.
 *
 * @note This function is blocking and waits for autonegotiation completion.
 * @note After this function, the `greth_dev` structure fields `sp`, `gb`, `fd`,
 *       and `auto_neg` are updated to reflect the negotiated speed, gigabit
 *       support, full-duplex mode, and autonegotiation status, respectively.
 */
bool PHY_auto_negotiate(struct greth_netif_state *greth_dev, 
                                                struct phy_device_info *phy_dev)
{
  greth_debug_printf("greth_phy.c: Autonegotiation started: check on cable if \
                                                          it's connected!\r\n");
  uint32_t tmp1;
  greth_dev->gb = greth_dev->fd = greth_dev->sp = greth_dev->auto_neg = 0;
  struct timespec tstart, tnow;
  timespecclear(&greth_dev->auto_neg_time);
  if ((phy_dev->phyCtrl >> 12) & 1) { 
    greth_dev->auto_neg = 1;
    if (rtems_clock_get_uptime(&tstart) != RTEMS_SUCCESSFUL){
      printk("\nrtems_clock_get_uptime failed\n");
    }  
    MDIOPhyRegRead(greth_dev, phy_dev->phyAddr, 1, &phy_dev->phyStatus);
    while (!((phy_dev->phyStatus >> 5) & 1)) {
      if (rtems_clock_get_uptime(&tnow) != RTEMS_SUCCESSFUL){
        printk("rtems_clock_get_uptime failed\n");
      }              
      timespecsub(&tnow, &tstart, &greth_dev->auto_neg_time); 
      if (timespeccmp(&greth_dev->auto_neg_time, &greth_tan, >)) {
        greth_dev->auto_neg = -1; /* Failed */
        MDIOPhyRegRead(greth_dev, phy_dev->phyAddr, 0, &tmp1);
        greth_dev->gb = ((phy_dev->phyCtrl >> 6) & 1) && 
                                                !((phy_dev->phyCtrl >> 13) & 1); 
        greth_dev->sp = !((phy_dev->phyCtrl >> 6) & 1) && 
                                                ((phy_dev->phyCtrl >> 13) & 1); 
        greth_dev->fd = (phy_dev->phyCtrl >> 8) & 1;
        phy_dev->autoneg = FALSE;
        return FALSE;
      }
      rtems_task_wake_after(rtems_clock_get_ticks_per_second()/32);
    }
    MDIOPhyRegRead(greth_dev, phy_dev->phyAddr, 4, &greth_dev->phy_dev.adv);
    
    MDIOPhyRegRead(greth_dev, phy_dev->phyAddr, 5, &greth_dev->phy_dev.part);
    if ((phy_dev->phyStatus >> 8) & 1) { 
      MDIOPhyRegRead(greth_dev, phy_dev->phyAddr, 9,&greth_dev->phy_dev.extadv); 
      MDIOPhyRegRead(greth_dev, phy_dev->phyAddr, 10, 
                                                  &greth_dev->phy_dev.extpart);
      if ((greth_dev->phy_dev.extadv & GRETH_MII_EXTADV_1000FD) &&
        (greth_dev->phy_dev.extpart & GRETH_MII_EXTPRT_1000FD)){
        greth_dev->gb = 1;
        greth_dev->fd = 1;
      }
      else if ((greth_dev->phy_dev.extadv & GRETH_MII_EXTADV_1000HD) &&
        (greth_dev->phy_dev.extpart & GRETH_MII_EXTPRT_1000HD)){
        greth_dev->gb = 1;
        greth_dev->fd = 0;
      }
    }
    if ((greth_dev->gb == 0) || ((greth_dev->gb == 1) && 
                                                  (greth_dev->gbit_mac == 0))){
      if ((greth_dev->phy_dev.adv & GRETH_MII_100TXFD) &&
        (greth_dev->phy_dev.part & GRETH_MII_100TXFD)) {
        greth_dev->sp = 1;
        greth_dev->fd = 1;
      }
      else if ((greth_dev->phy_dev.adv & GRETH_MII_100TXHD) &&
        (greth_dev->phy_dev.part & GRETH_MII_100TXHD)) {
        greth_dev->sp = 1;
        greth_dev->fd = 0;
      }
      else if ((greth_dev->phy_dev.adv & GRETH_MII_10FD) &&
        (greth_dev->phy_dev.part & GRETH_MII_10FD)) {
        greth_dev->fd = 1;
      }
    }
  }
  phy_dev->autoneg = TRUE;
  return TRUE;
}

/**
 * @brief Perform post-autonegotiation PHY configuration.
 *
 * This function is called after the autonegotiation process has completed.
 * It reads PHY manufacturing information (vendor, device, revision), updates
 * the GRETH device link parameters, and performs any necessary PHY resets or
 * special configurations depending on PHY type and MAC capabilities.
 *
 * @param greth_dev Pointer to the GRETH device state structure.
 * @param phy_dev   Pointer to the PHY device information structure.
 *
 * @return bool
 * @retval TRUE  Post-autonegotiation configuration succeeded.
 * @retval FALSE Autonegotiation was not completed, so post-configuration
 *               was not performed.
 */
bool PHY_post_auto_negotiate(struct greth_netif_state *greth_dev, 
                                                struct phy_device_info *phy_dev)
{
  if(phy_dev->autoneg == TRUE){
    //set PHY Manufacturing Information
    phy_dev->vendor = 0;
    phy_dev->device = 0;
    phy_dev->rev = 0;
    MDIOPhyRegRead(greth_dev, phy_dev->phyAddr, 1, &phy_dev->phyStatus);

    uint32_t tmp1, tmp2, tmp3;

    /*Read out PHY info if extended registers are available */
    if (phy_dev->phyStatus & 1) {  
      MDIOPhyRegRead(greth_dev, phy_dev->phyAddr, 2, &tmp1);
      MDIOPhyRegRead(greth_dev, phy_dev->phyAddr, 3, &tmp2);

      greth_dev->phy_dev.vendor = (tmp1 << 6) | ((tmp2 >> 10) & 0x3F);
      greth_dev->phy_dev.rev = tmp2 & 0xF;
      greth_dev->phy_dev.device = (tmp2 >> 4) & 0x3F;
    }

    if ((phy_dev->phyStatus & 1) && (phy_dev->vendor == 0x005043) && 
                                                    (phy_dev->device == 0x0C)) {
      if (((greth_dev->gb) && !(greth_dev->gbit_mac)) || 
                                              !((phy_dev->phyCtrl >> 12) & 1)) { 
        greth_dev->sp = greth_dev->sp << 13;
        tmp3 = 0x8000;
        MDIOPhyRegWrite(greth_dev, phy_dev->phyAddr, 0, greth_dev->sp);
        MDIOPhyRegWrite(greth_dev, phy_dev->phyAddr, 0, tmp3);
        greth_dev->gb = 0;
        greth_dev->sp = 0;
        greth_dev->fd = 0;
      }
    } 
    else {
      if (((greth_dev->gb) && !(greth_dev->gbit_mac))  || 
                                              !((phy_dev->phyCtrl >> 12) & 1)) { 
        greth_dev->sp = greth_dev->sp << 13;
        tmp3 = 0x8000;
        MDIOPhyRegWrite(greth_dev, phy_dev->phyAddr, 0, greth_dev->sp);
        greth_dev->gb = 0;
        greth_dev->sp = 0;
        greth_dev->fd = 0;
      }
    }
    MDIOPhyRegRead(greth_dev, phy_dev->phyAddr, 0, &tmp3);
    while (tmp3 & 0x8000); 

    greth_dev->regs->ctrl = 0;
    greth_dev->regs->ctrl = GRETH_CTRL_RST;	
    greth_dev->regs->ctrl = 0;

    return TRUE;
  }
  else{
    return FALSE;
  }
}