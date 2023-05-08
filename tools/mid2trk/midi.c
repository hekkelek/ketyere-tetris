/*! *******************************************************************************************************
* Copyright (c) 2018-2023 K. Sz. Horvath
*
* All rights reserved
*
* \file midi.c
*
* \brief MIDI file parser
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "types.h"
#include "sound_synth.h"
#include "tracker.h"

// Own include
#include "midi.h"

//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/
#define MIDI_NOTE_OFF                        (0x80u)
#define MIDI_NOTE_OFF_MASK                   (0x0Fu)
#define MIDI_NOTE_ON                         (0x90u)
#define MIDI_NOTE_ON_MASK                    (0x0Fu)
#define MIDI_POLY_ON                         (0xA0u)
#define MIDI_POLY_ON_MASK                    (0x0Fu)
#define MIDI_CTRL_CHANGE                     (0xB0u)
#define MIDI_CTRL_CHANGE_MASK                (0x0Fu)
#define MIDI_PROG_CHANGE                     (0xC0u)
#define MIDI_PROG_CHANGE_MASK                (0x0Fu)
#define MIDI_CHANNEL_PRESSURE_CHANGE         (0xD0u)
#define MIDI_CHANNEL_PRESSURE_CHANGE_MASK    (0x0Fu)
#define MIDI_PITCH_BEND_CHANGE               (0xE0u)
#define MIDI_PITCH_BEND_CHANGE_MASK          (0x0Fu)

#define MIDI_SYSEX                           (0xF0u)
#define MIDI_SYSEX_MASK                      (0x00u)
#define MIDI_METAEVENT                       (0xFFu)
#define MIDI_METAEVENT_MASK                  (0x00u)


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/
//! \brief MIDI chunk
typedef struct
{
  U8  au8ChunkType[ 4u ];
  U32 u32ChunkLength;
  U8* pu8ChunkBody;
} S_CHUNK;


//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/
// Midi-related variables
static S_CHUNK* gapsMidiChunks;
static U32      gu32ChunkNum;
static U16      gu16Format = 0u;
static U16      gu16NumTracks = 0u;
static U16      gu16Division = 0u;
// Tracker format
static S_TRACKER_INSTRUCTION* gasTrackerInstructions = NULL;  // dynamic array
static U32                    gu32TrackerInstructions = 0u;
static U32                    gu32TimeMs = 0u;

//TODO: instruments, etc.
// Helper variables
static float gcafFreqTable[ 128u ];  //!< MIDI note -> frequency lookup table
static float gfMsPerBeat = 0.0;


//--------------------------------------------------------------------------------------------------------/
// Static function declarations
//--------------------------------------------------------------------------------------------------------/
static void GenKeyFreqTable( void );
U32 GetNotePhaseIncrease( U8 u8MIDINote );
static void ParseStream( U32 u32Chunk );
static void Track_Init( void );
static void Track_AddEvent( U8 u8Channel, E_TRACKER_OPCODE eOpCode, U32 u32Operand );


//--------------------------------------------------------------------------------------------------------/
// Static functions
//--------------------------------------------------------------------------------------------------------/
 /*! *******************************************************************
 * \brief  Generates MIDI note -- frequency lookup table
 * \param  -
 * \return -
 *********************************************************************/
void GenKeyFreqTable( void )
{
  U32 u32Index;

  // Even tuning
  for( u32Index = 0u; u32Index < 128u; u32Index++ )
  {
    if( u32Index < 9u )
    {
      gcafFreqTable[ u32Index ] = 13.75/pow( pow( 2, 1./12), ( 9u - u32Index ) % 12u );
    }
    else
    {
      gcafFreqTable[ u32Index ] = 13.75*pow(2,floor(( u32Index - 9u ) / 12u))*pow( pow( 2, 1./12), ( u32Index - 9u ) % 12u );
    }
  }
}

/*! *******************************************************************
 * \brief  Returns the phase increase value for a given MIDI note
 * \param  u8MIDINote: the given MIDI note (0...127)
 * \return Phase increase value (U16.16 fractional format)
 *********************************************************************/
