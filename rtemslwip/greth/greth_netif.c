/* SPDX-License-Identifier: BSD-2-Clause */

/**
 * @defgroup GRETH GRETH lwIP Driver
 * @brief THis group contains documentation for code for GRETH lwIP driver
 */

/**
 * @file 
 * 
 * @ingroup GRETH
 * 
 * @brief This source file contains various functions to initialize GRETH lwIP 
 * Driver and perform RX and Gigabit as well as non-Gigabit TX operations.
 * 
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

#include "lwip/init.h"
#if LWIP_VERSION_MAJOR >= 2
#include "lwip/timeouts.h"
#else /*LWIP_VERSION_MAJOR*/
#include "lwip/timers.h"        
#endif /*LWIP_VERSION_MAJOR*/
#include "lwip/sys.h" /*!<includes - lwip/opt.h, lwip/err.h, arch/sys_arch.h */
#include "lwip/tcpip.h"/* includes - lwip/opt.h, lwip/api_msg.h, lwip/netifapi.h
                       , lwip/pbuf.h, lwip/api.h, lwip/sys.h, lwip/timers.h, 
                       lwip/netif.h */
#include "lwip/stats.h"    /* includes - lwip/mem.h, lwip/memp.h, lwip/opt.h */
#include "netif/etharp.h"  /* includes - lwip/ip.h, lwip/netif.h, lwip/ip_addr.h
                          , lwip/pbuf.h */
#include <lwip/netifapi.h>

#include "greth_emac.h"         
#include "greth_mdio.h"
#include "greth_netif.h"
#include "eth_lwip.h"
#include "./leon3/include/leon3_greth.h"

#include<stddef.h>
#include<string.h>
#include<stdlib.h> 
#include<inttypes.h>
#include<math.h>
#include "lwipbspopts.h"


#define IFNAME0 'g'
#define IFNAME1 'r'

#define TIME_PHY_AUTONEG   4000  /* Time to wait for autonegotiation in ticks.*/
#define MAX_TRANSFER_UNIT     1520    /* take in account oversized frames */

#define MEM_ALIGN_BOUNDARY      4
#define GRETH_INTERRUPT_VEC_NUM 6


#ifdef malloc
#undef malloc
#endif
#ifdef free
#undef free
#endif

#define RBUF_SIZE 1512

/* Used when reading from memory written by GRETH DMA unit */
#ifndef GRETH_MEM_LOAD
#define GRETH_MEM_LOAD(addr) (*(volatile unsigned int *)(addr))
#endif

/* Maximum number of PBUFs preallocated in the driver
 * init function to be used for the RX
 */
#define MAX_RX_PBUF_ALLOC           10
#define MIN_PKT_LEN                 60

/* StartUp initialized indicator */
static bool initialized = false;

/**************************Forward Declarations********************************/

/* A. Interrupt and control structure related functions */
static err_t greth_init_control_structures(struct netif *netif);
static err_t 
greth_install_interrupt_handlers(struct greth_netif_state *nf_state);
SYS_IRQ_HANDLER_FNC(greth_interrupt_handler);
static void greth_process_irq_request(void *argument);
static void greth_process_irq(void *argument);

/* B. initialization of GRETH lwIP Driver */
struct greth_netif_state *greth_init_state(void);
static void greth_lwip_netif_fill(struct netif *netif);
err_t greth_init_dev_and_lwip_netif(struct netif *netif);
static void *almalloc(size_t size, struct greth_netif_state *nf_state);
static void greth_init_buffer_descriptors(struct greth_netif_state *nf_state);
static err_t greth_init_hw(struct netif *netif);
static void greth_print_init_info(struct greth_netif_state *nf_state);
static void 
greth_hw_set_hwaddr(struct greth_netif_state *greth_dev, uint8_t *macAddr);
static void greth_rx_pbuf_refill(struct greth_netif_state *nf_state);

/* C. Transmission related Functions */
static err_t greth_process_irq_tx(void *arg);
static err_t greth_process_irq_tx_gbit(void *arg);
static err_t greth_send_gbit(struct netif *netif, struct pbuf *p_start);
static err_t greth_send(struct netif *netif, struct pbuf *p); 

/* D. Reception related Functions */
static void greth_process_irq_rx(struct netif* ntf);

/**
 * @brief Initialize semaphore and thread for interrupt
 * @param netif Pointer to lwIP netif structure
 * @retval ERR_OK Successful Thread and Semaphore creation
 * @retval err_t Error in Thread or Semaphore creation
 */
static err_t greth_init_control_structures(struct netif *netif)
{
  err_t res;
  sys_thread_t tx_thread_id;
  struct greth_netif_state *nf_state;

  nf_state = netif->state;

  res = sys_sem_new(&nf_state->intPend_sem, 0); 
  if (res != ERR_OK) {
    sys_arch_printk("\nERROR! semaphore creation error - 0x%08lx\n", (long)res);
  }
  tx_thread_id = sys_thread_new("GRLW", greth_process_irq_request, netif, 
                                                              4096, 100); 
  if (tx_thread_id == 0) {
    sys_arch_printk("\nERROR! lwip interrupt thread not created");
    res = !ERR_OK;
  }
  return res;
}

/**
 * @brief Installing Interrupt handlers for TX and RX 
 * 
 * @param nf_state Pointer to GRETH Device State Structure
 * @retval ERR_OK Successful installation of Interrupt handlers
 * @retval ERR_IF Failure to install interrupt handlers
 */
static err_t 
greth_install_interrupt_handlers(struct greth_netif_state *nf_state){
  int res;
  greth_debug_printf("[DBG] greth_install_interrupt_handlers: Setting GRETH \
                        Interrupt (TX/RX) Vector Number : %u\n", nf_state->vec);
  res = sys_request_irq(nf_state->vec, greth_interrupt_handler, 0, 
                                                        "interrupt", nf_state);
  if (res < 0) {
    iprintf("[ERR] greth_install_interrupt_handlers: Failed to install \
                                      Interrupt handler; returned : %d\n", res);
    return ERR_IF;
  }
  return ERR_OK;
}

/**
 * @brief Interrupt Service Routine (ISR) for GRETH Ethernet driver.
 *
 * This handler is invoked whenever a GRETH interrupt occurs. It performs the
 * following actions:
 * - Retrieves GRETH driver state in the interrupt context.
 * - Masks the interrupt source to prevent re-entry until it is serviced.
 * - Signals the pending interrupt semaphore to wake up the driver task.
 *
 * @param arg Context provided by the SYS_IRQ_HANDLER_FNC macro
 *                (driver state pointer retrieved internally).
 */
