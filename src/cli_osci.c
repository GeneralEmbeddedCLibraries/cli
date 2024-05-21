// Copyright (c) 2024 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      cli_osci.c
*@brief     Command Line Interface Osciloscope
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      21.05.2024
*@version   V1.4.0
*/
////////////////////////////////////////////////////////////////////////////////
/*!
* @addtogroup CLI_OSCI
* @{ <!-- BEGIN GROUP -->
*
* @brief	This module contains software osciloscope functionalities coupled
*           into CLI module.
*
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "cli_util.h"
#include "cli_osci.h"
#include "cli_par.h"

#if ( 1 == CLI_CFG_PAR_OSCI_EN )



////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *  Oscilloscope mode
 */
typedef enum
{
    eCLI_OSCI_MODE_OFF = 0,     /**<Oscilloscope turned OFF */
    eCLI_OSCI_MODE_AUTO,        /**<Automatic mode */
    eCLI_OSCI_MODE_NORMAL,      /**<Normal mode */
    eCLI_OSCI_MODE_SINGLE,      /**<Single mode */

    eCLI_OSCI_MODE_NUM_OF,
} cli_osci_mode_t;

/**
 *  Oscilloscope triggers
 */
typedef enum
{
    eCLI_OSCI_TRIG_NONE = 0,        /**<No trigger */
    eCLI_OSCI_TRIG_EDGE_RISING,     /**<Trigger on rising edge */
    eCLI_OSCI_TRIG_EDGE_FALLING,    /**<Trigger on falling edge */
    eCLI_OSCI_TRIG_EDGE_BOTH,       /**<Trigger on both (rising or falling) edge */
    eCLI_OSCI_TRIG_VAL_LOWER,       /**<Trigger on equal value as trigger threshold */
    eCLI_OSCI_TRIG_VAL_HIGHER,      /**<Trigger on value higher than trigger threshold */
    eCLI_OSCI_TRIG_VAL_EQUAL,       /**<Trigger on value lower than trigger threshold */

    eCLI_OSCI_TRIG_NUM_OF,
} cli_osci_trig_t;

/**
 *  Oscilloscope state
 */
typedef enum
{
    eCLI_OSCI_STATE_IDLE = 0,       /**<No operation ongoing */
    eCLI_OSCI_STATE_WAITING,        /**<Waiting for trigger */
    eCLI_OSCI_STATE_SAMPLING,       /**<Sampling phase */
    eCLI_OSCI_STATE_FINISH,         /**<Sampling done */
} cli_osci_state_t;

/**
 *  Oscilloscope control block
 */
typedef struct
{
    /**<Trigger */
    struct
    {
        par_type_t      th;     /**<Trigger threshold */
        par_num_t       par;    /**<Device parameter used for triggering */
        cli_osci_trig_t type;   /**<Trigger type*/
    } trigger;

    /**<Parameters in osci */
    struct
    {
        par_num_t   list[CLI_CFG_PAR_MAX_IN_OSCI];
        uint32_t    num_of;
    } par;

    /**<Sample buffer */
    struct
    {
        float32_t   buf[CLI_CFG_PAR_OSCI_SAMP_BUF_SIZE];    /**<Sample buffer data */
        uint32_t    idx;                                    /**<Sample buffer index */
    } samp;

    cli_osci_mode_t     mode;       /**<Oscilloscope mode */
    cli_osci_state_t    state;      /**<Oscilloscope state */
} cli_osci_t;

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static void cli_osci_start(const uint8_t * p_attr);



////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 *  Oscilloscope control block
 */
static volatile cli_osci_t g_cli_osci = {0};


/**
 *      Oscilloscope CLI commands
 */
static const cli_cmd_table_t g_cli_osci_table =
{
    // List of commands
    .cmd =
    {
        // ------------------------------------------------------------------------------------------------------
        //  name                    function                help string
        // ------------------------------------------------------------------------------------------------------

        {   "osci_start",           cli_osci_start,         "Start oscilloscope"                                },

    },

    // Total number of listed commands
    .num_of = 1
};


