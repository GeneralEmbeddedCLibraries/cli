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

#include "middleware/ring_buffer/src/ring_buffer.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 *  Oscilloscope triggers
 */
typedef enum
{
    eCLI_OSCI_TRIG_NONE = 0,        /**<No trigger */
    eCLI_OSCI_TRIG_EDGE_RISING,     /**<Trigger on rising edge */
    eCLI_OSCI_TRIG_EDGE_FALLING,    /**<Trigger on falling edge */
    eCLI_OSCI_TRIG_EDGE_BOTH,       /**<Trigger on both (rising or falling) edge */
    eCLI_OSCI_TRIG_EQUAL,           /**<Value equal to threshold value */
    eCLI_OSCI_TRIG_ABOVE,           /**<Value above threshold value */
    eCLI_OSCI_TRIG_BELOW,           /**<Value below threshold value */

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

    eCLI_OSCI_STATE_NUM_OF,
} cli_osci_state_t;

/**
 *  Oscilloscope control block
 */
typedef struct
{
    /**<Trigger */
    struct
    {
        float32_t       th;         	/**<Trigger threshold */
        par_num_t       par;        	/**<Device parameter used for triggering */
        cli_osci_trig_t type;       	/**<Trigger type*/
        float32_t       pretrigger; 	/**<Pretrigger */
        uint32_t        trig_idx;       /**<Trigger sample buffer index */
    } trigger;

    /**<Channels */
    struct
    {
        par_num_t   list[CLI_CFG_PAR_MAX_IN_OSCI];  /**<List of channels, Device Parameters enumerations */
        uint32_t    num_of;                         /**<Number of channels */
    } channel;

    /**<Sample buffer */
    struct
    {
        p_ring_buffer_t buf;                                        /**<Sample buffer - ring buffer */
        float32_t       data[CLI_CFG_PAR_OSCI_SAMP_BUF_SIZE];       /**<Sample buffer data */
        int32_t         idx;                                        /**<Sample buffer index */
        uint32_t        downsample_factor;                          /**<Downsample factor */
    } samp;

    cli_osci_state_t    state;      /**<Oscilloscope state */
} cli_osci_t;

/**
 *  State handler pointer function
 */
typedef void (*pf_osci_state_hndl_t)(void);

/**
 *  Trigger detection pointer function
 */
typedef bool (*pf_cli_trig_check)(const float32_t sig, const float32_t th);

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static cli_status_t cli_osci_init_buf       	(void);
static void         cli_osci_take_sample    	(void);
static void         osci_state_waiting_hndl 	(void);
static void         osci_state_sapling_hndl 	(void);
static bool         cli_osci_trig_equal     	(const float32_t sig, const float32_t th);
static bool         cli_osci_trig_above     	(const float32_t sig, const float32_t th);
static bool         cli_osci_trig_below         (const float32_t sig, const float32_t th);
static bool         cli_osci_trig_edge_rising   (const float32_t sig, const float32_t th);
static bool         cli_osci_trig_edge_falling  (const float32_t sig, const float32_t th);
static bool         cli_osci_trig_edge_both     (const float32_t sig, const float32_t th);

// Cli functions
static void cli_osci_start      (const uint8_t * p_attr);
static void cli_osci_stop       (const uint8_t * p_attr);
static void cli_osci_data       (const uint8_t * p_attr);
static void cli_osci_channel    (const uint8_t * p_attr);
static void cli_osci_trigger    (const uint8_t * p_attr);
static void cli_osci_downsample (const uint8_t * p_attr);
static void cli_osci_state      (const uint8_t * p_attr);
static void cli_osci_info       (const uint8_t * p_attr);


////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 *  Oscilloscope control block
 */
static volatile cli_osci_t __attribute__ (( section( CLI_CFG_PAR_OSCI_SECTION ))) g_cli_osci = {0};

/**
 *  Osci state handlers
 */
static const pf_osci_state_hndl_t gpf_cli_osci_state_hndl[eCLI_OSCI_STATE_NUM_OF] =
{
    [eCLI_OSCI_STATE_IDLE]      = NULL,
    [eCLI_OSCI_STATE_WAITING]   = osci_state_waiting_hndl,
    [eCLI_OSCI_STATE_SAMPLING]  = osci_state_sapling_hndl,
    [eCLI_OSCI_STATE_DONE]      = NULL,
};