SYS_IRQ_HANDLER_FNC(greth_interrupt_handler){
  struct greth_netif_state *nf_state = 
                      (struct greth_netif_state *)sys_irq_handler_get_context();

  sys_arch_mask_interrupt_source(nf_state->vector); 
  if (nf_state != NULL)
  sys_sem_signal_from_ISR(&nf_state->intPend_sem);
}

/** 
 * @brief Background interrpt processing task
 * - This function runs in an infinite loop, while waiting on an interrupt
 * pending semaphore signalled by GRETH ISR ('greth_interrupt_handler`)
 * - Once unblocked, it schedules actual interrupt processing routine to run in 
 * lwIP TCP/IP thread context
 * 
 * @param argument Void Pointer to lwIP netif struct
 * 
 * @retval void
 */
void static greth_process_irq_request(void *argument)
{
  struct netif *netif = (struct netif *)argument;
  struct greth_netif_state *nf_state;

  nf_state = netif->state;

  for (;;){
    sys_arch_sem_wait(&nf_state->intPend_sem, 0);
    tcpip_callback((tcpip_callback_fn)greth_process_irq, netif);
  }
}

/**
 * @brief Process interrupt events for GRETH lwIP Driver
 *
 * This function is scheduled from @ref greth_process_irq_request() and handles
 * deferred interrupt processing outside the ISR context. It:
 * - Reads and clears the interrupt status register.
 * - Checks for RX or TX events and dispatches to the appropriate handler.
 * - Unmasks the interrupt source after servicing the interrupt.
 *
 * @param argument Pointer to the lwIP netif struct.
 *
 * @note
 * - Runs in the TCP/IP thread context, not directly in the hardware ISR.
 * - RX events trigger @ref greth_process_irq_rx().
 * - TX events trigger @ref greth_process_irq_tx() or
 *   @ref greth_process_irq_tx_gbit() depending on hardware capabilities.
 * - Interrupts are masked in the ISR ( @ref greth_interrupt_handler() ) and 
 * unmasked here once processed.
 */
static void greth_process_irq(void *argument){
  struct netif *netif = (struct netif *)argument;
  struct greth_netif_state* nf_state = netif->state;
  uint32_t status = nf_state->regs->status;
  nf_state->regs->status = status;
  uint32_t ctrl = nf_state->regs->ctrl;
  greth_debug_printf("[DBG] greth_process_irq: Entered...\n");

  if (nf_state == NULL){
    iprintf("[MSG] greth_process_irq: GRETH Driver not initialized; \
                                                     greth_netif_state = NULL");
    return;
  }

  if ((ctrl & GRETH_CTRL_RXIRQ) && 
                          (status & (GRETH_STATUS_RXERR | GRETH_STATUS_RXIRQ))){ 
    greth_process_irq_rx(netif);
    sys_arch_unmask_interrupt_source(nf_state->vector);
  }
  if ( (ctrl & GRETH_CTRL_TXIRQ) 
                    && (status & (GRETH_STATUS_TXERR | GRETH_STATUS_TXIRQ)) ) {
    if(nf_state->gbit_mac){
      greth_process_irq_tx_gbit(netif);
    }
    else{
      greth_process_irq_tx(netif);
    }
    sys_arch_unmask_interrupt_source(nf_state->vector);
  }
}

/************************Initialization****************************************/

/**
 * @brief Initialize a new GRETH network interface state structure.
 *
 * Allocates memory for a @ref greth_netif_state object and initializes
 * default fields (such as PHY auto-negotiation timeout when RTEMS is
 * running with an OS). 
 *
 * @retval nf_state Pointer to an allocated and partially initialized
 *          @ref greth_netif_state structure or 
 * @retval NULL if memory allocation fails.
 */
struct greth_netif_state *greth_init_state(void)
{
  struct greth_netif_state *nf_state = 
          (struct greth_netif_state *)malloc(sizeof(struct greth_netif_state));

#if !NO_SYS
  nf_state->waitTimeForPHYAnegSec = TIME_PHY_AUTONEG;
#endif
  return nf_state;
}

/**
 * @brief Configure and initialize lwIP netif struct fields for GRETH.
 *
 * This helper function fills in the essential parameters of an lwIP
 * @ref netif structure for the GRETH Ethernet driver. It sets the hostname,
 * interface name, output function, MTU, and link flags. 
 *
 * @param netif Pointer to the lwIP netif struct to configure. The function 
 *              modifies its fields in place.
 */
static void greth_lwip_netif_fill(struct netif *netif)
{
  #if LWIP_NETIF_HOSTNAME
    netif->hostname = "greth";
  #endif

  netif->name[0] = IFNAME0;
  netif->name[1] = IFNAME1;

  netif->output = etharp_output;     
  
  /* maximum transfer unit */
  netif->mtu = MAX_TRANSFER_UNIT; 

  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP; 
}

/**
 * @brief Perform overall initialization of the GRETH device and its lwIP 
 *        netif struct.
 *
 * This function 
 * - attaches the GRETH driver
 * - allocates and initializes the @ref greth_netif_state structure
 * - configures the lwIP @ref netif structure
 * - sets the hardware MAC address
 * - initializes the GRETH hardware
 * - initializes control structures
 * - sets up buffer descriptors
 * - refills RX packet buffers for all RX descriptors
 * - installs interrupt handlers
 * It prints progress messages for each step of initialization.
 *
 * @param netif Pointer to the lwIP network interface structure associated with 
 *              this GRETH device. 
 *
 * @retval ERR_OK   Initialization completed successfully.
 * @retval ERR_IF   Driver attach failed, @ref greth_netif_state allocation 
 *                  failed, or GRETH device already initialized.
 * @retval other    Error code returned by low-level GRETH initialization
 *                  functions (e.g., @ref greth_init_hw(),
 *                  @ref greth_init_control_structures(),
 *                  @ref greth_install_interrupt_handlers()).
 */
