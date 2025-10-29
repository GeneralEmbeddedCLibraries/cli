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

// Common goods
#include "common/utils/src/utils.h"

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
 *
 * @note  Const table with writable link cell
 *        - The CLI table is a const object in flash (read-only).
 *        - `p_next` does not point to the next table directly; it points to a
 *          writable pointer (the table’s “link cell”) stored in RAM.
 *        - At registration time we modify that cell via `*p_next` to chain tables.
 *        - This design avoids embedding a writable field inside the const table and
 *          removes any fixed limit on the number of tables.
 */
typedef struct cli_cmd_table
{
    const cli_cmd_t *       p_cmd;      /**<Command table */
    const uint32_t          num_of;     /**<Number of commands */
    struct cli_cmd_table ** p_next;     /**<Pointer to next table */
} cli_cmd_table_t;

/**
 *  Define CLI command table helper
 */
#define CLI_DEFINE_CMD_TABLE(name,...)                                              \
    static const cli_cmd_table_t name =                                             \
    {                                                                               \
        .p_cmd  = (cli_cmd_t[]){__VA_ARGS__},   /**<Non-const anonymous array */    \
        .num_of = ARRAY_SIZE(((cli_cmd_t[]){__VA_ARGS__})),                         \
        .p_next = &(cli_cmd_table_t*){NULL},                                        \
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
cli_status_t cli_register_cmd_table (cli_cmd_table_t *  p_cmd_table);
cli_status_t cli_osci_hndl          (void);

#endif // __CLI_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
