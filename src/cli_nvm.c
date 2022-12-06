// Copyright (c) 2022 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      cli_nvm.c
*@brief     Command Line Interface NVM storage
*@author    Ziga Miklosic
*@date      06.12.2022
*@version   V1.1.0
*/
////////////////////////////////////////////////////////////////////////////////
/*!
* @addtogroup CLI_NVM
* @{ <!-- BEGIN GROUP -->
*
* @pre		NVM module shall have memory region called "CLI".
*
* 			NVM module must be initialized before calling any of following
* 			functions.
*
*
* @brief	This module is responsible for storing CLI streaming info to NVM
*
* 			CLI storage is reserved in "CLI" region of NVM. Look
* 			at the nvm_cfg.h/c module for NVM region descriptions.
*
*			CLI streaming info are stored into NVM in little endianness format.
*
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Includes
////////////////////////////////////////////////////////////////////////////////
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "cli_nvm.h"

#if ( 1 == CLI_CFG_PAR_USE_EN )

	#include "middleware/nvm/nvm/src/nvm.h"

	/**
	 * 	Check NVM module compatibility
	 */
	_Static_assert( 1 == NVM_VER_MAJOR );
	_Static_assert( 0 == NVM_VER_MINOR );

#endif

////////////////////////////////////////////////////////////////////////////////
// Definitions
////////////////////////////////////////////////////////////////////////////////


/**
 * 	CLI NVM signature and size in bytes
 */
#define CLI_NVM_SIGN							( 0xFF00AA55 )
#define CLI_NVM_SIGN_SIZE						( 4UL )

/**
 * 	CLI NVM streaming period size
 *
 * 	Unit: byte
 */
#define CLI_NVM_STREAM_PERIOD_SIZE				( 4UL )

/**
 * 	CLI NVM number of streaming parameters size
 *
 * 	Unit: byte
 */
#define CLI_NVM_NUMBER_OF_SIZE					( 1UL )

/**
 * 	CLI NVM streaming active flag
 *
 * 	Unit: byte
 */
#define CLI_NVM_STREAM_ACTIVE_SIZE				( 1UL )

/**
 * 	CLI CRC size
 *
 * 	Unit: byte
 */
#define CLI_NVM_CRC_SIZE						( 2UL )

/**
 * 	CLI NVM header content address start
 *
 * 	@note 	This is offset to reserved NVM region. For absolute address
 * 			add that value to NVM start region.
 */
#define CLI_NVM_HEAD_ADDR_OFFSET                ( 0x00 )
#define CLI_NVM_HEAD_SIGN_ADDR					( CLI_NVM_HEAD_ADDR_OFFSET )
#define CLI_NVM_HEAD_STREAM_PERIOD_ADDR			( CLI_NVM_HEAD_SIGN_ADDR            + CLI_NVM_SIGN_SIZE             )
#define CLI_NVM_HEAD_NUMBER_OF_ADDR				( CLI_NVM_HEAD_STREAM_PERIOD_ADDR 	+ CLI_NVM_STREAM_PERIOD_SIZE 	)
#define CLI_NVM_HEAD_STREAM_ACTIVE_ADDR			( CLI_NVM_HEAD_NUMBER_OF_ADDR 		+ CLI_NVM_NUMBER_OF_SIZE 		)
#define CLI_NVM_HEAD_CRC_ADDR					( CLI_NVM_HEAD_STREAM_ACTIVE_ADDR 	+ CLI_NVM_STREAM_ACTIVE_SIZE 	)

/**
 * 	Parameters first data object start address
 *
 * 	Unit: byte
 */
#define CLI_NVM_FIRST_STREAM_PAR_ADDR			( CLI_NVM_HEAD_ADDR_OFFSET + 0x10 )

/**
 * 	Parameter NVM header object
 */
typedef struct
{
    uint32_t    sign;               /**<Signature */
    uint32_t    stream_period;		/**<Period of streaming in ms */
    uint8_t     par_nb;             /**<Number of parameters inside live watch */
    uint8_t     active;             /**<Active flag */
    uint16_t    crc;                /**<Header CRC */
} cli_nvm_head_obj_t;


////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static cli_status_t	cli_nvm_corrupt_signature			(void);
static cli_status_t	cli_nvm_write_signature				(void);
static cli_status_t cli_nvm_read_header					(cli_nvm_head_obj_t * const p_head_obj);
static cli_status_t cli_nvm_write_header				(const cli_nvm_head_obj_t * const p_head_obj);
static cli_status_t cli_nvm_validate_header				(void);
static uint16_t 	cli_nvm_calc_crc					(const uint8_t * const p_data, const uint8_t size);

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////
    