err_t greth_init_dev_and_lwip_netif(struct netif *netif)
{

  err_t retVal;
  int total_tests=9, curr_test=0;
  struct greth_netif_state *nf_state = netif->state;

  int greth_attach = rtems_lwip_leon3_greth_driver_attach(nf_state);
  if(greth_attach != 0){
    return ERR_IF;
  }
  ++curr_test;
  iprintf("[%d/%d] greth_init_dev_and_lwip_netif: GRETH driver attached \
                                      successfully\n", curr_test, total_tests);
  
  if(initialized){
    return ERR_IF;
  }    
  if (nf_state == NULL) {
    if ((nf_state = greth_init_state()) == 0) {
      return ERR_IF;
    }
    netif->state = nf_state;
  }
  ++curr_test;
  iprintf("[%d/%d] greth_init_dev_and_lwip_netif: GRETH Network Interface \
                        State created successfully\n", curr_test, total_tests);

  greth_lwip_netif_fill(netif);
  ++curr_test;
  iprintf("[%d/%d] greth_init_dev_and_lwip_netif: lwIP Network Interface \
                          initialized successfully\n", curr_test, total_tests);

  greth_hw_set_hwaddr(nf_state, netif->hwaddr);
  ++curr_test;
  iprintf("[%d/%d] greth_init_dev_and_lwip_netif: GRETH MAC (Hardware) Address \
                          initialized successfully\n", curr_test, total_tests);

  if ((retVal = greth_init_hw(netif)) != ERR_OK) {
    iprintf("[ERR] greth_init_hw: %d\n", retVal);
    return retVal;
  }
  ++curr_test;
  iprintf("[%d/%d] greth_init_dev_and_lwip_netif: GRETH Hardware initialized \
                                      successfully\n", curr_test, total_tests);

  if ((retVal = greth_init_control_structures(netif)) != ERR_OK) {
    iprintf("greth_init_control_structures (ERR): %d\n", retVal);
    return retVal;
  }
  greth_print_init_info(nf_state);
  ++curr_test;
  iprintf("[%d/%d] greth_init_dev_and_lwip_netif: GRETH Control Structures \
                          initialized successfully\n", curr_test, total_tests);
  
  greth_init_buffer_descriptors(nf_state);
  ++curr_test;
  iprintf("[%d/%d] greth_init_dev_and_lwip_netif: GRETH Buffer Descriptors \
                          initialized successfully\n", curr_test, total_tests);

  nf_state->rx_channel.head = 0;
  for(int i=0; i<nf_state->num_rx_bd; i++){
    greth_rx_pbuf_refill(nf_state);
  }
  ++curr_test;
  iprintf("[%d/%d] greth_init_dev_and_lwip_netif: GRETH RX PBUFs refilled \
                                      successfully\n", curr_test, total_tests);

  if ((retVal = greth_install_interrupt_handlers(nf_state)) != ERR_OK) {
    iprintf("greth_install_interrupt_handlers (ERR): %d\n", retVal);
    return retVal;
  }
  ++curr_test;
  iprintf("[%d/%d] greth_init_dev_and_lwip_netif: GRETH Interrupt Handler \
                          initialized successfully\n", curr_test, total_tests);
  initialized = true;
  rtems_interrupt_lock_initialize(&nf_state->isr_lock, "greth");
  return ERR_OK;
}

/**
 * @brief Get the number of GRETH buffer descriptors.
 *
 * This function returns the number of buffer descriptors used by the GRETH
 * device. The value depends on whether the interface is operating in gigabit 
 * mode or standard mode, in which case the number is determined from the 
 * hardware status register.
 *
 * @param nf_state Pointer to the GRETH network interface state structure.
 *
 * @return The number of buffer descriptors as a `size_t`.
 *         - If `nf_state->gb` is true, returns 128 descriptors.
 *         - Otherwise, returns 128 multiplied by 2^N, where N is extracted
 *           from bits [27:24] of `nf_state->regs->status`.
 */
size_t get_number_of_descriptors(struct greth_netif_state* nf_state){
  if(nf_state->gb){
    return 128;
  }
  else{
    return (128 * (1U << ((nf_state->regs->status >> 24) & 0xF)));
  }
}

/**
 * @brief Allocates memory aligned to a specific boundary and zero-initialize it
 *
 * This function allocates a memory block of size @p size bytes, aligned 
 * according to the type of MAC interface in the GRETH network interface state. 
 * For a gigabit autonegotiation, the alignment is 1024 bytes. For non-gigabit, 
 * the alignment is calculated based on the status register and the size of the 
 * descriptor table. The allocated memory is then set to zero.
 *
 * @param size The size of memory to allocate in bytes.
 * @param nf_state Pointer to the GRETH network interface state structure.
 *
 * @return Pointer to the zero-initialized, aligned memory block if successful,
 *         or NULL if allocation fails.
 *
 * @note The size passed should ideally be a multiple of the alignment to ensure
 *       proper behavior with aligned_alloc.
 */
static void *almalloc(size_t size, struct greth_netif_state *nf_state)
{
  size_t num_desc = get_number_of_descriptors(nf_state);
  size_t align = num_desc * sizeof(struct emac_bd);
  void *tmp1 = aligned_alloc(align, size);
  if(tmp1 != NULL){
    return memset(tmp1, 0, size);
  }
  return NULL;
}


/**
 * @brief Initializes the GRETH transmit (TX) and receive (RX) descriptors.
 *
 * This function sets up the TX and RX buffer descriptor (BD) circular arrays 
 * for the GRETH driver. It allocates memory for the descriptors and associated
 * pbuf references, initializes control and address fields of descriptors, and 
 * links the descriptor arrays to the GRETH hardware registers.
 *
 * For TX descriptors:
 * - If the MAC is non-gigabit, each descriptor's `addr` field is allocated 
 * memory  with proper alignment (`MEM_ALIGN_BOUNDARY`) and `ctrl` is set to 0.
 * - If the MAC is gigabit, `addr` and `ctrl` are initialized to 0.
 *
 * For RX descriptors:
 * - Each descriptor's `ctrl` field is initialized with `GRETH_RXD_ENABLE` and
 *   `GRETH_RXD_IRQ`
 * - `addr` is initialized to 0.
 *
 * @param nf_state Pointer to the GRETH network interface state structure.
 */
