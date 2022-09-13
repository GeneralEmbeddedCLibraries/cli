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

#if ( 1 == CLI_CFG_PAR_USE_EN )
	// TODO: Remove
	//#include "middleware/parameters/parameters/src/par.h"
#endif

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * 		Working buffer size
 *
 * 	Unit: byte
 */
#define CLI_PARSER_BUF_SIZE					( 128 )

/**
 * 		Maximum number of user defined tables
 */
#define CLI_USER_CMD_TABLE_MAX_COUNT		( 8 )

/**
 * 	Debug communication port macros
 */
#if ( 1 == CLI_CFG_DEBUG_EN )
	#define CLI_DBG_PRINT(...)				( cli_printf((char*) __VA_ARGS__ ))
#else
	#define CLI_DBG_PRINT(...)				{ ; }

#endif

/**
 * 	Get max
 */
#define CLI_MAX(a,b) 						((a >= b) ? (a) : (b))


////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static cli_status_t cli_send_str					(const uint8_t * const p_str);
static cli_status_t cli_parser_hndl					(void);
static void 		cli_execute_cmd					(const uint8_t * const p_cmd);
static bool 		cli_basic_table_check_and_exe	(const char * p_cmd, const uint32_t cmd_size, const char * attr);
static bool 		cli_user_table_check_and_exe	(const char * p_cmd, const uint32_t cmd_size, const char * attr);
static uint32_t		cli_calc_cmd_size				(const char * p_cmd, const char * attr);

// Basic CLI functions
static void cli_help		  	(const uint8_t * p_attr);
static void cli_reset	   	  	(const uint8_t * p_attr);
static void cli_sw_version  	(const uint8_t * p_attr);
static void cli_hw_version  	(const uint8_t * p_attr);
static void cli_proj_info  		(const uint8_t * p_attr);
static void cli_unknown	  		(const uint8_t * p_attr);

#if ( 1 == CLI_CFG_CHANNEL_EN )
	static void cli_ch_info		(const uint8_t * p_attr);
	static void cli_ch_enable	(const uint8_t * p_attr);
	static void cli_ch_disable	(const uint8_t * p_attr);
#endif

#if ( 1 == CLI_CFG_PAR_USE_EN )
	static void cli_par_print	  	(const uint8_t * p_attr);
	static void cli_par_set		  	(const uint8_t * p_attr);
	static void cli_par_get		  	(const uint8_t * p_attr);
	static void cli_par_def		  	(const uint8_t * p_attr);
	static void cli_par_def_all	  	(const uint8_t * p_attr);
	static void cli_par_store	  	(const uint8_t * p_attr);
#endif

#if ( 1 == CLI_CFG_INTRO_STRING_EN )
	static void			cli_send_intro			(void);
#endif

static bool 			cli_validate_user_table	(const cli_cmd_table_t * const p_cmd_table);
static const char * 	cli_find_char			(const char * const str, const char target_char, const uint32_t size);
static const int32_t 	cli_find_char_pos		(const char * const str, const char target_char, const uint32_t size);

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

/**
 * 		Basic CLI commands
 */
static cli_cmd_t g_cli_basic_table[] =
{
	// ------------------------------------------------------------------------------------------
	// 	name					function				help string
	// ------------------------------------------------------------------------------------------
	{ 	"help", 				cli_help, 				"Print all commands help" 				},
	{ 	"reset", 				cli_reset, 				"Reset device" 							},
	{ 	"sw_ver", 				cli_sw_version, 		"Print device software version" 		},
	{ 	"hw_ver", 				cli_hw_version, 		"Print device hardware version" 		},
	{ 	"proj_info", 			cli_proj_info, 			"Print project informations" 			},

#if ( 1 == CLI_CFG_CHANNEL_EN )
	{	"com_ch_info",			cli_ch_info,			"Show COM channels info"				},
	{	"com_ch_enable",		cli_ch_enable,			"Enable COM channels [chID]"			},
	{	"com_ch_disable",		cli_ch_disable,			"Disable COM channels [chID]"			},
#endif

#if ( 1 == CLI_CFG_PAR_USE_EN )
	{	"par_print",			cli_par_print,		    "Prints parameters"						},
	{	"par_set", 				cli_par_set,			"Set parameter [parID,value]"			},
	{	"par_get",				cli_par_get,		    "Get parameter [parID]"					},
	{	"par_def",				cli_par_def,	    	"Set parameter to default [parID]"		},
	{	"par_def_all",			cli_par_def_all,    	"Set all parameters to default"			},
	{	"par_save",				cli_par_store,	    	"Save parameter to NVM"					},
#endif
};

