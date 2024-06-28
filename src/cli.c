// Copyright (c) 2024 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      cli.c
*@brief     Command Line Interface
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      28.06.2024
*@version   V2.0.0
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
#include <assert.h>

#include "cli.h"
#include "cli_util.h"
#include "cli_nvm.h"
#include "cli_par.h"
#include "cli_osci.h"

#include "../../cli_cfg.h"
#include "../../cli_if.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

/**
 * 	Get max
 */
#define CLI_MAX(a,b) 						((a >= b) ? (a) : (b))

/**
 *  CLI Command Table
 */
typedef struct
{
    cli_cmd_t * p_cmd;      /**<Command table */
    uint32_t    num_of;     /**<Number of commands */
} cli_cmd_table_t;

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
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
static void cli_ch_info  		(const uint8_t * p_attr);
static void cli_ch_en  			(const uint8_t * p_attr);

#if ( 1 == CLI_CFG_INTRO_STRING_EN )
static void	cli_send_intro		(const uint8_t * p_attr);
#endif

static bool             cli_validate_user_table (const cli_cmd_t * const p_cmd_table, const uint8_t num_of_cmd);
static const char * 	cli_find_char			(const char * const str, const char target_char, const uint32_t size);
static int32_t 	        cli_find_char_pos		(const char * const str, const char target_char, const uint32_t size);

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

/**
 * 		Initialization guard
 */
static bool gb_is_init = false;

/**
 * 		Basic CLI commands
 */
static cli_cmd_t g_cli_basic_table[] =
{
	// ------------------------------------------------------------------------------------------------------
	// 	name					function				help string
	// ------------------------------------------------------------------------------------------------------
	{ 	"help", 				cli_help, 				"Print help message" 							    },

#if ( 1 == CLI_CFG_INTRO_STRING_EN )
	{ 	"intro", 				cli_send_intro,         "Print intro message" 							    },
#endif

	{ 	"reset", 				cli_reset, 				"Reset device" 										},
	{ 	"sw_ver", 				cli_sw_version, 		"Print device software version" 					},
	{ 	"hw_ver", 				cli_hw_version, 		"Print device hardware version" 					},
	{ 	"proj_info", 			cli_proj_info, 			"Print project informations" 						},
	{ 	"ch_info", 				cli_ch_info, 			"Print COM channel informations" 					},
	{ 	"ch_en", 				cli_ch_en, 				"Enable/disable COM channel. Args: [chEnum][en]"    },
};

/**
 *     Number of basic commands
 */
static const uint32_t gu32_basic_cmd_num_of = ((uint32_t)( sizeof( g_cli_basic_table ) / sizeof( cli_cmd_t )));

/**
 * 	References to user defined CLI tables
 */
static cli_cmd_table_t g_cli_user_tables[CLI_CFG_MAX_NUM_OF_USER_TABLES] = { NULL };

/**
 * 	User defined table counts
 */