static void greth_init_buffer_descriptors(struct greth_netif_state *nf_state)
{
  
  greth_regs *reg = nf_state->regs;

  uint32_t num_tx_bd = nf_state->num_tx_bd;  
  uint32_t num_rx_bd = nf_state->num_rx_bd;  

  size_t all_txbd_size = num_tx_bd * sizeof(struct emac_bd);
  size_t all_rxbd_size = num_rx_bd * sizeof(struct emac_bd);
  iprintf("[MSG] Size of all TX descriptors : %zu\n", all_txbd_size);

  struct tx_comm_chn *txch = &nf_state->tx_channel; 
  struct rx_comm_chn *rxch = &nf_state->rx_channel;

  txch->tx_pbuf_ref = calloc(nf_state->num_tx_bd, sizeof(*txch->tx_pbuf_ref));
  rxch->rx_pbuf_ref = calloc(nf_state->num_rx_bd, sizeof(*rxch->rx_pbuf_ref));

  txch->desc_array = 
  (volatile struct emac_bd *) almalloc(all_txbd_size, nf_state);
  txch->head = 0;
  reg->txdesc = (uintptr_t)txch->desc_array; 

  greth_debug_printf("[DBG] No. of TX Descriptors: %d\n", nf_state->num_tx_bd);
  greth_debug_printf("[DBG] No. of RX Descriptors: %d\n", nf_state->num_rx_bd);

  greth_debug_printf("[DBG] greth_init_buffer_descriptors: \
                                txch->desc_array[0] => %p\n", txch->desc_array);
  greth_debug_printf("[DBG] greth_init_buffer_descriptors: \
                                    before : reg->txdesc => %d\n", reg->txdesc);

  size_t max = GRETH_MAXBUF_LEN;
  if(!nf_state->gbit_mac){
    for (int i = 0; i < nf_state->num_tx_bd; i++) {
      txch->desc_array[i].addr = 
              (uintptr_t)((uint32_t*)(aligned_alloc(MEM_ALIGN_BOUNDARY, max)));
      txch->desc_array[i].ctrl = 0;
      greth_debug_printf("[DBG] greth_init_buffer_descriptors: \
                        current TX desc addr: %x\n", txch->desc_array[i].addr); 
      greth_debug_printf("[DBG] greth_init_buffer_descriptors: \
                        current TX desc ctrl: %x\n", txch->desc_array[i].ctrl);
    }
  }
  else{
    for (int i = 0; i < nf_state->num_tx_bd; i++) {
      txch->desc_array[i].addr = 0;
      txch->desc_array[i].ctrl = 0;
      greth_debug_printf("[DBG] greth_init_buffer_descriptors: \
                        current TX desc addr: %x\n", txch->desc_array[i].addr); 
      greth_debug_printf("[DBG] greth_init_buffer_descriptors: \
                        current TX desc ctrl: %x\n", txch->desc_array[i].ctrl);
    }
  }  

  greth_debug_printf("[DBG] greth_init_buffer_descriptors: after : \
                                            reg->txdesc => %d\n", reg->txdesc);

  rxch->desc_array = 
                  (volatile struct emac_bd *) almalloc(all_rxbd_size, nf_state); 
  rxch->head = rxch->tail = 0;
  rxch->freed_pbuf_len = PBUF_LEN_MAX;

  reg->rxdesc = (uintptr_t)rxch->desc_array; 
  greth_debug_printf("[DBG] greth_init_buffer_descriptors: \
                                rxch->desc_array[0] => %p\n", rxch->desc_array);
  greth_debug_printf("[DBG] greth_init_buffer_descriptors: before : \
                                            reg->rxdesc => %d\n", reg->rxdesc);

  for (int i = 0; i < (nf_state->num_rx_bd); i++) {
    rxch->desc_array[i].addr = 0;
    rxch->desc_array[i].ctrl = GRETH_RXD_ENABLE | GRETH_RXD_IRQ;
    greth_debug_printf("[DBG] greth_init_buffer_descriptors: \
                        current RX desc addr: %x\n", rxch->desc_array[i].addr); 
    greth_debug_printf("[DBG] greth_init_buffer_descriptors: \
                        current RX desc ctrl: %x\n", rxch->desc_array[i].ctrl);
  }
  greth_debug_printf("[DBG] greth_init_buffer_descriptors: after : \
                                            reg->rxdesc => %d\n", reg->rxdesc);
  txch->head = rxch->head = 0;
}

/**
 * @brief Initializes the GRETH hardware and PHY for a network interface.
 *
 * This function performs hardware initialization and PHY configuration for the
 * GRETH (Gigabit Ethernet) driver. It resets the GRETH controller, initializes
 * PHY settings, performs autonegotiation, and sets up the network interface
 * linkoutput functions according to the autonegotiated speed.
 *
 * Steps performed:
 * - Reset the GRETH controller and initialize control registers.
 * - Reset the PHY using PHY_reset().
 * - Check MAC type (gigabit or non-gigabit) 
 * - Configure PHY control registers if necessary.
 * - Start autonegotiation using PHY_auto_negotiate() and validate with
 *    PHY_post_auto_negotiate().
 * - Configure link output function for gigabit or normal MAC.
 * - Set TX interrupt generation threshold.
 * - Set maximum fragment size for non-gigabit MAC.
 * - Enable RX, RX IRQ in GRETH control register and set flags of duplex and 
 * speed settings
 *
 * @param netif Pointer to the LWIP network interface structure.
 *
 * @return ERR_OK if initialization was successful,
 *         ERR_IF if autonegotiation or post-autonegotiation failed.
 * @note The function supports conditional compilation for GRETH gigabit test 
 * mode using @ref GRETH_GBIT_TEST.
 */
static err_t greth_init_hw(struct netif *netif){

  struct greth_netif_state* nf_state = (struct greth_netif_state* )netif->state;
  
  nf_state->rxInterrupts = 0;
  nf_state->rxPackets = 0;

  struct phy_device_info *phy = &nf_state->phy_dev;
  nf_state->regs->ctrl = 0;
  nf_state->regs->ctrl = GRETH_CTRL_RST;
  nf_state->regs->ctrl = 0;
  iprintf("[MSG] greth_hw_init: GRETH Driver Reset\n");
  PHY_reset(nf_state);

  uint32_t gbit_mac = (nf_state->regs->ctrl >> 27) & 0x01;
  uint32_t tmp1 = 0;

  if(!gbit_mac && phy->phyStatus & 0x100){
    MDIOPhyRegRead(nf_state, phy->phyAddr, 9, &tmp1);
  }

  if(phy->phyStatus & 0x08){
    tmp1 = phy->phyCtrl | 0x1200;
    MDIOPhyRegWrite(nf_state, phy->phyAddr, 0, tmp1);
    MDIOPhyRegRead(nf_state, phy->phyAddr, 0, &phy->phyCtrl);
  }
 iprintf("greth_hw_init: Autonegotiation Started\n");
  bool autoneg_done = PHY_auto_negotiate(nf_state, &nf_state->phy_dev);
  if(!autoneg_done){
   iprintf("[ERR] greth_hw_init : Autonegotiation Unsuccessful.");
    return ERR_IF;
  }
 iprintf("[MSG] greth_hw_init: Autonegotiation Succuessful\n");
  bool post_autoneg = PHY_post_auto_negotiate(nf_state, &nf_state->phy_dev);
  if(!post_autoneg){
   iprintf("[ERR] greth_hw_init : Post-Autonegotiation Unsuccessful.\n");
    return ERR_IF;
  }
 iprintf("[ERR] greth_hw_init: Post-Autonegotiation Succuessful\n");

  #ifdef GRETH_GBIT_TEST
    nf_state->gb = 1;
    nf_state->gbit_mac = 1;
    nf_state->fd = 1;
    nf_state->sp = 1;
    greth_debug_printf("[DBG] GBit test mode set");
  #endif

  if(nf_state->gbit_mac){
    netif->linkoutput = greth_send_gbit;
    greth_debug_printf("[DBG] Using greth_send_gbit as linkoutput\n");
  } 
  else{
    netif->linkoutput = greth_send;
    greth_debug_printf("[DBG] Using greth_send as linkoutput\n");
  }

  if ( nf_state->num_tx_bd < 10 ) {
    nf_state->tx_int_gen = nf_state->tx_int_gen_cur = 1;
  }else{
    nf_state->tx_int_gen = nf_state->tx_int_gen_cur = nf_state->num_tx_bd/2;
  }

  if (!nf_state->gbit_mac){
    nf_state->max_fragsize_ever = 1;
  }
  nf_state->regs->status = 0xFFFFFFFF; 
  nf_state->regs->ctrl |= GRETH_CTRL_RXEN | (nf_state->fd << 4) | 
                  GRETH_CTRL_RXIRQ | (nf_state->sp << 7) | (nf_state->gb << 8);

  return ERR_OK;
}