/**
 *     Number of basic commands
 */
static const uint32_t gu32_basic_cmd_num_of = ((uint32_t)( sizeof( g_cli_basic_table ) / sizeof( cli_cmd_t )));

/**
 * 	Pointer array to user defined tables
 */
static cli_cmd_table_t * gp_cli_user_tables[CLI_USER_CMD_TABLE_MAX_COUNT] = { NULL };

/**
 * 	User defined table counts
 */
static uint32_t	gu32_user_table_count = 0;

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

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Parse input string
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
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
			CLI_DBG_PRINT( "CLI: Overrun Error!" );
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

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Find & execute Command Line Interface command
*
* @note			Entering that function means CR or LF has been received. Input
* 				string is terminated with '\0'.
*
* @note			Attribute to all CLI functions are after space character.
*
* 				Format: >>>cmd_name "attr"
*
*
* 				E.g. with cmd_name="par_get", attr="12"
*
* 					   >>>par_get 12
* 				                 ||--> start of attributes
*								 |
*							empty space
*
* 				will raise function:
*
* 					void cli_par_set(const uint8_t * attr)
* 					{
* 						// attr = "12"
* 					}
*
*
*
* @param[in]	p_cmd	- NULL terminated input string
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_execute_cmd(const uint8_t * const p_cmd)
{
	bool cmd_found = false;

	// Get command options
	const char * attr = cli_find_char((const char * const) p_cmd, ' ', CLI_PARSER_BUF_SIZE );

	// Calculate size of command string
	const uint32_t cmd_size = cli_calc_cmd_size((const char*) p_cmd, (const char*) attr );

	// First check and execute for basic commands
	cmd_found = cli_basic_table_check_and_exe((const char*) p_cmd, cmd_size, attr );

	// Command not founded jet
	if ( false == cmd_found )
	{
		// Check and execute user defined commands
		cmd_found = cli_user_table_check_and_exe((const char*) p_cmd, cmd_size, attr );
	}

	// No command found in any of the tables
	if ( false == cmd_found )
	{
		cli_unknown( NULL );
	}
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Check and execute basic table commands
*
* @note			Commands are divided into simple and combined commands
*
* 				SIMPLE COMMAND: Do not pass additional attributes
*
* 					E.g.: >>>help
*
* 				COMBINED COMMAND: 	Has additional attributes separated by
* 									empty spaces (' ').
*
*					E.g.: >>>par_get 0
*
*
* @param[in]	p_cmd		- NULL terminated input string
* @param[in]	attr		- Additional command attributes
* @return       cmd_found	- Command found flag
*/
////////////////////////////////////////////////////////////////////////////////
static bool cli_basic_table_check_and_exe(const char * p_cmd, const uint32_t cmd_size, const char * attr)
{
	uint32_t 	cmd_idx 		= 0UL;
	char* 		name_str 		= NULL;
	uint32_t	size_to_compare = 0UL;
	bool 		cmd_found 		= false;

	// Walk thru basic commands
	for ( cmd_idx = 0; cmd_idx < gu32_basic_cmd_num_of; cmd_idx++ )
	{
		// Get cmd name
		name_str = g_cli_basic_table[cmd_idx].p_name;

		// String size to compare
		size_to_compare = CLI_MAX( cmd_size, strlen((const char*) name_str));

		// Valid command?
		if ( 0 == ( strncmp((const char*) p_cmd, (const char*) name_str, size_to_compare )))
		{
			// Execute command
			g_cli_basic_table[cmd_idx].p_func((const uint8_t*) attr );

			// Command found
			cmd_found = true;

			break;
		}
	}

	return cmd_found;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Check and execute user table commands
*
* @note			Commands are divided into simple and combined commands
*
* 				SIMPLE COMMAND: Do not pass additional attributes
*
* 					E.g.: >>>help
*
* 				COMBINED COMMAND: 	Has additional attributes separated by
* 									empty spaces (' ').
*
*					E.g.: >>>par_get 0
*
*
* @param[in]	p_cmd		- NULL terminated input string
* @param[in]	attr		- Additional command attributes
* @return       cmd_found	- Command found flag
*/
////////////////////////////////////////////////////////////////////////////////
static bool cli_user_table_check_and_exe(const char * p_cmd, const uint32_t cmd_size, const char * attr)
{
	uint32_t 	cmd_idx 		= 0;
	uint32_t 	table_idx		= 0;
	char* 		name_str 		= NULL;
	uint32_t	size_to_compare = 0UL;
	bool 		cmd_found 		= false;

	// Search thru user defined tables
	for ( table_idx = 0; table_idx < CLI_USER_CMD_TABLE_MAX_COUNT; table_idx++ )
	{
		// Check if registered
		if ( NULL != gp_cli_user_tables[table_idx] )
		{
			// Get number of user commands inside single table
			const uint32_t num_of_user_cmd = gp_cli_user_tables[table_idx]->num_of;

			// Go thru command table
			for ( cmd_idx = 0; cmd_idx < num_of_user_cmd; cmd_idx++ )
			{
				// Get cmd name
				name_str = gp_cli_user_tables[table_idx]->cmd[cmd_idx].p_name;

				// String size to compare
				size_to_compare = CLI_MAX( cmd_size, strlen((const char*) name_str));

				// Valid command?
				if ( 0 == ( strncmp((const char*) p_cmd, (const char*) name_str, size_to_compare )))
				{
					// Execute command
					gp_cli_user_tables[table_idx]->cmd[cmd_idx].p_func((const uint8_t*) attr );

					// Command founded
					cmd_found = true;

					break;
				}
			}
		}

		// When command is found stop searching
		if ( true == cmd_found )
		{
			break;
		}
	}

	return cmd_found;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Calculate inputed command size
*
* @param[in]	p_cmd	- Command string
* @param[in]	attr 	- Rest of the command string
* @return       size 	- Size of the command
*/
////////////////////////////////////////////////////////////////////////////////
static uint32_t	cli_calc_cmd_size(const char * p_cmd, const char * attr)
{
	uint32_t size = 0;

	// Simple command
    if ( NULL == attr )
    {
        size = strlen((const char*) p_cmd );
    }

    // Combined command - must have a empty space
    else
    {
    	size = cli_find_char_pos( p_cmd, ' ', CLI_PARSER_BUF_SIZE );
    }

	return size;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Show help
*
* @param[in]	attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_help(const uint8_t * p_attr)
{
	uint32_t cmd_idx 		= 0;
	uint32_t user_cmd_idx 	= 0;

	// No additional attributes
	if ( NULL == p_attr )
	{
		// Basic command table printout
		for ( cmd_idx = 0; cmd_idx < gu32_basic_cmd_num_of; cmd_idx++ )
		{
			// Get name and help string
			const char * name_str = g_cli_basic_table[cmd_idx].p_name;
			const char * help_str = g_cli_basic_table[cmd_idx].p_help ;

			// Left adjust for 20 chars
			cli_printf( "%-20s%s", name_str, help_str );
		}

		// User defined tables
		for ( cmd_idx = 0; cmd_idx < CLI_USER_CMD_TABLE_MAX_COUNT; cmd_idx++ )
		{
			// Check if registered
			if ( NULL != gp_cli_user_tables[cmd_idx] )
			{
				// Get number of user commands inside single table
				const uint32_t num_of_user_cmd = gp_cli_user_tables[cmd_idx]->num_of;

				// Show help for that table
				for ( user_cmd_idx = 0; user_cmd_idx < num_of_user_cmd; user_cmd_idx++ )
				{
					// Get name and help string
					const char * name_str = gp_cli_user_tables[cmd_idx]->cmd[user_cmd_idx].p_name;
					const char * help_str = gp_cli_user_tables[cmd_idx]->cmd[user_cmd_idx].p_help;

					// Left adjust for 20 chars
					cli_printf( "%-20s%s", name_str, help_str );
				}
			}
		}
	}
	else
	{
		cli_unknown(NULL);
	}
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Reset device
*
* @param[in]	aattr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_reset(const uint8_t * p_attr)
{
	if ( NULL == p_attr )
	{
		cli_printf("OK, reseting device...");
		cli_if_device_reset();
	}
	else
	{
		cli_unknown(NULL);
	}
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Show SW version
*
* @param[in]	attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_sw_version(const uint8_t * p_attr)
{
	if ( NULL == p_attr )
	{
		cli_printf( "OK, SW ver.: %s", 	CLI_CFG_INTRO_SW_VER );
	}
	else
	{
		cli_unknown(NULL);
	}
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Show HW version
*
* @param[in]	attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_hw_version(const uint8_t * p_attr)
{
	if ( NULL == p_attr )
	{
		cli_printf( "OK, HW ver.: %s", 	CLI_CFG_INTRO_HW_VER );
	}
	else
	{
		cli_unknown(NULL);
	}
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Show detailed project informations
*
* @param[in]	attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_proj_info(const uint8_t * p_attr)
{
	if ( NULL == p_attr )
	{
		cli_printf( "OK, Project Info..." );
	}
	else
	{
		cli_unknown(NULL);
	}
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Unknown command received
*
* @param[in]	attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_unknown(const uint8_t * p_attr)
{
	cli_printf( "ERR, Unknown command!" );
}

#if ( 1 == CLI_CFG_CHANNEL_EN )

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief 		Print communication channels details
	*
	* @note			Command format: >>>com_ch_info
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return 		void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void cli_ch_info(const uint8_t * p_attr)
	{
		// TODO: ...
		cli_printf("Needs to be defined...");
	}

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief 		Print communication channels details
	*
	* @note			Command format: >>>com_ch_enable [chID]
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return 		void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void cli_ch_enable(const uint8_t * p_attr)
	{
		// TODO: ...
		cli_printf("Needs to be defined...");
	}

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief 		Print communication channels details
	*
	* @note			Command format: >>>com_ch_disable [chID]
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return 		void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void cli_ch_disable(const uint8_t * p_attr)
	{
		// TODO: ...
		cli_printf("Needs to be defined...");
	}

#endif

#if ( 1 == CLI_CFG_PAR_USE_EN )

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief 		Print parameter details
	*
	* @note			Command format: >>>par_print
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return 		void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void cli_par_print(const uint8_t * p_attr)
	{
		// TODO: ...
		cli_printf("Needs to be defined...");
	}

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief 		Set parameter value
	*
	* @note			Command format: >>>par_set [ID,value]
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return 		void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void cli_par_set(const uint8_t * p_attr)
	{
		// TODO: ...
		cli_printf("Needs to be defined...");
	}

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief 		get parameter value
	*
	* @note			Command format: >>>par_get [ID]
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return 		void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void cli_par_get(const uint8_t * p_attr)
	{
		// TODO: ...
		cli_printf("Needs to be defined...");
	}

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief 		Set parameter value to default
	*
	* @note			Command format: >>>par_def [ID]
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return 		void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void cli_par_def(const uint8_t * p_attr)
	{
		// TODO: ...
		cli_printf("Needs to be defined...");
	}

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief 		Set all parameters value to default
	*
	* @note			Command format: >>>par_def_all
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return 		void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void cli_par_def_all(const uint8_t * p_attr)
	{
		// TODO: ...
		cli_printf("Needs to be defined...");
	}

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief 		Store all persistent parameters to NVM
	*
	* @note			Command format: >>>par_store
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return 		void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void cli_par_store(const uint8_t * p_attr)
	{
		// TODO: ...
		cli_printf("Needs to be defined...");
	}

#endif

#if ( 1 == CLI_CFG_INTRO_STRING_EN )

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief        Send intro string
	*
	* @return       void
	*/
	////////////////////////////////////////////////////////////////////////////////
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
/*!
* @brief        Validate user defined table
*
* @param[in]	p_cmd_table		- User defined cli command table
* @return		valid			- Validation flag
*/
////////////////////////////////////////////////////////////////////////////////
static bool cli_validate_user_table(const cli_cmd_table_t * const p_cmd_table)
{
			bool 		valid = true;
	const 	uint32_t	num_of = p_cmd_table->num_of;

	// Check max. num of commands
	if ( num_of <= CLI_CFG_MAX_NUM_OF_COMMANDS )
	{
		// Roll thru all commands
		for (uint32_t cmd_num = 0; cmd_num < num_of; cmd_num++)
		{
			// Get name, func and help
			const char * name 	= p_cmd_table->cmd[cmd_num].p_name;
			const char * help 	= p_cmd_table->cmd[cmd_num].p_help;
			pf_cli_cmd func 	= p_cmd_table->cmd[cmd_num].p_func;

			// Missing definitions?
			if 	(	( NULL == name )
				||	( NULL == help )
				||	( NULL == func ))
			{
				valid = false;
				break;
			}
		}
	}
	else
	{
		valid = false;
	}

	return valid;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Find character inside string
*
* @note			Returned sub-string is string from target_char on and not with
* 				it!
*
* 				E.g: 	str 			= "Hello World"
* 						target_char 	= ' '
* 						sub_str 		= "World"
*
* @note			If target string is last char in string it will return NULL!
*
* @param[in]	str				- Search string
* @param[in]	target_char		- Character to find
* @param[in]	size			- Total size to search for
* @return		sub_str			- Sub-string from target char on
*/
////////////////////////////////////////////////////////////////////////////////
static const char * cli_find_char(const char * const str, const char target_char, const uint32_t size)
{
	char * 	sub_str = NULL;
	uint32_t 	ch 		= 0;

	for ( ch = 0; ch < size; ch++)
	{
		// End string reached
		if ( '\0' == str[ch] )
		{
			break;
		}

		// Target char found
		else if ( target_char == str[ch] )
		{
			// Return sub-string wihtout target char
			sub_str = (char*)( str + ch + 1 );

			break;
		}
		else
		{
			// No actions...
		}
	}

	return sub_str;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Find character inside string
*
* @note			E.g: 	str 			= "Hello World"
* 						target_char 	= ' '
* 						pos				= 6
*
* @note			If "target_char" is not found it returns -1!
*
* @param[in]	str				- Search string
* @param[in]	target_char		- Character to find
* @param[in]	size			- Total size to search for
* @return		pos				- Position of target char inside string
*/
////////////////////////////////////////////////////////////////////////////////
static const int32_t cli_find_char_pos(const char * const str, const char target_char, const uint32_t size)
{
	int32_t 	pos = -1;
	uint32_t 	ch 	= 0;

	for ( ch = 0; ch < size; ch++)
	{
		// End string reached
		if ( '\0' == str[ch] )
		{
			break;
		}

		// Target char found
		else if ( target_char == str[ch] )
		{
			pos = ch;
			break;
		}
		else
		{
			// No actions...
		}
	}

	return pos;
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

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Initialize command line interface
*
* @return       status	- Status of initialization
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

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        De-Initialize command line interface
*
* @return       status	- Status of de-initialization
*/
////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Get initialization flag
*
* @param[out]	p_is_init	- Initialization flag
* @return       status		- Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
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

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Main Command Line Interface handler
*
* @return       status	- Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_hndl(void)
{
	cli_status_t status = eCLI_OK;

	// Cli parser handler
	status = cli_parser_hndl();

	// Data streaming
	// TODO: ...

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Print formated string
*
* @param[in]	p_format	- Formated string
* @return       status		- Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_printf(char * p_format, ...)
{
	cli_status_t 	status = eCLI_OK;
	va_list 		args;

	CLI_ASSERT( NULL != p_format );

	if ( true == gb_is_init )
	{
		if ( NULL != p_format )
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
			status = eCLI_ERROR;
		}
	}
	else
	{
		status = eCLI_ERROR_INIT;
	}

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Register user defined CLI command table
*
* @param[in]	p_cmd_table	- Pointer to user cmd table
* @return       status		- Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_register_cmd_table(const cli_cmd_table_t * const p_cmd_table)
{
	cli_status_t status = eCLI_OK;

	CLI_ASSERT( NULL != p_cmd_table );

	if ( true == gb_is_init )
	{
		if ( NULL != p_cmd_table )
		{
			// Is there any space left for user tables?
			if ( gu32_user_table_count < CLI_USER_CMD_TABLE_MAX_COUNT )
			{
				// User table defined OK
				if ( true == cli_validate_user_table( p_cmd_table ))
				{
					// Store
					gp_cli_user_tables[gu32_user_table_count] = (cli_cmd_table_t *) p_cmd_table;
					gu32_user_table_count++;
				}

				// User table definition error
				else
				{
					CLI_DBG_PRINT( "CLI ERROR: Invalid definition of user table!");
					CLI_ASSERT( 0 );
					status = eCLI_ERROR;
				}
			}
			else
			{
				status = eCLI_ERROR;
			}
		}
		else
		{
			status = eCLI_ERROR;
		}
	}
	else
	{
		status = eCLI_ERROR_INIT;
	}

	return status;
}

#if ( 1 == CLI_CFG_CHANNEL_EN )

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief        Print formated string within debug channel
	*
	* @param[in]	ch			- Debug channel
	* @param[in]	p_format	- Formated string
	* @return       status		- Status of initialization
	*/
	////////////////////////////////////////////////////////////////////////////////
	cli_status_t cli_printf_ch(const cli_ch_opt_t ch, char * p_format, ...)
	{
		cli_status_t 	status = eCLI_OK;
		va_list 		args;

		CLI_ASSERT( NULL != p_format );

		if ( true == gb_is_init )
		{
			if ( NULL != p_format )
			{
				// Taking args from stack
				va_start(args, p_format);
				vsprintf((char*) gu8_tx_buffer, (const char*) p_format, args);
				va_end(args);

				// TODO: Append debug channel name...

				// Add line termination and print message
				strcat( (char*)gu8_tx_buffer, (char*) CLI_CFG_TERMINATION_STRING );

				// Send string
				status = cli_send_str((const uint8_t*) &gu8_tx_buffer);
			}
			else
			{
				status = eCLI_ERROR;
			}
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
