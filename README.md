# **Command Line Interface - CLI**

Command Line Interface (CLI) is general purpose console based interpretor independent from communication media. It purpose is to easily and quickly setup embedded device configurations and diagnosis via application defined communication channel. Only constrain for communication channel is usage od ASCII formated characters. 

CLI is build around command tables where command name, function and help message is specified. E.g.:
```C
// -----------------------------------------------------------------------------
// 	name			function		help string
// -----------------------------------------------------------------------------
{ 	"help", 		cli_help, 		"Print all commands help" },
{ 	"reset", 		cli_reset,		"Reset device" 			  },
```

CLI divides two types of command tables:
 - ***BASIC COMMAND TABLE***: Is compile-time defined by CLI module itself and highly depends on user configurations of module. Commands inside basic table serves for PC tool interfacing with the embedded device.
 - ***USER COMMAND TABLE***: Is run-time defined list of command defined by the user application purposes.


## **Dependencies**

### **1. Device Parameters**
In case of using parameters *CLI_CFG_PAR_USE_EN = 1*, then [Device Parameters](https://github.com/GeneralEmbeddedCLibraries/parameters) must pe part of project. 
Device Parameters module must take following path:
```
"root/middleware/parameters/parameters/src/par.h"
```

### **2. NVM Module**
In case of using NVM module *CLI_CFG_STREAM_NVM_EN = 1*, then [NVM module](https://github.com/GeneralEmbeddedCLibraries/nvm) must pe part of project. 
NVM module must take following path:
```
"root/middleware/nvm/nvm/src/nvm.h"
```

## **General Embedded C Libraries Ecosystem**
In order to be part of *General Embedded C Libraries Ecosystem* this module must be placed in following path: 

```
root/middleware/cli/cli/"module_space"
```

## **API**
| API Functions | Description | Prototype |
| --- | ----------- | ----- |
| **cli_init** | Initialization of CLI module | cli_status_t cli_init(void) |
| **cli_deinit** | De-initialization of CLI module | cli_status_t cli_deinit				(void) |
| **cli_is_init** | Get initialization status | cli_status_t cli_is_init(bool * const p_is_init) |
| **cli_hndl** | Main handler for CLI module | cli_status_t cli_hndl(void) |
| **cli_printf** | Print formated string thrugh CLI COM port | cli_status_t cli_printf(char * p_format, ...) |
| **cli_printf_ch** | Print COM channel formated string thrugh CLI COM port | cli_status_t cli_printf_ch(const cli_ch_opt_t ch, char * p_format, ...) |
| **cli_register_cmd_table** | Register user define CLI command table | cli_status_t cli_register_cmd_table (const cli_cmd_table_t * const p_cmd_table) |


## **Usage**

**GENERAL NOTICE: Put all user code between sections: USER CODE BEGIN & USER CODE END!**

1. Copy template files to root directory of module.
2. Configure CLI module for application needs. Configuration options are following:

| Configuration | Description |
| --- | --- |
| **CLI_CFG_INTRO_STRING_EN** 			| Enable/Disable introduction string at CLI initialization. |
| **CLI_CFG_INTRO_PROJECT_NAME** 		| Project name string. Part of intro string. |
| **CLI_CFG_INTRO_SW_VER** 				| Software version. Part of intro string. |
| **CLI_CFG_INTRO_HW_VER** 				| Hardware version. Part of intro string. |
| **CLI_CFG_INTRO_PROJ_INFO** 			| Project detailed info. Part of "revision" module. |
| **CLI_CFG_TERMINATION_STRING** 		| String that will be send after each "cli_printf" and "cli_printf_ch". |
| **CLI_CFG_TX_BUF_SIZE** 				| Transmitting buffer size in bytes. |
| **CLI_CFG_RX_BUF_SIZE** 				| Reception buffer size in bytes. |
| **CLI_CFG_MAX_NUM_OF_COMMANDS** 		| Maximum number of user defined commands inside single table. |
| **CLI_CFG_MAX_NUM_OF_USER_TABLES** 	| Maximum number of user define command tables. |
| **CLI_CFG_MUTEX_EN** 					| Enable/Disable usage of mutex in order to protect low level communication driver. |
| **CLI_CFG_PAR_USE_EN** 				| Enable/Disable usage of Device Parameters. |
| **CLI_CFG_HNDL_PERIOD_MS** 			| Time period of "cli_hndl()" function call in ms. (Applicable only if CLI_CFG_PAR_USE_EN=1) |
| **CLI_CFG_DEF_STREAM_PER_MS** 		| Defaulf time period of parameter streaming in ms. (Applicable only if CLI_CFG_PAR_USE_EN=1) |
| **CLI_CFG_PAR_MAX_IN_LIVE_WATCH** 	| Maximum number of parameter in streaming list. (Applicable only if CLI_CFG_PAR_USE_EN=1) |
| **CLI_CFG_STREAM_NVM_EN** 			| Enable/Disable storing streaming info to NVM. (Applicable only if CLI_CFG_PAR_USE_EN=1) |
| **CLI_CFG_NVM_REGION** 				| CLI NVM region space. (Applicable only if CLI_CFG_STREAM_NVM_EN=1) |
| **CLI_CFG_AUTO_STREAM_STORE_EN** 		| Enable/Disable automatic storing of streaming info to NVM. (Applicable only if CLI_CFG_STREAM_NVM_EN=1). If enabled streaming info will be stored after following command is executed: *status_des*, *status_start*, *status_stop* and *status_rate*. |
| **CLI_CFG_DEBUG_EN** 					| Enable/Disable debugging mode. |
| **CLI_CFG_ASSERT_EN** 				| Enable/Disable asserts. Shall be disabled in release build! |
| **CLI_ASSERT** 						| Definition of assert |

3. Configure communication channels if needed. See **Defining communication channels** chapter bellow.
4. Prepare interface files *cli_if.c* and *cli_if.h*. All examples are inside *template* folder.
5. Initilize CLI module:
	```C
	// Initialize CLI
	if ( eCLI_OK != cli_init())
	{
		// Initialization error...
		// Furhter actions here...
	}
	```
6. Register wanted cli commands. See **Registration of user command** chapter bellow.
7. Make sure to call *cli_hndl()* at fixed period (e.g. 10ms):
	```C
	// 10 ms loop
	if ( flags & SHELL_TIMER_EVENT_10_MS )
	{
		// Handle Command Line Interface
		cli_hndl();
	}
	```



### **Registration of user command**

Registration of user command is done in run-time with no pre-conditions. Maximum number of commands inside table is defined by *CLI_CFG_MAX_NUM_OF_COMMANDS* macro inside *cli_cfg.h*. By default it is set to 10, meaning that up to 10 user commands can be registered inside single table. Additionally there is also maximum number of all user table limitation. It is adjustable by *CLI_CFG_MAX_NUM_OF_USER_TABLES* macro inside *cli_cfg.h*.

Example of registration of user defined CLI command table:

```C
// User test_1 function definiton
void test_1 (const uint8_t * attr)
{
	if ( NULL != attr ) cli_printf("User command test 1... Attr: <%s>", attr);
	else                cli_printf("User command test 1... Attr: NULL");
}

// User test_2 function definiton
void test_2 (const uint8_t * attr)
{
	if ( NULL != attr ) cli_printf("User command test 2... Attr: <%s>", attr);
	else                cli_printf("User command test 2... Attr: NULL");
}

// User test_3 function definiton
void test_3 (const uint8_t * attr)
{
	if ( NULL != attr ) cli_printf("User command test 3... Attr: <%s>", attr);
	else                cli_printf("User command test 3... Attr: NULL");
}

// User test_4 function definiton
void test_4 (const uint8_t * attr)
{
	if ( NULL != attr ) cli_printf("User command test 4... Attr: <%s>", attr);
	else                cli_printf("User command test 4... Attr: NULL");
}

// Define user table
// NOTE: .num_of must be lower or equal than CLI_CFG_MAX_NUM_OF_COMMANDS!
static volatile const cli_cmd_table_t my_table =
{
	// List of commands
	.cmd =
	{
		// ----------------------------------------------------------------------
		//     name         function            help string
		// ----------------------------------------------------------------------
		{ "test_1",         test_1,             "Test 1 Help" },
		{ "test_2",         test_2,             "Test 2 Help" },
		{ "test_3",         test_3,             "Test 3 Help" },
		{ "test_4",         test_4,             "Test 4 Help" },
	},

	// Total number of listed commands
	.num_of = 4
};

void register_my_cli_commands()
{
	// Register shell commands
	cli_register_cmd_table((const cli_cmd_table_t*) &my_table );
}
```

### **Defining communication channels**

NOTICE: Change only code between ***USER CODE BEGIN*** and ***USER CODE END*** sections:

1. Define communication channels enumerations inside *cli_cfg.h*. 
	```C
	/**
	 * 		List of communication channels
	 *
	 * @note	Warning and error communication channels must
	 * 			always be present!
	 *
	 * 	@note	Change code only between "USER_CODE_BEGIN" and
	 * 			"USER_CODE_END" section!
	 */
	typedef enum
	{
		eCLI_CH_WAR = 0,		/**<Warning channel */
		eCLI_CH_ERR,			/**<Error channel */

		// USER_CODE_BEGIN

		eCLI_CH_SCP,
		eCLI_CH_SICP_SER,
		eCLI_CH_SICP,
		eCLI_CH_APP,

		// USER_CODE_END

		eCLI_CH_NUM_OF			/**<Leave unchange - Must be last! */
	} cli_ch_opt_t;
	```

2. Define channel name and default state of each comunication channel inside *cli_cfg.c*:
	```C
	/**
	* 		Communication channels names and default active
	* 		state definition
	*
	* 	@note	Change code only between "USER_CODE_BEGIN" and
	* 			"USER_CODE_END" section!
	*/
	static cli_cfg_ch_data_t g_cli_ch[eCLI_CH_NUM_OF] =
	{
		// --------------------------------------------------------------------------
		//						Name of channel				Default state of channel
		// --------------------------------------------------------------------------
		[eCLI_CH_WAR] 		= {	.name = "WARNING", 			.en = true 				},
		[eCLI_CH_ERR] 		= {	.name = "ERROR", 			.en = true 				},

		// USER_CODE_BEGIN

		[eCLI_CH_SCP] 		= {	.name = "SCP", 				.en = true 				},
		[eCLI_CH_SICP_SER] 	= {	.name = "SICP_Serial", 		.en = true 				},
		[eCLI_CH_SICP] 		= {	.name = "SICP", 			.en = true 				},
		[eCLI_CH_APP] 		= {	.name = "APP", 				.en = true 				},

		// USER_CODE_END
	};
	```

### **Using device parameters**

1. Make sure to have [Device Parameter](https://github.com/GeneralEmbeddedCLibraries/parameters) up and running. It is mandatory to use *General Embedded C Libraries Ecosystem* path for parameters module (*root/middleware/parameters/parameters/*) !

2. Enable following *CLI_CFG_PAR_USE_EN* macro by setting it to "1" inside *cli_cfg.h* file:
	```C
	/**
	 * 	Enable/Disable parameters usage
	 *
	 * 	@brief	Usage of device parameters.
	 * 			Link to repository: https://github.com/GeneralEmbeddedCLibraries/parameters
	 */
	#define CLI_CFG_PAR_USE_EN						( 1 )
	```

Now you have everything setup to use Device Parameters module in combination with CLI.


### **Storing streaming info to NVM**

Main purpose of storing streaming (live watch) info to NVM is than after power-on or reset of the device you don't have to configure streaming (live watch) config over again. Therefore if using that feature, device will boot and setup stored streaming configuration. Meaning that if device was configured to stream specific parameters for specific measurement it will start sending parameters value right after boot, without any configuration needed. This can save a lot of time when performing extensive measurements.

Streaming (or live watch) info consist of:
 - list of stream paramter enumerations
 - number of parameters in list
 - streaming period
 - streaming active flag

When CLI module is configured to store streaming info to NVM (*CLI_CFG_STREAM_NVM_EN* = 1) user must also include [NVM module](https://github.com/GeneralEmbeddedCLibraries/nvm) into project. Additionally user must define NVM region for CLI storage purposes inside *nvm_cfg.h/.c*. Name of NVM region must be pass to CLI configuration to *CLI_CFG_NVM_REGION* macro.

Code section from *nvm_cfg.h*:
```C
/**
 * 		NVM region definitions
 *
 *	@brief	User shall specified NVM regions name, start, size
 *			and pointer to low level driver.
 *
 * 	@note	Special care with start address and its size!
 */
static const nvm_region_t g_nvm_region[ eNVM_REGION_NUM_OF ] =
{
	// --------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//											Region Name						Start address			Size [byte]			Low level driver
	// --------------------------------------------------------------------------------------------------------------------------------------------------------------------

	[eNVM_REGION_EEPROM_DEV_PAR]	=	{	.name = "device parameters",	.start_addr = 0x0,		.size = 1024,		.p_driver = &g_mem_driver[ eNVM_MEM_DRV_EEPROM ]	},
	[eNVM_REGION_EEPROM_CLI]		=	{	.name = "CLI settings",			.start_addr = 0x400,	.size = 256,		.p_driver = &g_mem_driver[ eNVM_MEM_DRV_EEPROM ]	},

	// --------------------------------------------------------------------------------------------------------------------------------------------------------------------
};
```

Code section from *cli_cfg.h*:
```C
/**
 *      Enable/Disable storing streaming info to NVM
 *
 *  @note   When enabled NVM module must be part of the project!
 * 			Link to repository: https://github.com/GeneralEmbeddedCLibraries/nvm
 */
#define CLI_CFG_STREAM_NVM_EN              ( 1 )

/**
 *      NVM parameter region option
 *
 * 	@note 	User shall select region based on nvm_cfg.h region
 * 			definitions "nvm_region_name_t"
 */
#define CLI_CFG_NVM_REGION                 ( eNVM_REGION_EEPROM_CLI )
```

CLI NVM memory layout and field description is shown in picture below:
![](doc/cli_nvm_layout.png)

Additional option is to enable automatic storage of streaming info via *CLI_CFG_AUTO_STREAM_STORE_EN* macro. This feature provides refreshing of streaming info inside NVM each time it changes in RAM. Meaning that each time either of streaming info data is being change by user it re-writes new streaming configuration to NVM. This routine is triggered on execution of following CLI command:
 - *status_des*
 - *status_start*
 - *status_stop*
 - *status_rate* 

Code section from *cli_cfg.h*:
```C
/**
 *     Enable/Disable automatic storage of streaming infor to NVM
 *
 * @note   When enabled streaming info is stored on following 
 *         commands execution:
 *             - status_des
 *             - status_start
 *             - status_stop
 *			   - status_rate
*/
#define CLI_CFG_AUTO_STREAM_STORE_EN      ( 1 )
```

