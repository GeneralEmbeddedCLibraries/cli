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


////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static cli_status_t cli_send_str(const uint8_t * const p_str);

// Basic CLI functions
static void cli_help		  	(const uint8_t* attr);
static void cli_reset	   	  	(const uint8_t* attr);
static void cli_fw_version  	(const uint8_t* attr);
static void cli_hw_version  	(const uint8_t* attr);
static void cli_proj_info  		(const uint8_t* attr);
static void cli_unknown	  		(const uint8_t* attr);



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

}

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

		if ( eCLI_OK == status )
		{
			gb_is_init = true;
		}
	}
	else
	{
		status = eCLI_ERROR_INIT;
	}

	CLI_ASSERT( eCLI_OK == status );

	return status;
}


cli_status_t cli_deinit(void)
{
	cli_status_t status = eCLI_OK;

	if ( true == gb_is_init )
	{
		status = cli_if_deinit();

		if ( eCLI_OK == status )
		{
			gb_is_init = false;
		}
	}
	else
	{
		status = eCLI_ERROR_INIT;
	}

	CLI_ASSERT( eCLI_OK == status );

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


	return status;
}


cli_status_t cli_printf(const uint8_t * p_format, ...)
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
	cli_status_t cli_printf_ch	(const cli_ch_opt_t ch, const uint8_t * p_format, ...)
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
