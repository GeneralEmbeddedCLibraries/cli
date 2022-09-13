// Copyright (c) 2022 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      cli.c
*@brief     Command Line Interface
*@author    Ziga Miklosic
*@date      11.09.2022
*@version   V0.0.1
*/
////////////////////////////////////////////////////////////////////////////////
/*!
* @addtogroup CLI
* @{ <!-- BEGIN GROUP -->
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

#include "cli.h"
#include "../../cli_cfg.h"
#include "../../cli_if.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * 		Working buffer size
 *
 * 	Unit: byte
 */
#define CLI_PARSER_BUF_SIZE					( 128 )

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static cli_status_t cli_send_str			(const uint8_t * const p_str);
static cli_status_t cli_parser_hndl			(void);
static void 		cli_execute_cmd			(const uint8_t * const p_cmd);

// Basic CLI functions
static void cli_help		  	(const uint8_t* attr);
static void cli_reset	   	  	(const uint8_t* attr);
static void cli_fw_version  	(const uint8_t* attr);
static void cli_hw_version  	(const uint8_t* attr);
static void cli_proj_info  		(const uint8_t* attr);
static void cli_unknown	  		(const uint8_t* attr);

#if ( 1 == CLI_CFG_INTRO_STRING_EN )
	static void			cli_send_intro			(void);
#endif


////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 * 		Initialization guard
 */
static bool gb_is_init = false;

/**
 * 		Transmit buffer for printf
 */
static uint8_t gu8_tx_buffer[CLI_CFG_PRINTF_BUF_SIZE] = {0};

/**
 * 		Parser data
 */
static uint8_t gu8_parser_buffer[CLI_PARSER_BUF_SIZE] = {0};

#if __GNUC__
	#pragma GCC diagnostic  push
	#pragma GCC diagnostic  ignored "-Wpointer-sign"	// Ignore pointer sign for basic table
#endif // __GNUC__

	/**
	 * 		Basic CLI commands
	 */
	static cli_cmd_t g_cli_basic_table[] =
	{
		// -----------------------------------------------------------------------------
		// 	name			function			help string
		// -----------------------------------------------------------------------------
		{ 	"help", 		cli_help, 			"Print all commands help" 			},
		{ 	"reset", 		cli_reset, 			"Reset device" 						},
		{ 	"fw_ver", 		cli_fw_version, 	"Print device firmware version" 	},
		{ 	"hw_ver", 		cli_hw_version, 	"Print device hardware version" 	},
		{ 	"proj_info", 	cli_proj_info, 		"Print project informations" 		},
	};

#if __GNUC__
	#pragma GCC diagnostic  pop		// Restore all warnings
#endif // __GNUC__

/**
 * 	Number of basic commands
 */
static const uint32_t gu32_basic_cmd_num_of = ((uint32_t)( sizeof( g_cli_basic_table ) / sizeof( cli_cmd_t )));


////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Send string
*
* @param[in]    p_str	- Pointer to string
* @return       status	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static cli_status_t cli_send_str(const uint8_t * const p_str)
{
	cli_status_t status = eCLI_OK;

	#if ( 1 == CLI_CFG_MUTEX_EN )

		// Mutex obtain
		if ( eCLI_OK == cli_if_aquire_mutex())
		{
			// Write to cli port
			status |= cli_if_transmit( p_str );

			// Release mutex if taken
			status |= cli_if_release_mutex();
		}
		else
		{
			status = eCLI_ERROR;
		}

	#else

		// Write to cli port
		cli_if_transmit( p_str );

	#endif

	return status;
}

static cli_status_t cli_parser_hndl(void)
{
			cli_status_t 	status 	= eCLI_OK;
	static 	uint32_t  		buf_idx	= 0;

	// Take all data from reception buffer
	while ( eCLI_OK == cli_if_receive( &gu8_parser_buffer[buf_idx] ))
	{
		// Check for termination character
		if 	(	( '\r' == gu8_parser_buffer[buf_idx] )
			||	( '\n' == gu8_parser_buffer[buf_idx] ))
		{
			// Replace end termination with NULL
			gu8_parser_buffer[buf_idx] = '\0';

			// Reset buffer index
			buf_idx = 0;

			// Execute command
			cli_execute_cmd( gu8_parser_buffer );

			break;
		}

		// Still size in buffer?
		else if ( buf_idx < ( CLI_PARSER_BUF_SIZE - 2 ))
		{
			buf_idx++;
		}

		// No more size in buffer --> OVERRUN ERROR
		else
		{
			CLI_DBG_PRINT( "CLI: Overrun Error!" )
			CLI_ASSERT( 0 );

			// Reset index
			buf_idx = 0;

			status = eCLI_ERROR;

			break;
		}

		// TODO: Implement protection again infinite loop!
	}

	return status;
}

