/*! *******************************************************************************************************
* Copyright (c) 2023 K. Sz. Horvath
*
* All rights reserved
*
* \file spi_flash.h
*
* \brief SPI flash memory access routines
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

#ifndef SPI_FLASH_H
#define SPI_FLASH_H

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Interface functions
//--------------------------------------------------------------------------------------------------------/
void SPIFlash_Write_Polling( U32 u32StartAddress, U8* pu8Buffer, U32 u32Length );
void SPIFlash_ChipErase_Polling( void );
void SPIFlash_EraseSector_Polling( U32 u32SectorAddress );
void SPIFlash_Read_Polling( U32 u32StartAddress, U8* pu8Buffer, U32 u32Length );


#endif  // SPI_FLASH_H

//-----------------------------------------------< EOF >--------------------------------------------------/