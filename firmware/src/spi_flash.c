/*! *******************************************************************************************************
* Copyright (c) 2023 K. Sz. Horvath
*
* All rights reserved
*
* \file spi_flash.c
*
* \brief SPI flash memory access routines
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/
#include "types.h"
#include "main.h"

// Own include
#include "spi_flash.h"


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
// Static function declarations
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Static functions
//--------------------------------------------------------------------------------------------------------/
/*! *******************************************************************
 * \brief
 * \param
 * \return
 *********************************************************************/


//--------------------------------------------------------------------------------------------------------/
// Interface functions
//--------------------------------------------------------------------------------------------------------/
/*! *******************************************************************
 * \brief  Write flash through SPI and wait for the operation to complete
 * \param  u32StartAddress: the address to start write to
 * \param  pu8Buffer: data to write
 * \param  u32Length: buffer size to write
 * \return -
 *********************************************************************/
void SPIFlash_Write_Polling( U32 u32StartAddress, U8* pu8Buffer, U32 u32Length )
{
  U8 u8SPITxData = 0xFFu;
  U8 u8SPIRxData;
  U32 u32Index;
  static volatile U32 u32Flags;
  
  // Wait before everything
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );

  for( U32 u32Page = 0u; u32Page < (1u+(u32Length/256u)); u32Page++ )
  {
    // nCS low
    HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_RESET );
    
    // Send opcode
    u8SPITxData = 0x06u;  // Write enable operation
    LL_SPI_TransmitData8( SPI1, u8SPITxData );
    u32Flags = SPI1->SR;
    while( (SPI1->SR & SPI_SR_BSY) );
    u32Flags = SPI1->SR;
    u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );

    // nCS high
    HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_SET );

    // nCS low
    HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_RESET );

    // Send opcode
    u8SPITxData = 0x02u;  // Page write operation
    LL_SPI_TransmitData8( SPI1, u8SPITxData );
    u32Flags = SPI1->SR;
    while( (SPI1->SR & SPI_SR_BSY) );
    u32Flags = SPI1->SR;
    u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
    
    // Send address
    u8SPITxData = ((u32StartAddress+256u*u32Page) & 0x00FF0000u)>>16u;  // Address 23..16
    LL_SPI_TransmitData8( SPI1, u8SPITxData );
    u32Flags = SPI1->SR;
    while( (SPI1->SR & SPI_SR_BSY) );
    u32Flags = SPI1->SR;
    u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );

    u8SPITxData = ((u32StartAddress+256u*u32Page) & 0x0000FF00u)>>8u;  // Address 15..8
    LL_SPI_TransmitData8( SPI1, u8SPITxData );
    u32Flags = SPI1->SR;
    while( (SPI1->SR & SPI_SR_BSY) );
    u32Flags = SPI1->SR;
    u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );

    u8SPITxData = ((u32StartAddress+256u*u32Page) & 0xFF0000FFu)>>0u;  // Address 7..0
    LL_SPI_TransmitData8( SPI1, u8SPITxData );
    u32Flags = SPI1->SR;
    while( (SPI1->SR & SPI_SR_BSY) );
    u32Flags = SPI1->SR;
    u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
    
    // Write bytes
    for( u32Index = 0u; u32Index < (u32Length>(u32Page*256u)?256u:(u32Length%256)); u32Index++ )
    {
      u8SPITxData = pu8Buffer[ (u32Page*256u) + u32Index ];
      LL_SPI_TransmitData8( SPI1, u8SPITxData );
      u32Flags = SPI1->SR;
      while( (SPI1->SR & SPI_SR_BSY) );
      u32Flags = SPI1->SR;
      u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
    }

    // Read status and wait until write page is finished
    u8SPIRxData = 0x01u;  // busy flag
    while( u8SPIRxData & 0x01u )
    {
      // nCS high
      HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_SET );

      // nCS low
      HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_RESET );

      // Send opcode
      u8SPITxData = 0x05u;  // Read status register 1 operation
      LL_SPI_TransmitData8( SPI1, u8SPITxData );
      u32Flags = SPI1->SR;
      while( (SPI1->SR & SPI_SR_BSY) );
      u32Flags = SPI1->SR;
      u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );

      // Receive status register
      u8SPITxData = 0x00u;  // Don't care
      LL_SPI_TransmitData8( SPI1, u8SPITxData );
      u32Flags = SPI1->SR;
      while( (SPI1->SR & SPI_SR_BSY) );
      u32Flags = SPI1->SR;
      u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
    }
    // nCS high
    HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_SET );
  }
}

 /*! *******************************************************************
 * \brief  Whole chip erase
 * \param  -
 * \return -
 * \note   Takes a long time (>30 sec)
 *********************************************************************/
void SPIFlash_ChipErase_Polling( void )
{
  U8 u8SPITxData = 0xFFu;
  U8 u8SPIRxData;
  static volatile U32 u32Flags;
  
  // Wait before everything
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );

  // nCS low
  HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_RESET );
  
  // Send opcode
  u8SPITxData = 0x06u;  // Write enable operation
  LL_SPI_TransmitData8( SPI1, u8SPITxData );
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );
  u32Flags = SPI1->SR;
  u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
  
  // nCS high
  HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_SET );
  
  // nCS low
  HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_RESET );
  
  // Send opcode
  u8SPITxData = 0x60u;  // Chip erase operation
  LL_SPI_TransmitData8( SPI1, u8SPITxData );
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );
  u32Flags = SPI1->SR;
  u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
  
  // Read status and wait until erase is finished
  u8SPIRxData = 0x01u;  // busy flag
  while( u8SPIRxData & 0x01u )
  {
    // nCS high
    HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_SET );
    
    // nCS low
    HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_RESET );
    
    // Send opcode
    u8SPITxData = 0x05u;  // Read status register 1 operation
    LL_SPI_TransmitData8( SPI1, u8SPITxData );
    u32Flags = SPI1->SR;
    while( (SPI1->SR & SPI_SR_BSY) );
    u32Flags = SPI1->SR;
    u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
    
    // Receive status register
    u8SPITxData = 0x00u;  // Don't care
    LL_SPI_TransmitData8( SPI1, u8SPITxData );
    u32Flags = SPI1->SR;
    while( (SPI1->SR & SPI_SR_BSY) );
    u32Flags = SPI1->SR;
    u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
  }
  // nCS high
  HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_SET );
}

 /*! *******************************************************************
 * \brief  Erase 64 kBytes sector
 * \param  u32SectorAddress: beginning of the 64k sector
 * \return -
 *********************************************************************/