/**
 * @brief Prints GRETH driver and PHY initialization information.
 *
 * This function outputs details about the attached GRETH network driver and
 * the connected PHY device, including vendor, device ID, revision, and the
 * current operating mode (speed and duplex). It also optionally prints the
 * autonegotiation time if GRETH_AUTONEGO_PRINT_TIME is defined.
 *
 * @param nf_state Pointer to the GRETH network interface state structure.
 */
static void greth_print_init_info(struct greth_netif_state *nf_state){
 iprintf("\ngreth: driver attached\n");
  if ( nf_state->auto_neg == -1 ){ 
   iprintf("Auto negotiation timed out. Selecting default config\n");
  }
 iprintf("**** PHY ****\n");
 iprintf("Vendor: %x   Device: %x   Revision: %d\n",nf_state->phy_dev.vendor, 
                              nf_state->phy_dev.device, nf_state->phy_dev.rev);
 iprintf("Current Operating Mode: \n");
  if (nf_state->gb) {
   iprintf("1 Gbit ");
  } else if (nf_state->sp) {
   iprintf("100 Mbit ");
  } else {
   iprintf("10 Mbit ");
  }
  if (nf_state->fd) {
   iprintf("Full Duplex\n");
  } else {
   iprintf("Half Duplex\n");
  }
#ifdef GRETH_AUTONEGO_PRINT_TIME
  if ( nf_state->phy_dev.autoneg ) {
    greth_debug_printf("Autonegotiation Time: %lldms\n\n", 
                        nf_state->auto_neg_time.tv_sec*1000 + 
                        nf_state->auto_neg_time.tv_nsec/1000000);
  }
#endif
}

/**
 * @brief Sets the hardware MAC address for the GRETH device.
 *
 * The function internally calls `EMACMACSrcAddrSet()` to write the address
 * to the GRETH hardware registers.
 *
 * @param greth_dev Pointer to the GRETH network interface state structure.
 * @param macAddr Pointer to a 6-byte array containing the MAC address to set.
 *
 */
static void greth_hw_set_hwaddr(struct greth_netif_state *greth_dev, 
                                                              uint8_t *macAddr)
{
  EMACMACSrcAddrSet(greth_dev, macAddr);
}

/**
 * @brief Refills the next RX buffer descriptor with a new pbuf.
 *
 * This function allocates a pbuf from the PBUF_POOL and assigns it to the
 * current RX buffer descriptor in the GRETH device. It updates the descriptor's
 * address and control fields, and advances the RX head pointer. The last 
 * descriptor in the ring is marked with the WRAP flag.
 *
 * @param nf_state Pointer to the GRETH network interface state structure.
 *
 * @note If pbuf allocation fails, the corresponding entry in array of pbuf 
 *        reference is set to NULL and an error message is printed.
 * @note Control flags set for each descriptor:
 *       - GRETH_RXD_ENABLE : Enables reception for this descriptor
 *       - GRETH_RXD_IRQ    : Generates an interrupt when packet is received
 *       - GRETH_RXD_WRAP   : Set only for the last descriptor in the ring
 * @note Uses `pbuf_alloc()` to allocate memory for incoming Ethernet frames.
 */
static void greth_rx_pbuf_refill(struct greth_netif_state *nf_state)
{
  struct rx_comm_chn *rxch = &nf_state->rx_channel;
  uint32_t head = rxch->head;

  struct pbuf *p = pbuf_alloc(PBUF_RAW, PBUF_POOL_BUFSIZE, PBUF_POOL);
  if (!p) {
      iprintf("[ERR] greth_rx_pbuf_refill: pbuf_alloc failed at head=%u\n", \
                                                                          head);
      rxch->rx_pbuf_ref[head] = NULL;
      return;
  }

  rxch->rx_pbuf_ref[head] = p;

  rxch->desc_array[head].addr = (uintptr_t)(p->payload);

  if (head == nf_state->num_rx_bd - 1) {
      rxch->desc_array[head].ctrl = 
                              GRETH_RXD_ENABLE | GRETH_RXD_IRQ | GRETH_RXD_WRAP;
  } else {
      rxch->desc_array[head].ctrl = GRETH_RXD_ENABLE | GRETH_RXD_IRQ;
  }

  rxch->head = (head + 1) % nf_state->num_rx_bd;

  greth_debug_printf("[RX-REFILL] BD[%u] -> pbuf=%p payload=%p len=%u \
        ctrl=0x%X\n", head, p, p->payload, p->len, rxch->desc_array[head].ctrl);
}

/***********************Transmission related Functions************************/

static int inside_tx_irq=0;
static int inside_tx_gbit_irq=0;
static int inside_greth_send=0;
static int inside_greth_send_gbit=0;

/**
 * @brief Handles TX interrupt for gigabit communication.
 *
 * This function processes a TX interrupt for the gigabit speed.
 * It checks if the current TX buffer descriptor has completed transmission,
 * frees the associated pbuf chain, updates the TX head pointer, and ensures
 * proper interrupt re-enabling. Re-entrancy is detected to prevent nested
 * handling of the same TX interrupt.
 *
 * @param arg Pointer to the lwIP network interface structure (`struct netif *`)
 *
 * @return ERR_OK if the interrupt was handled successfully,
 *         ERR_IF if a re-entrant interrupt is detected or the descriptor is 
 *                still in use.
 */
