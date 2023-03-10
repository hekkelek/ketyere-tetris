/*! *******************************************************************************************************
* Copyright (c) 2018-2023 K. Sz. Horvath
*
* All rights reserved
*
* \file display.h
*
* \brief Drawing routines for display
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

#ifndef DISPLAY_H
#define DISPLAY_H

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/
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


//--------------------------------------------------------------------------------------------------------/
// Interface functions
//--------------------------------------------------------------------------------------------------------/
void Display_DrawLine( U8 u8X0, U8 u8Y0, U8 u8X1, U8 u8Y1, BOOL bIsOn );


#endif  // DISPLAY_H

//-----------------------------------------------< EOF >--------------------------------------------------/