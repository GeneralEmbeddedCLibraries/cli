// Copyright (c) 2022 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      cli.c
*@brief     Command Line Interface
*@author    Ziga Miklosic
*@date      08.12.2022
*@version   V1.2.0
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
#include "cli_nvm.h"
#include "../../cli_cfg.h"
#include "../../cli_if.h"

#if ( 1 == CLI_CFG_PAR_USE_EN )
	#include "middleware/parameters/parameters/src/par.h"

	/**
	 * 	Compatibility check with Parameters module
	 *
	 * 	Support version V2.0.x
	 */
	_Static_assert( 2 == PAR_VER_MAJOR );
	_Static_assert( 0 <= PAR_VER_MINOR );

#endif

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

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
static void cli_ch_info  		(const uint8_t * p_attr);
static void cli_ch_en  			(const uint8_t * p_attr);
static void cli_unknown	  		(const uint8_t * p_attr);

#if ( 1 == CLI_CFG_PAR_USE_EN )
    static void         cli_par_print_info_legacy   (const par_cfg_t * const p_par_cfg, const uint32_t par_val);
    static void         cli_par_print_info          (const par_cfg_t * const p_par_cfg, const uint32_t par_val);
	static void 		cli_par_print               (const uint8_t * p_attr);
	static void 		cli_par_set                 (const uint8_t * p_attr);
	static void 		cli_par_get                 (const uint8_t * p_attr);
	static void 		cli_par_def                 (const uint8_t * p_attr);
	static void 		cli_par_def_all	  		    (const uint8_t * p_attr);
	static void 		cli_par_store	  		    (const uint8_t * p_attr);
	static void 		cli_status_start  		    (const uint8_t * p_attr);
	static void 		cli_status_stop  		    (const uint8_t * p_attr);
	static void 		cli_status_des  		    (const uint8_t * p_attr);
	static void 		cli_status_rate  		    (const uint8_t * p_attr);
	static void 		cli_status_info  		    (const uint8_t * p_attr);
	static float32_t 	cli_par_val_to_float	    (const par_type_list_t par_type, const void * p_val);
	static void			cli_par_live_watch_hndl	    (void);
	static void 		cli_par_group_print		    (const par_num_t par_num);

	#if ( 1 == CLI_CFG_DEBUG_EN )
		static void cli_par_store_reset(const uint8_t * p_attr);
	#endif

    #if ( 1 == CLI_CFG_STREAM_NVM_EN )
        static void cli_status_save(const uint8_t * p_attr);
    #endif
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
static uint8_t gu8_tx_buffer[CLI_CFG_TX_BUF_SIZE] = {0};

/**
 * 		Reception buffer for parsing purposes
 */
static uint8_t gu8_rx_buffer[CLI_CFG_RX_BUF_SIZE] = {0};

/**
 * 		Basic CLI commands
 */
static cli_cmd_t g_cli_basic_table[] =
{
	// ------------------------------------------------------------------------------------------------------
	// 	name					function				help string
	// ------------------------------------------------------------------------------------------------------
	{ 	"help", 				cli_help, 				"Print all commands help" 							},
	{ 	"reset", 				cli_reset, 				"Reset device" 										},
	{ 	"sw_ver", 				cli_sw_version, 		"Print device software version" 					},
	{ 	"hw_ver", 				cli_hw_version, 		"Print device hardware version" 					},
	{ 	"proj_info", 			cli_proj_info, 			"Print project informations" 						},
	{ 	"ch_info", 				cli_ch_info, 			"Print COM channel informations" 					},
	{ 	"ch_en", 				cli_ch_en, 				"Enable/disable COM channel [chEnum][en]" 			},

#if ( 1 == CLI_CFG_PAR_USE_EN )
	{	"par_print",			cli_par_print,		    "Prints parameters"									},
	{	"par_set", 				cli_par_set,			"Set parameter [parID,value]"						},
	{	"par_get",				cli_par_get,		    "Get parameter [parID]"								},
	{	"par_def",				cli_par_def,	    	"Set parameter to default [parID]"					},
	{	"par_def_all",			cli_par_def_all,    	"Set all parameters to default"						},
	{	"par_save",				cli_par_store,	    	"Save parameter to NVM"								},
	#if (( 1 == CLI_CFG_DEBUG_EN ) && ( 1 == PAR_CFG_NVM_EN ))
		{	"par_save_clean",		cli_par_store_reset,	"Clean saved parameters space in NVM"           },
	#endif
	{	"status_start", 		cli_status_start,		"Start data streaming"  			 				},
	{	"status_stop", 			cli_status_stop,		"Stop data streaming"	  			 				},
	{	"status_des",			cli_status_des,			"Status description"	  			 				},
	{	"status_rate",			cli_status_rate,		"Change data streaming period [miliseconds]"        },
	{	"status_info",			cli_status_info,		"Get streaming configuration info"                  },
    #if ( 1 == CLI_CFG_STREAM_NVM_EN )
        {	"status_save",			cli_status_save,		"Save streaming into to NVM"                    },
    #endif
#endif
};

