// Copyright (c) 2024 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      cli_util.c
*@brief     Command Line Interface Utility
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      04.08.2024
*@version   V2.0.1
*/
////////////////////////////////////////////////////////////////////////////////
/*!
* @addtogroup CLI_UTIL
* @{ <!-- BEGIN GROUP -->
*
* @brief	This module has common goods used across CLI sub-components
*
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>

#include "cli_util.h"
#include "cli_par.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 *      Common Transmit buffer for all CLI sub-modules
 */
static uint8_t gu8_tx_buffer[CLI_CFG_TX_BUF_SIZE] = {0};

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup CLI_UTIL_API
* @{ <!-- BEGIN GROUP -->
*
* 	Following function are part of CLI Utility API.
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Common CLI response to unknown command
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
void cli_util_unknown_cmd_rsp(void)
{
    cli_printf( "ERR, Unknown command!" );
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Get pointer to common Tx buffer
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
uint8_t * cli_util_get_tx_buf(void)
{
    return (uint8_t*) &gu8_tx_buffer;
}

#if ( 1 == CLI_CFG_PAR_USE_EN )

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief    Convert parameter any value type to float
*
* @param[in]    par         - Parameter
* @return       f32_par_val - Floating representation of parameter value
*/
////////////////////////////////////////////////////////////////////////////////
float32_t cli_util_par_val_to_float(const par_num_t par)
{
    float32_t   f32_par_val = 0.0f;
    par_cfg_t   par_cfg     = {0};

    // Get parameter type
    if ( ePAR_OK == par_get_config( par, &par_cfg ))
    {
        // Based on type convert to float
        switch ( par_cfg.type )
        {
            case ePAR_TYPE_U8:
                uint8_t u8_val = 0U;
                (void) par_get( par, (uint8_t*) &u8_val );
                f32_par_val = (float32_t) u8_val;
                break;

            case ePAR_TYPE_I8:
                int8_t i8_val = 0U;
                (void) par_get( par, (int8_t*) &i8_val );
                f32_par_val = (float32_t) i8_val;
                break;

            case ePAR_TYPE_U16:
                uint16_t u16_val = 0U;
                (void) par_get( par, (uint16_t*) &u16_val );
                f32_par_val = (float32_t) u16_val;
                break;

            case ePAR_TYPE_I16:
                int16_t i16_val = 0U;
                (void) par_get( par, (int16_t*) &i16_val );
                f32_par_val = (float32_t) i16_val;
                break;

            case ePAR_TYPE_U32:
                uint32_t u32_val = 0U;
                (void) par_get( par, (uint32_t*) &u32_val );
                f32_par_val = (float32_t) u32_val;
                break;

            case ePAR_TYPE_I32:
                int32_t i32_val = 0U;
                (void) par_get( par, (int32_t*) &i32_val );
                f32_par_val = (float32_t) i32_val;
                break;

            case ePAR_TYPE_F32:
                (void) par_get( par, (float32_t*) &f32_par_val );
                break;

            case ePAR_TYPE_NUM_OF:
            default:
                PAR_ASSERT( 0 );
                break;
        }
    }

    return f32_par_val;
}

#endif // ( 1 == CLI_CFG_PAR_USE_EN )

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
