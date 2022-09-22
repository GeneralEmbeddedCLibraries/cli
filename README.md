# **Command Line Interface - CLI**

Command Line Interface (CLI) is general purpose console based interpretor independent from communication media. It purpose is to easily and quickly setup embedded device configurations and diagnosis via application defined communication channel. Only constrain for communication channel is usage od ASCII formated characters. 

CLI is build around command tables where command name, function and help message is specified. E.g.:
```C
	// ------------------------------------------------------------------------------------------
	// 	name					function				help string
	// ------------------------------------------------------------------------------------------
	{ 	"help", 				cli_help, 				"Print all commands help" 				},
	{ 	"reset", 				cli_reset, 				"Reset device" 							},
```

CLI divides two types of command tables:
 - *BASIC COMMAND TABLE*: Is compile-time defined by CLI module itself and highly depends on user configurations of module. Commands inside basic table serves for PC tool interfacing with the embedded device.
 - *USER COMMAND TABLE*: Is run-time defined list of command defined by the user application purposes.


## **Dependencies**
--- 

1. Definition of ***float32_t*** must be provided by user. In current implementation it is defined in "*project_config.h*". Just add following statement to your code where it suits the best.

```C
// Define float
typedef float float32_t;
```

2. CLI module uses "static_assert()" function defined in <assert.h>.

3. In case of using device parameters (CLI_CFG_PAR_USE_EN = 1) it is mandatory to use [Parameters module](https://github.com/GeneralEmbeddedCLibraries/parameters).

## **General Embedded C Libraries Ecosystem**
In order to be part of *General Embedded C Libraries Ecosystem* this module must be placed in following path: 
```
root/middleware/cli/cli/"module_space"
```

## **API**
---
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


### **Preparing files**
**Put all user code between sections: USER CODE BEGIN & USER CODE END!**

1. Copy template files to root directory of module.

### **Configuration setup**

### **Initialization and first run**

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
		// -----------------------------------------------------------------------------
		//     name            function            help string
		// -----------------------------------------------------------------------------
		{ "test_1",         test_1,             "Test 1 Help" },
		{ "test_2",         test_2,             "Test 2 Help" },
		{ "test_3",         test_3,             "Test 3 Help" },
		{ "test_4",         test_4,             "Test 4 Help" },
	},

	// Total number of listed commands
	.num_of = 4
};
```

### **Defining communication channels**

NOTICE: Change only code between ***USER CODE BEGIN** and ***USER CODE END*** sections:

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

1. Make sure to have [Device Parameter](https://github.com/GeneralEmbeddedCLibraries/parameters) up and running.

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