////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

// Internal functions....
cli_status_t cli_osci_reset         (void);
cli_status_t cli_osci_configure     (const cli_osci_mode_t mode, const cli_osci_trig_t, const float32_t threshold);



static void cli_osci_start(const uint8_t * p_attr)
{
    if ( NULL == p_attr )
    {

        // TODO: only for testing...
        g_cli_osci.state = eCLI_OSCI_STATE_SAMPLING;
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}



////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup CLI_OSCI_API
* @{ <!-- BEGIN GROUP -->
*
* 	Following function are part of CLI Osciloscope API.
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Initialize CLI Oscilloscope
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_osci_init(void)
{
    cli_status_t status = eCLI_OK;

    // TODO: Load saved osci settings from NVM...



    // TODO: Only for testing...
    g_cli_osci.par.list[0] = ePAR_SPIRAL_1_CUR;
    g_cli_osci.par.list[1] = ePAR_SPIRAL_1_CUR_FILT;
    g_cli_osci.par.num_of = 2;

    g_cli_osci.mode = eCLI_OSCI_MODE_AUTO;



    // Register Device Parameters CLI table
    cli_register_cmd_table((const cli_cmd_table_t*) &g_cli_osci_table );

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Handle CLI Oscilloscope
*
* @note     Shall not be called from ISR!
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_osci_hndl(void)
{
    cli_status_t status = eCLI_OK;

    if ( eCLI_OSCI_MODE_AUTO == g_cli_osci.mode )
    {
        // Sampling finished
        if ( eCLI_OSCI_STATE_FINISH == g_cli_osci.state )
        {
            // Get pointer to Tx buffer
            uint8_t * p_tx_buf = cli_util_get_tx_buf();

            // Loop thru sample buffer
            for ( uint32_t samp_it = 0U; samp_it < CLI_CFG_PAR_OSCI_SAMP_BUF_SIZE; samp_it += g_cli_osci.par.num_of )
            {
                // Loop thru parameter list
                for ( uint8_t par_it = 0; par_it < g_cli_osci.par.num_of; par_it++ )
                {
                    // Get value from sample buffer
                    const float32_t samp_val = g_cli_osci.samp.buf[ ( samp_it + par_it )];

                    // Convert to string
                    sprintf((char*) p_tx_buf, "%g", samp_val );

                    // Send
                    cli_send_str( p_tx_buf );

                    // If not last -> send delimiter
                    if ( par_it < ( g_cli_osci.par.num_of - 1 ))
                    {
                        cli_send_str((const uint8_t*) "," );
                    }
                }

                // Terminate line
                cli_printf("");
            }


            // Osci idle
            g_cli_osci.state = eCLI_OSCI_STATE_IDLE;
        }
    }



    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        CLI Oscilloscope Sampling handler
*
* @note     This function shall be called in time equidistant period!
*
*           Can be called from ISR!
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
void cli_osci_samp_hndl(void)
{

    // TODO: Handle trigger here...


    // Sample data
    if ( eCLI_OSCI_STATE_SAMPLING == g_cli_osci.state )
    {
        // Take sample of each parameter in osci list
        for ( uint32_t par_it = 0U; par_it < g_cli_osci.par.num_of; par_it++ )
        {
            // Get parameter value
            const float32_t par_val = com_util_par_val_to_float( g_cli_osci.par.list[par_it] );

            // Set value to sample buffer
            g_cli_osci.samp.buf[ ( g_cli_osci.samp.idx + par_it )] = par_val;
        }

        if ( g_cli_osci.samp.idx < ( CLI_CFG_PAR_OSCI_SAMP_BUF_SIZE - g_cli_osci.par.num_of ))
        {
            g_cli_osci.samp.idx += g_cli_osci.par.num_of;
        }
        else
        {
            g_cli_osci.state = eCLI_OSCI_STATE_FINISH;
            g_cli_osci.samp.idx = 0;

        }
    }
}

#endif // ( 1 == CLI_CFG_PAR_USE_EN )

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
