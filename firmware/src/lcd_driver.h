﻿/*! *******************************************************************************************************
* Copyright (c) 2018-2023 K. Sz. Horvath
*
* All rights reserved
*
* \file lcd_driver.h
*
* \brief SPI Nokia LCD driver
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

#ifndef LCD_DRIVER_H
#define LCD_DRIVER_H

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/
#define LCD_SIZE_X    (84u)  //!< Number of pixels per line
#define LCD_SIZE_Y    (48u)  //!< Number of lines per screen


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/
extern U8 gau8LCDFrameBuffer[ (LCD_SIZE_X * LCD_SIZE_Y)/8u ];


//--------------------------------------------------------------------------------------------------------/
// Interface functions
//--------------------------------------------------------------------------------------------------------/
void LCD_Init( void );
void LCD_Update( void );
void LCD_SetContrast( U8 u8Contrast );
void LCD_Pixel( U8 u8PosX, U8 u8PosY, BOOL bIsOn );


#endif  // LCD_DRIVER_H

//-----------------------------------------------< EOF >--------------------------------------------------/
