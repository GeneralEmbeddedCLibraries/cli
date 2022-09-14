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
| **cli_register_cmd_table** | Register user define CLI command table | cli_status_t cli_register_cmd_table (const cli_cmd_table_t * const p_cmd_table) |


## Usage


#### Preparing files
**Put all user code between sections: USER CODE BEGIN & USER CODE END!**

1. Copy template files to root directory of module.

### Configuration setup

### Initialization and first run

### Registration of user command

### Using device parameters





