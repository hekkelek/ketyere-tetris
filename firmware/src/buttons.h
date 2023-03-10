/*! *******************************************************************************************************
* Copyright (c) 2018-2023 K. Sz. Horvath
*
* All rights reserved
*
* \file buttons.h
*
* \brief Layer for reading and debouncing push buttons
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

#ifndef BUTTONS_H
#define BUTTONS_H

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/
#define BUTTONS_NUM_ROWS                                 (3u)  //!< Number of rows (outputs with 1 as default output)
#define BUTTONS_NUM_COLS                                 (3u)  //!< Number of columns (inputs with pull-up resistor)
#define NUM_BUTTONS       (BUTTONS_NUM_ROWS*BUTTONS_NUM_COLS)  //!< Number of buttons present


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/
//! \brief Each push buttons have a state like this
typedef enum
{
  BUTTON_INACTIVE,  //!< The button is not pressed (debounced)
  BUTTON_BOUNCING,  //!< The button is pressed and currently bouncing
  BUTTON_ACTIVE,    //!< The button is pressed (debounced)
  BUTTON_RELEASING  //!< The button is released and currently bouncing
} E_BUTTONS_STATE;

//! \brief Index of each button
typedef enum
{
  BUTTON_DOWN = 0u,
  BUTTON_MENU,
  BUTTON_FIRE_A,
  BUTTON_LEFT,
  BUTTON_START,
  BUTTON_FIRE_B,
  BUTTON_RIGHT,
  BUTTON_UP,
  BUTTON_PHANTOM
} E_BUTTONS_INDEX;

//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/
extern volatile E_BUTTONS_STATE gaeButtonsState[ NUM_BUTTONS ];


//--------------------------------------------------------------------------------------------------------/
// Interface functions
//--------------------------------------------------------------------------------------------------------/
void Buttons_Init( void );
void Buttons_TimerIT( void );


#endif  // BUTTONS_H

//-----------------------------------------------< EOF >--------------------------------------------------/
