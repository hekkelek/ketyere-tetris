/*! *******************************************************************************************************
* Copyright (c) 2018-2023 K. Sz. Horvath
*
* All rights reserved
*
* \file template.c
*
* \brief ASDASDASDASDASDASDASDASDASD
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/
#include <string.h>  // for memset, etc.
#include "main.h"
#include "i2s.h"
#include "types.h"
#include "system.h"
#include "sound_synth.h"

// Own include
#include "sound.h"


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/
#define SOUND_BUFFER_SIZE  (512u)  //!< Sound buffer size in words


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/
//! \brief Sound output buffer
//! \note  Stereo sound, so even words are left samples and odd samples are right samples.
volatile I16 gi16SoundBuffer[ SOUND_BUFFER_SIZE ];


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

 /*! *******************************************************************
 * \brief
 * \param
 * \return
 *********************************************************************/


//--------------------------------------------------------------------------------------------------------/
// Interface functions
//--------------------------------------------------------------------------------------------------------/
/*! *******************************************************************
 * \brief  Initialize sound interface
 * \param  -
 * \return -
 *********************************************************************/
void Sound_Init( void )
{
  // Initialize sound buffer
  memset( (void*)gi16SoundBuffer, 0, sizeof( gi16SoundBuffer ) );
  
  // Initialize the synthesiser
  SoundSynth_Init();
  
  // Start sound output
  HAL_I2S_Transmit_DMA( &hi2s2, (U16*)gi16SoundBuffer, sizeof(gi16SoundBuffer)/sizeof(U16) );
  
  // Turn on internal audio amplifier
  HAL_GPIO_WritePin( AMP_SHUTDOWN_GPIO_Port, AMP_SHUTDOWN_Pin, GPIO_PIN_RESET );
}

 /*! *******************************************************************
 * \brief  Interrupt service routine for sound generation
 * \param  -
 * \return -
 *********************************************************************/
void Sound_IT( void )
{
  BOOL bLowerHalf;
  U16 u16Index, u16Offset;
  I16 i16SampleLeft, i16SampleRight;
  U32 u32DMADataIndex = hdma_spi2_tx.Instance->NDTR;
  
  bLowerHalf = ( u32DMADataIndex > (SOUND_BUFFER_SIZE/2) ) ? FALSE : TRUE;
  u16Offset = ( TRUE == bLowerHalf ) ? 0u : SOUND_BUFFER_SIZE/2u;
  
  for( u16Index = 0u; u16Index < (SOUND_BUFFER_SIZE/4u); u16Index++ )
  {
    i16SampleLeft = SoundSynth_Sample();  // Generate sample
    i16SampleRight = i16SampleLeft;  // Mono
    // Left
    gi16SoundBuffer[ u16Offset + (u16Index*2u) + 0u ] = (( (I32)i16SampleLeft*((I32)gsRuntimeGlobals.u8Volume+1) )/256u);
    // Right
    gi16SoundBuffer[ u16Offset + (u16Index*2u) + 1u ] = (( (I32)i16SampleRight*((I32)gsRuntimeGlobals.u8Volume+1) )/256u);
  }
}


//-----------------------------------------------< EOF >--------------------------------------------------/