static err_t greth_process_irq_tx_gbit(void *arg){

  struct netif *network = (struct netif *)arg;
  struct greth_netif_state *nf_state = 
                                    (struct greth_netif_state *)network->state;
  nf_state = (struct greth_netif_state *)network->state;
  int count = 0; 
  struct tx_comm_chn *txch = &nf_state->tx_channel;
  rtems_interrupt_lock_context level;

  if(inside_tx_gbit_irq){
    iprintf("[ERR] Re-entered TX GBIT IRQ!\n");
    return ERR_IF;
  }

  inside_tx_gbit_irq = 1;

  greth_debug_printf("[TX-GBIT-IRQ] Entered TX GBIT IRQ...\n"); 
    
  if(!(GRETH_MEM_LOAD(&txch->desc_array[txch->head].ctrl) & GRETH_TXD_ENABLE)){
    greth_debug_printf("[TX-GBIT-IRQ] Entered while loop\n");
    int pbuf_chain_len = 0;
    int head_now = txch->head;
    while(((struct pbuf*)(txch->tx_pbuf_ref[head_now]))->tot_len != 
                      ((struct pbuf*)(txch->desc_array[txch->head].addr))->len){
      greth_debug_printf("[TX-GBIT-IRQ] Entered inner while loop\n");
      --nf_state->curr_bd_in_use;
      txch->head = (txch->head + 1) % nf_state->num_tx_bd;
      ++count;
      ++pbuf_chain_len;
    }
    pbuf_free((struct pbuf*)txch->tx_pbuf_ref[txch->head - pbuf_chain_len]);
    greth_debug_printf("[DBG] Freed previous packet's pbuf chain!\n");
  }
  else{
    iprintf("[ERR] greth_process_irq_tx_gbit: \
                            GRETH is still using current buffer descriptor!\n");
    return ERR_IF;
  }    

  rtems_interrupt_lock_acquire(&nf_state->isr_lock, &level);
  nf_state->regs->ctrl |= GRETH_CTRL_TXIRQ;
  rtems_interrupt_lock_release(&nf_state->isr_lock, &level);

  greth_debug_printf("[TX-GB-IRQ] Exited TX GBIT IRQ\n");

  inside_tx_gbit_irq = 0;
    
  return ERR_OK;
}

/**
 * @brief Sends a packet over the gigabit.
 *
 * This function transmits a packet represented as a pbuf chain over a GRETH
 * gigabit MAC interface. It maps each pbuf fragment to a transmit descriptor,
 * handles padding for short packets, updates control flags, and enables 
 * transmission.
 *
 * Steps performed:
 * - Checks for re-entrancy using `inside_greth_send_gbit`.
 * - Validates that the current buffer descriptor is free.
 * - Counts the number of fragments in the pbuf chain.
 * - Ensures sufficient TX descriptors are available for the packet.
 * - Assigns each pbuf to a TX descriptor, sets control flags (`GRETH_TXD_MORE`, 
 * `GRETH_TXD_IRQ`, `GRETH_TXD_WRAP`), and updates `curr_bd_in_use`.
 * - Adds padding if the packet length is below `MIN_PKT_LEN`.
 * - Enables transmission by setting `GRETH_CTRL_TXEN` in the control register.
 *
 * @param netif Pointer to the lwIP network interface structure.
 * @param p_start Pointer to the first pbuf of the packet to send.
 *
 * @return ERR_OK if the packet was successfully queued for transmission,
 *         ERR_IF if re-entrancy is detected or the current buffer descriptor is 
 *                still in use,
 *         ERR_MEM if the pbuf chain cannot be sent due to insufficient 
 *                 descriptors.
 */
static err_t greth_send_gbit(struct netif *netif, struct pbuf *p_start){
  unsigned int ctrl, pktlen, padlen=0;
  struct pbuf *p_temp, *p;
  struct netif *network = netif;
  struct greth_netif_state *nf_state = 
                                    (struct greth_netif_state *)network->state;
  struct tx_comm_chn *txch = &nf_state->tx_channel;
  rtems_interrupt_lock_context level;
  int frags=1, len=0, diff, int_en, used_pbufs=0, tx_start_idx=txch->head, j;
  volatile struct emac_bd* curr_bd;

  iprintf("[DBG] greth_send_gbit: greth_send_gbit entered!!!\n");
  if (inside_greth_send_gbit) {
    iprintf("[ERR] greth_send_gbit: greth_send_gbit re-entered!\n");
    return ERR_IF;
  }
  inside_greth_send_gbit = 1;

  if (GRETH_MEM_LOAD(&txch->desc_array[txch->head].ctrl) & GRETH_TXD_ENABLE){ 
    inside_greth_send = 0;
    iprintf("[ERR] greth_send_gbit: GRETH is still using current Buffer \
                                                                Descriptor.\n");
    return ERR_IF;
  }

  p_temp = p_start;
  while(p_temp->next){
    ++frags;
    p_temp = p_temp->next;
  }
  greth_debug_printf("[DBG] greth_send_gbit: Total number of fragments \
                                                              => %d\n", frags);


  if(frags > nf_state->max_fragsize_ever){
    nf_state->max_fragsize_ever = frags;
  }

  if (frags > nf_state->num_tx_bd){
    inside_greth_send_gbit = 0;
    iprintf("[ERR] greth_send_gbit: PBUF-chain cannot be sent. Increase \
                                                          descriptor count.\n");
    return ERR_MEM;
  }
  
  if (frags > (diff = (nf_state->num_tx_bd - nf_state->curr_bd_in_use))){
    inside_greth_send_gbit = 0;
    greth_debug_printf("greth_send_gbit (ERR): Required number of Buffer \
                                            Descriptors (BDs) Not Available\n");
    greth_debug_printf("greth_send_gbit: Total BDs required : %d; More BDs \
                                                required : %d\n", frags, diff);
    return frags;
  }

  nf_state->tx_int_gen_cur -= frags;
  if ( nf_state->tx_int_gen_cur <= 0 ){
    nf_state->tx_int_gen_cur = nf_state->tx_int_gen;
    int_en = GRETH_TXD_IRQ;
  }else{
    int_en = GRETH_TXD_IRQ;
  }

  p = p_start;

  pktlen = p_start->tot_len;
  if (pktlen < MIN_PKT_LEN) {
    padlen = MIN_PKT_LEN - pktlen;
    pktlen = MIN_PKT_LEN;
    ++frags;
    greth_debug_printf("[MSG] greth_send_gbit: padding required and used\n");
  }

  txch->tx_pbuf_ref[txch->head] = p_start;

  while(p){ 
    greth_debug_printf("[MSG] greth_send_gbit: inside while loop\n");
    len += p->len;
    greth_debug_printf("[DBG] len=>%d\n", len);
    txch->desc_array[txch->head].addr = (uintptr_t)((uint16_t *)p->payload);
    greth_debug_printf("[DBG] txch->desc_array[txch->head].addr =>%x\n", 
                                            txch->desc_array[txch->head].addr);
    ++used_pbufs;

    if (txch->head < nf_state->num_tx_bd-1) {
      ctrl = 0;
    }else{
      ctrl = GRETH_TXD_WRAP;
    }

    if (p->len == p_start->tot_len) {
      txch->desc_array[txch->head].ctrl = ctrl | GRETH_TXD_IRQ | (p->len);
      break;
    }else{
      txch->desc_array[txch->head].ctrl = GRETH_TXD_MORE | ctrl | GRETH_TXD_IRQ 
                                                                    | (p->len);
    }

    ++nf_state->curr_bd_in_use;
    txch->head = ( txch->head + 1) % nf_state->num_tx_bd;

    p = p->next;
  }
  greth_debug_printf("[MSG] greth_send_gbit: exited while loop\n");

  if (padlen){
    curr_bd = &txch->desc_array[txch->head];
    curr_bd->addr = (uintptr_t)((uint16_t*)p_start->payload);
    curr_bd->ctrl = ctrl | int_en | (padlen & 0x7FF);
    txch->desc_array[txch->head - 1].ctrl |= GRETH_TXD_MORE;
    txch->head = ( txch->head + 1) % nf_state->num_tx_bd;
    ++nf_state->curr_bd_in_use;
    ++used_pbufs;
  }

  if(used_pbufs == frags){
    greth_debug_printf("[MSG] greth_send_gbit: used_pbufs = frags\n");
    for(j=0; j<(j-1-tx_start_idx);j++){
      txch->desc_array[j].ctrl |= GRETH_TXD_ENABLE;
      j = ((j-1+nf_state->num_tx_bd))%nf_state->num_tx_bd;

    }
    txch->desc_array[j].ctrl |= GRETH_TXD_ENABLE;
  }

  rtems_interrupt_lock_acquire(&nf_state->isr_lock, &level);
  nf_state->regs->ctrl |= GRETH_CTRL_TXEN;
  rtems_interrupt_lock_release(&nf_state->isr_lock, &level);

  inside_greth_send_gbit = 0;
  
  greth_debug_printf("[MSG] greth_send_gbit: exiting greth_send_gbit!!!\n\n");
  return ERR_OK;
}