static void cli_execute_cmd(const uint8_t * const p_cmd)
{
	uint32_t cmd_idx = 0;

	// Basic command check
	for ( cmd_idx = 0; cmd_idx < gu32_basic_cmd_num_of; cmd_idx++ )
	{
		if ( 0 == ( strncmp((const char*) p_cmd, (const char*) g_cli_basic_table[cmd_idx].p_name, strlen((const char*) g_cli_basic_table[cmd_idx].p_name ))))
		{
			g_cli_basic_table[cmd_idx].p_func(NULL);
			break;
		}
	}

	// No command found in side table
	if ( cmd_idx >= ( gu32_basic_cmd_num_of - 1 ))
	{
		cli_unknown(NULL);
	}
}

static void cli_help(const uint8_t* attr)
{

}


static void cli_reset(const uint8_t* attr)
{

}

static void cli_fw_version(const uint8_t* attr)
{

}

static void cli_hw_version(const uint8_t* attr)
{

}

static void cli_proj_info(const uint8_t* attr)
{

}


static void cli_unknown(const uint8_t* attr)
{
	cli_printf( "Unknown command!" );
}

#if ( 1 == CLI_CFG_INTRO_STRING_EN )


	static void	cli_send_intro(void)
	{
		cli_printf( "**************************************************" );
		cli_printf( "\tProject:\t%s", 	CLI_CFG_INTRO_PROJECT_NAME );
		cli_printf( "\tFW ver.:\t%s", 	CLI_CFG_INTRO_SW_VER );
		cli_printf( "\tHW ver.:\t%s", 	CLI_CFG_INTRO_HW_VER );
		cli_printf( " ");
		cli_printf( " See 'help' for further details." );
		cli_printf( "**************************************************" );
		cli_printf( "Ready to take orders..." );
	}

#endif

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup CLI_API
* @{ <!-- BEGIN GROUP -->
*
* 	Following function are part of CLI API.
*/
////////////////////////////////////////////////////////////////////////////////

cli_status_t cli_init(void)
{
	cli_status_t status = eCLI_OK;

	if ( false == gb_is_init )
	{
		// Initialize interface
		status = cli_if_init();

		// Low level driver init error!
		CLI_ASSERT( eCLI_OK == status );

		if ( eCLI_OK == status )
		{
			// Init success
			gb_is_init = true;

			#if ( 1 == CLI_CFG_INTRO_STRING_EN )
				cli_send_intro();
			#endif
		}
	}
	else
	{
		status = eCLI_ERROR_INIT;
	}

	return status;
}


cli_status_t cli_deinit(void)
{
	cli_status_t status = eCLI_OK;

	if ( true == gb_is_init )
	{
		// De-init interface
		status = cli_if_deinit();

		// Low level driver de-init error
		CLI_ASSERT( eCLI_OK == status );

		if ( eCLI_OK == status )
		{
			gb_is_init = false;
		}
	}
	else
	{
		status = eCLI_ERROR_INIT;
	}

	return status;
}


cli_status_t cli_is_init(bool * const p_is_init)
{
	cli_status_t status = eCLI_OK;

	CLI_ASSERT( NULL != p_is_init  );

	if ( NULL != p_is_init )
	{
		*p_is_init = gb_is_init;
	}
	else
	{
		status = eCLI_ERROR;
	}

	return status;
}


cli_status_t cli_hndl(void)
{
	cli_status_t status = eCLI_OK;

	// Cli parser handler
	status = cli_parser_hndl();

	// Data streaming
	// TODO: ...

	return status;
}


cli_status_t cli_printf(char * p_format, ...)
{
	cli_status_t 	status = eCLI_OK;
	va_list 		args;

	if ( true == gb_is_init )
	{
		// Taking args from stack
		va_start(args, p_format);
		vsprintf((char*) gu8_tx_buffer, (const char*) p_format, args);
		va_end(args);

		// Add line termination and print message
		strcat( (char*)gu8_tx_buffer, (char*) CLI_CFG_TERMINATION_STRING );

		// Send string
		status = cli_send_str((const uint8_t*) &gu8_tx_buffer);
	}
	else
	{
		status = eCLI_ERROR_INIT;
	}

	return status;
}

#if ( 1 == CLI_CFG_CHANNEL_EN )

	// TODO: Append channel name...
	cli_status_t cli_printf_ch	(const cli_ch_opt_t ch, char * p_format, ...)
	{
		cli_status_t 	status = eCLI_OK;
		va_list 		args;

		if ( true == gb_is_init )
		{
			// Taking args from stack
			va_start(args, p_format);
			vsprintf((char*) gu8_tx_buffer, (const char*) p_format, args);
			va_end(args);

			// Add line termination and print message
			strcat( (char*)gu8_tx_buffer, (char*) CLI_CFG_TERMINATION_STRING );

			// TODO: Append channel name...

			// Send string
			status = cli_send_str((const uint8_t*) &gu8_tx_buffer);
		}
		else
		{
			status = eCLI_ERROR_INIT;
		}

		return status;
	}
#endif


////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