void SPIFlash_EraseSector_Polling( U32 u32SectorAddress )
{
  U8 u8SPITxData = 0xFFu;
  U8 u8SPIRxData;
  static volatile U32 u32Flags;
  
  // Wait before everything
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );

  // nCS low
  HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_RESET );
  
  // Send opcode
  u8SPITxData = 0x06u;  // Write enable operation
  LL_SPI_TransmitData8( SPI1, u8SPITxData );
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );
  u32Flags = SPI1->SR;
  u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
  
  // nCS high
  HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_SET );
  
  // nCS low
  HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_RESET );
  
  // Send opcode
  u8SPITxData = 0xD8u;  // 64kB sector erase operation
  LL_SPI_TransmitData8( SPI1, u8SPITxData );
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );
  u32Flags = SPI1->SR;
  u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
  
  // Send address
  u8SPITxData = (u32SectorAddress & 0x00FF0000u)>>16u;  // Address 23..16
  LL_SPI_TransmitData8( SPI1, u8SPITxData );
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );
  u32Flags = SPI1->SR;
  u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );

  u8SPITxData = (u32SectorAddress & 0x0000FF00u)>>8u;  // Address 15..8
  LL_SPI_TransmitData8( SPI1, u8SPITxData );
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );
  u32Flags = SPI1->SR;
  u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );

  u8SPITxData = (u32SectorAddress & 0xFF0000FFu)>>0u;  // Address 7..0
  LL_SPI_TransmitData8( SPI1, u8SPITxData );
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );
  u32Flags = SPI1->SR;
  u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
  
  // Read status and wait until erase is finished
  u8SPIRxData = 0x01u;  // busy flag
  while( u8SPIRxData & 0x01u )
  {
    // nCS high
    HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_SET );
    
    // nCS low
    HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_RESET );
    
    // Send opcode
    u8SPITxData = 0x05u;  // Read status register 1 operation
    LL_SPI_TransmitData8( SPI1, u8SPITxData );
    u32Flags = SPI1->SR;
    while( (SPI1->SR & SPI_SR_BSY) );
    u32Flags = SPI1->SR;
    u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
    
    // Receive status register
    u8SPITxData = 0x00u;  // Don't care
    LL_SPI_TransmitData8( SPI1, u8SPITxData );
    u32Flags = SPI1->SR;
    while( (SPI1->SR & SPI_SR_BSY) );
    u32Flags = SPI1->SR;
    u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
  }
  // nCS high
  HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_SET );
}

 /*! *******************************************************************
 * \brief  Read data from SPI flash
 * \param  u32StartAddress: address to start reading from
 * \param  pu8Buffer: buffer to read to
 * \param  u32Length: buffer size
 * \return -
 *********************************************************************/
void SPIFlash_Read_Polling( U32 u32StartAddress, U8* pu8Buffer, U32 u32Length )
{
  U8 u8SPITxData = 0xFFu;
  U8 u8SPIRxData;
  U32 u32Index;
  static volatile U32 u32Flags;
  
  // Wait before everything
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );

  // nCS low
  HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_RESET );
  
  // Send opcode
  u8SPITxData = 0x03u;  // Read operation
  LL_SPI_TransmitData8( SPI1, u8SPITxData );
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );
  u32Flags = SPI1->SR;
  u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
  
  // Send address
  u8SPITxData = (u32StartAddress & 0x00FF0000u)>>16u;  // Address 23..16
  LL_SPI_TransmitData8( SPI1, u8SPITxData );
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );
  u32Flags = SPI1->SR;
  u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );

  u8SPITxData = (u32StartAddress & 0x0000FF00u)>>8u;  // Address 15..8
  LL_SPI_TransmitData8( SPI1, u8SPITxData );
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );
  u32Flags = SPI1->SR;
  u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );

  u8SPITxData = (u32StartAddress & 0xFF0000FFu)>>0u;  // Address 7..0
  LL_SPI_TransmitData8( SPI1, u8SPITxData );
  u32Flags = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );
  u32Flags = SPI1->SR;
  u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
  
  // Read bytes
  for( u32Index = 0u; u32Index < u32Length; u32Index++ )
  {
    u8SPITxData = 0x00u;  // Don't care
    LL_SPI_TransmitData8( SPI1, u8SPITxData );
    u32Flags = SPI1->SR;
    while( (SPI1->SR & SPI_SR_BSY) );
    u32Flags = SPI1->SR;
    u8SPIRxData = LL_SPI_ReceiveData8( SPI1 );
    
    pu8Buffer[ u32Index ] = u8SPIRxData;
  }
  // nCS high
  HAL_GPIO_WritePin( FLASH_nCS_GPIO_Port, FLASH_nCS_Pin, GPIO_PIN_SET );
}

 /*! *******************************************************************
 * \brief
 * \param
 * \return
 *********************************************************************/


//-----------------------------------------------< EOF >--------------------------------------------------/
