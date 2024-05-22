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
    eCLI_OSCI_STATE_DONE,           /**<Sampling done */
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
static void cli_osci_start      (const uint8_t * p_attr);
static void cli_osci_stop       (const uint8_t * p_attr);
static void cli_osci_data       (const uint8_t * p_attr);
static void cli_osci_channel    (const uint8_t * p_attr);
static void cli_osci_trigger    (const uint8_t * p_attr);
static void cli_osci_downsample (const uint8_t * p_attr);
static void cli_osci_info       (const uint8_t * p_attr);



////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 *  Oscilloscope control block
 */
static volatile cli_osci_t __attribute__ (( section( CLI_CFG_PAR_OSCI_SECTION ))) g_cli_osci = {0};


/**
 *      Oscilloscope CLI commands
 */
static const cli_cmd_table_t g_cli_osci_table =
{
    // List of commands
    .cmd =
    {
        // ----------------------------------------------------------------------------------------------------------------------
        //  name                    function                    help string
        // ----------------------------------------------------------------------------------------------------------------------

        {   "osci_start",           cli_osci_start,         	"Start (trigger) oscilloscope"                                  },
        {   "osci_stop",            cli_osci_stop,          	"Stop or cancel ongoing sampling"                               },
        {   "osci_data",            cli_osci_data,          	"Get oscilloscope sampled data"                                 },
        {   "osci_channel",         cli_osci_channel,       	"Set oscilloscope channels [par1,par2,...,parN]"                },
        {   "osci_trigger",         cli_osci_trigger,       	"Set oscilloscope trigger [type,par,threshold,pre-trigger]"     },
        {   "osci_downsample",      cli_osci_downsample,    	"Set oscilloscope downsample factor [downsample]"               },
        {   "osci_info",            cli_osci_info,              "Get information of oscilloscope configuration"                 },

    },

    // Total number of listed commands
    .num_of = 7
};


////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

// Internal functions....
cli_status_t cli_osci_reset         (void);
cli_status_t cli_osci_configure     (const cli_osci_mode_t mode, const cli_osci_trig_t, const float32_t threshold);


////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Start oscilloscope
*
*       If oscilloscope is in "eCLI_OSCI_STATE_IDLE" state, then it will enter
*       waiting for trigger state. In case of no trigger is set, sampling will
*       be started right away.
*
*
* @note In case oscilloscope is started before reading out sampled data, all
*       data will be overwritten by the new sampling session!
*
*       Make sure to call "osci_data" command first, to retrieve sampled data, before
*       starting oscilloscope again!
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_osci_start(const uint8_t * p_attr)
{
    if ( NULL == p_attr )
    {
        // Osci idle
        if  (   ( eCLI_OSCI_STATE_IDLE  == g_cli_osci.state )
            ||  ( eCLI_OSCI_STATE_DONE  == g_cli_osci.state ))
        {
            // Enter waiting state
            g_cli_osci.state = eCLI_OSCI_STATE_WAITING;
        }
        else
        {
            cli_printf( "WAR, Oscilloscope is already running..." );
        }
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Stop or cancel ongoing sampling
**
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_osci_stop(const uint8_t * p_attr)
{
    if ( NULL == p_attr )
    {
        // Enter idle state
        g_cli_osci.state = eCLI_OSCI_STATE_IDLE;
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Get oscilloscope sampled data
*
* @note     Available only after successful sampling session!
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_osci_data(const uint8_t * p_attr)
{
    if ( NULL == p_attr )
    {
        // Sampling finished
        if ( eCLI_OSCI_STATE_DONE  == g_cli_osci.state )
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
        }
        else
        {
            cli_printf( "WAR, Sampled data not available at the moment..." );
        }
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Set oscilloscope channels
*
*           Command: >>>osci_channels [par1,par2,...parN]
*
*           where parX is Device Parameter ID number
*
*
* @note     Shall only be called when oscilloscope is not in running mode.
*
*           Oscilloscope shall be stopped before configuring it!
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_osci_channel(const uint8_t * p_attr)
{
    if ( NULL != p_attr )
    {
        // Osci idle
        if  (   ( eCLI_OSCI_STATE_IDLE  == g_cli_osci.state )
            ||  ( eCLI_OSCI_STATE_DONE  == g_cli_osci.state ))
        {
            // TODO: ...
        }
        else
        {
            cli_printf( "WAR, Oscilloscope cfg cannot be changed during sampling!" );
        }
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Set oscilloscope trigger
*
*
*           Command: >>>osci_trigger [type,par,threshold,pre-trigger]
*
*               -type:          Trigger type (none, rising edge, falling edge, both edges)
*               -par:           Parameter ID used for triggering
*               -threshold:     Trigger threshold value
*               -pre-trigger:   Pre-trigger value from [0,100] %
*                               (0% - No Pre-Triggering, 50% - Half of the sample buffer used for pre-trigger)
*
*
* @note     Shall only be called when oscilloscope is not in running mode.
*
*           Oscilloscope shall be stopped before configuring it!
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_osci_trigger(const uint8_t * p_attr)
{
    if ( NULL != p_attr )
    {
        // Osci idle
        if  (   ( eCLI_OSCI_STATE_IDLE  == g_cli_osci.state )
            ||  ( eCLI_OSCI_STATE_DONE  == g_cli_osci.state ))
        {
            // TODO: ...
        }
        else
        {
            cli_printf( "WAR, Oscilloscope cfg cannot be changed during sampling!" );
        }
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Get oscilloscope sampled data
*
*           Command: >>>osci_downsample [factor]
*
*               -factor:    Downsample factor
*                           (0 - No downsampling, 2 - TBD:...)
*
*
* @note     Shall only be called when oscilloscope is not in running mode.
*
*           Oscilloscope shall be stopped before configuring it!
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_osci_downsample(const uint8_t * p_attr)
{
    if ( NULL != p_attr )
    {
        // Osci idle
        if  (   ( eCLI_OSCI_STATE_IDLE  == g_cli_osci.state )
            ||  ( eCLI_OSCI_STATE_DONE  == g_cli_osci.state ))
        {
            // TODO: ...
        }
        else
        {
            cli_printf( "WAR, Oscilloscope cfg cannot be changed during sampling!" );
        }
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}


static void cli_osci_info(const uint8_t * p_attr)
{
    if ( NULL == p_attr )
    {
        // TODO: ...
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


    //=====================================================================================================
    // PRE-TRIGGERING PART

    if ( eCLI_OSCI_STATE_WAITING == g_cli_osci.state )
    {
        // No trigger
        if ( eCLI_OSCI_TRIG_NONE == g_cli_osci.trigger.type )
        {
            g_cli_osci.state = eCLI_OSCI_STATE_SAMPLING;
        }
    }

    //=====================================================================================================




    //=====================================================================================================
    // SAMPLING PART

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

        // Sample buffer not full
        if ( g_cli_osci.samp.idx < ( CLI_CFG_PAR_OSCI_SAMP_BUF_SIZE - g_cli_osci.par.num_of ))
        {
            g_cli_osci.samp.idx += g_cli_osci.par.num_of;
        }

        // Sample buffer full
        else
        {
            g_cli_osci.state = eCLI_OSCI_STATE_DONE;
            g_cli_osci.samp.idx = 0;
        }
    }

    //=====================================================================================================

}

#endif // ( 1 == CLI_CFG_PAR_USE_EN )

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
