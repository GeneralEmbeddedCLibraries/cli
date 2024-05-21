// Copyright (c) 2023 Ziga Miklosic
// All Rights Reserved
// This software is under MIT licence (https://opensource.org/licenses/MIT)
////////////////////////////////////////////////////////////////////////////////
/**
*@file      cli_nvm.c
*@brief     Command Line Interface NVM storage
*@author    Ziga Miklosic
*@email     ziga.miklosic@gmail.com
*@date      17.02.2023
*@version   V1.3.0
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
#include <string.h>

#include "cli_nvm.h"

#if ( 1 == CLI_CFG_STREAM_NVM_EN )

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
#define CLI_NVM_HEAD_ADDR                       ( 0x00 )
#define CLI_NVM_HEAD_SIGN_ADDR					( CLI_NVM_HEAD_ADDR )
#define CLI_NVM_HEAD_STREAM_PERIOD_ADDR			( CLI_NVM_HEAD_SIGN_ADDR            + CLI_NVM_SIGN_SIZE             )
#define CLI_NVM_HEAD_NUMBER_OF_ADDR				( CLI_NVM_HEAD_STREAM_PERIOD_ADDR 	+ CLI_NVM_STREAM_PERIOD_SIZE 	)
#define CLI_NVM_HEAD_STREAM_ACTIVE_ADDR			( CLI_NVM_HEAD_NUMBER_OF_ADDR 		+ CLI_NVM_NUMBER_OF_SIZE 		)
#define CLI_NVM_HEAD_CRC_ADDR					( CLI_NVM_HEAD_STREAM_ACTIVE_ADDR 	+ CLI_NVM_STREAM_ACTIVE_SIZE 	)

/**
 * 	Parameters first data object start address
 *
 * 	Unit: byte
 */
#define CLI_NVM_FIRST_STREAM_PAR_ADDR			( CLI_NVM_HEAD_ADDR + 0x10 )

/**
 * 	Parameter NVM header object
 */
typedef struct
{
    uint32_t    sign;               /**<Signature */
    uint32_t    stream_period;		/**<Period of streaming in ms */
    uint8_t     num_of;             /**<Number of parameters inside live watch */
    uint8_t     active;             /**<Active flag */
    uint16_t    crc;                /**<Header CRC */
} cli_nvm_head_obj_t;

////////////////////////////////////////////////////////////////////////////////
// Function prototypes
////////////////////////////////////////////////////////////////////////////////
static cli_status_t	cli_nvm_erase_signature (void);
static cli_status_t	cli_nvm_write_signature	(void);
static cli_status_t cli_nvm_read_header		(cli_nvm_head_obj_t * const p_head_obj);
static cli_status_t cli_nvm_write_header	(const cli_nvm_head_obj_t * const p_head_obj);
static cli_status_t cli_nvm_read_par_list   (uint16_t * const p_par_list);
static cli_status_t cli_nvm_write_par_list  (const uint16_t * const p_par_list);
static uint16_t 	cli_nvm_calc_crc		(const uint8_t * const p_data, const uint8_t size);
static uint16_t     cli_nvm_calc_crc_whole  (const cli_nvm_head_obj_t * const p_header, const uint16_t * const p_par_list);
static cli_status_t cli_nvm_sync            (void);