/**
 *     Number of basic commands
 */
static const uint32_t gu32_basic_cmd_num_of = ((uint32_t)( sizeof( g_cli_basic_table ) / sizeof( cli_cmd_t )));

/**
 * 	Pointer array to user defined tables
 */
static cli_cmd_table_t * gp_cli_user_tables[CLI_CFG_MAX_NUM_OF_USER_TABLES] = { NULL };

/**
 * 	User defined table counts
 */
static uint32_t	gu32_user_table_count = 0;

#if ( 1 == CLI_CFG_PAR_USE_EN )

	/**
	 * 		Live watch data
	 *
	 * 	Inside "par_list" there is parameter enumeration number not parameter ID!
	 */
	static cli_live_watch_t g_cli_live_watch = { .period = CLI_CFG_DEF_STREAM_PER_MS, .period_cnt = (uint32_t)(CLI_CFG_DEF_STREAM_PER_MS/CLI_CFG_HNDL_PERIOD_MS), .active = false, .num_of = 0, .par_list = {0} };

#endif

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
		status = cli_if_transmit( p_str );

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
			cli_status_t 	status      = eCLI_OK;
	static 	uint32_t  		buf_idx 	= 0;
            uint32_t        escape_cnt  = 0;

	// Take all data from reception buffer
	while   (   ( eCLI_OK == cli_if_receive( &gu8_rx_buffer[buf_idx] ))
            &&  ( escape_cnt < 10000UL ))
	{
		// Find termination character
        char * p_term_str_start = strstr((char*) &gu8_rx_buffer, (char*) CLI_CFG_TERMINATION_STRING );
        
        // Termination string found
        if ( NULL != p_term_str_start )
		{
			// Replace all termination character with NULL
            memset((char*) p_term_str_start, 0, strlen( CLI_CFG_TERMINATION_STRING ));

			// Reset buffer index
			buf_idx = 0;

			// Execute command
			cli_execute_cmd( gu8_rx_buffer );

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
	for ( table_idx = 0; table_idx < CLI_CFG_MAX_NUM_OF_USER_TABLES; table_idx++ )
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
		cli_printf( "    List of device commands " );
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
			// Check if registered
			if ( NULL != gp_cli_user_tables[cmd_idx] )
			{
				// Get number of user commands inside single table
				const uint32_t num_of_user_cmd = gp_cli_user_tables[cmd_idx]->num_of;

				// Print separator between user commands
				cli_printf( "--------------------------------------------------------" );

				// Show help for that table
				for ( user_cmd_idx = 0; user_cmd_idx < num_of_user_cmd; user_cmd_idx++ )
				{
					// Get name and help string
					const char * name_str = gp_cli_user_tables[cmd_idx]->cmd[user_cmd_idx].p_name;
					const char * help_str = gp_cli_user_tables[cmd_idx]->cmd[user_cmd_idx].p_help;

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
		cli_printf("OK, Reseting device...");
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
		#if ( 1 == CLI_CFG_INTRO_STRING_EN )
			cli_printf( "OK, %s", CLI_CFG_INTRO_SW_VER );
		#else
			cli_printf( "WAR, Not used..." );
		#endif
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
		#if ( 1 == CLI_CFG_INTRO_STRING_EN )
			cli_printf( "OK, %s", CLI_CFG_INTRO_HW_VER );
		#else
			cli_printf( "WAR, Not used..." );
		#endif
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
		#if ( 1 == CLI_CFG_INTRO_STRING_EN )
			cli_printf( "OK, %s", CLI_CFG_INTRO_PROJ_INFO );
		#else
			cli_printf( "WAR, Not used..." );
		#endif
	}
	else
	{
		cli_unknown(NULL);
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
		cli_unknown(NULL);
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
			cli_unknown(NULL);
		}
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

#if ( 1 == CLI_CFG_PAR_USE_EN )

    ////////////////////////////////////////////////////////////////////////////////
    /*!
    * @brief        Print parameter information in legacy format
    *
    * @param[in]	p_par_cfg   - Pointer to paramter configurations
    * @param[in]	par_val     - Parameter value
    * @return       void
    */
    ////////////////////////////////////////////////////////////////////////////////
    static void cli_par_print_info_legacy(const par_cfg_t * const p_par_cfg, const uint32_t par_val)
    {
        // Parameter has description
        if ( NULL != p_par_cfg->desc )
        {
            // Par info response
            cli_printf( "%u, %s, %g, %g, %g, %g, %s,f,4",
                    (int) p_par_cfg->id,
                    p_par_cfg->name,
                    cli_par_val_to_float( p_par_cfg->type, &par_val ),
                    cli_par_val_to_float( p_par_cfg->type, &( p_par_cfg->def.u32 )),
                    cli_par_val_to_float( p_par_cfg->type, &( p_par_cfg->min.u32 )),
                    cli_par_val_to_float( p_par_cfg->type, &( p_par_cfg->max.u32 )),
                    p_par_cfg->desc );
        }
        else
        {
            // Par info response
            cli_printf("%u, %s, %g, %g, %g, %g, ,f,4",
                    (int) p_par_cfg->id,
                    p_par_cfg->name,
                    cli_par_val_to_float( p_par_cfg->type, &par_val ),
                    cli_par_val_to_float( p_par_cfg->type, &( p_par_cfg->def.u32 )),
                    cli_par_val_to_float( p_par_cfg->type, &( p_par_cfg->min.u32 )),
                    cli_par_val_to_float( p_par_cfg->type, &( p_par_cfg->max.u32 )));
        }
    }

    ////////////////////////////////////////////////////////////////////////////////
    /*!
    * @brief        Print parameter information 
    *
    * @param[in]	p_par_cfg   - Pointer to paramter configurations
    * @param[in]	par_val     - Parameter value
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
        cli_printf( "%u, %s, %g, %g, %g, %g, %s, %s",
                (int) p_par_cfg->id,
                p_par_cfg->name,
                cli_par_val_to_float( p_par_cfg->type, &par_val ),
                cli_par_val_to_float( p_par_cfg->type, &( p_par_cfg->def.u32 )),
                cli_par_val_to_float( p_par_cfg->type, &( p_par_cfg->min.u32 )),
                cli_par_val_to_float( p_par_cfg->type, &( p_par_cfg->max.u32 )),
                unit_str,
                desc_str );
    }

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
        par_cfg_t 	par_cfg 	= { 0 };
        uint32_t 	par_num		= 0UL;
        uint32_t	par_val		= 0UL;

		if ( NULL == p_attr )
		{
			// Send header
			cli_printf(";Par.ID, Par.Name, Par.value, Par.def, Par.Min, Par.Max, Comment, Type, Access level");
            
            #if ( 1 == CLI_CFG_LEGACY_EN )
    			cli_printf( ":PARAMETER ACCESS LEGEND" );
    			cli_printf( ":RO - Read Only" );
    			cli_printf( ":RW - Read Write" );
    			cli_printf( ": " );
            #endif

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
                #if ( 1 == CLI_CFG_LEGACY_EN )
                    cli_par_print_info_legacy((const par_cfg_t*) &par_cfg, par_val ); 
                #else
                    cli_par_print_info((const par_cfg_t*) &par_cfg, par_val ); 
                #endif
			}

			// Table termination string
			cli_printf(";END");
		}
		else
		{
			cli_unknown(NULL);
		}
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
		uint32_t 	   	par_id		= 0UL;
		par_num_t 	   	par_num		= 0UL;
		par_type_t   	par_data	= { .u32 = 0UL };
		par_status_t	status 		= ePAR_OK;
		par_cfg_t		par_cfg		= {0};

        if ( NULL != p_attr )
        {
    		// Check input command
    		if ( 2U == sscanf((const char*) p_attr, "%u,%f", (unsigned int*)&par_id, &par_data.f32 ))
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
    							par_data.u8 = (uint8_t)par_data.f32;
    							status = par_set( par_num, (uint8_t*) &par_data.u8 );
    							cli_printf( "OK,PAR_SET=%u", par_data.u8);
    						break;

    						case ePAR_TYPE_I8:
    							par_data.i8 = (int8_t)par_data.f32;
    							status = par_set( par_num, (int8_t*) &par_data.i8 );
    							cli_printf( "OK,PAR_SET=%i", (int) par_data.i8);
    						break;

    						case ePAR_TYPE_U16:
    							par_data.u16 = (uint16_t)par_data.f32;
    							status = par_set( par_num, (uint16_t*) &par_data.u16 );
    							cli_printf( "OK,PAR_SET=%u", par_data.u16);
    						break;

    						case ePAR_TYPE_I16:
    							par_data.i16 = (int16_t)par_data.f32;
    							status = par_set( par_num, (int16_t*) &par_data.i16 );
    							cli_printf( "OK,PAR_SET=%i", (int) par_data.i16);
    						break;

    						case ePAR_TYPE_U32:
    							par_data.u32 = (uint32_t)par_data.f32;
    							status = par_set( par_num, (uint32_t*) &par_data.u32 );
    							cli_printf( "OK,PAR_SET=%u", (int) par_data.u32);
    						break;

    						case ePAR_TYPE_I32:
    							par_data.i32 = (int32_t)par_data.f32;
    							status = par_set( par_num, (int32_t*) &par_data.i32 );
    							cli_printf( "OK,PAR_SET=%i", (int) par_data.i32);
    						break;

    						case ePAR_TYPE_F32:
    							status = par_set( par_num, (float32_t*) &par_data.f32 );
    							cli_printf( "OK,PAR_SET=%g", par_data.f32);
    						break;

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
			cli_unknown(NULL);
		}
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
		uint32_t 	   	par_id		= 0UL;
		par_num_t 	   	par_num		= 0UL;
		par_type_t   	par_data	= { .u32 = 0UL };
		par_status_t	status 		= ePAR_OK;
		par_cfg_t		par_cfg		= {0};

        if ( NULL != p_attr )
        {
    		// Check input command
    		if ( 1U == sscanf((const char*) p_attr, "%u", (unsigned int*)&par_id ))
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
    						status = par_get( par_num, (uint8_t*) &par_data.u8 );
    						cli_printf( "OK,PAR_GET=%u", par_data.u8 );
    					break;

    					case ePAR_TYPE_I8:
    						status = par_get( par_num, (int8_t*) &par_data.i8 );
    						cli_printf(  "OK,PAR_GET=%i", (int) par_data.i8 );
    					break;

    					case ePAR_TYPE_U16:
    						status = par_get( par_num, (uint16_t*) &par_data.u16 );
    						cli_printf(  "OK,PAR_GET=%u", par_data.u16 );
    					break;

    					case ePAR_TYPE_I16:
    						status = par_get( par_num, (int16_t*) &par_data.i16 );
    						cli_printf(  "OK,PAR_GET=%i", (int) par_data.i16 );
    					break;

    					case ePAR_TYPE_U32:
    						status = par_get( par_num, (uint32_t*) &par_data.u32 );
    						cli_printf(  "OK,PAR_GET=%u", (int) par_data.u32 );
    					break;

    					case ePAR_TYPE_I32:
    						status = par_get( par_num, (int32_t*) &par_data.i32 );
    						cli_printf(  "OK,PAR_GET=%i", (int) par_data.i32 );
    					break;

    					case ePAR_TYPE_F32:
    						status = par_get( par_num, (float32_t*) &par_data.f32 );
    						cli_printf(  "OK,PAR_GET=%g", par_data.f32 );
    					break;

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
			cli_unknown(NULL);
		}
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
		par_num_t 	par_num	= 0UL;
		uint16_t	par_id	= 0UL;

        if ( NULL != p_attr )
        {
    		// Check input command
    		if ( 1U == sscanf((const char*) p_attr, "%u", (unsigned int*)&par_id ))
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
			cli_unknown(NULL);
		}
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
		if ( NULL == p_attr )
		{
			// Set to default
			par_set_all_to_default();

			// Rtn msg
			cli_printf( "OK, All parameters set to default!" );
		}
		else
		{
			cli_unknown(NULL);
		}
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
			cli_unknown(NULL);
		}
	}

	#if (( 1 == CLI_CFG_DEBUG_EN ) && ( 1 == PAR_CFG_NVM_EN ))

		////////////////////////////////////////////////////////////////////////////////
		/*!
		* @brief 		Clean parameter NVM region
		*
		* @note			Command format: >>>par_save_clean
		*
		* @param[in] 	attr 	- Inputed command attributes
		* @return 		void
		*/
		////////////////////////////////////////////////////////////////////////////////
		static void cli_par_store_reset(const uint8_t * p_attr)
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
				cli_unknown(NULL);
			}
		}

	#endif

    #if (( 1 == CLI_CFG_PAR_USE_EN ) && ( 1 == CLI_CFG_STREAM_NVM_EN ))


		////////////////////////////////////////////////////////////////////////////////
		/*!
		* @brief 		Store streaming informations to NVM
		*
		* @note			Command format: >>>status_save
		*
		* @param[in] 	attr 	- Inputed command attributes
		* @return 		void
		*/
		////////////////////////////////////////////////////////////////////////////////
        static void cli_status_save(const uint8_t * p_attr)
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
                cli_unknown(NULL);
            }
        }
    #endif

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief        Start live watch streaming
	*
	* @note			Command format: >>>status_start
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return       void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void cli_status_start(const uint8_t * p_attr)
	{
		if ( NULL == p_attr )
		{
            if ( g_cli_live_watch.num_of > 0 )
            {
                g_cli_live_watch.active = true;

                cli_printf( "OK, Streaming started!" );

                #if ( 1 == CLI_CFG_AUTO_STREAM_STORE_EN )
                    cli_status_save( NULL );
                #endif
            }
            else
            {
               cli_printf( "ERR, Streaming parameter list empty!" ); 
            }
		}
		else
		{
			cli_unknown(NULL);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief        Stop live watch streaming
	*
	* @note			Command format: >>>status_stop
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return       void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void cli_status_stop(const uint8_t * p_attr)
	{
		if ( NULL == p_attr )
		{
			g_cli_live_watch.active = false;

            cli_printf( "OK, Streaming stopped!" );

            #if ( 1 == CLI_CFG_AUTO_STREAM_STORE_EN )
                cli_status_save( NULL );
            #endif
		}
		else
		{
			cli_unknown(NULL);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief        Put parameters to live watch
	*
	* @note			Command format: >>>status_des [parID1,parID2,..parIDn]
	*
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return       void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void cli_status_des(const uint8_t * p_attr)
	{
		uint32_t 	ch_cnt      = 0;
		uint32_t 	par_id      = 0;
		par_cfg_t	par_cfg     = {0};
		par_num_t	par_num     = 0;
        bool        invalid_par = false;

        if ( NULL != p_attr )
        {
    		// Reset counts
    		g_cli_live_watch.num_of = 0;

    		// Parse live watch request command
    		while(		( g_cli_live_watch.num_of <= CLI_CFG_PAR_MAX_IN_LIVE_WATCH )
    				&& 	( 1U == sscanf((const char*) p_attr, "%d%n", (int*) &par_id, (int*) &ch_cnt )))
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
    			// Send sample time
    			snprintf((char*) &gu8_tx_buffer, CLI_CFG_TX_BUF_SIZE, "OK,%g", ( g_cli_live_watch.period / 1000.0f ));
    			cli_send_str( gu8_tx_buffer );

    			// Print streaming parameters/variables
    			for ( uint8_t par_idx = 0; par_idx < g_cli_live_watch.num_of; par_idx++ )
    			{
    				// Get parameter configurations
    				par_get_config( g_cli_live_watch.par_list[ par_idx ], &par_cfg );

    				// Format string with parameters info
    				sprintf((char*) &gu8_tx_buffer, ",%s,d,1", par_cfg.name );

    				// Send
    				cli_send_str( gu8_tx_buffer );
    			}

    			// Terminate line
    			cli_printf("");

                #if ( 1 == CLI_CFG_AUTO_STREAM_STORE_EN )
                    cli_status_save( NULL );
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
			cli_unknown(NULL);
		}
	}

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief        Change rate of live watch streaming period
	*
	* @note			Command format: >>>status_rate [period_in_ms]
    *
	* @example      >>>status_rate 100 --> Will change period to 100 ms
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return       void
	*/
	////////////////////////////////////////////////////////////////////////////////
    static void cli_status_rate(const uint8_t * p_attr)
    {
        uint32_t period;

        if ( NULL != p_attr )
        {
            if ( 1U == sscanf((const char*) p_attr, "%d", (int*) &period ))
            {
                // Check if within wanted range
                if  (   ( period >= CLI_CFG_HNDL_PERIOD_MS )
                    &&  ( period <= 60000UL ))
                {
                    // Check if multiple of defined period
                    if (( period % CLI_CFG_HNDL_PERIOD_MS ) == 0 )
                    {
                        g_cli_live_watch.period = period;
                        g_cli_live_watch.period_cnt = (uint32_t) ( g_cli_live_watch.period / CLI_CFG_HNDL_PERIOD_MS );

                        cli_printf( "OK, Period changed to %d ms", g_cli_live_watch.period );

						#if ( 1 == CLI_CFG_AUTO_STREAM_STORE_EN )
							cli_status_save( NULL );
						#endif
                    }
                    else
                    {
                        cli_printf( "ERR, Wanted period is not multiple of \"CLI_CFG_HNDL_PERIOD_MS\"!" );
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
			cli_unknown(NULL);
		}
    }

    ////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief        Get streaming configuration info
	*
	* @note			Command format: >>>status_info
	*
	* @param[in] 	attr 	- Inputed command attributes
	* @return       void
	*/
	////////////////////////////////////////////////////////////////////////////////
    static void cli_status_info(const uint8_t * p_attr)
    {
        uint16_t par_id = 0U;

        if ( NULL == p_attr )
        {
            // Send streaming info as
            // OK, PERIOD,ACTIVE,NUM_OF,PAR_LIST
            sprintf((char*) &gu8_tx_buffer, "OK, %d,%d,%d", g_cli_live_watch.period, g_cli_live_watch.active, g_cli_live_watch.num_of );  
            cli_send_str( gu8_tx_buffer );

            // Print streaming parameters/variables
            for ( uint8_t par_idx = 0; par_idx < g_cli_live_watch.num_of; par_idx++ )
            {
                // Get parameter ID
                (void) par_get_id( g_cli_live_watch.par_list[par_idx], &par_id );

                // Format string with parameters info
                sprintf((char*) &gu8_tx_buffer, ",%d", par_id );

                // Send
                cli_send_str( gu8_tx_buffer );
            }

            // Terminate line
            cli_printf("");
        }
        else
		{
			cli_unknown(NULL);
		}        
    }

	////////////////////////////////////////////////////////////////////////////////
	/*!
	* @brief 	Convert parameter any value type to float
	*
	* 	@note	It is being used for sprintf functionalities
	*
	* @param[in] 	par_type	- Data type of parameter
	* @param[in] 	p_val		- Pointer to parameter value
	* @return 		f32_par_val	- Floating representation of parameter value
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
				f32_par_val = *(uint32_t*) p_val;
				break;

			case ePAR_TYPE_I32:
				f32_par_val = *(int32_t*) p_val;
				break;

			case ePAR_TYPE_F32:
				f32_par_val = *(float32_t*) p_val;
				break;

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
	*				Executes in main "cli_hndl()" and streams parameters inside
	*				live watch queue if live watch is enabled.
	*
	* @return       void
	*/
	////////////////////////////////////////////////////////////////////////////////
	static void	cli_par_live_watch_hndl(void)
	{
		par_type_t 	par_val	= { .u32 = 0UL };
		par_cfg_t	par_cfg	= {0};

		// Stream data only if:
		//		1. Live watch is active
		//	AND	2. Any parameter to stream
		if 	(	( true == g_cli_live_watch.active )
			&& 	( g_cli_live_watch.num_of > 0 ))
		{
			// Loop thru streaming parameters
			for(uint8_t par_idx = 0; par_idx < g_cli_live_watch.num_of; par_idx++)
			{
				// Get parameter data type
				par_get_config( g_cli_live_watch.par_list[par_idx], &par_cfg );

				// Get parameter
				par_get( g_cli_live_watch.par_list[par_idx], &par_val.u32 );

				// Based on type fill streaming buffer
				switch ( par_cfg.type )
				{
					case ePAR_TYPE_U8:
						sprintf((char*) &gu8_tx_buffer, "%d", (int)par_val.u8 );
						break;
					case ePAR_TYPE_U16:
						sprintf((char*) &gu8_tx_buffer, "%d", (int)par_val.u16 );
					break;
					case ePAR_TYPE_U32:
						sprintf((char*) &gu8_tx_buffer, "%d", (int)par_val.u32 );
					break;
					case ePAR_TYPE_I8:
						sprintf((char*) &gu8_tx_buffer, "%i", (int)par_val.i8 );
						break;
					case ePAR_TYPE_I16:
						sprintf((char*) &gu8_tx_buffer, "%i", (int)par_val.i16 );
					break;
					case ePAR_TYPE_I32:
						sprintf((char*) &gu8_tx_buffer, "%i", (int)par_val.i32 );
					break;
					case ePAR_TYPE_F32:
						sprintf((char*) &gu8_tx_buffer, "%g", par_val.f32 );
					break;

					default:
						// No actions..
					break;
				}

				// Send
				cli_send_str( gu8_tx_buffer );

                // If not last -> send delimiter
                if ( par_idx < ( g_cli_live_watch.num_of - 1 ))
                {
                    cli_send_str( "," );
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
	* @param[in]	par_num	- Parameter enumeration number
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

            #if ( 1 == CLI_CFG_STREAM_NVM_EN )

				// Init NVM
				if ( eNVM_OK == nvm_init())
				{
					// Read streaming info
					status = cli_nvm_read( &g_cli_live_watch );
				}
				else
				{
					status = eCLI_ERROR_INIT;
				}

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
	#if ( 1 == CLI_CFG_PAR_USE_EN )

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

		
	#endif

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

			// Send string
			status = cli_send_str((const uint8_t*) &gu8_tx_buffer);
			status |= cli_send_str((const uint8_t*) CLI_CFG_TERMINATION_STRING );
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
			// Is channel enabled
			if ( true == cli_cfg_get_ch_en( ch ))
			{
				// Taking args from stack
				va_start(args, p_format);
				vsprintf((char*) gu8_tx_buffer, (const char*) p_format, args);
				va_end(args);

				// Send channel name
				status |= cli_send_str((const uint8_t*) cli_cfg_get_ch_name( ch ));
				status |= cli_send_str((const uint8_t*) ": " );

				// Send string
				status |= cli_send_str((const uint8_t*) &gu8_tx_buffer );
				status |= cli_send_str((const uint8_t*) CLI_CFG_TERMINATION_STRING );
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
* @param[in]	p_cmd_table	- Pointer to user cmd table
* @return       status		- Status of initialization
*/
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_register_cmd_table(const cli_cmd_table_t * const p_cmd_table)
{
	cli_status_t status = eCLI_OK;

	CLI_ASSERT( NULL != p_cmd_table );

	#if ( 1 == CLI_CFG_MUTEX_EN )

		// Mutex obtain
		if ( eCLI_OK == cli_if_aquire_mutex())
		{
	#endif
			if ( NULL != p_cmd_table )
			{
				// Is there any space left for user tables?
				if ( gu32_user_table_count < CLI_CFG_MAX_NUM_OF_USER_TABLES )
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
		#if ( 1 == CLI_CFG_MUTEX_EN )

			// Release mutex
			cli_if_release_mutex();
		}
		else
		{
			status = eCLI_ERROR;
		}
		#endif

	return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
