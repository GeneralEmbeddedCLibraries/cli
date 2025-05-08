// Copyright (c) 2025 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      cli.h
*@brief     Command Line Interface API
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      08.05.2025
*@version   V2.2.0
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
#define CLI_VER_MAJOR		( 2 )
#define CLI_VER_MINOR		( 2 )
#define CLI_VER_DEVELOP		( 0 )

/**
 * 	CLI Status
 */
typedef enum
{
	eCLI_OK				= 0U,		/**<Normal operation */
	eCLI_ERROR			= 0x01U,	/**<General error code */
	eCLI_ERROR_INIT		= 0x02U,	/**<Initialization error or usage before initialization */
    eCLI_ERROR_NVM      = 0x04U,    /**<Read/Write to NVM error */
} cli_status_t;

/**
 * 	 CLI Command Function
 */
typedef void(*pf_cli_cmd)(const uint8_t * const p_attr);

/**
 * 	 Single CLI Command
 *
 * 	 Sizeof: 12 bytes
 */
typedef struct
{
	char * 		p_name;		/**<Command name*/
	pf_cli_cmd	p_func;		/**<Command function */
	char * 		p_help;		/**<Command help string */
} cli_cmd_t;

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_init				(void);
cli_status_t cli_deinit				(void);
cli_status_t cli_is_init			(bool * const p_is_init);
cli_status_t cli_hndl				(void);
cli_status_t cli_send_str           (const uint8_t * const p_str);
cli_status_t cli_printf				(char * p_format, ...);
cli_status_t cli_printf_ch			(const cli_ch_opt_t ch, char * p_format, ...);
cli_status_t cli_register_cmd_table (const cli_cmd_t * const p_cmd_table, const uint8_t num_of_cmd);
cli_status_t cli_osci_hndl          (void);

#endif // __CLI_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
