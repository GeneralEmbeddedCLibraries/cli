// Copyright (c) 2025 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      cli_par.h
*@brief     Command Line Interface Device Parameters
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      08.05.2025
*@version   V2.2.0
*/
////////////////////////////////////////////////////////////////////////////////
/*!
* @addtogroup CLI_PAR
* @{ <!-- BEGIN GROUP -->
*
* @brief	This module is responsible for handling Device Parameter via CLI.
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
#include "cli_par.h"
#include "cli_nvm.h"

#include "common/utils/src/utils.h"

#if ( 1 == CLI_CFG_PAR_USE_EN )

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static void         cli_par_print_info          (const par_cfg_t * const p_par_cfg, const uint32_t par_val);
static void         cli_par_print_header        (void);

// Device parameters CLI commands
static void         cli_par_info                (const char * p_attr);
static void         cli_par_set                 (const char * p_attr);
static void         cli_par_get                 (const char * p_attr);
static void         cli_par_def                 (const char * p_attr);
static void         cli_par_def_all             (const char * p_attr);
static void         cli_par_store               (const char * p_attr);

// Live watch CLI commands
static void         cli_watch_start             (const char * p_attr);
static void         cli_watch_stop              (const char * p_attr);
static void         cli_watch_channel           (const char * p_attr);
static void         cli_watch_rate              (const char * p_attr);
static void         cli_watch_info              (const char * p_attr);

static float32_t    cli_par_val_to_float        (const par_type_list_t par_type, const void * p_val);
static void         cli_par_live_watch_hndl     (void);
static void         cli_par_group_print         (const par_num_t par_num);

#if (( 1 == CLI_CFG_DEBUG_EN ) && ( 1 == PAR_CFG_NVM_EN ))
    static void cli_par_store_reset(const char * p_attr);
#endif

#if ( 1 == CLI_CFG_PAR_STREAM_NVM_EN )
    static void cli_watch_save(const char * p_attr);
#endif

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 *      Device Parameters CLI commands
 */
static const cli_cmd_t g_cli_par_table[] =
{
    // ----------------------------------------------------------------------------------------------------------
    //  name                    function                help string
    // ----------------------------------------------------------------------------------------------------------
    {   "par_info",             cli_par_info,           "Get device parameter informations"                     },
    {   "par_set",              cli_par_set,            "Set parameter. Args: [parId,value]"                    },
    {   "par_get",              cli_par_get,            "Get parameter. Args: [parId]"                          },
    {   "par_def",              cli_par_def,            "Set parameter to default. Args: [parId]"               },
    {   "par_def_all",          cli_par_def_all,        "Set all parameters to default"                     	},
    {   "par_save",             cli_par_store,          "Save parameter to NVM"                             	},

#if (( 1 == CLI_CFG_DEBUG_EN ) && ( 1 == PAR_CFG_NVM_EN ))
    {   "par_save_clean",       cli_par_store_reset,    "Clean saved parameters space in NVM"                   },
#endif
};

/**
 *      Live watch CLI commands
 */
static const cli_cmd_t g_cli_watch_table[] =
{
    // ----------------------------------------------------------------------------------------------------------
    //  name                    function                help string
    // ----------------------------------------------------------------------------------------------------------
    {   "watch_start",          cli_watch_start,        "Start parameter value live watch"                              },
    {   "watch_stop",           cli_watch_stop,         "Stop parameter value live watch"                               },
    {   "watch_channel",        cli_watch_channel,      "Set live watch channels. Args: [parId1,parId2,...,parIdN]"     },
    {   "watch_rate",           cli_watch_rate,         "Change live watch streaming period. Args: [miliseconds]"       },
    {   "watch_info",           cli_watch_info,         "Get live watch configuration info"                             },

#if ( 1 == CLI_CFG_PAR_STREAM_NVM_EN )
    {   "watch_save",          cli_watch_save,          "Save live watch configuration into to NVM"                     },
#endif
};

/**
 *      Live watch data
 *
 *  Inside "par_list" there is parameter enumeration number not parameter ID!
 */