static uint32_t	gu32_user_table_count = 0;

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Parse input string
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static cli_status_t cli_parser_hndl(void)
{
			cli_status_t 	status      = eCLI_OK;
	static 	uint32_t  		buf_idx 	= 0;
            uint32_t        escape_cnt  = 0;
     static uint8_t         rx_buffer[CLI_CFG_RX_BUF_SIZE] = {0};

	// Take all data from reception buffer
	while   (   ( eCLI_OK == cli_if_receive( &rx_buffer[buf_idx] ))
            &&  ( escape_cnt < 10000UL ))
	{
		// Find termination character
        char * p_term_str_start = strstr((char*) &rx_buffer, (char*) CLI_CFG_TERMINATION_STRING );
        
        // Termination string found
        if ( NULL != p_term_str_start )
		{
			// Replace all termination character with NULL
            memset((char*) p_term_str_start, 0, strlen( CLI_CFG_TERMINATION_STRING ));

			// Reset buffer index
			buf_idx = 0;

			// Execute command
			cli_execute_cmd( rx_buffer );

			break;
		}

		// Still size in buffer?
		else if ( buf_idx < ( CLI_CFG_RX_BUF_SIZE - 2 ))
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

		// Increment escape count in order to prevent infinite loop
        escape_cnt++;
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
* @param[in]	p_cmd	- NULL terminated input string
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_execute_cmd(const uint8_t * const p_cmd)
{
	bool cmd_found = false;

	// Get command options
	const char * attr = cli_find_char((const char * const) p_cmd, ' ', CLI_CFG_RX_BUF_SIZE );

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
		cli_util_unknown_cmd_rsp();
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
	for ( table_idx = 0; table_idx < CLI_CFG_MAX_NUM_OF_USER_TABLES; table_idx++ )
	{
        // Get number of user commands inside single table
        const uint32_t num_of_user_cmd = g_cli_user_tables[table_idx].num_of;

        // Go thru command table
        for ( cmd_idx = 0; cmd_idx < num_of_user_cmd; cmd_idx++ )
        {
            // Get cmd name
            name_str = g_cli_user_tables[table_idx].p_cmd[cmd_idx].p_name;

            // String size to compare
            size_to_compare = CLI_MAX( cmd_size, strlen((const char*) name_str));

            // Valid command?
            if ( 0 == ( strncmp((const char*) p_cmd, (const char*) name_str, size_to_compare )))
            {
                // Execute command
                g_cli_user_tables[table_idx].p_cmd[cmd_idx].p_func((const uint8_t*) attr );

                // Command founded
                cmd_found = true;

                break;
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
    	size = cli_find_char_pos( p_cmd, ' ', CLI_CFG_RX_BUF_SIZE );
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
		cli_printf( " " );
		cli_printf( "    List of device commands" );
		cli_printf( "--------------------------------------------------------" );

		// Basic command table printout
		for ( cmd_idx = 0; cmd_idx < gu32_basic_cmd_num_of; cmd_idx++ )
		{
			// Get name and help string
			const char * name_str = g_cli_basic_table[cmd_idx].p_name;
			const char * help_str = g_cli_basic_table[cmd_idx].p_help ;

			// Left adjust for 25 chars
			cli_printf( "%-25s%s", name_str, help_str );
		}

		// User defined tables
		for ( cmd_idx = 0; cmd_idx < CLI_CFG_MAX_NUM_OF_USER_TABLES; cmd_idx++ )
		{
            // Are there any commands
            if ( g_cli_user_tables[cmd_idx].num_of > 0U )
            {
                // Print separator between user commands
                cli_printf( "--------------------------------------------------------" );

                // Show help for that table
                for ( user_cmd_idx = 0; user_cmd_idx < g_cli_user_tables[cmd_idx].num_of; user_cmd_idx++ )
                {
                    // Get name and help string
                    const char * name_str = g_cli_user_tables[cmd_idx].p_cmd[user_cmd_idx].p_name;
                    const char * help_str = g_cli_user_tables[cmd_idx].p_cmd[user_cmd_idx].p_help;

                    // Left adjust for 25 chars
                    cli_printf( "%-25s%s", name_str, help_str );
                }
            }
		}

		// Print separator at the end
		cli_printf( "--------------------------------------------------------" );
	}
	else
	{
		cli_util_unknown_cmd_rsp();
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
		cli_printf("OK, Reseting device...");
		cli_if_device_reset();
	}
	else
	{
		cli_util_unknown_cmd_rsp();
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
		#if ( 1 == CLI_CFG_INTRO_STRING_EN )
			cli_printf( "OK, %s", CLI_CFG_INTRO_SW_VER );
		#else
			cli_printf( "WAR, Not used..." );
		#endif
	}
	else
	{
		cli_util_unknown_cmd_rsp();
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
		#if ( 1 == CLI_CFG_INTRO_STRING_EN )
			cli_printf( "OK, %s", CLI_CFG_INTRO_HW_VER );
		#else
			cli_printf( "WAR, Not used..." );
		#endif
	}
	else
	{
		cli_util_unknown_cmd_rsp();
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
		#if ( 1 == CLI_CFG_INTRO_STRING_EN )
			cli_printf( "OK, %s", CLI_CFG_INTRO_PROJ_INFO );
		#else
			cli_printf( "WAR, Not used..." );
		#endif
	}
	else
	{
		cli_util_unknown_cmd_rsp();
	}
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Show communication channel info
*
* @note			Command format: >>>cli_ch_info
*
* @param[in]	attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_ch_info(const uint8_t * p_attr)
{
	if ( NULL == p_attr )
	{
		cli_printf( "--------------------------------------------------------" );
		cli_printf( "        Communication Channels Info" );
		cli_printf( "--------------------------------------------------------" );
		cli_printf( "  %-8s%-20s%s", "chEnum", "Name", "State" );
		cli_printf( " ------------------------------------" );

		for (uint8_t ch = 0; ch < eCLI_CH_NUM_OF; ch++)
		{
			cli_printf("    %02d    %-20s%s", ch, cli_cfg_get_ch_name(ch), (cli_cfg_get_ch_en(ch) ? ( "Enable" ) : ( "Disable" )));
		}

		cli_printf( "--------------------------------------------------------" );
	}
	else
	{
		cli_util_unknown_cmd_rsp();
	}
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Enable/Disable communication channel
*
*
* @note			Command format: >>>cli_ch_en [chEnum,en]
*
* 				E.g.:	>>>cli_ch_en 0, 1	// Disable "WARNING" channel
* 				E.g.:	>>>cli_ch_en 1, 0	// Enable "ERROR" channel
*
* @param[in]	attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_ch_en(const uint8_t * p_attr)
{
	uint32_t ch;
	uint32_t en;

	if ( NULL != p_attr )
	{
		// Check input command
		if ( 2U == sscanf((const char*) p_attr, "%u,%u", (unsigned int*)&ch, (unsigned int*)&en ))
		{
			if ( ch < eCLI_CH_NUM_OF )
			{
				cli_cfg_set_ch_en( ch, en );
				cli_printf( "OK, %s channel %s", (( true == en ) ? ("Enabling") : ("Disabling")), cli_cfg_get_ch_name( ch ));
			}
			else
			{
				cli_printf( "ERR, Invalid chEnum!" );
			}
		}
		else
		{
			cli_util_unknown_cmd_rsp();
		}
	}
	else
	{
		cli_util_unknown_cmd_rsp();
	}
}

#if ( 1 == CLI_CFG_INTRO_STRING_EN )

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief        Send intro string
	*
	* @return       void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void	cli_send_intro(const uint8_t * p_attr)
	{
	    (void) p_attr;

		cli_printf( " " );
		cli_printf( "********************************************************" );
		cli_printf( "        %s", 	CLI_CFG_INTRO_PROJECT_NAME );
		cli_printf( "********************************************************" );
		cli_printf( " %s", 	CLI_CFG_INTRO_SW_VER );
		cli_printf( " %s", 	CLI_CFG_INTRO_HW_VER );
		cli_printf( " ");
		cli_printf( " Enter 'help' to display supported commands" );
		cli_printf( "********************************************************" );
		cli_printf( "Ready to take orders..." );
	}

#endif

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Validate user defined table
*
* @param[in]	p_cmd_table		- User defined cli command table
* @param[in]	num_of_cmd		- Number of commands
* @return		valid			- Validation flag
*/
////////////////////////////////////////////////////////////////////////////////
static bool cli_validate_user_table(const cli_cmd_t * const p_cmd_table, const uint8_t num_of_cmd)
{
    bool valid = true;

    // Roll thru all commands
    for (uint32_t cmd = 0; cmd < num_of_cmd; cmd++)
    {
        // Missing definitions?
        if 	(	( NULL == p_cmd_table[cmd].p_name )
            ||	( NULL == p_cmd_table[cmd].p_help )
            ||	( NULL == p_cmd_table[cmd].p_func ))
        {
            valid = false;
            break;
        }
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
	char *      sub_str = NULL;
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
			// Return sub-string without target char
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
static int32_t cli_find_char_pos(const char * const str, const char target_char, const uint32_t size)
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

		// Initialize cli sub-components
        #if ( 1 == CLI_CFG_PAR_USE_EN )
		    status |= cli_par_init();

            #if ( 1 == CLI_CFG_PAR_OSCI_EN )
                status |= cli_osci_init();
            #endif
        #endif

		// Low level driver init error!
		CLI_ASSERT( eCLI_OK == status );

		if ( eCLI_OK == status )
		{
			// Init success
			gb_is_init = true;

			#if ( 1 == CLI_CFG_INTRO_STRING_EN )
				cli_send_intro( NULL );
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
* @return       status      - Status of operation
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
* @note     Shall not be used in ISR!
*
* @return       status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_hndl(void)
{
	cli_status_t status = eCLI_OK;

	// Cli parser handler
	status = cli_parser_hndl();

	#if ( 1 == CLI_CFG_PAR_USE_EN )

        // Data streaming
        status |= cli_par_hndl();

	#endif

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Send string
*
* @note     Shall not be used in ISR!
*
* @param[in]    p_str   - Pointer to string
* @return       status  - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_send_str(const uint8_t * const p_str)
{
    cli_status_t status = eCLI_OK;

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

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Print formated string
*
* @note     Shall not be used in ISR!
*
* @param[in]	p_format	- Formated string
* @return       status      - Status of operation
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
		    // Get pointer to Tx buffer
		    uint8_t * p_tx_buf = cli_util_get_tx_buf();

            // Mutex obtain
            if ( eCLI_OK == cli_if_aquire_mutex())
            {
                // Taking args from stack
                va_start(args, p_format);
                vsprintf((char*) p_tx_buf, (const char*) p_format, args);
                va_end(args);

                // Send string
                status = cli_send_str((const uint8_t*) p_tx_buf );
                status |= cli_send_str((const uint8_t*) CLI_CFG_TERMINATION_STRING );

                // Release mutex
                cli_if_release_mutex();
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

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Print formated string within debug channel
*
* @note     Shall not be used in ISR!
*
* @param[in]	ch			- Debug channel
* @param[in]	p_format	- Formated string
* @return       status		- Status of operation
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
			// Is channel enabled
			if ( true == cli_cfg_get_ch_en( ch ))
			{
			    // Get pointer to Tx buffer
			    uint8_t * p_tx_buf = cli_util_get_tx_buf();

			    // Mutex obtain
			    if ( eCLI_OK == cli_if_aquire_mutex())
			    {
                    // Taking args from stack
                    va_start(args, p_format);
                    vsprintf((char*) p_tx_buf, (const char*) p_format, args);
                    va_end(args);

                    // Send channel name
                    status |= cli_send_str((const uint8_t*) cli_cfg_get_ch_name( ch ));
                    status |= cli_send_str((const uint8_t*) ": " );

                    // Send string
                    status |= cli_send_str((const uint8_t*) p_tx_buf );
                    status |= cli_send_str((const uint8_t*) CLI_CFG_TERMINATION_STRING );

                    // Release mutex
                    cli_if_release_mutex();
			    }
			    else
			    {
			        status = eCLI_ERROR;
			    }
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

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Register user defined CLI command table
*
* @note     Shall not be used in ISR!
*
* @param[in]	p_cmd_table	- Pointer to user cmd table
* @param[in]	num_of_cmd	- Number of commands
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_register_cmd_table(const cli_cmd_t * const p_cmd_table, const uint8_t num_of_cmd)
{
	cli_status_t status = eCLI_OK;

	CLI_ASSERT( NULL != p_cmd_table );

    // Mutex obtain
    if ( eCLI_OK == cli_if_aquire_mutex())
    {
        if ( NULL != p_cmd_table )
        {
            // Is there any space left for user tables?
            if ( gu32_user_table_count < CLI_CFG_MAX_NUM_OF_USER_TABLES )
            {
                // User table defined OK
                if ( true == cli_validate_user_table( p_cmd_table, num_of_cmd ))
                {
                    // Store
                    g_cli_user_tables[gu32_user_table_count].p_cmd    = (cli_cmd_t*) p_cmd_table;
                    g_cli_user_tables[gu32_user_table_count].num_of   = num_of_cmd;
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

        // Release mutex
        cli_if_release_mutex();
    }
    else
    {
        status = eCLI_ERROR;
    }

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
cli_status_t cli_osci_hndl(void)
{
#if ( 1 == CLI_CFG_PAR_OSCI_EN )
    cli_osci_samp_hndl();
#endif

    return eCLI_OK;
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