U32 GetNotePhaseIncrease( U8 u8MIDINote )
{
  #warning "ˇˇEz csak 512 hosszú mintákkal megy!"
  // Sample rate / 512 == base frequency of the sample (phase increase = 65536)
  //
  return round( 512.0*65536.0*gcafFreqTable[ u8MIDINote ] / (1.0 * SAMPLE_RATE ) );
}

/*! *******************************************************************
 * \brief  Parses MIDI stream
 * \param  u32Chunk: chunk index
 * \return -
 *********************************************************************/
void ParseStream( U32 u32Chunk )
{
  U32  u32Index = 0u;
  U32  u32DeltaTime;
  U32  u32SpecialLength;
  static U32 u32LastEventTimeStamp = 0;

  gu32TimeMs = 0u;  // restart timer

  while( u32Index < gapsMidiChunks[ u32Chunk ].u32ChunkLength )
  {
    // Track event: <delta-time in variable-length format><MIDI event>
    // Delta time
    for( u32DeltaTime = gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & 0x7Fu; gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] > 0x7Fu; u32Index++ )
    {
      u32DeltaTime = u32DeltaTime<<7u;
      u32DeltaTime |= ( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+1u ] & 0x7Fu );
    }
    u32Index++;
    gu32TimeMs += u32DeltaTime*gfMsPerBeat;

    printf( "Deltatime: %u, abstime: %u\n", u32DeltaTime, gu32TimeMs );
    // MIDI event
    if( ( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & ~MIDI_NOTE_OFF_MASK ) == MIDI_NOTE_OFF )
    {
      if( u32LastEventTimeStamp != gu32TimeMs )
      {
        Track_AddEvent( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & MIDI_NOTE_OFF_MASK, TRACKER_OPCODE_WAITMS, gu32TimeMs - u32LastEventTimeStamp );
        u32LastEventTimeStamp = gu32TimeMs;
      }
      // Note off event
      printf( "Note off, channel: %u, note: %u, velocity: %u\n", gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & MIDI_NOTE_OFF_MASK, gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+1u ], gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+2u ] );
      Track_AddEvent( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & MIDI_NOTE_OFF_MASK, TRACKER_OPCODE_KEYOFF, 0u );
      u32Index++;
      u32Index++;
      u32Index++;
    }
    else if( ( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & ~MIDI_NOTE_ON_MASK ) == MIDI_NOTE_ON )
    {
      if( u32LastEventTimeStamp != gu32TimeMs )
      {
        Track_AddEvent( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & MIDI_NOTE_OFF_MASK, TRACKER_OPCODE_WAITMS, gu32TimeMs - u32LastEventTimeStamp );
        u32LastEventTimeStamp = gu32TimeMs;
      }
      // Note on event
      printf( "Note on, channel: %u, note: %u, velocity: %u\n", gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & MIDI_NOTE_ON_MASK, gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+1u ], gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+2u ] );
      Track_AddEvent( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & MIDI_NOTE_ON_MASK, TRACKER_OPCODE_KEYON, GetNotePhaseIncrease( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+1u ] ) );
      u32Index++;
      u32Index++;
      u32Index++;
    }
    else if( ( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & ~MIDI_POLY_ON_MASK ) == MIDI_POLY_ON )
    {
      // Polyphonic key pressure event
      printf( "Polyphonic key pressure, channel: %u, note: %u, velocity: %u\n", gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & MIDI_POLY_ON_MASK, gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+1u ], gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+2u ] );
      u32Index++;
      u32Index++;
      u32Index++;
    }
    else if( ( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & ~MIDI_CTRL_CHANGE_MASK ) == MIDI_CTRL_CHANGE )
    {
      // Control change event
      printf( "Control change, channel: %u, control number: %u, control value: %u\n", gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & MIDI_CTRL_CHANGE_MASK, gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+1u ], gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+2u ] );
      u32Index++;
      u32Index++;
      u32Index++;
    }
    else if( ( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & ~MIDI_PROG_CHANGE_MASK ) == MIDI_PROG_CHANGE )
    {
      // Program change event
      printf( "Program change, channel: %u, new program: %u\n", gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & MIDI_PROG_CHANGE_MASK, gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+1u ] );
      u32Index++;
      u32Index++;
    }
    else if( ( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & ~MIDI_CHANNEL_PRESSURE_CHANGE_MASK ) == MIDI_CHANNEL_PRESSURE_CHANGE )
    {
      // Channel pressure change event
      printf( "Channel pressure change, channel: %u, new pressure: %u\n", gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & MIDI_CHANNEL_PRESSURE_CHANGE_MASK, gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+1u ] );
      u32Index++;
      u32Index++;
    }
    else if( ( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & ~MIDI_PITCH_BEND_CHANGE_MASK ) == MIDI_PITCH_BEND_CHANGE )
    {
      // Pitch bend change event
      printf( "Pitch bend change, channel: %u, pitch bend: %u\n", gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & MIDI_PITCH_BEND_CHANGE_MASK, gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+1u ] + ( (U16)gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+2u ]<<7u ) );
      u32Index++;
      u32Index++;
      u32Index++;
    }
    else if( ( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & ~MIDI_SYSEX_MASK ) == MIDI_SYSEX )
    {
      printf( "Sysex message\n" );
      // Variable length
      u32Index++;
      for( u32SpecialLength = gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ]; gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] > 0x7Fu; u32Index++ )
      {
        u32SpecialLength = u32SpecialLength<<7u;
        u32SpecialLength |= ( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+1u ] & 0x7Fu );
      }
      u32Index += u32SpecialLength  + 1u;
    }
    else if( ( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] & ~MIDI_METAEVENT_MASK ) == MIDI_METAEVENT )
    {
      // FF + type + variable length + payload
      printf( "Meta event, type: %u\n", gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+1u ] );
      // Variable length
      u32Index++;
      u32Index++;
      for( u32SpecialLength = gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ]; gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] > 0x7Fu; u32Index++ )
      {
        u32SpecialLength = u32SpecialLength<<7u;
        u32SpecialLength |= ( gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index+1u ] & 0x7Fu );
      }
      u32Index += u32SpecialLength + 1u;
    }
    else
    {
      printf( "Unknown MIDI event token: 0x%x\n", gapsMidiChunks[ u32Chunk ].pu8ChunkBody[ u32Index ] );
      u32Index++;
    }
  }
}

 /*! *******************************************************************
 * \brief  Initializes own track
 * \param  -
 * \return -
 *********************************************************************/
