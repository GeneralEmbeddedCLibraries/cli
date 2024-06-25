# Changelog
All notable changes to this project/module will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project/module adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---
## V2.0.0 - xx.06.2024

### Added 
 - Added software oscilloscope functionalities
 - Added "intro" to basic command table

### Changed
 - Template interface files added RTOS mutex example
 - Complete module implementation re-work, spliting tasks by files
 - Transmit buffer is being shared between parameter and main CLI sub-component
 - User CLI command table registration API changed, added input for number of commands
 - Device parameters CLI command name changed
     - "par_print" -> "par_info"
     - Live watch commands: "status" -> "watch"

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