////////////////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// Functions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/**
*		Erase signature
*
* @note     Function returns
*           - eCLI_OK: if signature is erased ok
*           - eCLI_ERROR_NVM: if NVM interface error
*
* @return		status 	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static cli_status_t	cli_nvm_erase_signature(void)
{
    cli_status_t status = eCLI_OK;

    // Erase signature
    if ( eNVM_OK != nvm_erase( CLI_CFG_NVM_REGION, CLI_NVM_HEAD_SIGN_ADDR, CLI_NVM_SIGN_SIZE ))
    {
        status = eCLI_ERROR_NVM;
        cli_printf( "ERR, CLI NVM error during signature corruption!" );
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*		Write correct signature
*
* @note     Function returns
*           - eCLI_OK: if signature is written ok
*           - eCLI_ERROR_NVM: if NVM interface error
*
* @return		status 	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static cli_status_t	cli_nvm_write_signature(void)
{
            cli_status_t    status  = eCLI_OK;
    const   uint32_t        sign    = CLI_NVM_SIGN;

    // Write signature
    if ( eNVM_OK != nvm_write( CLI_CFG_NVM_REGION, CLI_NVM_HEAD_SIGN_ADDR, CLI_NVM_SIGN_SIZE, (uint8_t*) &sign ))
    {
        status = eCLI_ERROR_NVM;
        cli_printf( "ERR, CLI NVM error during signature write!" );
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*		Read header
*
* @note     Function returns
*           - eCLI_OK: if header is read ok
*           - eCLI_ERROR_NVM: if NVM interface error
*
* @return		status 	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static cli_status_t cli_nvm_read_header(cli_nvm_head_obj_t * const p_head_obj)
{
    cli_status_t status = eCLI_OK;

    // Read signature
    if ( eNVM_OK != nvm_read( CLI_CFG_NVM_REGION, CLI_NVM_HEAD_ADDR, sizeof(cli_nvm_head_obj_t), (uint8_t*) p_head_obj ))
    {
        status = eCLI_ERROR_NVM;
        cli_printf( "ERR, CLI NVM error during header read!" );
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*		Write header
*
* @note     Function returns
*           - eCLI_OK: if header is read ok
*           - eCLI_ERROR_NVM: if NVM interface error
*
* @return		status 	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static cli_status_t cli_nvm_write_header(const cli_nvm_head_obj_t * const p_head_obj)
{
    cli_status_t status = eCLI_OK;

    // Write signature
    if ( eNVM_OK != nvm_write( CLI_CFG_NVM_REGION, CLI_NVM_HEAD_ADDR, sizeof(cli_nvm_head_obj_t), (uint8_t*) p_head_obj ))
    {
        status = eCLI_ERROR_NVM;
        cli_printf( "ERR, CLI NVM error during header write!" );
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*		Read parameter list
*
* @note     Function returns
*           - eCLI_OK: if header is read ok
*           - eCLI_ERROR_NVM: if NVM interface error
*
* @return		status 	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static cli_status_t cli_nvm_read_par_list(uint16_t * const p_par_list)
{
    cli_status_t status = eCLI_OK;

    // Read parameter list
    if ( eNVM_OK != nvm_read( CLI_CFG_NVM_REGION, CLI_NVM_FIRST_STREAM_PAR_ADDR, CLI_CFG_PAR_MAX_IN_LIVE_WATCH, (uint8_t*) p_par_list ))
    {
        status = eCLI_ERROR_NVM;
        cli_printf( "ERR, CLI NVM error during parameter list reading!" );
    }

    return status;
}

////////////////////////////////////////////////////////////////////////////////
/**
*		Write parameter list
*
* @note     Function returns
*           - eCLI_OK: if header is read ok
*           - eCLI_ERROR_NVM: if NVM interface error
*
* @return		status 	- Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static cli_status_t cli_nvm_write_par_list(const uint16_t * const p_par_list)
{
    cli_status_t status = eCLI_OK;

    // Read parameter list
    if ( eNVM_OK != nvm_write( CLI_CFG_NVM_REGION, CLI_NVM_FIRST_STREAM_PAR_ADDR, CLI_CFG_PAR_MAX_IN_LIVE_WATCH, (uint8_t*) p_par_list ))
    {
        status = eCLI_ERROR_NVM;
        cli_printf( "ERR, CLI NVM error during parameter list writing!" );
    }

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

    for ( uint8_t i = 0; i < size; i++ )
    {
        crc16 = ( crc16 ^ ( p_data[i] << 8U ));

        for  (uint8_t j = 0U; j < 8U; j++ )
        {
            if ( crc16 & 0x8000U )
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
*		Calculate CRC-16 on complete CLI NVM space
*
* @param[in]	p_data      - Pointer to NVM header
* @param[in]	p_par_list	- Pointer to parameter list
* @return		crc16       - Calculated CRC
*/
////////////////////////////////////////////////////////////////////////////////
static uint16_t cli_nvm_calc_crc_whole(const cli_nvm_head_obj_t * const p_header, const uint16_t * const p_par_list)
{
    uint16_t crc16 = 0;

    // Calculate crc over header
    // NOTE: Ignore signature & CRC in header!
    crc16 = cli_nvm_calc_crc((uint8_t*) &(p_header->stream_period), ( CLI_NVM_STREAM_PERIOD_SIZE + CLI_NVM_NUMBER_OF_SIZE + CLI_NVM_STREAM_ACTIVE_SIZE ));

    // Calculate crc over parameter list
    crc16 ^= cli_nvm_calc_crc((uint8_t*) p_par_list, CLI_CFG_PAR_MAX_IN_LIVE_WATCH );

    return crc16;
}

////////////////////////////////////////////////////////////////////////////////
/**
*		Sync NVM module
*
* @return		status - Status of operation
*/
////////////////////////////////////////////////////////////////////////////////
static cli_status_t cli_nvm_sync(void)
{
    cli_status_t status = eCLI_OK;

    if ( eNVM_OK != nvm_sync( CLI_CFG_NVM_REGION ))
    {
        status = eCLI_ERROR_NVM;
    }

    return status;
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
            cli_status_t        status                                  = eCLI_OK;
            cli_nvm_head_obj_t  header                                  = {0};
    static  uint16_t            par_list[CLI_CFG_PAR_MAX_IN_LIVE_WATCH] = {0};
            uint16_t            crc_calc                                = 0;

    CLI_ASSERT( NULL != p_watch_info );

    if ( NULL != p_watch_info )
    {
        // Read status
        status = cli_nvm_read_header( &header );

        // Header read OK
        if ( eCLI_OK == status )
        {
            // Signature OK
            if ( CLI_NVM_SIGN == header.sign )
            {
                // Read streaming parameters list
                status |= cli_nvm_read_par_list((uint16_t*) &par_list );

                // Calculate CRC
                crc_calc = cli_nvm_calc_crc_whole( &header, (uint16_t*) &par_list );

                // CRC OK
                if ( crc_calc == header.crc )
                {
                    // Return readed streaming info
                    memcpy((uint8_t*) &p_watch_info->par_list, (uint8_t*) &par_list, ( sizeof(uint16_t) * header.num_of ));
                    p_watch_info->num_of = header.num_of;
                    p_watch_info->period = header.stream_period;
                    p_watch_info->active = header.active;

                    // Calculate period counts
                    p_watch_info->period_cnt = (uint32_t) ( p_watch_info->period / CLI_CFG_HNDL_PERIOD_MS );
                }

                // CRC corrupted
                else
                {
                    status = eCLI_ERROR;
                    cli_printf( "ERR, CLI NVM CRC corrupted!" );
                }
            }

            // Signature corrupted
            else
            {
                status = eCLI_ERROR;
                cli_printf( "ERR, CLI NVM signature corrupted!" );
            }
        }
    }
    else
    {
        status = eCLI_ERROR;
    }

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
    cli_status_t        status = eCLI_OK;
    cli_nvm_head_obj_t  header = { 0 };

    CLI_ASSERT( NULL != p_watch_info );

    if ( NULL != p_watch_info )
    {
        // Corrupt signature (enter critical)
        status |= cli_nvm_erase_signature();

        // Assemble header
        header.stream_period    = p_watch_info->period;
        header.num_of           = p_watch_info->num_of;
        header.active           = p_watch_info->active;

        // Calculate CRC
        header.crc = cli_nvm_calc_crc_whole( &header, p_watch_info->par_list );

        // Write header
        status |= cli_nvm_write_header( &header );

        // Write parameters
        status |= cli_nvm_write_par_list( p_watch_info->par_list );

        // Write signature (exit critical)
        status |= cli_nvm_write_signature();

        // Sync NVM
        status |= cli_nvm_sync();
    }
    else
    {
        status = eCLI_ERROR;
    }

    return status;
}

#endif // ( 1 == CLI_CFG_STREAM_NVM_EN )

////////////////////////////////////////////////////////////////////////////////
/**
* @} <!-- END GROUP -->
*/
////////////////////////////////////////////////////////////////////////////////
