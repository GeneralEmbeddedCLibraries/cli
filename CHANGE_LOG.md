# Changelog
All notable changes to this project/module will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project/module adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---
## V1.3.0 - 17.02.2022

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