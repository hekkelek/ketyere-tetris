/*! *******************************************************************************************************
* Copyright (c) 2023 K. Sz. Horvath
*
* All rights reserved
*
* \file system.c
*
* \brief System functions and global variables
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/
#include <stdio.h>  // for snprintf
#include <string.h>  // for memset, etc.
#include "main.h"
#include "types.h"
#include "buttons.h"
#include "lcd_driver.h"
#include "display.h"

// Own include
#include "system.h"


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/
#define MENUITEM_Y_OFFSET  (14u)  //!< Y offset of the first menu item on screen
#define MENUITEM_X_OFFSET  (10u)  //!< X offset of the first menu item on screen
#define MENU_ITEMS          (4u)  //!< Number of menu items in the main menu


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/
//! \brief Runtime global variables
volatile S_RUNTIMEGLOBALS gsRuntimeGlobals;


//--------------------------------------------------------------------------------------------------------/
// Static function declarations
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Static functions
//--------------------------------------------------------------------------------------------------------/
/*! *******************************************************************
 * \brief  Draw a horizontal bar plot on the screen
 * \param  u8Y: Vertical coordinate of the bar
 * \param  u8RangeMin: Minimum value of the bar
 * \param  u8RangeMax: Maximum value of the bar
 * \param  u8Value: current value of the bar
 * \return -
 *********************************************************************/