void Track_Init( void )
{

}

 /*! *******************************************************************
 * \brief  Adds tracker event to given channel
 * \param  u8Channel:
 * \param  eOpCode:
 * \param  u32Operand:
 * \return -
 *********************************************************************/
static void Track_AddEvent( U8 u8Channel, E_TRACKER_OPCODE eOpCode, U32 u32Operand )
{
  S_TRACKER_INSTRUCTION sNewInstruction;

  if( u8Channel >= NUMBER_OF_OSCILLATORS )
  {
    printf( "Fatal error: there is no channel %u!\n", u8Channel );
    exit(-1);
  }

  sNewInstruction.u8Channel = u8Channel;
  sNewInstruction.u8OpCode = (U8)eOpCode;
  sNewInstruction.u32Operand = u32Operand;

  gu32TrackerInstructions++;

  // allocate RAM
  if( NULL != gasTrackerInstructions )
  {
    gasTrackerInstructions = realloc( gasTrackerInstructions, gu32TrackerInstructions*sizeof( S_TRACKER_INSTRUCTION ) );
  }
  else  // first call
  {
    gasTrackerInstructions = malloc( gu32TrackerInstructions*sizeof( S_TRACKER_INSTRUCTION ) );
  }

  // Write new instruction
  gasTrackerInstructions[ gu32TrackerInstructions-1u ] = sNewInstruction;
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
 * \brief  Parses MIDI file
 * \param  pu8MidiFile: pointer to the MIDI file in memory
 * \param  u32MidiFileLength: the length of the MIDI file
 * \return -
 * \note   The parsed file will be present in the memory
 *********************************************************************/
void Midi_Parse( U8* pu8MidiFile, U32 u32MidiFileLength )
{
  U8  au8ChunkType[ 4u ];
  U32 u32ChunkSize;
  U32 u32Index = 0u;
  U32 u32Time;
  U32 u32Channel;

  // Initialization
  GenKeyFreqTable();
  Track_Init();

  gu32ChunkNum = 0u;
  gapsMidiChunks = malloc( sizeof( S_CHUNK ) );

  // Search for chunks in file
  // by default, the file starts with a chunk
  while( u32Index < u32MidiFileLength )  // u32Index == byte index inside of file!
  {
    // Get information
    au8ChunkType[ 0u ] = pu8MidiFile[ u32Index + 0u ];
    au8ChunkType[ 1u ] = pu8MidiFile[ u32Index + 1u ];
    au8ChunkType[ 2u ] = pu8MidiFile[ u32Index + 2u ];
    au8ChunkType[ 3u ] = pu8MidiFile[ u32Index + 3u ];
    u32ChunkSize = ( pu8MidiFile[ u32Index + 4u ]<<24u ) + ( pu8MidiFile[ u32Index + 5u ]<<16u ) + ( pu8MidiFile[ u32Index + 6u ]<<8u ) + ( pu8MidiFile[ u32Index + 7u ]<<0u );
    // Store information
    gapsMidiChunks = realloc( gapsMidiChunks, (gu32ChunkNum+1u)*sizeof( S_CHUNK ) );
    memcpy( gapsMidiChunks[ gu32ChunkNum ].au8ChunkType, au8ChunkType, sizeof( au8ChunkType ) );
    gapsMidiChunks[ gu32ChunkNum ].u32ChunkLength = u32ChunkSize;
    gapsMidiChunks[ gu32ChunkNum ].pu8ChunkBody = &pu8MidiFile[ u32Index + sizeof( au8ChunkType ) + sizeof( u32ChunkSize ) ];
    // Increment index
    gu32ChunkNum++;
    u32Index += sizeof( au8ChunkType ) + sizeof( u32ChunkSize ) + u32ChunkSize;
  }

  // Parse chunks
  for( u32Index = 0u; u32Index < gu32ChunkNum; u32Index++ )
  {
    // Header chunk
    if( 0 == memcmp( gapsMidiChunks[ u32Index ].au8ChunkType, "MThd", sizeof( au8ChunkType ) ) )
    {
      // Headers contain 3 16-bit words
      gu16Format = ( gapsMidiChunks[ u32Index ].pu8ChunkBody[ 0u ]<<8u ) + ( gapsMidiChunks[ u32Index ].pu8ChunkBody[ 1u ]<<0u );
      gu16NumTracks = ( gapsMidiChunks[ u32Index ].pu8ChunkBody[ 2u ]<<8u ) + ( gapsMidiChunks[ u32Index ].pu8ChunkBody[ 3u ]<<0u );
      gu16Division = ( gapsMidiChunks[ u32Index ].pu8ChunkBody[ 4u ]<<8u ) + ( gapsMidiChunks[ u32Index ].pu8ChunkBody[ 5u ]<<0u );
      // Print to screen
      printf( "\nFound MIDI header:\n" );
      printf( "File format: %u\n", gu16Format );
      printf( "Number of tracks: %u\n", gu16NumTracks );
      printf( "Time division: %u ", gu16Division );
      if( gu16Division >= 32768 )
      {
        printf( "(metric time)\n" );
        printf( "Error: not implemented!" );
        exit(-1);
      }
      else  // <32768
      {
        printf( "(non-metric time)\n" );
        gfMsPerBeat = 512.0/(gu16Division);
      }
    }
    // Track chunk
    else if( 0 == memcmp( gapsMidiChunks[ u32Index ].au8ChunkType, "MTrk", sizeof( au8ChunkType ) ) )
    {
      // MIDI stream
      ParseStream( u32Index );
    }
  }
  // Add END event to signal the end of track
  Track_AddEvent( u32Index, TRACKER_OPCODE_END, 0u );
}

 /*! *******************************************************************
 * \brief  Writes out track
 * \param  psOutFile: reference to the (open) out file
 * \return -
 *********************************************************************/
void Midi_ExportTracker( FILE* psOutFile )
{
  U8  u8Channel;
  U32 u32Index;
  PACKED_TYPES_BEGIN
  S_MODULE_HEADER sModuleHeader;
  PACKED_TYPES_END

  // Put timing base
  sModuleHeader.u16MsPerBeat = gfMsPerBeat;

  // Put music notes
  //sModuleHeader.u8NumberOfNotes = gu8NumberOfNotes;

  // Put music sheet offset
//  sModuleHeader.u16MusicSheetOffset = sizeof( S_MODULE_HEADER ) + sizeof(U16)*sModuleHeader.u8NumberOfNotes + sizeof( S_INSTRUMENT ) + sizeof( sModuleInstruments.astWaveTable );
#warning "^^ Ez csak 1 hangszerrel működik!"

  #define BINARY_OUTPUT
  #ifdef BINARY_OUTPUT
  // Write header to output
  for( u32Index = 0u; u32Index < sizeof( sModuleHeader ); u32Index++ )
  {
    putc( ( (U8*)&sModuleHeader )[ u32Index ], psOutFile );
  }

/*
  // Write note lookup table
  for( u32Index = 0u; u32Index < sizeof( U16 )*gu8NumberOfNotes; u32Index++ )
  {
    putc( ( (U8*)gpu16Notes )[ u32Index ], psOutFile );
  }

  // Write instruments
  for( u32Index = 0u; u32Index < sizeof( sModuleInstruments ); u32Index++ )
  {
    putc( ( (U8*)&sModuleInstruments )[ u32Index ], psOutFile );
  }
*/
  // Write tracks
  for( u32Index = 0u; u32Index < gu32TrackerInstructions*sizeof( S_TRACKER_INSTRUCTION ); u32Index++ )
  {
    putc( ((U8*)gasTrackerInstructions)[ u32Index ], psOutFile );
  }
  #else  //C file output

  // File header
  fprintf( psOutFile, "// Generated by MID2TRK, build time: %s, %s\r\n\r\n", __DATE__, __TIME__ );

  // Defines
  fprintf( psOutFile, "#define NUMBEROF_INSTRUMENTS   (%uu) \r\n", 1u );
  fprintf( psOutFile, "\r\n" );

  //
  fprintf( psOutFile, "const U8 gau8TrackerModule[] = {\r\n" );

  // Write header to output
  for( u32Index = 0u; u32Index < sizeof( sModuleHeader ); u32Index++ )
  {
    fprintf( psOutFile, "%u, ", ( (U8*)&sModuleHeader )[ u32Index ] );
  }
  fprintf( psOutFile, "\r\n" );  // TODO: debug, comment out

  // Write note lookup table
  for( u32Index = 0u; u32Index < sizeof( U16 )*gu8NumberOfNotes; u32Index++ )
  {
    fprintf( psOutFile, "%u, ", ( (U8*)gpu16Notes )[ u32Index ] );
  }
  fprintf( psOutFile, "\r\n" );  // TODO: debug, comment out

  // Write instruments
  for( u32Index = 0u; u32Index < sizeof( sModuleInstruments ); u32Index++ )
  {
    fprintf( psOutFile, "%u, ", ( (U8*)&sModuleInstruments )[ u32Index ] );
  }
  fprintf( psOutFile, "\r\n" );  // TODO: debug, comment out

  // Write tracks
  for( u32Index = 0u; u32Index < gu32TrackLen; u32Index++ )
  {
    fprintf( psOutFile, "%u, ", gau8Track[ u32Index ] );
  }
  fprintf( psOutFile, "\r\n};\r\n" );
  fprintf( psOutFile, "\r\n" );

  //
  fprintf( psOutFile, "S_MODULE_HEADER* const gpsTrackerModule = (S_MODULE_HEADER*)gau8TrackerModule;\r\n" );
  fprintf( psOutFile, "\r\n" );

  #endif

}


//-----------------------------------------------< EOF >--------------------------------------------------/