/**
 * @brief Handles TX interrupt for non-gigabit speed.
 *
 * This function processes a transmit (TX) interrupt for a standard 
 * (non-gigabit) communication. It checks whether the current TX descriptor has 
 * completed transmission, if yes, frees the associated pbuf, updates the TX 
 * head pointer, and re-enables the TX interrupt. Re-entrancy is detected to 
 * prevent nested handling of the same TX interrupt.
 *
 * @param arg Pointer to the LWIP network interface structure (`struct netif *`).
 *
 * @return 0 on successful handling of the TX interrupt,
 *         ERR_IF if a re-entrant interrupt is detected or the descriptor is 
 * still in use.
 */
static err_t greth_process_irq_tx(void *arg){

  struct netif *network = (struct netif *)arg;
  struct greth_netif_state *nf_state = 
                                    (struct greth_netif_state *)network->state;
  struct tx_comm_chn* txch = &nf_state->tx_channel;
  rtems_interrupt_lock_context level;
  
  if(inside_tx_irq){
    iprintf("[ERR] Re-entered TX IRQ!\n");
    return ERR_IF;
  }

  inside_tx_irq = 1;

  iprintf("[TX-IRQ] Entered TX IRQ\n");
  if(!(GRETH_MEM_LOAD(&(txch->desc_array[txch->head].ctrl)) 
                                                          & GRETH_TXD_ENABLE)){
    pbuf_free(txch->tx_pbuf_ref[txch->head]);
    greth_debug_printf("[DBG] Freed previous descriptor's pbuf\n");
    --nf_state->curr_bd_in_use;
    txch->head = (txch->head + 1) % nf_state->num_tx_bd;
  }
  else{
    iprintf("[ERR] greth_process_irq_tx: GRETH is still using current \
                                                                descriptor!\n");
    return ERR_IF;
  }

  rtems_interrupt_lock_acquire(&nf_state->isr_lock, &level);
  nf_state->regs->ctrl |= GRETH_CTRL_TXIRQ;
  rtems_interrupt_lock_release(&nf_state->isr_lock, &level);
  iprintf("[TX-IRQ] Exited TX IRQ\n");
    
  return ERR_OK;
}

/**
 * @brief Sends a packet over the non-gigabit.
 *
 * This function transmits a packet represented as a single pbuf over a standard
 * GRETH MAC interface. It copies the packet payload into the current transmit
 * buffer descriptor, handles padding for packets smaller than the Ethernet
 * minimum frame size, updates control flags, and enables transmission.
 *
 * Steps performed:
 * - Checks for re-entrancy using `inside_greth_send`.
 * - Validates that the current TX buffer descriptor is free.
 * - Copies the payload of the pbuf chain into the descriptor buffer.
 * - Adds padding if the packet length is below `MIN_PKT_LEN`.
 * - Frees the pbuf chain after copying the data.
 * - Updates descriptor control flags (`GRETH_TXD_ENABLE`, `GRETH_TXD_WRAP`, 
 * `GRETH_TXD_IRQ`) and increments `curr_bd_in_use`.
 * - Enables transmission by setting `GRETH_CTRL_TXEN` in the control register.
 * - Debug prints provide detailed logging of descriptor addresses, lengths, and 
 * control flags.
 *
 * @param netif Pointer to the LWIP network interface structure.
 * @param p_start Pointer to the first pbuf of the packet to send.
 *
 * @return ERR_OK if the packet was successfully queued for transmission,
 *         ERR_IF if re-entrancy is detected or the current buffer descriptor
 *                is still in use.
 */
