/*! *******************************************************************************************************
* Copyright (c) 2018-2023 K. Sz. Horvath
*
* All rights reserved
*
* \file display.c
*
* \brief Drawing routines for display
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/
#include <stdlib.h>
#include "types.h"
#include "lcd_driver.h"

// Own include
#include "display.h"


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
 * \brief  Line drawing algorithm for low gradients
 * \param  u8X0: origin X coordinate
 * \param  u8Y0: origin Y coordinate
 * \param  u8X1: destination X coordinate
 * \param  u8Y1: destination Y coordinate
 * \param  bIsOn: If TRUE: line will be set; if FALSE: line will be cleared
 * \return -
 *********************************************************************/
static void PlotLineLow( U8 u8X0, U8 u8Y0, U8 u8X1, U8 u8Y1, BOOL bIsOn )
{
  I8  i8dx, i8dy, i8yi;
  I16 i16D;
  U8  u8X, u8Y;
  
  i8dx = u8X1 - u8X0;
  i8dy = u8Y1 - u8Y0;

  i8yi = 1;
  if( i8dy < 0 )
  {
    i8yi = -1;
    i8dy = -i8dy;
  }
  i16D = ( i8dy * 2 ) - i8dx;
  
  u8Y = u8Y0;
  for( u8X = u8X0; u8X <= u8X1; u8X++ )
  {
    LCD_Pixel( u8X, u8Y, bIsOn );
    if( i16D > 0 )
    {
      u8Y += i8yi;
      i16D = i16D - ( i8dx * 2 );
    }
    i16D += ( i8dy * 2 );
  }
}

/*! *******************************************************************
 * \brief  Line drawing algorithm for high gradients
 * \param  u8X0: origin X coordinate
 * \param  u8Y0: origin Y coordinate
 * \param  u8X1: destination X coordinate
 * \param  u8Y1: destination Y coordinate
 * \param  bIsOn: If TRUE: line will be set; if FALSE: line will be cleared
 * \return -
 *********************************************************************/
static void PlotLineHigh( U8 u8X0, U8 u8Y0, U8 u8X1, U8 u8Y1, BOOL bIsOn )
{
  I8 i8dx, i8dy, i8xi;
  I16 i16D;
  U8 u8X, u8Y;
  
  i8dx = u8X1 - u8X0;
  i8dy = u8Y1 - u8Y0;

  i8xi = 1;
  if( i8dx < 0 )
  {
    i8xi = -1;
    i8dx = -i8dx;
  }
  i16D = ( i8dx * 2 ) - i8dy;
  
  u8X = u8X0;
  for( u8Y = u8Y0; u8Y <= u8Y1; u8Y++ )
  {
    LCD_Pixel( u8X, u8Y, bIsOn );
    if( i16D > 0 )
    {
      u8X += i8xi;
      i16D = i16D - ( i8dy * 2 );
    }
    i16D += ( i8dx * 2 );
  }
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
 * \brief  Draws a line on screen using Bresenham's line algorithm
 * \param  u8X0: origin X coordinate
 * \param  u8Y0: origin Y coordinate
 * \param  u8X1: destination X coordinate
 * \param  u8Y1: destination Y coordinate
 * \param  bIsOn: If TRUE: line will be set; if FALSE: line will be cleared
 * \return -
 *********************************************************************/
void Display_DrawLine( U8 u8X0, U8 u8Y0, U8 u8X1, U8 u8Y1, BOOL bIsOn )
{
  //TODO: needs some function parameter validation -- at least for debug mode
  
  // Note: this can be optimized!
  if( abs( u8Y1 - u8Y0 ) < abs( u8X1 - u8X0 ) )
  {
    if( u8X0 > u8X1 )
    {
      PlotLineLow( u8X1, u8Y1, u8X0, u8Y0, bIsOn );
    }
    else
    {
      PlotLineLow( u8X0, u8Y0, u8X1, u8Y1, bIsOn );
    }
  }
  else
  {
    if( u8Y0 > u8Y1 )
    {
      PlotLineHigh( u8X1, u8Y1, u8X0, u8Y0, bIsOn );
    }
    else
    {
      PlotLineHigh( u8X0, u8Y0, u8X1, u8Y1, bIsOn );
    }
  }
}

 /*! *******************************************************************
 * \brief
 * \param
 * \return
 *********************************************************************/



//-----------------------------------------------< EOF >--------------------------------------------------/
