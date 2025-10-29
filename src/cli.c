// Copyright (c) 2025 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      cli.c
*@brief     Command Line Interface
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      29.10.2025
*@version   V3.0.0
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

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static cli_status_t cli_parser_hndl			(void);
static void 		cli_execute_cmd			(const char * const p_cmd);
static bool         cli_table_check_and_exe (const char * p_cmd, const uint32_t cmd_size, const char * attr);
static uint32_t		cli_calc_cmd_size		(const char * p_cmd, const char * attr);

// Basic CLI functions
static void cli_help		  	(const cli_cmd_t * p_cmd, const char * p_attr);
static void cli_reset	   	  	(const cli_cmd_t * p_cmd, const char * p_attr);
static void cli_sw_version  	(const cli_cmd_t * p_cmd, const char * p_attr);
static void cli_hw_version  	(const cli_cmd_t * p_cmd, const char * p_attr);
static void cli_boot_version  	(const cli_cmd_t * p_cmd, const char * p_attr);
static void cli_proj_info  		(const cli_cmd_t * p_cmd, const char * p_attr);
static void cli_uptime 		    (const cli_cmd_t * p_cmd, const char * p_attr);

static void cli_ch_info  		(const cli_cmd_t * p_cmd, const char * p_attr);
static void cli_ch_en  			(const cli_cmd_t * p_cmd, const char * p_attr);
static void	cli_send_intro		(const cli_cmd_t * p_cmd, const char * p_attr);
static void cli_show_intro      (void);

#if ( 1 == CLI_CFG_ARBITRARY_RAM_ACCESS_EN )
static void cli_ram_write       (const cli_cmd_t * p_cmd, const char * p_attr);
static void cli_ram_read        (const cli_cmd_t * p_cmd, const char * p_attr);
#endif

static bool             cli_validate_user_table (const cli_cmd_t * const p_cmd_table, const uint8_t num_of_cmd);
static const char * 	cli_find_char			(const char * const str, const char target_char, const uint32_t size);

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
CLI_DEFINE_CMD_TABLE( g_cli_basic_table,

    // --------------------------------------------------------------------------------------------------------------------------
    //  name                    function                help string                                                     context
    // --------------------------------------------------------------------------------------------------------------------------
    {   "help",                 cli_help,               "Print help message",                                           NULL    },
    {   "intro",                cli_send_intro,         "Print intro message",                                          NULL    },
    {   "reset",                cli_reset,              "Reset device",                                                 NULL    },
    {   "sw_ver",               cli_sw_version,         "Print device software version",                                NULL    },
    {   "hw_ver",               cli_hw_version,         "Print device hardware version",                                NULL    },
    {   "boot_ver",             cli_boot_version,       "Print device bootloader (sw) version",                         NULL    },
    {   "proj_info",            cli_proj_info,          "Print project informations",                                   NULL    },
    {   "uptime",               cli_uptime,             "Get device uptime [ms]",                                       NULL    },
    {   "ch_info",              cli_ch_info,            "Print COM channel informations",                               NULL    },
    {   "ch_en",                cli_ch_en,              "Enable/disable COM channel. Args: [chEnum][en]",               NULL    },

#if ( 1 == CLI_CFG_ARBITRARY_RAM_ACCESS_EN )
    {   "ram_write",            cli_ram_write,          "Write data to RAM. Args: [address<hex>][size][value<hex>]",    NULL    },
    {   "ram_read",             cli_ram_read,           "Read data from RAM. Args: [address<hex>][size]",               NULL    },
#endif
);

/**
 *  Pointer to first registered CLI command table
 */
