// Copyright (c) 2024 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      cli_nvm.h
*@brief     Command Line Interface NVM storage
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      04.08.2024
*@version   V2.0.1
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup CLI_NVM_API
* @{ <!-- BEGIN GROUP -->
*
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef __CLI_NVM_H
#define __CLI_NVM_H

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>

#include "cli.h"

#if ( 1 == CLI_CFG_PAR_STREAM_NVM_EN )

#include "middleware/nvm/nvm/src/nvm.h"

/**
 * 	Check NVM module compatibility
 */
_Static_assert( 2 == NVM_VER_MAJOR );
_Static_assert( 1 <= NVM_VER_MINOR );

#endif // ( 1 == CLI_CFG_PAR_STREAM_NVM_EN )

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

#if ( 1 == CLI_CFG_PAR_USE_EN )

/**
 *  Streaming info
 */
typedef struct
{
    uint16_t	par_list[CLI_CFG_PAR_MAX_IN_LIVE_WATCH];	/**<Parameters number inside live watch queue. Values are paraemters enumeration not parameter ID! */
    uint32_t    period;                                     /**<Period of streaming in ms */
    uint32_t    period_cnt;                                 /**<Period of streaming in multiple of CLI_CFG_HNDL_PERIOD_MS */
    uint8_t		num_of;                                     /**<Number of parameters inside live watch */
    bool 		active;                                     /**<Active flag */
} cli_live_watch_t;

#endif // ( 1 == CLI_CFG_PAR_USE_EN )

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
#if ( 1 == CLI_CFG_PAR_STREAM_NVM_EN )

cli_status_t cli_nvm_read   (cli_live_watch_t * const p_watch_info);
cli_status_t cli_nvm_write  (const cli_live_watch_t * const p_watch_info);

#endif // ( 1 == CLI_CFG_PAR_STREAM_NVM_EN )

#endif // __CLI_NVM_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
