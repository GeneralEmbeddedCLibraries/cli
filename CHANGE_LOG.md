# Changelog
All notable changes to this project/module will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project/module adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---
## V3.0.0 - 28.10.2025

### Added
 - Context to CLI comamnd function
 - Added pointer to CLI command itself in function

### Changed
 - API change: CLI command function prototype changed
 - Remove support for long long dependency as increase portability and reduces memory footprint

### Fixed


---
## V2.2.0 - 08.05.2025

### Added
 - Added support for writting/reading from arbitrary RAM location
 - Added support to retrieve device uptime
 - Added "utils" dependency 
 - Added timeout functionality for received commands

### Fixed
 - Fixed compiler warnings (implicit conversions)

---
## V2.1.0 - 21.01.2025

### Added
 - Added bootloader version to basic commands

---
## V2.0.1 - 04.08.2024

### Fixed
 - Bug in osci pre-trigger configurations due to invalid buffer indexing

---
## V2.0.0 - 28.06.2024

### Notice 
 CLI V2.0.0 is compatible with PC tool V0.4.1 or newer.

### Added 
 - Added software oscilloscope functionalities
 - Added "intro" to basic command table
 - Added RTOS mutex example to template interface file

### Changed
 - Complete module implementation re-work, spliting tasks by files
 - Transmit buffer is being shared between parameter and main CLI sub-component
 - User CLI command table registration API changed, added input for number of commands
 - Device parameters CLI command name changed
     - "par_print" -> "par_info"
     - Live watch commands: "status" -> "watch"
 - In case watch config in NVM is corrupted, then it will override with default watch config

### Removed
 - Removing unused configuration switches (CLI_CFG_LEGACY_EN, CLI_CFG_MAX_NUM_OF_COMMANDS)
 - Removed doxygen and licence files

---
## V1.3.0 - 17.02.2023

### Added 
 - Streaming configuration info
 - Added new configuration to enable/disable legacy mode (Legacy mode support CLI interface formate for PC tool up to V0.2.0)

### Changed
 - Parameter print command has different format (it is not back-compatible, therefore legacy mode config is added)

### Fixed
 - Configuration not compilable when parameters are enabled and NVM is disabled
 - CLI NVM header CRC calculation bug fix

---
## V1.2.0 - 08.12.2022

### Added
 - Configuration for default paramter streaming period
 - Streaming info storing to NVM
 - Option to automatically storing streaming infor to NVM on change

---
## V1.1.0 - 01.12.2022

### Added
 - Ability to change live watch time period. Added "status_rate" command to basic table
 - Streaming CLI commands feedback

### Fixed
 - Checking for NULL pointers at basic commands
 - Parsing inputed string termination was fixed to "\r" or "\n". Solved to search for CLI_CFG_TERMIANTION_STRING as defined in config file
 - Inifinite loop escape check

---
## V1.0.0 - 04.11.2022

### Added
 - Implementation of command line parser
 - Definition of basic command functions
 - Implementation of user command table registration during runtime
 - Implementation of communication channels
 - Device parameters support (par_set, par_get, par_print,...)
 - Live watch of parameters support
---