static cli_cmd_table_t * gp_cli_cmd_tables = NULL;

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
	 static uint32_t  		buf_idx 	= 0;
            uint32_t        escape_cnt  = 0;
     static uint8_t         rx_buffer[CLI_CFG_RX_BUF_SIZE] = {0};
     static uint32_t        first_byte_time = 0U;

	// Take all data from reception buffer
	while   (   ( eCLI_OK == cli_if_receive( &rx_buffer[buf_idx] ))
            &&  ( escape_cnt < 10000UL ))
	{
	    if( 0 == buf_idx )
	    {
	        first_byte_time = CLI_GET_SYSTICK();
	    }

		// Find termination character
        char * p_term_str_start = strstr((char*) &rx_buffer, (char*) CLI_CFG_TERMINATION_STRING );
        
        // Termination string found
        if ( NULL != p_term_str_start )
		{
			// Replace all termination character with NULL
            memset((char*) p_term_str_start, 0, strlen( CLI_CFG_TERMINATION_STRING ));

            // Execute command
            cli_execute_cmd( (const char *)rx_buffer );

            // Reset buffer
			memset( &rx_buffer, 0U, sizeof( rx_buffer ));
            buf_idx = 0;

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

            // Reset buffer
            memset( &rx_buffer, 0U, sizeof( rx_buffer ));
            buf_idx = 0;
			status = eCLI_ERROR;
			break;
		}

		// Increment escape count in order to prevent infinite loop
        escape_cnt++;
	}

    // Expected that complete command will be received within 100ms
    if  (   ((uint32_t)( CLI_GET_SYSTICK() - first_byte_time ) >= 100U ) 
        &&  ( buf_idx > 0 ))    // Reception ongoing
    {
        CLI_DBG_PRINT( "CLI: Timeout!" );
        memset( &rx_buffer, 0U, sizeof( rx_buffer ));
        buf_idx = 0;
        status = eCLI_ERROR;
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
static void cli_execute_cmd(const char * const p_cmd)
{
	// Get command options
	const char * attr = cli_find_char(p_cmd, ' ', CLI_CFG_RX_BUF_SIZE );

	// Calculate size of command string
	const uint32_t cmd_size = cli_calc_cmd_size( p_cmd, attr );

    // Check and execute for basic commands
    const bool cmd_found = cli_table_check_and_exe( p_cmd, cmd_size, attr );

	// No command found in any of the tables
	if ( false == cmd_found )
	{
		cli_util_unknown_cmd_rsp();
	}
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Check and execute table commands
*
* @note         Commands are divided into simple and combined commands
*
*               SIMPLE COMMAND: Do not pass additional attributes
*
*                   E.g.: >>>help
*
*               COMBINED COMMAND:   Has additional attributes separated by
*                                   empty spaces (' ').
*
*                   E.g.: >>>par_get 0
*
*
* @param[in]    p_cmd       - NULL terminated input string
* @param[in]    attr        - Additional command attributes
* @return       cmd_found   - Command found flag
*/
////////////////////////////////////////////////////////////////////////////////
static bool cli_table_check_and_exe(const char * p_cmd, const uint32_t cmd_size, const char * attr)
{
    // Iterate thru table linked list
    for ( const cli_cmd_table_t * table = gp_cli_cmd_tables; NULL != table; table = (*table->p_next))
    {
        // Iterate thru all commands in table
        for (size_t cmd = 0; cmd < table->num_of; cmd++)
        {
            // String size to compare
            const size_t size_to_compare = MAX( cmd_size, strlen( table->p_cmd[cmd].name ));

            // Valid command?
            if ( 0 == ( strncmp( p_cmd, table->p_cmd[cmd].name, size_to_compare )))
            {
                // Execute command
                table->p_cmd[cmd].func( &table->p_cmd[cmd], attr );

                // Command founded
                return true;
            }
        }
    }

    return false;
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
    	const char * p_cmd_end = cli_find_char( p_cmd, ' ', CLI_CFG_RX_BUF_SIZE );
        if (NULL != p_cmd_end)
        {
            size = (uint32_t)(p_cmd_end - p_cmd - 1); // -1 because find character function returns position +1
        }
    }

	return size;
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Show help
*
* @param[in]    p_cmd   - Pointer to command
* @param[in]	p_attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_help(const cli_cmd_t * p_cmd, const char * p_attr)
{
    UNUSED(p_cmd);

	// No additional attributes
	if ( NULL == p_attr )
	{
		cli_printf( " " );
		cli_printf( "    List of device commands" );

	    // Iterate thru table linked list
	    for ( const cli_cmd_table_t * table = gp_cli_cmd_tables; NULL != table; table = (*table->p_next))
	    {
            // Are there any commands
            if ( table->num_of > 0U )
            {
                // Print separator between user commands
                cli_printf( "--------------------------------------------------------" );

                // Show help for that table
                for ( uint32_t cmd_idx = 0; cmd_idx < table->num_of; cmd_idx++ )
                {
                    // Get name and help string
                    const char * name_str = table->p_cmd[cmd_idx].name;
                    const char * help_str = table->p_cmd[cmd_idx].help;

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
* @param[in]    p_cmd   - Pointer to command
* @param[in]	p_attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_reset(const cli_cmd_t * p_cmd, const char * p_attr)
{
    UNUSED(p_cmd);

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
* @param[in]    p_cmd   - Pointer to command
* @param[in]	p_attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_sw_version(const cli_cmd_t * p_cmd, const char * p_attr)
{
    UNUSED(p_cmd);

	if ( NULL == p_attr )
	{
        cli_printf( "OK, %s", CLI_CFG_INTRO_SW_VER );
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
* @param[in]    p_cmd   - Pointer to command
* @param[in]	p_attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_hw_version(const cli_cmd_t * p_cmd, const char * p_attr)
{
    UNUSED(p_cmd);

	if ( NULL == p_attr )
	{
        cli_printf( "OK, %s", CLI_CFG_INTRO_HW_VER );
	}
	else
	{
		cli_util_unknown_cmd_rsp();
	}
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Show bootloader (SW) version
*
* @param[in]    p_cmd   - Pointer to command
* @param[in]	p_attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_boot_version(const cli_cmd_t * p_cmd, const char * p_attr)
{
    UNUSED(p_cmd);

    if ( NULL == p_attr )
    {
        cli_printf( "OK, %s", CLI_CFG_INTRO_BOOT_VER );
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
* @param[in]    p_cmd   - Pointer to command
* @param[in]	p_attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_proj_info(const cli_cmd_t * p_cmd, const char * p_attr)
{
    UNUSED(p_cmd);

	if ( NULL == p_attr )
	{
        cli_printf( "OK, %s", CLI_CFG_INTRO_PROJ_INFO );
	}
	else
	{
		cli_util_unknown_cmd_rsp();
	}
}

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Get device uptime [ms]
*
* @param[in]    p_cmd   - Pointer to command
* @param[in]	p_attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_uptime(const cli_cmd_t * p_cmd, const char * p_attr)
{
    UNUSED(p_cmd);

	if ( NULL == p_attr )
	{
        const uint64_t uptime = cli_if_get_uptime();
        cli_printf("OK, %lu%09lums", (uint32_t)(uptime / 1000000000ULL), (uint32_t)(uptime % 1000000000ULL));
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
* @param[in]    p_cmd   - Pointer to command
* @param[in]	p_attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_ch_info(const cli_cmd_t * p_cmd, const char * p_attr)
{
    UNUSED(p_cmd);

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
* @param[in]    p_cmd   - Pointer to command
* @param[in]	p_attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_ch_en(const cli_cmd_t * p_cmd, const char * p_attr)
{
    UNUSED(p_cmd);

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

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Show intro
*
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void cli_show_intro(void)
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

////////////////////////////////////////////////////////////////////////////////
/*!
* @brief        Send intro string
*
* @param[in]    p_cmd   - Pointer to command
* @param[in]	p_attr 	- Inputed command attributes
* @return       void
*/
////////////////////////////////////////////////////////////////////////////////
static void	cli_send_intro(const cli_cmd_t * p_cmd, const char * p_attr)
{
    UNUSED(p_cmd);
    UNUSED(p_attr);

    cli_show_intro();
}

#if ( 1 == CLI_CFG_ARBITRARY_RAM_ACCESS_EN )
    ////////////////////////////////////////////////////////////////////////////////
    /*!
    * @brief        Write data to RAM
    *
    *
    * @note			Command format: >>>cli_ram_write [address,size,value]
    *               Address and value arguments must be inputed in hexadecimal format
    *               with '0x' prefix and followed by lowercase characters.
    *               Size must be in decimal format, with only valid values being:
    *               1, 2 and 4.
    *
    * 				E.g.:	>>>cli_ram_write 0xabcdef01,2,0x1234
    * 				E.g.:	>>>cli_ram_write 0x1234,4,0xab112233
    *
    * @param[in]    p_cmd   - Pointer to command
    * @param[in]    p_attr  - Inputed command attributes
    * @return       void
    */
    ////////////////////////////////////////////////////////////////////////////////
    static void cli_ram_write(const cli_cmd_t * p_cmd, const char * p_attr)
    {
        UNUSED(p_cmd);

        uint32_t addr;
        uint32_t size;
        uint32_t val;

        // Make sure we can cast uint32_t to unsigned int below to supress compiler warning when types do not match exactly
        // for example unsigned long to unsigned int
        STATIC_ASSERT_TYPES(uint32_t, unsigned int);

        if ( NULL != p_attr )
        {
            if ( 3U == sscanf((const char*) p_attr, "0x%x,%u,0x%x", (unsigned int *)&addr, (unsigned int *)&size, (unsigned int *)&val ))
            {
                if ((1 == size) || (2 == size) || (4 == size))
                {
                    if (cli_if_check_ram_addr_range(addr, size) == eCLI_OK)
                    {
                        switch (size)
                        {
                            case 1:
                                *(uint8_t *)addr = (uint8_t)val;
                                break;
                            case 2:
                                *(uint16_t *)addr = (uint16_t)val;
                                break;
                            case 4:
                                *(uint32_t *)addr = (uint32_t)val;
                                break;
                            default:
                                // Internal inconsistency. Should not reach here since we check
                                // size above.
                                CLI_ASSERT(0);
                                break;
                        }

                        cli_printf( "OK, [0x%08x,0x%08x] = 0x%x", addr, addr + size - 1, val);
                    }
                    else
                    {
                        cli_printf( "ERR, Invalid address!" );
                    }
                }
                else
                {
                    cli_printf( "ERR, Invalid size!" );
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

    ////////////////////////////////////////////////////////////////////////////////
    /*!
    * @brief        Read data from RAM
    *
    *
    * @note			Command format: >>>cli_ram_read [address,size]
    *               Address argument must be inputed in hexadecimal format with '0x'
    *               prefix and followed by lowercase characters.
    *               Size must be in decimal format, with only valid values being:
    *               1, 2 and 4.
    *
    * 				E.g.:	>>>cli_ram_read 0xabcdef01,1
    * 				E.g.:	>>>cli_ram_read 0x1234,4
    *
    * @param[in]    p_cmd   - Pointer to command
    * @param[in]    p_attr  - Inputed command attributes
    * @return       void
    */
    ////////////////////////////////////////////////////////////////////////////////
    static void cli_ram_read(const cli_cmd_t * p_cmd, const char * p_attr)
    {
        UNUSED(p_cmd);

        uint32_t addr;
        uint32_t size;

        // Make sure we can cast uint32_t to unsigned int below to supress compiler warning when types do not match exactly
        // for example unsigned long to unsigned int
        STATIC_ASSERT_TYPES(uint32_t, unsigned int);

        if ( NULL != p_attr )
        {
            if ( 2U == sscanf((const char*) p_attr, "0x%x,%u", (unsigned int *)&addr, (unsigned int *)&size ))
            {
                if ((1 == size) || (2 == size) || (4 == size))
                {
                    if (cli_if_check_ram_addr_range(addr, size) == eCLI_OK)
                    {
                        uint32_t val = 0;

                        switch (size)
                        {
                            case 1:
                                val = *(uint8_t *)addr;
                                break;
                            case 2:
                                val = *(uint16_t *)addr;
                                break;
                            case 4:
                                val = *(uint32_t *)addr;
                                break;
                            default:
                                // Internal inconsistency. Should not reach here since we check
                                // size above.
                                CLI_ASSERT(0);
                                break;
                        }

                        cli_printf( "0x%x", val);
                    }
                    else
                    {
                        cli_printf( "ERR, Invalid address!" );
                    }
                }
                else
                {
                    cli_printf( "ERR, Invalid size!" );
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
#endif // CLI_CFG_ARBITRARY_RAM_ACCESS_EN

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
        if 	(	( NULL == p_cmd_table[cmd].name )
            ||	( NULL == p_cmd_table[cmd].help )
            ||	( NULL == p_cmd_table[cmd].func ))
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

        // Register basic table
        cli_register_cmd_table((cli_cmd_table_t*) &g_cli_basic_table );

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
				cli_show_intro();
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

        // Disable all channels
        for (cli_ch_opt_t ch = 0; ch < eCLI_CH_NUM_OF; ch++)
        {
            cli_cfg_set_ch_en( ch, false );
        }

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
* @return       Initialization flag
*/
////////////////////////////////////////////////////////////////////////////////
bool cli_is_init(void)
{
	return gb_is_init;
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
cli_status_t cli_send_str(const char * const p_str)
{
    cli_status_t status = eCLI_OK;

    // Mutex obtain
    if ( eCLI_OK == cli_if_aquire_mutex())
    {
        // Write to cli port
        status |= cli_if_transmit( (const uint8_t *)p_str );

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

                // Use vsnprintf to prevent buffer overflow and get the length written
                const int printed_len = vsnprintf((char*) p_tx_buf, CLI_CFG_TX_BUF_SIZE, (const char*) p_format, args);
                va_end(args);

                // Handle buffer overflow or encoding error from vsnprintf
                // The string might be truncated or invalid
                if  (   ( printed_len < 0 ) 
                    ||  ( printed_len >= CLI_CFG_TX_BUF_SIZE )) 
                {
                    status = eCLI_ERROR;
                    CLI_ASSERT(0);
                } 
                else 
                {
                    // Append the termination string to the buffer
                    // Check if there's enough space left for the termination string
                    const size_t term_len = strlen(CLI_CFG_TERMINATION_STRING);

                    if ((size_t)printed_len + term_len < CLI_CFG_TX_BUF_SIZE ) 
                    {
                        // Append termination string and send
                        strcat((char*) p_tx_buf, (const char*) CLI_CFG_TERMINATION_STRING);
                        status = cli_send_str((const char*) p_tx_buf );
                    } 
                    else 
                    {
                        // Not enough space for termination string
                        status = eCLI_ERROR;
                        CLI_ASSERT(0);
                    }
                }

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
                    vsprintf((char*) p_tx_buf, p_format, args);
                    va_end(args);

                    // Send channel name
                    status |= cli_send_str( cli_cfg_get_ch_name( ch ));
                    status |= cli_send_str( ": " );

                    // Send string
                    status |= cli_send_str((const char*) p_tx_buf );
                    status |= cli_send_str( CLI_CFG_TERMINATION_STRING );

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
* @param[in]	p_cmd_table	- Pointer to command table node
* @return       status      - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_register_cmd_table(const cli_cmd_table_t * const p_cmd_table)
{
    cli_status_t status = eCLI_OK;
    static cli_cmd_table_t * prev_table = NULL;

    CLI_ASSERT( NULL != p_cmd_table );
    if ( NULL == p_cmd_table ) return eCLI_ERROR;

    // User table defined OK
    if ( cli_validate_user_table( p_cmd_table->p_cmd, p_cmd_table->num_of ))
    {
        // Mutex obtain
        if ( eCLI_OK == cli_if_aquire_mutex())
        {
            // First table registration entry -> store start of the table linked list
            if ( NULL == gp_cli_cmd_tables )
            {
                gp_cli_cmd_tables = (cli_cmd_table_t*) p_cmd_table;
            }

            // On non-first table registration assign next pointer of lastly registrated table to the current one...
            else
            {
                (*prev_table->p_next) = (cli_cmd_table_t*) p_cmd_table;
            }

            // Store previous table
            prev_table = (cli_cmd_table_t*) p_cmd_table;

            // Release mutex
            cli_if_release_mutex();
        }
    }

    // User table definition error
    else
    {
        CLI_DBG_PRINT( "CLI ERROR: Invalid definition of user table!");
        CLI_ASSERT( 0 );
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
