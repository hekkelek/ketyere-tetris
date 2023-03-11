/*! *******************************************************************************************************
* Copyright (c) 2018-2023 K. Sz. Horvath
*
* All rights reserved
*
* \file buttons.c
*
* \brief Layer for reading and debouncing push buttons
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

// Own include
#include "buttons.h"


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/
#define BOUNCE_PERIOD                   (50u)  //!< Button bounce time period in msec


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/
//! \brief GPIO pin descriptor
typedef struct
{
  GPIO_TypeDef* psGPIOPort;  //!< Pointer to the GPIO port 
  U16           u16GPIOPin;  //!< GPIO pin index
} S_GPIO_PIN;


//--------------------------------------------------------------------------------------------------------/
// Constants
//--------------------------------------------------------------------------------------------------------/
//! \brief Lookup table for button row GPIO pins
static const S_GPIO_PIN casRows[ BUTTONS_NUM_ROWS ] = { {BUTTON_ROW0_GPIO_Port, BUTTON_ROW0_Pin}, {BUTTON_ROW1_GPIO_Port, BUTTON_ROW1_Pin}, {BUTTON_ROW2_GPIO_Port, BUTTON_ROW2_Pin} };
//! \brief Lookup table for button column GPIO pins
static const S_GPIO_PIN casCols[ BUTTONS_NUM_COLS ] = { {BUTTON_COL0_GPIO_Port, BUTTON_COL0_Pin}, {BUTTON_COL1_GPIO_Port, BUTTON_COL1_Pin}, {BUTTON_COL2_GPIO_Port, BUTTON_COL2_Pin} };


//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/
//! \brief States of each buttons. Organization: RowIndex*NUM_ROWS + ColIndex
volatile E_BUTTONS_STATE gaeButtonsState[ NUM_BUTTONS ];
//! \brief Button events since last check
volatile E_BUTTONS_EVENT gaeButtonsEvent[ NUM_BUTTONS ];
//! \brief Debounce timers
volatile U32 gau32ButtonsTimer[ NUM_BUTTONS ];


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
 * \brief  Initialize layer
 * \param  -
 * \return -
 *********************************************************************/
void Buttons_Init( void )
{
  U32 u32Index;
  
  memset( (void*)gau32ButtonsTimer, 0, sizeof( gau32ButtonsTimer ) );
  for( u32Index = 0u; u32Index < NUM_BUTTONS; u32Index++ )
  {
    gaeButtonsState[ u32Index ] = BUTTON_INACTIVE;
    gaeButtonsEvent[ u32Index ] = BUTTON_NOEVENT;
  }
  for( u32Index = 0u; u32Index < BUTTONS_NUM_ROWS; u32Index++ )
  {
    HAL_GPIO_WritePin( casRows[ u32Index ].psGPIOPort, casRows[ u32Index ].u16GPIOPin, GPIO_PIN_RESET );
  }
}

 /*! *******************************************************************
 * \brief  Callback from msec timer routine
 * \param  -
 * \return -
 *********************************************************************/
