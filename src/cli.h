// Copyright (c) 2022 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      cli.h
*@brief     Command Line Interface API
*@author    Ziga Miklosic
*@date      11.09.2022
*@version   V0.0.1
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup CLI_API
* @{ <!-- BEGIN GROUP -->
*
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef __CLI_H
#define __CLI_H

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>

#include "../../cli_cfg.h"


////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * 	Module version
 */
#define CLI_VER_MAJOR		( 0 )
#define CLI_VER_MINOR		( 0 )
#define CLI_VER_DEVELOP		( 1 )

/**
 * 	CLI Status
 */
typedef enum
{
	eCLI_OK				= 0,		/**<Normal operation */

	eCLI_ERROR			= 0x01,		/**<General error code */
	eCLI_ERROR_INIT		= 0x02,		/**<Initialization error or usage before initialization */

} cli_status_t;

/**
 * 	 CLI Command Function
 */
typedef void(*pf_cli_cmd)(const uint8_t * const p_attr);

/**
 * 	 CLI Command Table
 */
typedef struct
{
	const uint8_t * p_name;		/**<Command name*/
	pf_cli_cmd		p_func;		/**<Command function */
	const uint8_t * p_help;		/**<Command help string */
} cli_cmd_t;


////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_init		(void);
cli_status_t cli_deinit		(void);
cli_status_t cli_is_init	(bool * const p_is_init);
cli_status_t cli_hndl		(void);
cli_status_t cli_printf		(char * p_format, ...);

#if ( 1 == CLI_CFG_CHANNEL_EN )
	cli_status_t cli_printf_ch	(const cli_ch_opt_t ch, char * p_format, ...);
#endif


#endif // __CLI_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
