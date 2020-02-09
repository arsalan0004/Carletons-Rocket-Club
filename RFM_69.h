/**
 * @file RFM69.h
 * @desc Driver for RFM69HCW radio module 
 * @author Arsalan Syed
 * @date 
 * Last Author: 
 * Last Edited On:
 */

#ifndef RFM69_h
#define RFM69_h

#include "global.h"

#include "RFM69registers.h"

#include "sercom-spi.h"

/*maximum number of interpts which may be enabled */
#define RFM69_MAX_NUM_INTERRUPTS  4 

#define RFM69_BAUD_RATE      8000000UL


struct RFM69_desc_t {
    /** Period with which the input registers should be polled automatically */
    uint32_t poll_period;
    /** Stores the last time at which the GPIO registers where polled */
    uint32_t last_polled;
    /** Callback function for interrupts */
    mcp23s17_int_callback interrupt_callback;
    /** SPI instance used to communicate with device */
    struct sercom_spi_desc_t *spi_inst;
    /** Mask for devices chip select pin */
    uint32_t cs_pin_mask;
    /** Group in which devices chip select pin is located */
    uint8_t cs_pin_group;
    
    /** Opcode, located here to provide easy writing of the entire register
        cache to the IO expander */
    uint8_t opcode;
    /** Register address, located here to provide easy writing of the entire
        register cache to the IO expander */
    uint8_t reg_addr;
    /** Cache of device register values */
    struct mcp23s17_register_map registers;
    
    /** Buffer used in SPI transaction to write only the OLAT register */
    uint8_t spi_out_buffer[4];
    /** ID to keep track of SPI transactions */
    uint8_t spi_transaction_id;