/**
 *  Trigger detection logic
 */
static const pf_cli_trig_check gpf_cli_osci_check_trig[eCLI_OSCI_TRIG_NUM_OF] =
{
    [eCLI_OSCI_TRIG_NONE]           = NULL,
    [eCLI_OSCI_TRIG_EDGE_RISING]    = cli_osci_trig_edge_rising,
    [eCLI_OSCI_TRIG_EDGE_FALLING]   = cli_osci_trig_edge_falling,
    [eCLI_OSCI_TRIG_EDGE_BOTH]      = cli_osci_trig_edge_both,
    [eCLI_OSCI_TRIG_EQUAL]          = cli_osci_trig_equal,
    [eCLI_OSCI_TRIG_ABOVE]          = cli_osci_trig_above,
    [eCLI_OSCI_TRIG_BELOW]          = cli_osci_trig_below,
};

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
        {   "osci_channel",         cli_osci_channel,       	"Set oscilloscope channels [parId1,parId2,...,parIdN]"          },
        {   "osci_trigger",         cli_osci_trigger,       	"Set oscilloscope trigger [type,par,threshold,pre-trigger]"     },
        {   "osci_downsample",      cli_osci_downsample,    	"Set oscilloscope downsample factor [downsample]"               },
        {   "osci_state",           cli_osci_state,             "Get oscilloscope state"                                        },
        {   "osci_info",            cli_osci_info,              "Get information of oscilloscope configuration"                 },
    },

    // Total number of listed commands
    .num_of = 8
};

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Initializing oscilloscope sample buffer
*
* @return       status - Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
static cli_status_t cli_osci_init_buf(void)
{
    cli_status_t status = eCLI_OK;

    // Static allocation of memory space, dump old samples
    const ring_buffer_attr_t buf_attr =
    {
        .name       = "Osci",
        .p_mem      = (void*) &g_cli_osci.samp.data,
        .item_size  = sizeof(float32_t),
        .override   = true,                         // Dump old data
    };

    // Init ring buffer
    if ( eRING_BUFFER_OK != ring_buffer_init((p_ring_buffer_t*) &g_cli_osci.samp.buf, CLI_CFG_PAR_OSCI_SAMP_BUF_SIZE, &buf_attr ))
    {
        status = eCLI_ERROR;
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Take sample and put it into sample buffer
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_osci_take_sample(void)
{
    // Take sample of each parameter in osci list
    for ( uint32_t par_it = 0U; par_it < g_cli_osci.channel.num_of; par_it++ )
    {
        // Get parameter value
        const float32_t par_val = com_util_par_val_to_float( g_cli_osci.channel.list[par_it] );

        // Set value to sample buffer
        (void) ring_buffer_add( g_cli_osci.samp.buf, (float32_t*) &par_val );
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Oscilloscope in WAITING state
*
* @note     This function might be called from ISR!
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void osci_state_waiting_hndl(void)
{
    static uint32_t pretrigger_samp_cnt = 0;

    // No trigger
    if ( eCLI_OSCI_TRIG_NONE == g_cli_osci.trigger.type )
    {
        g_cli_osci.state = eCLI_OSCI_STATE_SAMPLING;

        // We need to sample full buffer
        g_cli_osci.samp.idx = CLI_CFG_PAR_OSCI_SAMP_BUF_SIZE;
    }

    // Trigger selected
    else
    {
        // Take sample
        cli_osci_take_sample();

        // Pretrigger sampling done
        if ( pretrigger_samp_cnt >= g_cli_osci.trigger.trig_idx )
        {
            // Get trigger parameter value
            const float32_t par_val = com_util_par_val_to_float( g_cli_osci.trigger.par );

            if ( NULL != gpf_cli_osci_check_trig[g_cli_osci.trigger.type] )
            {
                // Check for trigger
                if ( true == gpf_cli_osci_check_trig[g_cli_osci.trigger.type]( par_val, g_cli_osci.trigger.th ))
                {
                    // Enter sampling phase on trigger detection
                    g_cli_osci.state = eCLI_OSCI_STATE_SAMPLING;

                    // Reset pretrigger samp
                    pretrigger_samp_cnt = 0U;

                    // We don't need to sample full buffer as we have a pretrigger going on
                    // NOTE: One sample is already taken, therefore -1
                    g_cli_osci.samp.idx = ( CLI_CFG_PAR_OSCI_SAMP_BUF_SIZE - g_cli_osci.trigger.trig_idx - 1U );
                }
            }
        }

        // Waiting for pretrigger
        else
        {
            pretrigger_samp_cnt++;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Oscilloscope in SAMPLING state
*
* @note     This function might be called from ISR!
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void osci_state_sapling_hndl(void)
{
    // Take sample
    cli_osci_take_sample();

    // Decrement number of samples to do
    g_cli_osci.samp.idx--;

    // All requested sampling done
    if ( g_cli_osci.samp.idx <= 0 )
    {
        g_cli_osci.state = eCLI_OSCI_STATE_DONE;
        g_cli_osci.samp.idx = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Oscilloscope trigger on equal threshold value
*
* @param[in]    sig         - Signal to check for trigger
* @param[in]    th          - Trigger threshold
* @return       detected    - True when triggers
*/
////////////////////////////////////////////////////////////////////////////////
static bool cli_osci_trig_equal(const float32_t sig, const float32_t th)
{
    bool detected = false;

    if ( sig == th )
    {
        detected = true;
    }

    return detected;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Oscilloscope trigger above threshold value
*
* @param[in]    sig         - Signal to check for trigger
* @param[in]    th          - Trigger threshold
* @return       detected    - True when triggers
*/
////////////////////////////////////////////////////////////////////////////////
static bool cli_osci_trig_above(const float32_t sig, const float32_t th)
{
    bool detected = false;

    if ( sig > th )
    {
        detected = true;
    }

    return detected;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Oscilloscope trigger below threshold value
*
* @param[in]    sig         - Signal to check for trigger
* @param[in]    th          - Trigger threshold
* @return       detected    - True when triggers
*/
////////////////////////////////////////////////////////////////////////////////
static bool cli_osci_trig_below(const float32_t sig, const float32_t th)
{
    bool detected = false;

    if ( sig < th )
    {
        detected = true;
    }

    return detected;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Oscilloscope trigger on rising edge
*
* @param[in]    sig         - Signal to check for trigger
* @param[in]    th          - Trigger threshold
* @return       detected    - True when triggers
*/
////////////////////////////////////////////////////////////////////////////////
static bool cli_osci_trig_edge_rising(const float32_t sig, const float32_t th)
{
    bool detected = false;

    (void) sig;
    (void) th;

    // TODO: Implement logic...

    return detected;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Oscilloscope trigger on falling edge
*
* @param[in]    sig         - Signal to check for trigger
* @param[in]    th          - Trigger threshold
* @return       detected    - True when triggers
*/
////////////////////////////////////////////////////////////////////////////////
static bool cli_osci_trig_edge_falling(const float32_t sig, const float32_t th)
{
    bool detected = false;

    (void) sig;
    (void) th;

    // TODO: Implement logic...

    return detected;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Oscilloscope trigger on both (rising & falling) edge
*
* @param[in]    sig         - Signal to check for trigger
* @param[in]    th          - Trigger threshold
* @return       detected    - True when triggers
*/
////////////////////////////////////////////////////////////////////////////////
static bool cli_osci_trig_edge_both(const float32_t sig, const float32_t th)
{
    bool detected = false;

    (void) sig;
    (void) th;

    // TODO: Implement logic...

    return detected;
}

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
            if ( g_cli_osci.channel.num_of > 0 )
            {
                // Reset sample buffer
                (void) ring_buffer_reset( g_cli_osci.samp.buf );

                // Enter waiting state
                g_cli_osci.state = eCLI_OSCI_STATE_WAITING;

                cli_printf( "OK, Osci started!" );
            }
            else
            {
                cli_printf( "ERR, Oscilloscope is not configured!" );
            }
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

        cli_printf( "OK, Osci stopped!" );
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
    float32_t samp_val = 0.0f;

    if ( NULL == p_attr )
    {
        // Sampling finished
        if ( eCLI_OSCI_STATE_DONE  == g_cli_osci.state )
        {
            // Get pointer to Tx buffer
            uint8_t * p_tx_buf = cli_util_get_tx_buf();

            // Calculate sample group interations
            const uint32_t num_of_samp = (uint32_t)( CLI_CFG_PAR_OSCI_SAMP_BUF_SIZE / g_cli_osci.channel.num_of );

            for ( uint32_t samp_it = 0U; samp_it < num_of_samp; samp_it++ )
            {
                // Loop thru parameter list
                for ( uint8_t par_it = 0U; par_it < g_cli_osci.channel.num_of; par_it++ )
                {
                    // Calculate buffer index as inverse access
                    const int32_t buf_idx = ( -CLI_CFG_PAR_OSCI_SAMP_BUF_SIZE + (( samp_it * g_cli_osci.channel.num_of ) + par_it ));

                    // Get value from sample buffer
                    if ( eRING_BUFFER_OK == ring_buffer_get_by_index( g_cli_osci.samp.buf, (float32_t*) &samp_val, buf_idx ))
                    {
                        // Convert to string
                        sprintf((char*) p_tx_buf, "%g", samp_val );

                        // Send
                        cli_send_str( p_tx_buf );

                        // If not last -> send delimiter
                        if ( par_it < ( g_cli_osci.channel.num_of - 1U ))
                        {
                            cli_send_str((const uint8_t*) "," );
                        }
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
*           Command: >>>osci_channels [parId1,parId2,...parIdN]
*
*           where parIdX is Device Parameter ID number
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
    uint32_t    ch_cnt      = 0;
    uint32_t    par_id      = 0;
    par_cfg_t   par_cfg     = {0};
    par_num_t   par_num     = 0;
    bool        invalid_par = false;

    if ( NULL != p_attr )
    {
        // Osci idle
        if  (   ( eCLI_OSCI_STATE_IDLE  == g_cli_osci.state )
            ||  ( eCLI_OSCI_STATE_DONE  == g_cli_osci.state ))
        {
            // Reset counts
            g_cli_osci.channel.num_of = 0U;

            // Parse live watch request command
            while(      ( g_cli_osci.channel.num_of <= CLI_CFG_PAR_MAX_IN_OSCI)
                    &&  ( 1U == sscanf((const char*) p_attr, "%d%n", (int*) &par_id, (int*) &ch_cnt )))
            {
                // Get parameter ID by number
                if ( ePAR_OK == par_get_num_by_id( par_id, &par_num ))
                {
                    // Add new parameter to streaming list
                    g_cli_osci.channel.list[ g_cli_osci.channel.num_of ] = par_num;
                    g_cli_osci.channel.num_of++;

                    // Increment attribute cursor
                    p_attr += ch_cnt;

                    // Skip comma
                    if ( ',' == *p_attr )
                    {
                        p_attr++;
                    }
                }

                // Invalid parameter ID
                else
                {
                    // Reset watch list
                    g_cli_osci.channel.num_of = 0;

                    // Raise invalid parameter flag
                    invalid_par = true;

                    cli_printf( "ERR, Wrong parameter ID! ID: %d does not exsist!", par_id );

                    // Exit reading command
                    break;
                }
            }

            // Check requested live watch paramter list
            if  (   ( g_cli_osci.channel.num_of > 0 )
                &&  ( g_cli_osci.channel.num_of <= CLI_CFG_PAR_MAX_IN_OSCI ))
            {
                // Get pointer to Tx buffer
                uint8_t * p_tx_buf = cli_util_get_tx_buf();

                // Send sample time
                snprintf((char*) p_tx_buf, CLI_CFG_TX_BUF_SIZE, "OK" );
                cli_send_str( p_tx_buf );

                // Print streaming parameters/variables
                for ( uint8_t par_idx = 0; par_idx < g_cli_osci.channel.num_of; par_idx++ )
                {
                    // Get parameter configurations
                    par_get_config( g_cli_osci.channel.list[ par_idx ], &par_cfg );

                    // Format string with parameters info
                    sprintf((char*) p_tx_buf, ",%s", par_cfg.name );

                    // Send
                    cli_send_str( p_tx_buf );
                }

                // Terminate line
                cli_printf("");
            }

            // Raise error only if all valid parameters
            else if ( false == invalid_par )
            {
                cli_printf( "ERR, Invalid number of osci channels!" );
            }

            else
            {
                // No actions...
            }
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
    uint32_t    type        = 0U;
    uint32_t    par_id      = 0U;
    float32_t   threshold   = 0.0f;
    float32_t   pretrigger  = 0U;
    par_num_t   par_num     = 0U;

    if ( NULL != p_attr )
    {
        // Osci idle
        if  (   ( eCLI_OSCI_STATE_IDLE  == g_cli_osci.state )
            ||  ( eCLI_OSCI_STATE_DONE  == g_cli_osci.state ))
        {
            if ( 4U == sscanf((const char*) p_attr, "%d,%d,%f,%f", (int*) &type, (int*) &par_id, (float*) &threshold, (float32_t*) &pretrigger ))
            {
                if  (   ( type < eCLI_OSCI_TRIG_NUM_OF )
                    &&  ( ePAR_OK == par_get_num_by_id( par_id, &par_num ))
                    &&  (( pretrigger >= 0.0f) && ( pretrigger <= 1.0f )))
                {
                    g_cli_osci.trigger.type 		= (cli_osci_trig_t) type;
                    g_cli_osci.trigger.par  		= par_num;
                    g_cli_osci.trigger.th   		= threshold;
                    g_cli_osci.trigger.pretrigger   = pretrigger;
                    g_cli_osci.trigger.trig_idx     = (uint32_t)( pretrigger * CLI_CFG_PAR_OSCI_SAMP_BUF_SIZE );

                    cli_printf( "OK, Oscilloscope trigger set!" );
                }
                else
                {
                    cli_printf( "ERR, Invalid trigger settings!" );
                }
            }
            else
            {
                cli_printf( "ERR, Invalid trigger settings!" );
            }
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
* @brief        Set oscilloscope downsample configuration
*
*           Command: >>>osci_rate [factor]
*
*               -factor:    Downsample factor. Valid: 1-1000
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
    uint32_t downsample = 0U;

    if ( NULL != p_attr )
    {
        // Osci idle
        if  (   ( eCLI_OSCI_STATE_IDLE  == g_cli_osci.state )
            ||  ( eCLI_OSCI_STATE_DONE  == g_cli_osci.state ))
        {
            if ( 1U == sscanf((const char*) p_attr, "%d", (int*) &downsample ))
            {
                if  (   ( downsample > 0U )
                    &&  ( downsample <= 1000U ))
                {
                    g_cli_osci.samp.downsample_factor = downsample;

                    cli_printf( "OK, Oscilloscope downsample set!" );
                }
                else
                {
                    cli_printf( "ERR, Invalid downsample settings!" );
                }
            }
            else
            {
                cli_printf( "ERR, Invalid downsample settings!" );
            }
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
* @brief        Get oscilloscope state
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_osci_state(const uint8_t * p_attr)
{
    if ( NULL == p_attr )
    {
        const char * status_str[] = { "IDLE", "WAITING", "SAMPLING", "DONE" };

        cli_printf( "OK, %s", status_str[g_cli_osci.state] );
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

    // Initialize sample ring buffer
    status = cli_osci_init_buf();

    // Init downsample rate
    g_cli_osci.samp.downsample_factor = 1U;

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
    static uint32_t samp_cnt = 0U;

    // Handle downsample
    if ( samp_cnt >= ( g_cli_osci.samp.downsample_factor - 1U ))
    {
        if ( NULL != gpf_cli_osci_state_hndl[g_cli_osci.state] )
        {
            gpf_cli_osci_state_hndl[g_cli_osci.state]();
        }

        samp_cnt = 0U;
    }
    else
    {
        samp_cnt++;
    }
}

#endif // ( 1 == CLI_CFG_PAR_USE_EN )

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
