/*! *******************************************************************************************************
* Copyright (c) 2023 K. Sz. Horvath
*
* All rights reserved
*
* \file sound_synth.c
*
* \brief Sound synthesiser
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/
#include <string.h>
#include "main.h"
#include "types.h"
#include "sound_wavetables.h"

// Own include
#include "sound_synth.h"


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/
//! \brief ADSR state machine state
typedef enum
{
  ADSR_ATTACK,
  ADSR_DECAY,
  ADSR_SUSTAIN,
  ADSR_RELEASE
} E_ADSR_STATE;

//! \brief Oscillator type
typedef struct
{
  U32          u32Phase;          //!< Current phase of the oscillator (U16.16 fixed-point number)
  U32          u32PhaseIncrease;  //!< Phase increase per sampling time (U16.16 fixed-point number)
  I16 const*   pi16WaveTable;     //!< Pointer to the wavetable
  U16          u16WaveTableSize;  //!< Size in words
  E_ADSR_STATE eADSRState;        //!< Current ADSR envelope section
  U16          u16ADSROutput;     //!< Output of the ADSR generator
  U32          u32ADSRTimer;      //!< ADSR timer (U3.29 fixed-point number)
  U32          u32Attack;         //!< Attack time
  U32          u32Decay;          //!< Decay time
  U16          u16Sustain;        //!< Sustain level
  U32          u32Release;        //!< Release time
} S_SYNTH_OSCILLATOR;


//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/
static S_SYNTH_OSCILLATOR gasOscillators[ NUMBER_OF_OSCILLATORS ];


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
 * \brief  Initialize synthesiser
 * \param  -
 * \return -
 *********************************************************************/
void SoundSynth_Init( void )
{
  U8 u8Index;
  
  memset( (void*)gasOscillators, 0, sizeof( gasOscillators ) );
  
  // Default: all oscillators are sine wave oscillators
  for( u8Index = 0u; u8Index < NUMBER_OF_OSCILLATORS; u8Index++ )
  {
    // Sine wave
    gasOscillators[ u8Index ].pi16WaveTable = gcai16SineWaveTable;
    gasOscillators[ u8Index ].u16WaveTableSize = sizeof( gcai16SineWaveTable )/sizeof( U16 );
    // ADSR
    gasOscillators[ u8Index ].eADSRState = ADSR_RELEASE;
    gasOscillators[ u8Index ].u32Attack = 0;
    gasOscillators[ u8Index ].u32Decay = 0;
    gasOscillators[ u8Index ].u16Sustain = 0xFFFFu;
    gasOscillators[ u8Index ].u32Release = 0;
    gasOscillators[ u8Index ].u32ADSRTimer = 0xFFFFFFFF;
    gasOscillators[ u8Index ].u16ADSROutput = 0;
  }

  //FIXME: DEBUG
  gasOscillators[ 0u ].u32PhaseIncrease = 2*334783;//(1<<16)-1;
  //
//  gasOscillators[ 0u ].eADSRState = ADSR_ATTACK;
  gasOscillators[ 0u ].u32Attack = 4410u;
  gasOscillators[ 0u ].u32Decay = 4410u;
  gasOscillators[ 0u ].u16Sustain = 0x0u;
  gasOscillators[ 0u ].u32Release = 0u;
//  gasOscillators[ 0u ].u32ADSRTimer = 0;
}

 /*! *******************************************************************
 * \brief  Generate one sound sample
 * \param  -
 * \return One sound sample
 *********************************************************************/
