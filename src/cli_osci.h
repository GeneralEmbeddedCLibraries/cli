// Copyright (c) 2024 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      cli_osci.h
*@brief     Command Line Interface Osciliscope
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      04.08.2024
*@version   V2.0.1
*/
////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup CLI_OSCI_API
* @{ <!-- BEGIN GROUP -->
*
*/
////////////////////////////////////////////////////////////////////////////////

#ifndef __CLI_OSCI_H
#define __CLI_OSCI_H

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>

#include "cli.h"

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
cli_status_t    cli_osci_init       (void);
cli_status_t    cli_osci_hndl       (void);
void            cli_osci_samp_hndl  (void);

#endif // __CLI_OSCI_H

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