static err_t greth_send(struct netif *netif, struct pbuf *p_start){
  int len=0;  
  unsigned int pktlen, padlen=0;
  struct pbuf *p;
  struct netif *network = netif;
  struct greth_netif_state *nf_state = 
                                    (struct greth_netif_state *)network->state;
  volatile struct emac_bd *curr_bd;
  struct tx_comm_chn *txch = &nf_state->tx_channel;
  unsigned char *temp;
  rtems_interrupt_lock_context level;

  curr_bd = &txch->desc_array[txch->head];
  greth_debug_printf("[DBG] greth_send: curr_bd => %p\n", curr_bd);

  iprintf("[DBG] greth_send: Entered...\n");
  if (inside_greth_send) {
    greth_debug_printf ("[ERR] greth_send_gbit : greth_send re-entered!\n");
  }
  inside_greth_send = 1;

  iprintf("[DBG] greth_send: Current tx_idx_in_chain = %u\n", txch->head);

  if (GRETH_MEM_LOAD(&curr_bd->ctrl) & GRETH_TXD_ENABLE){ 
    inside_greth_send = 0;
    iprintf("[ERR] greth_send: GRETH is still using current Buffer \
                                                                Descriptor.\n");
    return ERR_IF;
  }

  p = p_start;
  txch->tx_pbuf_ref[txch->head] = p_start;

  pktlen = p_start->tot_len;
  if (pktlen < MIN_PKT_LEN) {
    padlen = MIN_PKT_LEN - pktlen;
    pktlen = MIN_PKT_LEN;
  }

  temp = (unsigned char *) GRETH_MEM_LOAD(&curr_bd->addr); 
  while(p){
    len += p->len;
    if (len <= RBUF_SIZE){
      memcpy ((void *) temp, (char *) p->payload, p->len);
    }
    temp += p->len;
    if ((p = p->next) == NULL){
      break;
    }
  }

  if (padlen){
    memset ((void *) temp, 0, padlen);
    temp += padlen;
  }

  pbuf_free(p_start);

  if (len <= GRETH_MAXBUF_LEN) {
    if ((txch->head + 1) == nf_state->num_tx_bd) { 
      curr_bd->ctrl = GRETH_TXD_ENABLE | GRETH_TXD_WRAP | len | GRETH_TXD_IRQ;
    } else {
      curr_bd->ctrl = GRETH_TXD_ENABLE | len | GRETH_TXD_IRQ;
    }
    ++nf_state->curr_bd_in_use;
    txch->head = (txch->head + 1) % nf_state->num_tx_bd;  
  }
  else{
   iprintf("[ERR] greth_send: Too long packet\n");
  }
  greth_debug_printf("[DBG] greth_send: Packet construction complete. \
                                                  Sending using greth_send\n");
  rtems_interrupt_lock_acquire(&nf_state->isr_lock, &level);
  nf_state->regs->ctrl |= GRETH_CTRL_TXEN;
  greth_debug_printf("[DBG] greth_send: Control Register : %u\n", \
                                                          nf_state->regs->ctrl);
  rtems_interrupt_lock_release(&nf_state->isr_lock, &level);

  inside_greth_send = 0;

  return ERR_OK;
}


/************************Reception related Functions***************************/

/**
 * @brief Handles GRETH receive (RX) interrupts.
 *
 * This function is called when an RX interrupt occurs. It processes received
 * Ethernet frames by examining the RX buffer descriptors, checking for errors,
 * updating statistics, passing valid packets to the lwIP stack, and refilling
 * the RX descriptors with new pbufs.
 *
 * Steps performed:
 * 1. Continuously checks the current RX descriptor whether not enabled.
 * 2. Reads the length and status from the descriptor control field.
 * 3. Checks for errors such as:
 *    - GRETH_RXD_TOOLONG
 *    - GRETH_RXD_DRIBBLE
 *    - GRETH_RXD_CRCERR
 *    - GRETH_RXD_OVERRUN
 *    - GRETH_RXD_LENERR
 * 4. If no errors:
 *    - Updates the packet length in the pbuf.
 *    - Passes the pbuf to the network stack via `ntf->input()`.
 *    - Updates lwIP link statistics.
 * 5. If errors:
 *    - Frees the current pbuf.
 * 6. Refills RX descriptor with a new pbuf using @ref greth_rx_pbuf_refill()
 * 7. Updates the RX descriptor control flags (`GRETH_RXD_ENABLE`, 
 * `GRETH_RXD_IRQ`, `GRETH_RXD_WRAP`).
 * 8. Ensures RX is enabled in the control register 
 *
 * @param ntf Pointer to the LWIP network interface structure.
 */
static void greth_process_irq_rx(struct netif* ntf)
{
  struct greth_netif_state* nf_state = (struct greth_netif_state*) ntf->state;
  struct rx_comm_chn *rxch = &nf_state->rx_channel;
  unsigned int len_status, pkt_len, bad=0;
  struct pbuf *p;
  rtems_interrupt_lock_context level;

  iprintf("[RX-IRQ] Entered RX IRQ\n");

  while (!((len_status = GRETH_MEM_LOAD(&rxch->desc_array[rxch->head].ctrl)) 
                                                          & GRETH_RXD_ENABLE)) {
    if (len_status & GRETH_RXD_TOOLONG) { ++nf_state->rxLengthError; bad=1; }
    if (len_status & GRETH_RXD_DRIBBLE) { ++nf_state->rxNonOctet;    bad=1; }
    if (len_status & GRETH_RXD_CRCERR)  { ++nf_state->rxBadCRC;      bad=1; }
    if (len_status & GRETH_RXD_OVERRUN) { ++nf_state->rxOverrun;     bad=1; }
    if (len_status & GRETH_RXD_LENERR)  { ++nf_state->rxLengthError; bad=1; }

    if (!bad) {
      greth_debug_printf("[DBG] rxch->head => %d\n", rxch->head);
      pkt_len = len_status & 0x7FF;
      p = rxch->rx_pbuf_ref[rxch->head];

      LINK_STATS_INC(link.recv);          

      p->len = p->tot_len = pkt_len;
      if (ntf->input(p, ntf) != ERR_OK) {
          LINK_STATS_INC(link.memerr);
          LINK_STATS_INC(link.drop);
          pbuf_free(p);
      }
      greth_rx_pbuf_refill(nf_state);
    } 
    else {
      pbuf_free(rxch->rx_pbuf_ref[rxch->head]);
      greth_rx_pbuf_refill(nf_state);
    }

    if (rxch->head == nf_state->num_rx_bd - 1)
        rxch->desc_array[rxch->head].ctrl = GRETH_RXD_ENABLE | GRETH_RXD_IRQ 
                                                              | GRETH_RXD_WRAP;
    else
        rxch->desc_array[rxch->head].ctrl = GRETH_RXD_ENABLE | GRETH_RXD_IRQ;

    greth_debug_printf("rxch incremented head => %d\n", rxch->head);
    greth_debug_printf("rxch new pbuf => %x\n", \
                                            rxch->desc_array[rxch->head].addr);

    rtems_interrupt_lock_acquire(&nf_state->isr_lock, &level);
    nf_state->regs->ctrl |= GRETH_CTRL_RXEN;
    rtems_interrupt_lock_release(&nf_state->isr_lock, &level);
  }

  iprintf("[RX-IRQ] Exited RX IRQ\n");
}