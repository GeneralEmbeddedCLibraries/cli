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

struct cli_cmd;
/**
 *      CLI Command Function
 *
 *  @note   User shall use callback function declared as: "void my_cli_func(const cli_cmd_t *p_cmd, const char * const p_attr)"
 *
 * @param[in]   p_cmd   - Pointer to command itself
 * @param[in]   p_attr  - Pointer to additional attributes beside command
 * @return      void
 */
typedef void(*pf_cli_cmd)(const struct cli_cmd *p_cmd, const char * const p_attr);

/**
 * 	 Single CLI Command
 *
 * 	 Sizeof: 16 bytes
 */
typedef struct cli_cmd
{
	char * 		name;		    /**<Command name*/
	pf_cli_cmd	func;		    /**<Command function */
	char * 		help;		    /**<Command help string */
    void *      p_context;      /**<Pointer to command specific context */
} cli_cmd_t;

/**
 *  CLI Command Table
 */
typedef struct
{
    cli_cmd_t * p_cmd;      /**<Command table */
    uint32_t    num_of;     /**<Number of commands */
} cli_cmd_table_t;

/**
 *  CLI Command Table Node
 */
typedef struct cli_cmd_table_node
{
    cli_cmd_table_t *           p_table;    /**<Pointer to CLI command table */
    struct cli_cmd_table_node * next;       /**<Pointer to next CLI command table */
} cli_cmd_table_node_t;

/* Macro that keeps tables const and hides the node + register function. */
#define CLI_DECLARE_TABLE(TABLE_SYM, CMD_TABLE, NUM_OF_CMD)                                   \
    static const cli_cmd_table_t TABLE_SYM = {                                         \
        .p_cmd = (cli_cmd_t*) CMD_TABLE, .num_of = NUM_OF_CMD      \
    };                                                                            \
    static cli_cmd_table_node_t TABLE_SYM##_node = { .p_table = (cli_cmd_table_t*) &TABLE_SYM, NULL };                \
    static inline cli_status_t TABLE_SYM##_cli_register_cmd_table(void) {                                \
        return cli_register_cmd_table(&TABLE_SYM##_node);                                \
    }


////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
cli_status_t cli_init				(void);
cli_status_t cli_deinit				(void);
cli_status_t cli_is_init			(bool * const p_is_init);
cli_status_t cli_hndl				(void);
cli_status_t cli_send_str           (const char * const p_str);
cli_status_t cli_printf				(char * p_format, ...);
cli_status_t cli_printf_ch			(const cli_ch_opt_t ch, char * p_format, ...);


cli_status_t cli_register_cmd_table (cli_cmd_table_node_t * const p_cmd_table);

//cli_status_t cli_register_cmd_table (const cli_cmd_t * const p_cmd_table, const uint8_t num_of_cmd);

cli_status_t cli_osci_hndl          (void);

#endif // __CLI_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
