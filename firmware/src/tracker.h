/*! *******************************************************************************************************
* Copyright (c) 2018-2023 K. Sz. Horvath
*
* All rights reserved
*
* \file tracker.h
*
* \brief Music tracker implementation
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

#ifndef TRACKER_H
#define TRACKER_H

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/
//! \brief Tracker opcodes
typedef enum
{
  TRACKER_OPCODE_NOP = 0x00u,       //!< No operation
  TRACKER_OPCODE_KEYON,             //!< Hit a note
  TRACKER_OPCODE_KEYOFF,            //!< Release a note
  TRACKER_OPCODE_WAITMS,            //!< Wait for a given time (ms)
//  TRACKER_OPCODE_INSTRUMENTCHANGE,  //!< Change instrument to a different one
  TRACKER_OPCODE_END = 0xFFu        //!< End of track
} E_TRACKER_OPCODE;

PACKED_TYPES_BEGIN

//! \brief Tracker module header format
//! \note  Should be aligned to a half-word address!
typedef PACKED_STRUCT struct
{
  U16 u16MsPerBeat;                            //!< Timing information ("BPM")
  U16 u16MusicSheetOffset;                     //!< Music sheet offset
  U8  u8NumberOfInstruments;                   //!< Number of instruments
  U8  u8NumberOfNotes;                         //!< Number of musical notes used
} S_MODULE_HEADER;

//! \brief Tracker instruction
typedef PACKED_STRUCT struct
{
  U8  u8Channel;   //!< Index of the channel the instruction is performed at
  U8  u8OpCode;    //!< Opcode according to E_TRACKER_OPCODE
  U32 u32Operand;  //!< Operand of the instruction
} S_TRACKER_INSTRUCTION;

PACKED_TYPES_END


//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/


//--------------------------------------------------------------------------------------------------------/
// Interface functions
//--------------------------------------------------------------------------------------------------------/
void Tracker_Init( U32 u32TimeMs );
void Tracker_Play( U32 u32TimeMs );


#endif  // TRACKER_H

//-----------------------------------------------< EOF >--------------------------------------------------/
