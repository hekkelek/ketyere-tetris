/*! *******************************************************************************************************
* Copyright (c) 2018-2023 K. Sz. Horvath
*
* All rights reserved
*
* \file lcd_driver.c
*
* \brief SPI Nokia LCD driver
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/
//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/
#include <string.h>
#include "types.h"
#include "main.h"
#include "stm32f4xx_ll_spi.h"

// Own include
#include "lcd_driver.h"


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/
//! \brief Frame buffer used by this layer
U8 gau8LCDFrameBuffer[ (LCD_SIZE_X * LCD_SIZE_Y)/8u ];


//--------------------------------------------------------------------------------------------------------/
// Static function declarations
//--------------------------------------------------------------------------------------------------------/
static void Send( U8 u8Data );


//--------------------------------------------------------------------------------------------------------/
// Static functions
//--------------------------------------------------------------------------------------------------------/
/*! *******************************************************************
 * \brief  Send byte through SPI
 * \param  u8Data: data to send
 * \return -
 *********************************************************************/
static void Send( U8 u8Data )
{
  __disable_irq();
  HAL_GPIO_WritePin( LCD_CE_GPIO_Port, LCD_CE_Pin, GPIO_PIN_RESET );  // start transmission

  volatile U8 u32SR = SPI1->SR;
  LL_SPI_TransmitData8( SPI1, u8Data );
  u32SR = SPI1->SR;
  while( (SPI1->SR & SPI_SR_BSY) );
  u32SR = SPI1->SR;
  volatile U8 u8SPIRxData = *((U8*)&(SPI1->DR));
  u32SR = SPI1->SR;
  
  HAL_GPIO_WritePin( LCD_CE_GPIO_Port, LCD_CE_Pin, GPIO_PIN_SET );    // end transmission
  __enable_irq();
}

 /*! *******************************************************************
 * \brief
 * \param
 * \return
 *********************************************************************/


//--------------------------------------------------------------------------------------------------------/
// Interface functions
//--------------------------------------------------------------------------------------------------------/
/*! *******************************************************************
 * \brief  Initialize LCD hardware
 * \param  -
 * \return -
 *********************************************************************/
void LCD_Init( void )
{
  memset( gau8LCDFrameBuffer, 0, sizeof( gau8LCDFrameBuffer ) );
  HAL_GPIO_WritePin( LCD_CE_GPIO_Port, LCD_CE_Pin, GPIO_PIN_SET );    // default state
  HAL_GPIO_WritePin( LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET );  // command
  
  Send( 0x21u );  // power on, enable extended command set
  Send( 0x13u );  // set bias to 1:48
  Send( 0xC2u );  // Vop = 7 V
  Send( 0x20u );  // back to normal command set
  Send( 0x0Cu );  // set normal video mode
}

/*! *******************************************************************
 * \brief  Update the contents of the screen using the framebuffer data
 * \param  -
 * \return -
 *********************************************************************/
void LCD_Update( void )
{
  HAL_GPIO_WritePin( LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET );  // command
  Send( 0x40u );  // set y address to 0
  Send( 0x80u );  // set x address to 0
  HAL_GPIO_WritePin( LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_SET );  // data
  for( U16 u16Index = 0u; u16Index < sizeof( gau8LCDFrameBuffer ); u16Index++ )
  {
    Send( gau8LCDFrameBuffer[ u16Index ] );
  }
}

/*! *******************************************************************
 * \brief  Set contrast of the screen
 * \param  u8Contrast: contrast value (0..127)
 * \return -
 *********************************************************************/
void LCD_SetContrast( U8 u8Contrast )
{
  HAL_GPIO_WritePin( LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET );  // command
  Send( 0x21u );  // power on, enable extended command set
  Send( 0x80u | ( u8Contrast & 0x7Fu ) );  // set Vop
  Send( 0x20u );  // back to normal command set
}

/*! *******************************************************************
 * \brief  Draw a pixel at given coordinates
 * \param  u8PosX: coordinate X
 * \param  u8PosY: coordinate Y
 * \param  bIsOn: if TRUE then the pixel is set to be dark; otherwise it becomes transparent
 * \return -
 *********************************************************************/
void LCD_Pixel( U8 u8PosX, U8 u8PosY, BOOL bIsOn )
{
  // Inside, the LCD display uses a totally different addressing scheme
  // 1 byte corresponds to an 8-pixel tall block
  // So the display assumes that the screen is 84 byte wide and 5 line tall
  if( ( u8PosX < LCD_SIZE_X ) && ( u8PosY < LCD_SIZE_Y ) )
  {
    if( TRUE == bIsOn )
    {
      gau8LCDFrameBuffer[ u8PosX + ( LCD_SIZE_X*(u8PosY>>3u) ) ] |= 0x01u<<(u8PosY & 0x07u);
    }
    else
    {
      gau8LCDFrameBuffer[ u8PosX + ( LCD_SIZE_X*(u8PosY>>3u) ) ] &= ~( 0x01u<<(u8PosY & 0x07u) );
    }
  }
}

//-----------------------------------------------< EOF >--------------------------------------------------/
