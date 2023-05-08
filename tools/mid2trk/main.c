#include <stdio.h>
#include <stdlib.h>
#include "types.h"
#include "midi.h"

int main( int argc, char *argv[] )
{
  char  gcau8DefaultOutputFileName[] = "converterOutput.trk";
  char* au8InputFileName;
  char* au8OutputFileName;
  FILE  *psInputFile, *psOutputFile;
  U8*   pu8MidiFileInMemory;
  U32   u32MidiFileSize;
  U32   u32Index;

  printf( "MID2TRK by Hekk_Elek[Strlen]\n" );
  printf( "Build time: %s, %s\n\n", __DATE__, __TIME__ );

  if( argc < 2 )
  {
    printf( "Usage: mid2trk inputfile.mid [outputfile.trk]\n" );
    return -2;
  }
  else if( argc == 2 )  // only 1 argument
  {
    au8OutputFileName = gcau8DefaultOutputFileName;
  }
  else if( argc == 3 )  // 2 arguments
  {
    au8OutputFileName = argv[2];
  }
  else  // too many arguments
  {
    printf( "Usage: mid2trk inputfile.mid [outputfile.trk]\n" );
    return -2;
  }
  au8InputFileName = argv[1];

  printf( "Input file: %s\n", au8InputFileName );
  printf( "Output file: %s\n", au8OutputFileName );

  psInputFile = fopen( au8InputFileName, "rb" );

  if( NULL == psInputFile )
  {
    printf( "Can not open input file!\n" );
    return -1;
  }

  fseek( psInputFile, 0L, SEEK_END );
  u32MidiFileSize = ftell( psInputFile );
  rewind( psInputFile );

  pu8MidiFileInMemory = malloc( u32MidiFileSize );
  if( NULL == pu8MidiFileInMemory )
  {
    printf( "Out of memory!\n" );
    return -1;
  }

  for( u32Index = 0u; u32Index < u32MidiFileSize; u32Index++ )
  {
    pu8MidiFileInMemory[ u32Index ] = (U8)getc( psInputFile );
  }

  fclose( psInputFile );

  Midi_Parse( pu8MidiFileInMemory, u32MidiFileSize );

  psOutputFile = fopen( au8OutputFileName, "wb" );
  Midi_ExportTracker( psOutputFile );

  fclose( psOutputFile );

  return 0;
}
