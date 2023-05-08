/*! *******************************************************************************************************
* Copyright (c) 2018-2023 K. Sz. Horvath
*
* All rights reserved
*
* \file tracker.c
*
* \brief Music tracker implementation
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/
#include "types.h"
#include "sound_synth.h"

// Own include file
#include "tracker.h"


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/
#ifndef NUMBEROF_INSTRUMENTS
  #define NUMBEROF_INSTRUMENTS  (1u)  //!< This symbol is normally defined by the linker
#endif  // NUMBEROF_INSTRUMENTS


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/
//! \brief The tracker module, that must be linked to this project externally!
extern U8 gau8TrackerModule[];
S_MODULE_HEADER* const gpsTrackerModule = (S_MODULE_HEADER*)gau8TrackerModule;         //!< Pointer to the beginning of the music module

//! \brief Timestamp of next instruction execution
U32 gu32NextTimeCallMs;

//! \brief Index of the next instruction in the track
U32 gu32NextInstructionIdx;


//--------------------------------------------------------------------------------------------------------/
// Static function declarations
//--------------------------------------------------------------------------------------------------------/
static void ExecuteOpCode( U8 u8Channel, E_TRACKER_OPCODE eOpCode, U32 u32Operand );


//--------------------------------------------------------------------------------------------------------/
// Static functions
//--------------------------------------------------------------------------------------------------------/
/*! *******************************************************************
 * \brief  Executes the given opcode
 * \param  u8Channel: sound channel index
 * \param  eOpCode: given opcode
 * \param  u32Operand: operand to the opcode
 * \return -
 * \note   Also calculates the next opcode time
 *********************************************************************/
static void ExecuteOpCode( U8 u8Channel, E_TRACKER_OPCODE eOpCode, U32 u32Operand )
{
  switch( eOpCode )
  {
    case TRACKER_OPCODE_NOP:  // No operation
      break;
      
    case TRACKER_OPCODE_KEYON:  // Hit a note
      SoundSynth_Press( u32Operand, u8Channel );
      break;

    case TRACKER_OPCODE_KEYOFF:  // Release a note
      SoundSynth_Release( u8Channel );
      break;
      
    case TRACKER_OPCODE_WAITMS: // Wait for a given time
      gu32NextTimeCallMs += u32Operand;
      break;

    case TRACKER_OPCODE_END:  // End of track
      gu32NextInstructionIdx = 0u;
      break;

    default:  // This should not happen
      //ErrorHandler();
      break;
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
 * \brief  Initialize tracker module
 * \param  u32TimeMs: current time in ms
 * \return -
 *********************************************************************/
void Tracker_Init( U32 u32TimeMs )
{
  gu32NextInstructionIdx = 0u;
  gu32NextTimeCallMs = u32TimeMs;
}

 /*! *******************************************************************
 * \brief  Plays the song
 * \param  u32TimeMs: current time in ms
 * \return -
 *********************************************************************/
void Tracker_Play( U32 u32TimeMs )
{
  S_TRACKER_INSTRUCTION* psInstructions = (S_TRACKER_INSTRUCTION*)&(gau8TrackerModule[ sizeof( S_MODULE_HEADER ) ]);
  E_TRACKER_OPCODE eOpCode;
  U32 u32Operand;
  U8  u8Channel;
  
  while( u32TimeMs >= gu32NextTimeCallMs )
  {
    u8Channel = psInstructions[ gu32NextInstructionIdx ].u8Channel;
    eOpCode = (E_TRACKER_OPCODE)psInstructions[ gu32NextInstructionIdx ].u8OpCode;
    u32Operand = psInstructions[ gu32NextInstructionIdx ].u32Operand;
    gu32NextInstructionIdx++;
    ExecuteOpCode( u8Channel, eOpCode, u32Operand );
  }
}

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



//-----------------------------------------------< EOF >--------------------------------------------------/