static void BarPlot( U8 u8Y, U8 u8RangeMin, U8 u8RangeMax, U8 u8Value )
{
  U8 u8Index;
  U8 u8Bars;
  U8 au8String[ 4u ];
  
  // Minimum and maximum values
  snprintf( (char*)au8String, sizeof( au8String ), "%u", u8RangeMin );
  Display_PrintString( au8String, 0u, u8Y, TRUE );
  snprintf( (char*)au8String, sizeof( au8String ), "%u", u8RangeMax );
  Display_PrintString( au8String, 85u-(3u*8u), u8Y, TRUE );
  // Box
  Display_DrawLine( 3u*8u,       u8Y,    83u-(3u*8u), u8Y,    TRUE );
  Display_DrawLine( 3u*8u,       u8Y+7u, 83u-(3u*8u), u8Y+7u, TRUE );
  Display_DrawLine( 3u*8u,       u8Y,          3u*8u, u8Y+7u, TRUE );
  Display_DrawLine( 83u-(3u*8u), u8Y,    83u-(3u*8u), u8Y+7u, TRUE );
  // Value
  u8Bars = (36u*(U16)u8Value)/(u8RangeMax - u8RangeMin);
  for( u8Index = 3u*8u; u8Index < (3u*8u)+u8Bars; u8Index++ )
  {
    Display_DrawLine( u8Index, u8Y, u8Index, u8Y+7u, TRUE );
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
 * \brief  Initializes global variables
 * \param  -
 * \return -
 *********************************************************************/
void System_Init( void )
{
  memset( (void*)&gsRuntimeGlobals, 0, sizeof( gsRuntimeGlobals ) );
  
  // Initialize each variables in the struct to default values
  gsRuntimeGlobals.bMenuActive = FALSE;
  gsRuntimeGlobals.bBackLightActive = FALSE;
  gsRuntimeGlobals.u8LCDContrast = 0x42u;
  gsRuntimeGlobals.u8Volume = 0xFFu;  // full volume
}

 /*! *******************************************************************
 * \brief  Main cycle of system menu task
 * \param  -
 * \return TRUE, if the game can run; FALSE, if not
 *********************************************************************/
BOOL System_Cycle( void )
{
  BOOL bReturn = TRUE;
  static U8 u8MenuItem = 0u;
  static BOOL bSelected = FALSE;
  
  // Check menu button
  if( BUTTON_PRESSED == Buttons_GetEvent( BUTTON_MENU ) )
  {
    // Open/close system menu
    gsRuntimeGlobals.bMenuActive = ( FALSE == gsRuntimeGlobals.bMenuActive ) ? TRUE : FALSE;
    u8MenuItem = 0u;
    bSelected = FALSE;
  }
  
  // Draw menu only if it's activated
  if( TRUE == gsRuntimeGlobals.bMenuActive )
  {
    bReturn = FALSE;

    //NOTE: Assuming the display is cleared
    // If no menu items are selected, then draw the main menu
    if( FALSE == bSelected )
    {
      Display_PrintString( "System", 2, 2, TRUE );
      Display_DrawLine( 0, 0, 6*8+2, 0, TRUE );
      Display_DrawLine( 0, 0, 0, 12, TRUE );
      Display_DrawLine( 0, 12, 6*8+2, 12, TRUE );
      Display_DrawLine( 6*8+2, 0, 6*8+2, 12, TRUE );

      // Print menu items
      Display_PrintString( "Backlight", MENUITEM_X_OFFSET, MENUITEM_Y_OFFSET,       TRUE );
      Display_PrintString( "Volume",    MENUITEM_X_OFFSET, MENUITEM_Y_OFFSET + 8u,  TRUE );
      Display_PrintString( "Contrast",  MENUITEM_X_OFFSET, MENUITEM_Y_OFFSET + 16u, TRUE );
      Display_PrintString( "Turn off",  MENUITEM_X_OFFSET, MENUITEM_Y_OFFSET + 24u, TRUE );
      
      // Print arrow
      Display_PrintChar( 175u, 0u, MENUITEM_Y_OFFSET + 8u*u8MenuItem, TRUE );
      
      // Check up and down buttons and increase/decrease the menu item variable
      if( BUTTON_PRESSED == Buttons_GetEvent( BUTTON_UP ) )
      {
        if( u8MenuItem > 0u )
        {
          u8MenuItem--;
        }
      }
      if( BUTTON_PRESSED == Buttons_GetEvent( BUTTON_DOWN ) )
      {
        if( u8MenuItem < (MENU_ITEMS - 1u) )
        {
          u8MenuItem++;
        }
      }
      
      // Check the fire buttons to select the menu item
      if( ( BUTTON_PRESSED == Buttons_GetEvent( BUTTON_FIRE_A ) )
       || ( BUTTON_PRESSED == Buttons_GetEvent( BUTTON_FIRE_B ) ) )
      {
        bSelected = TRUE;
      }
    }
    else  // The menu item is selected
    {
      if( 0u == u8MenuItem )  // Backlight on/off
      {
        // Toggle backlight
        if( FALSE == gsRuntimeGlobals.bBackLightActive )
        {
          // Turn on backlight
          HAL_GPIO_WritePin( LCD_BACKLIGHT_GPIO_Port, LCD_BACKLIGHT_Pin, GPIO_PIN_SET );
          gsRuntimeGlobals.bBackLightActive = TRUE;
        }
        else
        {
          // Turn off backlight
          HAL_GPIO_WritePin( LCD_BACKLIGHT_GPIO_Port, LCD_BACKLIGHT_Pin, GPIO_PIN_RESET );
          gsRuntimeGlobals.bBackLightActive = FALSE;
        }
        bSelected = FALSE;  // back to main menu
      }
      else if( 1u == u8MenuItem )  // Volume
      {
        // Display header
        Display_PrintString( "Volume", 2, 2, TRUE );
        Display_DrawLine( 0, 0, 6*8+2, 0, TRUE );
        Display_DrawLine( 0, 0, 0, 12, TRUE );
        Display_DrawLine( 0, 12, 6*8+2, 12, TRUE );
        Display_DrawLine( 6*8+2, 0, 6*8+2, 12, TRUE );
        // Show bar
        BarPlot( 25u, 0u, 255u, gsRuntimeGlobals.u8Volume );
        // Increase or decrease contrast
        if( BUTTON_PRESSED == Buttons_GetEvent( BUTTON_LEFT ) )
        {
          if( gsRuntimeGlobals.u8Volume > 5u )
          {
            gsRuntimeGlobals.u8Volume -= 5u;
          }
        }
        if( BUTTON_PRESSED == Buttons_GetEvent( BUTTON_RIGHT ) )
        {
          if( gsRuntimeGlobals.u8Volume <= 250u )
          {
            gsRuntimeGlobals.u8Volume += 5u;
          }
        }
        // Exit by pressing either one of the fire buttons
        if( ( BUTTON_PRESSED == Buttons_GetEvent( BUTTON_FIRE_A ) )
         || ( BUTTON_PRESSED == Buttons_GetEvent( BUTTON_FIRE_B ) ) )
        {
          bSelected = FALSE;
        }
      }
      else if( 2u == u8MenuItem )  // Contrast
      {
        // Display header
        Display_PrintString( "Contrast", 2, 2, TRUE );
        Display_DrawLine( 0, 0, 8*8+2, 0, TRUE );
        Display_DrawLine( 0, 0, 0, 12, TRUE );
        Display_DrawLine( 0, 12, 8*8+2, 12, TRUE );
        Display_DrawLine( 8*8+2, 0, 8*8+2, 12, TRUE );
        // Show bar
        BarPlot( 25u, 0u, 127u, gsRuntimeGlobals.u8LCDContrast );
        // Increase or decrease contrast
        if( BUTTON_PRESSED == Buttons_GetEvent( BUTTON_LEFT ) )
        {
          if( gsRuntimeGlobals.u8LCDContrast > 0u )
          {
            gsRuntimeGlobals.u8LCDContrast--;
          }
        }
        if( BUTTON_PRESSED == Buttons_GetEvent( BUTTON_RIGHT ) )
        {
          if( gsRuntimeGlobals.u8LCDContrast < 127u )
          {
            gsRuntimeGlobals.u8LCDContrast++;
          }
        }
        LCD_SetContrast( gsRuntimeGlobals.u8LCDContrast );
        // Exit by pressing either one of the fire buttons
        if( ( BUTTON_PRESSED == Buttons_GetEvent( BUTTON_FIRE_A ) )
         || ( BUTTON_PRESSED == Buttons_GetEvent( BUTTON_FIRE_B ) ) )
        {
          bSelected = FALSE;
        }
      }
      else if( 3u == u8MenuItem )  // Turn off
      {
        Display_PrintString( "Bye!", 0u, 0u, TRUE );
        // Turn power off
        HAL_GPIO_WritePin( POWER_OFF_GPIO_Port, POWER_OFF_Pin, GPIO_PIN_SET );
      }
      else  // this should not happen
      {
        // Error handling
        bSelected = FALSE;  // back to main menu
      }
    }
  }
  return bReturn;
}

/*! *******************************************************************
 * \brief
 * \param
 * \return
 *********************************************************************/



//-----------------------------------------------< EOF >--------------------------------------------------/