void Buttons_TimerIT( void )
{
  U32 u32Index;
  E_BUTTONS_STATE eState;
  static U32 u32RowIndex = 0u;
  
  // Sample columns
  for( u32Index = 0u; u32Index < BUTTONS_NUM_COLS; u32Index++ )
  {
    eState = gaeButtonsState[ u32RowIndex*BUTTONS_NUM_ROWS + u32Index ];
    if( GPIO_PIN_SET == HAL_GPIO_ReadPin( casCols[ u32Index ].psGPIOPort, casCols[ u32Index ].u16GPIOPin ) )
    {
      // The button is pressed
      if( BUTTON_INACTIVE == eState )
      {
        // Start debouncing
        gau32ButtonsTimer[ u32RowIndex*BUTTONS_NUM_ROWS + u32Index ] = HAL_GetTick() + BOUNCE_PERIOD;
        gaeButtonsState[ u32RowIndex*BUTTONS_NUM_ROWS + u32Index ] = BUTTON_BOUNCING;
      }
      else if( BUTTON_BOUNCING == eState )
      {
        if( gau32ButtonsTimer[ u32RowIndex*BUTTONS_NUM_ROWS + u32Index ] > HAL_GetTick() )
        {
          // Timer has expired
          gaeButtonsState[ u32RowIndex*BUTTONS_NUM_ROWS + u32Index ] = BUTTON_ACTIVE;
          // Set event flag
          gaeButtonsEvent[ u32RowIndex*BUTTONS_NUM_ROWS + u32Index ] = BUTTON_PRESSED;
        }
      }
      else if( BUTTON_RELEASING == eState )
      {
        // Restart debounce timer
        gau32ButtonsTimer[ u32RowIndex*BUTTONS_NUM_ROWS + u32Index ] = HAL_GetTick() + BOUNCE_PERIOD;
      }
      else if( BUTTON_ACTIVE )
      {
        // Do nothing
      }
      else  // Should not happen, fatal error
      {
        Error_Handler();
      }
    }
    else
    {
      // The button is not pressed
      if( BUTTON_INACTIVE == eState )
      {
        // Do nothing
      }
      else if( BUTTON_BOUNCING == eState )
      {
        // Restart debounce timer
        gau32ButtonsTimer[ u32RowIndex*BUTTONS_NUM_ROWS + u32Index ] = HAL_GetTick() + BOUNCE_PERIOD;
      }
      else if( BUTTON_RELEASING == eState )
      {
        if( gau32ButtonsTimer[ u32RowIndex*BUTTONS_NUM_ROWS + u32Index ] > HAL_GetTick() )
        {
          // Timer has expired
          gaeButtonsState[ u32RowIndex*BUTTONS_NUM_ROWS + u32Index ] = BUTTON_INACTIVE;
          // Set event flag
          gaeButtonsEvent[ u32RowIndex*BUTTONS_NUM_ROWS + u32Index ] = BUTTON_RELEASED;
        }
      }
      else if( BUTTON_ACTIVE )
      {
        // Start debouncing
        gau32ButtonsTimer[ u32RowIndex*BUTTONS_NUM_ROWS + u32Index ] = HAL_GetTick() + BOUNCE_PERIOD;
        gaeButtonsState[ u32RowIndex*BUTTONS_NUM_ROWS + u32Index ] = BUTTON_RELEASING;
      }
      else  // Should not happen, fatal error
      {
        Error_Handler();
      }
    }
  }
  
  // Deactivate previous row
  HAL_GPIO_WritePin( casRows[ u32RowIndex ].psGPIOPort, casRows[ u32RowIndex ].u16GPIOPin, GPIO_PIN_RESET );
  
  // Next row
  u32RowIndex++;
  if( u32RowIndex >= BUTTONS_NUM_ROWS )
  {
    u32RowIndex = 0u;
  }
  HAL_GPIO_WritePin( casRows[ u32RowIndex ].psGPIOPort, casRows[ u32RowIndex ].u16GPIOPin, GPIO_PIN_SET );
}

 /*! *******************************************************************
 * \brief  Get last event on given button
 * \param  eButton: index of the button
 * \return Button event code
 * \note   Clears the event
 *********************************************************************/
E_BUTTONS_EVENT Buttons_GetEvent( E_BUTTONS_INDEX eButton )
{
  //TODO: validate eButton
  E_BUTTONS_EVENT eReturn;
  U32 u32PRIMASK = __get_PRIMASK();  // get PRIMASK so we know interrupts were enabled or not
  __disable_irq();                   // disable interrupts

  eReturn = gaeButtonsEvent[ eButton ];
  gaeButtonsEvent[ eButton ] = BUTTON_NOEVENT;

  if( 0 == u32PRIMASK )  // re-enable interrupts only if they were enabled before
  {
    __enable_irq();
  }
  return eReturn;
}


//-----------------------------------------------< EOF >--------------------------------------------------/