////////////////////////////////////////////////////////////////////////////////
/**
*		Corrupt parameter signature to NVM
*
* @brief	Return eCLI_OK if signature corrupted OK. In case of NVM error it returns
* 			eCLI_ERROR_NVM.
*
* @return		status 	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static cli_status_t	cli_nvm_corrupt_signature(void)
{
    cli_status_t status = eCLI_OK;

    if ( eNVM_OK != nvm_erase( CLI_CFG_NVM_REGION, CLI_NVM_HEAD_SIGN_ADDR, CLI_NVM_SIGN_SIZE ))
    {
        status = eCLI_ERROR_NVM;
        cli_printf( "CLI_NVM: NVM error during signature corruption!" );
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*		Write correct signature
*
* @brief	Return eCLI_OK if signature corrupted OK. In case of NVM error it returns
* 			eCLI_ERROR_NVM.
*
* @return		status 	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static cli_status_t	cli_nvm_write_signature(void)
{
            cli_status_t    status  = eCLI_OK;
    const   uint32_t        sign    = CLI_NVM_SIGN;

    if ( eNVM_OK != nvm_write( CLI_CFG_NVM_REGION, CLI_NVM_HEAD_SIGN_ADDR, CLI_NVM_SIGN_SIZE, (uint8_t*) &sign ))
    {
        status = eCLI_ERROR_NVM;
        cli_printf( "PAR_NVM: NVM error during signature write!" );
    }

    return status;
}


static cli_status_t cli_nvm_read_header(cli_nvm_head_obj_t * const p_head_obj)
{
    cli_status_t status = eCLI_OK;


    return status;
}


static cli_status_t cli_nvm_write_header(const cli_nvm_head_obj_t * const p_head_obj)
{
    cli_status_t status = eCLI_OK;


    return status;
}


static cli_status_t cli_nvm_validate_header(void)
{
    cli_status_t status = eCLI_OK;


    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*		Calculate CRC-16
*
* @param[in]	p_data	- Pointer to data
* @param[in]	size	- Size of data to calc crc
* @return		crc16	- Calculated CRC
*/
////////////////////////////////////////////////////////////////////////////////
static uint16_t cli_nvm_calc_crc(const uint8_t * const p_data, const uint8_t size)
{
    const 	uint16_t poly 	= 0x1021U;	// CRC-16-CCITT
    const 	uint16_t seed 	= 0x1234U;	// Custom seed
            uint16_t crc16 	= seed;

    // Check input
    CLI_ASSERT( NULL != p_data );
    CLI_ASSERT( size > 0 );

    for (uint8_t i = 0; i < size; i++)
    {
        crc16 = ( crc16 ^ ( p_data[i] << 8U ));

        for (uint8_t j = 0U; j < 8U; j++)
        {
            if (crc16 & 0x8000)
            {
                crc16 = (( crc16 << 1U ) ^ poly );
            }
            else
            {
                crc16 = ( crc16 << 1U );
            }
        }
    }

    return crc16;
}

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*@addtogroup CLI_NVM_API
* @{ <!-- BEGIN GROUP -->
*
* 	Following function are part of CLI NVM API.
*/
////////////////////////////////////////////////////////////////////////////////

#if ( 1 == CLI_CFG_PAR_USE_EN )

    ////////////////////////////////////////////////////////////////////////////////
    /*!
    * @brief        Read streaming (live watch) info
    *
    * @note     This function returns
    *               - eCLI_ERROR_NVM: in case of NVM read/write error
    *               - eCLI_ERROR:   in case streaming info are corrupted or not 
    *                               jet written to NVM
    *
    * @param[out]   p_watch_info    - Pointer to read streaming data from NVM
    * @return       status          - Status of initialization
    */
    ////////////////////////////////////////////////////////////////////////////////
    cli_status_t cli_nvm_read(cli_live_watch_t * const p_watch_info)
    {
        cli_status_t status = eCLI_OK;

        CLI_ASSERT( NULL != p_watch_info );



        return status;  
    }

    ////////////////////////////////////////////////////////////////////////////////
    /*!
    * @brief        Write streaming (live watch) info
    *
    * @note     This function returns
    *               - eCLI_ERROR_NVM: in case of NVM read/write error
    *
    * @param[in]    p_watch_info    - Pointer to streaming data
    * @return       status          - Status of initialization
    */
    ////////////////////////////////////////////////////////////////////////////////
    cli_status_t cli_nvm_write(const cli_live_watch_t * const p_watch_info)
    {
        cli_status_t status = eCLI_OK;

        CLI_ASSERT( NULL != p_watch_info );



        return status;    
    }

#endif

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