static cli_live_watch_t g_cli_live_watch =
{
    .period     = CLI_CFG_PAR_DEF_LIVE_WATCH_PER_MS,
    .period_cnt = (uint32_t)( CLI_CFG_PAR_DEF_LIVE_WATCH_PER_MS / CLI_CFG_PAR_HNDL_PERIOD_MS ),
    .active     = false,
    .num_of     = 0,
    .par_list   = {0}
};

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Print parameter information
*
* @note     Sending parameter informations in following format:
*
*           >>>ID,Name,Value,Default,Min,Max,Unit,Type,Access,Persistance,Description
*
* @param[in]    p_par_cfg   - Pointer to paramter configurations
* @param[in]    par_val     - Parameter value
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_par_print_info(const par_cfg_t * const p_par_cfg, const uint32_t par_val)
{
    char * unit_str = "";
    char * desc_str = "";

    // Unit defined -> replace empty string
    if ( NULL != p_par_cfg->unit )
    {
       unit_str = (char*) p_par_cfg->unit;
    }

    // Description defined -> replace empty string
    if ( NULL != p_par_cfg->desc )
    {
       desc_str = (char*) p_par_cfg->desc;
    }

    // Par info response
    cli_printf( "%u,%s,%g,%g,%g,%g,%s,%d,%d,%d,%s",
            (int) p_par_cfg->id,
            p_par_cfg->name,
            cli_par_val_to_float( p_par_cfg->type, &par_val ),
            cli_par_val_to_float( p_par_cfg->type, &( p_par_cfg->def.u32 )),
            cli_par_val_to_float( p_par_cfg->type, &( p_par_cfg->min.u32 )),
            cli_par_val_to_float( p_par_cfg->type, &( p_par_cfg->max.u32 )),
            unit_str,
            p_par_cfg->type,
            p_par_cfg->access,
            p_par_cfg->persistant,
            desc_str );
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Print parameter info header
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_par_print_header(void)
{
    cli_printf( ";ID,Name,Value,Def,Min,Max,Unit,Type,Access,Persistance,Description" );
    cli_printf( ": " );
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Print parameter details
*
* @note         Command format: >>>par_print
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_par_info(const char * p_attr)
{
    par_cfg_t   par_cfg     = { 0 };
    uint32_t    par_num     = 0UL;
    uint32_t    par_val     = 0UL;

    if ( NULL == p_attr )
    {
        // Send header
        cli_par_print_header();

        // For each parameter
        for ( par_num = 0; par_num < ePAR_NUM_OF; par_num++ )
        {
            // Get parameter configuration
            par_get_config( par_num, &par_cfg );

            // Get current parameter value
            par_get( par_num, &par_val );

            // Print group name
            cli_par_group_print( par_num );

            // Print parameter info
            cli_par_print_info((const par_cfg_t*) &par_cfg, par_val );
        }

        // Table termination string
        cli_printf(";END");
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Set parameter value
*
* @note         Command format: >>>par_set [ID,value]
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_par_set(const char * p_attr)
{
    uint16_t        par_id      = 0;
    par_num_t       par_num     = 0;
    par_type_t      par_data    = { .u32 = 0UL };
    par_status_t    status      = ePAR_OK;
    par_cfg_t       par_cfg     = {0};

    // Make sure we can cast uint32_t to unsigned int and int32_t to int below to supress compiler warning
    // when types do not match exactly for example unsigned long to unsigned int
    STATIC_ASSERT_TYPES(uint32_t, unsigned int);
    STATIC_ASSERT_TYPES(int32_t, int);

    if ( NULL != p_attr )
    {
        // Check input command
        if ( 2U == sscanf((const char*) p_attr, "%hu,%f", &par_id, &par_data.f32 ))
        {
            // Check if parameter exist
            if ( ePAR_OK == par_get_num_by_id( par_id, &par_num ))
            {
                // Get parameter configurations
                par_get_config( par_num, &par_cfg );

                // Check if parameter writable
                if ( ePAR_ACCESS_RW == par_cfg.access )
                {
                    // Based on type get parameter
                    switch( par_cfg.type )
                    {
                        case ePAR_TYPE_U8:
                            (void) sscanf( p_attr, "%hu,%hhu", &par_id, &par_data.u8 );
                            status = par_set( par_num, &par_data.u8 );
                            cli_printf( "OK,PAR_SET=%hhu", par_data.u8);
                        break;

                        case ePAR_TYPE_I8:
                            sscanf( p_attr, "%hu,%hhi", &par_id, &par_data.i8 );
                            status = par_set( par_num, &par_data.i8 );
                            cli_printf( "OK,PAR_SET=%hhi", par_data.i8);
                        break;

                        case ePAR_TYPE_U16:
                            sscanf( p_attr, "%hu,%hu", &par_id, &par_data.u16 );
                            status = par_set( par_num, &par_data.u16 );
                            cli_printf( "OK,PAR_SET=%hu", par_data.u16);
                        break;

                        case ePAR_TYPE_I16:
                            sscanf( p_attr, "%hu,%hi", &par_id, &par_data.i16 );
                            status = par_set( par_num, &par_data.i16 );
                            cli_printf( "OK,PAR_SET=%hi", par_data.i16);
                        break;

                        case ePAR_TYPE_U32:
                            sscanf( p_attr, "%hu,%u", &par_id, (unsigned int*)&par_data.u32 );
                            status = par_set( par_num, &par_data.u32 );
                            cli_printf( "OK,PAR_SET=%u", par_data.u32);
                        break;

                        case ePAR_TYPE_I32:
                            sscanf( p_attr, "%hu,%i", &par_id, (int*)&par_data.i32 );
                            status = par_set( par_num, &par_data.i32 );
                            cli_printf( "OK,PAR_SET=%i", par_data.i32);
                        break;

                        case ePAR_TYPE_F32:
                            status = par_set( par_num, &par_data.f32 );
                            cli_printf( "OK,PAR_SET=%g", par_data.f32);
                        break;

                        case ePAR_TYPE_NUM_OF:
                        default:
                            CLI_DBG_PRINT( "ERR, Invalid parameter type!" );
                            CLI_ASSERT( 0 );
                        break;
                    }

                    if ( ePAR_OK != status )
                    {
                        cli_printf( "ERR, err_code: %u", (uint16_t)status);
                    }
                }
                else
                {
                    cli_printf( "ERR, Parameter is read only!" );
                }
            }
            else
            {
                cli_printf( "ERR, Wrong parameter ID!" );
            }
        }
        else
        {
            cli_printf( "ERR, Wrong command!" );
        }
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        get parameter value
*
* @note         Command format: >>>par_get [ID]
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_par_get(const char * p_attr)
{
    uint16_t        par_id      = 0;
    par_num_t       par_num     = 0;
    par_type_t      par_data    = { .u32 = 0UL };
    par_status_t    status      = ePAR_OK;
    par_cfg_t       par_cfg     = {0};

    // Make sure we can cast uint32_t to unsigned int and int32_t to int below to supress compiler warning
    // when types do not match exactly for example unsigned long to unsigned int
    STATIC_ASSERT_TYPES(uint32_t, unsigned int);
    STATIC_ASSERT_TYPES(int32_t, int);

    if ( NULL != p_attr )
    {
        // Check input command
        if ( 1U == sscanf( p_attr, "%hu", &par_id ))
        {
            // Check if parameter exist
            if ( ePAR_OK == par_get_num_by_id( par_id, &par_num ))
            {
                // Get par configurations
                par_get_config( par_num, &par_cfg );

                // Based on type get parameter
                switch ( par_cfg.type )
                {
                    case ePAR_TYPE_U8:
                        status = par_get( par_num, &par_data.u8 );
                        cli_printf( "OK,PAR_GET=%hhu", par_data.u8 );
                    break;

                    case ePAR_TYPE_I8:
                        status = par_get( par_num, &par_data.i8 );
                        cli_printf(  "OK,PAR_GET=%hhi", par_data.i8 );
                    break;

                    case ePAR_TYPE_U16:
                        status = par_get( par_num, &par_data.u16 );
                        cli_printf(  "OK,PAR_GET=%hu", par_data.u16 );
                    break;

                    case ePAR_TYPE_I16:
                        status = par_get( par_num, &par_data.i16 );
                        cli_printf(  "OK,PAR_GET=%hi", par_data.i16 );
                    break;

                    case ePAR_TYPE_U32:
                        status = par_get( par_num, (unsigned int*) &par_data.u32 );
                        cli_printf(  "OK,PAR_GET=%u", par_data.u32 );
                    break;

                    case ePAR_TYPE_I32:
                        status = par_get( par_num, (int*) &par_data.i32 );
                        cli_printf(  "OK,PAR_GET=%i", par_data.i32 );
                    break;

                    case ePAR_TYPE_F32:
                        status = par_get( par_num, (float32_t*) &par_data.f32 );
                        cli_printf(  "OK,PAR_GET=%g", par_data.f32 );
                    break;

                    case ePAR_TYPE_NUM_OF:
                    default:
                        CLI_DBG_PRINT( "ERR, Invalid parameter type!" );
                        CLI_ASSERT( 0 );
                    break;
                }

                if ( ePAR_OK != status )
                {
                    cli_printf( "ERR, err_code: %u", (uint16_t)status);
                }
            }
            else
            {
                cli_printf( "ERR, Wrong parameter ID!" );
            }
        }
        else
        {
            cli_printf( "ERR, Wrong command!" );
        }
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Set parameter value to default
*
* @note         Command format: >>>par_def [ID]
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_par_def(const char * p_attr)
{
    par_num_t   par_num = 0UL;
    uint16_t    par_id  = 0UL;

    if ( NULL != p_attr )
    {
        // Check input command
        if ( 1U == sscanf( p_attr, "%u", (unsigned int*)&par_id ))
        {
            // Check if parameter exist
            if ( ePAR_OK == par_get_num_by_id( par_id, &par_num ))
            {
                // Set to default
                par_set_to_default( par_num );

                // Rtn msg
                cli_printf( "OK, Parameter %u set to default", par_id );
            }
            else
            {
                cli_printf( "ERR, Wrong parameter ID!" );
            }
        }
        else
        {
            cli_printf( "ERR, Wrong command!" );
        }
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Set all parameters value to default
*
* @note         Command format: >>>par_def_all
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_par_def_all(const char * p_attr)
{
    if ( NULL == p_attr )
    {
        // Set to default
        par_set_all_to_default();

        // Rtn msg
        cli_printf( "OK, All parameters set to default!" );
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Store all persistent parameters to NVM
*
* @note         Command format: >>>par_store
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_par_store(const char * p_attr)
{
    if ( NULL == p_attr )
    {
        #if ( 1 == PAR_CFG_NVM_EN )

            if ( ePAR_OK == par_save_all())
            {
                cli_printf( "OK, Parameter successfully store to NVM" );
            }
            else
            {
                cli_printf( "ERR, Error while storing to NVM" );
            }

        #else
            cli_printf( "ERR, Storing to NVM not supported!" );
        #endif
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

#if (( 1 == CLI_CFG_DEBUG_EN ) && ( 1 == PAR_CFG_NVM_EN ))

    ////////////////////////////////////////////////////////////////////////////////
    /*!
    * @brief        Clean parameter NVM region
    *
    * @note         Command format: >>>par_save_clean
    *
    * @param[in]    attr    - Inputed command attributes
    * @return       void
    */
    ////////////////////////////////////////////////////////////////////////////////
    static void cli_par_store_reset(const char * p_attr)
    {
        if ( NULL == p_attr )
        {
            if ( ePAR_OK == par_save_clean())
            {
                cli_printf( "OK, Parameter NVM region successfully cleaned" );
            }
            else
            {
                cli_printf( "ERR, Error while cleaning parameter space in NVM" );
            }
        }
        else
        {
            cli_util_unknown_cmd_rsp();
        }
    }

#endif

#if (( 1 == CLI_CFG_PAR_USE_EN ) && ( 1 == CLI_CFG_PAR_STREAM_NVM_EN ))

    ////////////////////////////////////////////////////////////////////////////////
    /*!
    * @brief        Store streaming informations to NVM
    *
    * @note         Command format: >>>watch_save
    *
    * @param[in]    attr    - Inputed command attributes
    * @return       void
    */
    ////////////////////////////////////////////////////////////////////////////////
    static void cli_watch_save(const char * p_attr)
    {
        if ( NULL == p_attr )
        {
            if ( eCLI_OK == cli_nvm_write( &g_cli_live_watch ))
            {
                cli_printf( "OK, Streaming info stored to NVM successfully" );
            }
            else
            {
                cli_printf( "ERR, Error while storing streaming info to NVM!" );
            }
        }
        else
        {
            cli_util_unknown_cmd_rsp();
        }
    }

#endif

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Start live watch streaming
*
* @note         Command format: >>>watch_start
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_watch_start(const char * p_attr)
{
    if ( NULL == p_attr )
    {
        if ( g_cli_live_watch.num_of > 0 )
        {
            g_cli_live_watch.active = true;

            cli_printf( "OK, Streaming started!" );

            #if ( 1 == CLI_CFG_PAR_AUTO_STREAM_STORE_EN )
                cli_watch_save( NULL );
            #endif
        }
        else
        {
           cli_printf( "ERR, Streaming parameter list empty!" );
        }
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Stop live watch streaming
*
* @note         Command format: >>>watch_stop
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_watch_stop(const char * p_attr)
{
    if ( NULL == p_attr )
    {
        g_cli_live_watch.active = false;

        cli_printf( "OK, Streaming stopped!" );

        #if ( 1 == CLI_CFG_PAR_AUTO_STREAM_STORE_EN )
            cli_watch_save( NULL );
        #endif
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Put parameters to live watch
*
* @note         Command format: >>>watch_des [parID1,parID2,..parIDn]
*
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_watch_channel(const char * p_attr)
{
    uint32_t    ch_cnt      = 0;
    uint16_t    par_id      = 0;
    par_cfg_t   par_cfg     = {0};
    par_num_t   par_num     = 0;
    bool        invalid_par = false;

    if ( NULL != p_attr )
    {
        // Reset counts
        g_cli_live_watch.num_of = 0;

        // Parse live watch request command
        while(      ( g_cli_live_watch.num_of <= CLI_CFG_PAR_MAX_IN_LIVE_WATCH )
                &&  ( 1U == sscanf( p_attr, "%hu%n", &par_id, (int*) &ch_cnt )))
        {
            // Get parameter ID by number
            if ( ePAR_OK == par_get_num_by_id( par_id, &par_num ))
            {
                // Add new parameter to streaming list
                g_cli_live_watch.par_list[ g_cli_live_watch.num_of ] = par_num;
                g_cli_live_watch.num_of++;

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
                g_cli_live_watch.num_of = 0;

                // Raise invalid parameter flag
                invalid_par = true;

                cli_printf( "ERR, Wrong parameter ID! ID: %d does not exsist!", par_id );

                // Exit reading command
                break;
            }
        }

        // Check requested live watch paramter list
        if  (   ( g_cli_live_watch.num_of > 0 )
            &&  ( g_cli_live_watch.num_of <= CLI_CFG_PAR_MAX_IN_LIVE_WATCH ))
        {
            // Get pointer to Tx buffer
            uint8_t * p_tx_buf = cli_util_get_tx_buf();

            // Send sample time
            snprintf((char*) p_tx_buf, CLI_CFG_TX_BUF_SIZE, "OK,%g", ( (float32_t)g_cli_live_watch.period / 1000.0f ));
            cli_send_str( p_tx_buf );

            // Print streaming parameters/variables
            for ( uint8_t par_idx = 0; par_idx < g_cli_live_watch.num_of; par_idx++ )
            {
                // Get parameter configurations
                par_get_config( g_cli_live_watch.par_list[ par_idx ], &par_cfg );

                // Format string with parameters info
                sprintf((char*) p_tx_buf, ",%s,d,1", par_cfg.name );

                // Send
                cli_send_str( p_tx_buf );
            }

            // Terminate line
            cli_printf("");

            #if ( 1 == CLI_CFG_PAR_AUTO_STREAM_STORE_EN )
                cli_watch_save( NULL );
            #endif
        }

        // Raise error only if all valid parameters
        else if ( false == invalid_par )
        {
            cli_printf( "ERR, Invalid number of streaming parameter!" );
        }

        else
        {
            // No actions...
        }
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Change rate of live watch streaming period
*
* @note         Command format: >>>watch_rate [period_in_ms]
*
* @example      >>>status_rate 100 --> Will change period to 100 ms
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_watch_rate(const char * p_attr)
{
    uint32_t period;

    if ( NULL != p_attr )
    {
        if ( 1U == sscanf( p_attr, "%d", (int*) &period ))
        {
            // Check if within wanted range
            if  (   ( period >= CLI_CFG_PAR_HNDL_PERIOD_MS )
                &&  ( period <= 60000UL ))
            {
                // Check if multiple of defined period
                if (( period % CLI_CFG_PAR_HNDL_PERIOD_MS ) == 0 )
                {
                    g_cli_live_watch.period = period;
                    g_cli_live_watch.period_cnt = (uint32_t) ( g_cli_live_watch.period / CLI_CFG_PAR_HNDL_PERIOD_MS );

                    cli_printf( "OK, Period changed to %d ms", g_cli_live_watch.period );

                    #if ( 1 == CLI_CFG_PAR_AUTO_STREAM_STORE_EN )
                        cli_watch_save( NULL );
                    #endif
                }
                else
                {
                    cli_printf( "ERR, Wanted period is not multiple of \"CLI_CFG_PAR_HNDL_PERIOD_MS\"!" );
                }
            }
            else
            {
                cli_printf( "ERR, Period out of valid range!" );
            }
        }
        else
        {
            cli_printf( "ERR, Wrong command!" );
        }
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Get streaming configuration info
*
* @note         Command format: >>>watch_info
*
* @param[in]    attr    - Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_watch_info(const char * p_attr)
{
    uint16_t par_id = 0U;

    if ( NULL == p_attr )
    {
        // Get pointer to Tx buffer
        uint8_t * p_tx_buf = cli_util_get_tx_buf();

        // Send streaming info as
        // OK, PERIOD,ACTIVE,NUM_OF,PAR_LIST
        sprintf((char*) p_tx_buf, "OK, %d,%d,%d", (int)g_cli_live_watch.period, g_cli_live_watch.active, g_cli_live_watch.num_of );
        cli_send_str( p_tx_buf );

        // Print streaming parameters/variables
        for ( uint8_t par_idx = 0; par_idx < g_cli_live_watch.num_of; par_idx++ )
        {
            // Get parameter ID
            (void) par_get_id( g_cli_live_watch.par_list[par_idx], &par_id );

            // Format string with parameters info
            sprintf((char*) p_tx_buf, ",%d", par_id );

            // Send
            cli_send_str( p_tx_buf );
        }

        // Terminate line
        cli_printf("");
    }
    else
    {
        cli_util_unknown_cmd_rsp();
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Convert parameter any value type to float
*
*   @note   It is being used for sprintf functionalities
*
* @param[in]    par_type    - Data type of parameter
* @param[in]    p_val       - Pointer to parameter value
* @return       f32_par_val - Floating representation of parameter value
*/
////////////////////////////////////////////////////////////////////////////////
static float32_t cli_par_val_to_float(const par_type_list_t par_type, const void * p_val)
{
    float32_t f32_par_val = 0.0f;

    switch( par_type )
    {
        case ePAR_TYPE_U8:
            f32_par_val = *(uint8_t*) p_val;
            break;

        case ePAR_TYPE_I8:
            f32_par_val = *(int8_t*) p_val;
            break;

        case ePAR_TYPE_U16:
            f32_par_val = *(uint16_t*) p_val;
            break;

        case ePAR_TYPE_I16:
            f32_par_val = *(int16_t*) p_val;
            break;

        case ePAR_TYPE_U32:
            f32_par_val = (float32_t)(*(uint32_t*) p_val);
            break;

        case ePAR_TYPE_I32:
            f32_par_val = (float32_t)(*(int32_t*) p_val);
            break;

        case ePAR_TYPE_F32:
            f32_par_val = *(float32_t*) p_val;
            break;

        case ePAR_TYPE_NUM_OF:
        default:
            // No actions..
            break;
    }

    return f32_par_val;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Parameter live watch handler
*
*               Executes in main "cli_hndl()" and streams parameters inside
*               live watch queue if live watch is enabled.
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_par_live_watch_hndl(void)
{
    par_type_t  par_val = { .u32 = 0UL };
    par_cfg_t   par_cfg = {0};

    // Stream data only if:
    //      1. Live watch is active
    //  AND 2. Any parameter to stream
    if  (   ( true == g_cli_live_watch.active )
        &&  ( g_cli_live_watch.num_of > 0 ))
    {
        // Get pointer to Tx buffer
        uint8_t * p_tx_buf = cli_util_get_tx_buf();

        // Loop thru streaming parameters
        for ( uint8_t par_it = 0; par_it < g_cli_live_watch.num_of; par_it++ )
        {
            // Get parameter data type
            par_get_config( g_cli_live_watch.par_list[par_it], &par_cfg );

            // Get parameter
            par_get( g_cli_live_watch.par_list[par_it], &par_val.u32 );

            // Based on type fill streaming buffer
            switch ( par_cfg.type )
            {
                case ePAR_TYPE_U8:
                    sprintf((char*) p_tx_buf, "%d", (int)par_val.u8 );
                    break;
                case ePAR_TYPE_U16:
                    sprintf((char*) p_tx_buf, "%d", (int)par_val.u16 );
                break;
                case ePAR_TYPE_U32:
                    sprintf((char*) p_tx_buf, "%d", (int)par_val.u32 );
                break;
                case ePAR_TYPE_I8:
                    sprintf((char*) p_tx_buf, "%i", (int)par_val.i8 );
                    break;
                case ePAR_TYPE_I16:
                    sprintf((char*) p_tx_buf, "%i", (int)par_val.i16 );
                break;
                case ePAR_TYPE_I32:
                    sprintf((char*) p_tx_buf, "%i", (int)par_val.i32 );
                break;
                case ePAR_TYPE_F32:
                    sprintf((char*) p_tx_buf, "%g", par_val.f32 );
                break;

                case ePAR_TYPE_NUM_OF:
                default:
                    // No actions..
                break;
            }

            // Send
            cli_send_str( p_tx_buf );

            // If not last -> send delimiter
            if ( par_it < ( g_cli_live_watch.num_of - 1 ))
            {
                cli_send_str((const uint8_t*) "," );
            }
        }

        // Terminate line
        cli_printf("");
    }
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Print parameter groupe name
*
* @param[in]    par_num - Parameter enumeration number
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_par_group_print(const par_num_t par_num)
{
    // Get group name
    const char * group_name = cli_cfg_get_par_groupe_str( par_num );

    // If defined print it
    if ( NULL != group_name )
    {
        cli_printf( ":%s", group_name );
    }
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup CLI_PAR_API
* @{ <!-- BEGIN GROUP -->
*
* 	Following function are part of CLI Device Parameters API.
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Initialize CLI Device Parameters sub-component
*
* @return       status      - Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_par_init(void)
{
    cli_status_t status = eCLI_OK;

    #if ( 1 == CLI_CFG_PAR_STREAM_NVM_EN )

        // Init NVM
        if ( eNVM_OK == nvm_init())
        {
            // Read streaming info
            status = cli_nvm_read( &g_cli_live_watch );

            // If at init CLI nvm is corrupted -> override it with default
            if ( eCLI_OK != status )
            {
                status = cli_nvm_write( &g_cli_live_watch );
            }
        }
        else
        {
            status = eCLI_ERROR_INIT;
        }

    #endif

    // Register Device Parameters CLI table
    cli_register_cmd_table((const cli_cmd_t*) &g_cli_par_table, ( sizeof(g_cli_par_table) / sizeof(cli_cmd_t)));
    cli_register_cmd_table((const cli_cmd_t*) &g_cli_watch_table, ( sizeof(g_cli_watch_table) / sizeof(cli_cmd_t)));

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        CLI Device Parameters handler
*
* @note     This is used for parameters live watch (streaming) purposes!
*
*           Shall not be used in ISR!
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_par_hndl(void)
{
    cli_status_t status = eCLI_OK;

    static uint32_t loop_cnt = 0;

    // Count main handler loops
    if ( loop_cnt >= ( g_cli_live_watch.period_cnt - 1 ))
    {
        loop_cnt = 0;

        // Handle streaming
        cli_par_live_watch_hndl();
    }
    else
    {
        loop_cnt++;
    }

    return status;
}

#endif // ( 1 == CLI_CFG_PAR_USE_EN )

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