I16 SoundSynth_Sample( void )
{
  U8  u8Index;
  I16 i16Sample = 0;
  U32 u32Phase;
  U16 u16ADSR;
  
  // Increase the phase of each oscillators
  for( u8Index = 0u; u8Index < NUMBER_OF_OSCILLATORS; u8Index++ )
  {
    u32Phase = gasOscillators[ u8Index ].u32PhaseIncrease;
    gasOscillators[ u8Index ].u32Phase += u32Phase;
    if( gasOscillators[ u8Index ].u32Phase > ((U32)gasOscillators[ u8Index ].u16WaveTableSize<<16u) )
    {
      u32Phase = ((U32)gasOscillators[ u8Index ].u16WaveTableSize<<16u);
      gasOscillators[ u8Index ].u32Phase -= u32Phase;
    }
  }
  
  // ADSR envelope generator
  for( u8Index = 0u; u8Index < NUMBER_OF_OSCILLATORS; u8Index++ )
  {
    switch( gasOscillators[ u8Index ].eADSRState )
    {
      case ADSR_ATTACK:
        if( 0u != gasOscillators[ u8Index ].u32Attack )
        {
          u16ADSR = (U64)0xFFFFu*gasOscillators[ u8Index ].u32ADSRTimer / gasOscillators[ u8Index ].u32Attack;
        }
        else
        {
          u16ADSR = 0xFFFFu;
        }
        gasOscillators[ u8Index ].u32ADSRTimer++;
        if( gasOscillators[ u8Index ].u32ADSRTimer > gasOscillators[ u8Index ].u32Attack )
        {
          gasOscillators[ u8Index ].u32ADSRTimer = 0u;
          gasOscillators[ u8Index ].eADSRState = ADSR_DECAY;
        }
        break;
        
      case ADSR_DECAY:
        if( 0u != gasOscillators[ u8Index ].u32Decay )
        {
          u16ADSR = 0xFFFFu - (U64)(0xFFFFu - gasOscillators[ u8Index ].u16Sustain)*gasOscillators[ u8Index ].u32ADSRTimer / gasOscillators[ u8Index ].u32Decay;
        }
        else
        {
          u16ADSR = gasOscillators[ u8Index ].u16Sustain;
        }
        gasOscillators[ u8Index ].u32ADSRTimer++;
        if( gasOscillators[ u8Index ].u32ADSRTimer > gasOscillators[ u8Index ].u32Decay )
        {
          gasOscillators[ u8Index ].u32ADSRTimer = 0u;
          gasOscillators[ u8Index ].eADSRState = ADSR_SUSTAIN;
        }
        break;
        
      case ADSR_SUSTAIN:
        u16ADSR = gasOscillators[ u8Index ].u16Sustain;
        break;
        
      case ADSR_RELEASE:
        if( 0u != gasOscillators[ u8Index ].u32Release )
        {
          u16ADSR = (U64)gasOscillators[ u8Index ].u16Sustain*( gasOscillators[ u8Index ].u32Release - gasOscillators[ u8Index ].u32ADSRTimer ) / gasOscillators[ u8Index ].u32Release;
        }
        else
        {
          u16ADSR = 0u;
        }
        if( gasOscillators[ u8Index ].u32ADSRTimer < gasOscillators[ u8Index ].u32Release )
        {
          gasOscillators[ u8Index ].u32ADSRTimer++;
        }
        break;
    }
    gasOscillators[ u8Index ].u16ADSROutput = u16ADSR;
  }
  
  // Mixer
  for( u8Index = 0u; u8Index < NUMBER_OF_OSCILLATORS; u8Index++ )
  {
    u32Phase = gasOscillators[ u8Index ].u32Phase>>16u;
    u16ADSR = gasOscillators[ u8Index ].u16ADSROutput;
    i16Sample += (U32)gasOscillators[ u8Index ].pi16WaveTable[ u32Phase ]*u16ADSR / (NUMBER_OF_OSCILLATORS*65536u);
  }
  
  return i16Sample;
}

 /*! *******************************************************************
 * \brief  Press a note, and start the envelope generator
 * \param  u32NoteFreq: frequency of the note
 * \param  u8Oscillator: index of the oscillator
 * \return -
 *********************************************************************/
void SoundSynth_Press( U32 u32NoteFreq, U8 u8Oscillator )
{
  if( u8Oscillator < NUMBER_OF_OSCILLATORS )
  {
#warning "Proper note calculation!"
    __disable_irq();
    gasOscillators[ u8Oscillator ].u32PhaseIncrease = u32NoteFreq;
    gasOscillators[ u8Oscillator ].u32ADSRTimer = 0u;
    gasOscillators[ u8Oscillator ].eADSRState = ADSR_ATTACK;
    __enable_irq();
  }
}

 /*! *******************************************************************
 * \brief  Release the sound on an oscillator
 * \param  u8Oscillator: index of the oscillator
 * \return -
 *********************************************************************/
void SoundSynth_Release( U8 u8Oscillator )
{
  if( u8Oscillator < NUMBER_OF_OSCILLATORS )
  {
    __disable_irq();
    gasOscillators[ u8Oscillator ].u32ADSRTimer = 0u;
    gasOscillators[ u8Oscillator ].eADSRState = ADSR_RELEASE;
    __enable_irq();
  }
}


//-----------------------------------------------< EOF >--------------------------------------------------/
