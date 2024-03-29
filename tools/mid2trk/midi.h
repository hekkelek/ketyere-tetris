/*! *******************************************************************************************************
* Copyright (c) 2018-2023 K. Sz. Horvath
*
* All rights reserved
*
* \file midi.h
*
* \brief MIDI file parser
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

#ifndef MIDI_H
#define MIDI_H

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/


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
void Midi_Parse( U8* pu8MidiFile, U32 u32MidiFileLength );
void Midi_ExportTracker( FILE* psOutFile );


#endif  // MIDI_H

//-----------------------------------------------< EOF >--------------------------------------------------/
