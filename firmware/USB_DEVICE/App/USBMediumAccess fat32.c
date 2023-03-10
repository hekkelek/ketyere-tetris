/*! *******************************************************************************************************
* Copyright (c) 2018-2022 K. Sz. Horvath
*
* All rights reserved
*
* \file USBMediumAccess.c
*
* \brief Virtual USB Mass Storage device with FAT32 file system
*
* \author K. Sz. Horvath
*
**********************************************************************************************************/

//--------------------------------------------------------------------------------------------------------/
// Include files
//--------------------------------------------------------------------------------------------------------/
#include "types.h"
#include "usbd_storage_if.h"
// Own include
#include "USBMediumAccess.h"


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/
#define FAT_SECTORSIZE         (512u)   //!< Sector size in bytes used by FAT filesystem
#define SECTORS_PER_CLUSTER   (0x01u)   //!< Number of sectors per cluster
// Virtual file system parameters
#define FAT_BLOCKS_START         (8u)   //!< 
#define NUM_FAT_BLOCKS          (64u)   //!< 
#define FIRST_FILE_BLOCK       (161u)   //!< Block index of the first file
#define LONGFILE_SIZE         (2000u)   //!< Note: file should be smaller than 32K


//--------------------------------------------------------------------------------------------------------/
// Macros
//--------------------------------------------------------------------------------------------------------/
//Time format (16Bits):
// Bits15~11 Hour, ranging from 0~23
// Bits10~5 Minute, ranging from 0~59
// Bits4~0 Second, ranging from 0~29. Each unit is 2 seconds.

//Get high byte of time format in 16bits format
#define TIME_HIGH(H,M,S) (((((H)<<3))|((M)>>3)))
//Get low byte of time format in 16bits format
#define TIME_LOW(H,M,S) (((0))|((M)<<5)|(S))

//Date format (16Bits):
// Bits15~9 Year, ranging from 0~127. Represent difference from 1980.
// Bits8~5 Month, ranging from 1~12
// Bits4~0 Day, ranging from 1~31

//Get high byte of date format in 16bits format
#define DATE_HIGH(Y,M,D) (((((Y)-1980)<<1)|((M)>>3)))
//Get low byte of date format in 16bits format
#define DATE_LOW(Y,M,D) ((0)|((M)<<5)|(D))


//--------------------------------------------------------------------------------------------------------/
// Types
//--------------------------------------------------------------------------------------------------------/
//! \brief Callback function pointer for file reading/writing
typedef void( *pFileCallBackFunction)( U32 u32FileOffset, U8* pu8Buffer, U32 u32Size );

//! \brief Type of read callback pointer
typedef enum
{
  CONST_DATA_FILE,
  FUNCTION_CALLBACK_FILE
} E_READ_CB_PTR_TYPE;

PACKED_TYPES_BEGIN
//! \brief Union for file reading operations: either a function or a data array
typedef union
{
    pFileCallBackFunction pFuncRead;
    U8 *pu8ConstantArray;
} U_READ_CB_PTR;

//! \brief File descriptor 
typedef struct
{
  U8  au8FileName[ 11u ];
  U8  au8FileTime[ 2u ];
  U8  au8FileDate[ 2u ];
  U32 u32FileSize;
  E_READ_CB_PTR_TYPE    eReadCallbackType;
  U_READ_CB_PTR         uReadHandler;
  pFileCallBackFunction pWriteHandler;
} S_FILE_DESCRIPTOR;

PACKED_TYPES_END


//--------------------------------------------------------------------------------------------------------/
// Global variables
//--------------------------------------------------------------------------------------------------------/
extern const S_FILE_DESCRIPTOR gcsFilesOnDrive[];
extern const U8 gcu8NumFilesOnDrive;

//! \brief Data of the boot block (FAT32 format)
static const U8 cau8BootBlockData[] =
{
    0xEBu, 0x3Eu, 0x90u,                                     // Jump instruction for bootstrap routine
    'M','S','D','O','S','5','.','0',                         // OEM Name string (MSDOS5.0)
    0xFFu&(FAT_SECTORSIZE>>0u), 0xFFu&(FAT_SECTORSIZE>>0u),  // Sector size in bytes
    SECTORS_PER_CLUSTER,                                     // Number of sectors per cluster
    0xFFu&(FAT_BLOCKS_START>>0u), 0xFFu&(FAT_BLOCKS_START>>8u),  // Number of reserved sectors before the FATs, including the boot block. Must be higher than 0.
    0x02u,                                                   // Number of FATs. For compatibility reasons, should not be other than 2.
    0x00u, 0x00u,                                            // Maximum number of root directory entries. For FAT16, this should be 512 for compatibility reasons, but for FAT32, this must be 0.
    0x00u, 0x00u,                                            // Total number of sectors if it's less than 0x10000 for FAT16. For FAT32, this should be 0. 
    0xF0u,                                                   // Media type: removable disk. Used only by MS-DOS 1.0
    0x40u, 0x00u,                                            // Number of sectors occupied by a FAT (64 in this case)
    0x20u, 0x00u,                                            // Number of sectors per track (32 in this case). Used only by BIOS.
    0x01u, 0x00u,                                            // Number of heads (1 in this case). Used only by BIOS.
    0x00, 0x00, 0x00, 0x00,                                  // Number of hidden physical sectors preceding the FAT volume. Should be 0 for non-partitioned disks.
    0xFFu&(MASS_BLOCK_COUNT>>0u), 0xFFu&(MASS_BLOCK_COUNT>>8u), 0xFFu&(MASS_BLOCK_COUNT>>16u), 0xFFu&(MASS_BLOCK_COUNT>>24u),  // Total number of sectors of the FAT volume. For FAT16 it is only used if it doesn't fit into 16 bits. Always valid for FAT32.
    // From now on, FAT32 specific data is stored
    0x40u, 0x00u, 0x00u, 0x00u,                              // Size of a FAT in unit of sector (in this case, 64 sectors).
    0x00u, 0x00u,                                            // Flags
    0x00u, 0x00u,                                            // FAT32 version 0.0
    0xFFu&((FIRST_FILE_BLOCK/SECTORS_PER_CLUSTER)>>0u), 0xFFu&((FIRST_FILE_BLOCK/SECTORS_PER_CLUSTER)>>8u), 0xFFu&((FIRST_FILE_BLOCK/SECTORS_PER_CLUSTER)>>16u), 0xFFu&((FIRST_FILE_BLOCK/SECTORS_PER_CLUSTER)>>24u),  // Cluster number of the root directory
    0x01u, 0x00u,                                            // Sector of the FSInfo structure (1 == it's right after the boot block)
    0x06u, 0x00u,                                            // Sector of the backup boot block. 
    0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u,  // Reserved
    0x00u, 0x00u, 0x00u, 0x00u,                              // Reserved
    0x80u,                                                   // Drive number (fixed disk)
    0x00u,                                                   // Reserved
    0x29u,                                                   // Extended boot signature
    0x00u, 0x00u, 0x00u, 0x00u,                              // Volume serial number
    'K', 'e', 't', 'y', 'e', 'r', 'e', ' ', 'M', 'S', 'D',   // Volume label
    'F', 'A', 'T', '3', '2', ' ', ' ', ' ',                  // File system type label
    // After this, the bootstrap program is placed here normally, but we don't have one.
    // The last 2 bytes of the 512-byte block should be the boot signature: 0x55, 0xAA
};

//! \brief Volume label entry
const U8 cau8VolumeLabel[ 32u ] =
{
    'K', 'e', 't', 'y', 'e', 'r', 'e', ' ', 'M', 'S', 'D',  // Volume label, should match the one in the boot block
    0x08u,                                                  // Directory attributes, in this case volume label
    0x00u,                                                  // Optional flags, not used here
    0x00u,                                                  // Creation time: 10 ms units
    TIME_LOW(12, 34, 56), TIME_HIGH(12, 34, 56),            // Creation time: hours, minutes and seconds
    DATE_LOW(2021, 1, 2), DATE_HIGH(2021, 1, 2),            // Creation date: year, month and day
    DATE_LOW(2021, 1, 2), DATE_HIGH(2021, 1, 2),            // Last access date: year, month and day
    0x00u, 0x00u,                                           // Upper two bytes of first cluster number
    TIME_LOW(12, 34, 56), TIME_HIGH(12, 34, 56),            // Last modified time: hours, minutes and seconds
    DATE_LOW(2021, 1, 2), DATE_HIGH(2021, 1, 2),            // Last modified date: year, month and day
    0x00u, 0x00u,                                           // Lower two bytes of first cluster number
    0x00u, 0x00u, 0x00u, 0x00u                              // File size (if applicable)
};


//--------------------------------------------------------------------------------------------------------/
// Static function declarations
//--------------------------------------------------------------------------------------------------------/
static void ReadBootBlock( U32 u32ByteOffset, U8* u8Buffer, U32 u32BufferSize );
static void ReadFSInfo( U32 u32ByteOffset, U8* pu8Buffer, U32 u32BufferSize );
static void ReadFAT( U32 u32BlockOffset, U8* pu8Buffer, U32 u32BufferSize );
static void ReadRootDir( U32 u32BlockOffset, U8* pu8Buffer, U32 u32BufferSize );


//--------------------------------------------------------------------------------------------------------/
// Static functions
//--------------------------------------------------------------------------------------------------------/
/*! *******************************************************************
 * \brief  Copies the boot block to a given buffer
 * \param  u32ByteOffset: offset from where the boot block should be read
 * \param  pu8Buffer: pointer to the output buffer
 * \param  u32BufferSize: the size of the buffer in bytes
 * \return -
 *********************************************************************/
static void ReadBootBlock( U32 u32ByteOffset, U8* pu8Buffer, U32 u32BufferSize )
{
  U32 u32Index;
  U32 u32BufferIndex = 0u;
  
  for( u32Index = u32ByteOffset; u32Index < (u32ByteOffset+u32BufferSize); u32Index++ )
  {
    if( u32Index < sizeof( cau8BootBlockData ) )
    {
      pu8Buffer[ u32BufferIndex ] = cau8BootBlockData[ u32Index ];
    }
    else if( 510u == u32Index )
    {
      pu8Buffer[ u32BufferIndex ] = 0x55u;
    }
    else if( 511u == u32Index )
    {
      pu8Buffer[ u32BufferIndex ] = 0xAAu;
    }
    else  // bootstrap code
    {
      pu8Buffer[ u32BufferIndex ] = 0x90u;  // Corresponds to NOP instruction on x86 processors
    }
    u32BufferIndex++;
  }
}

/*! *******************************************************************
 * \brief  Copies the FSInfo block to a given buffer
 * \param  u32ByteOffset: offset from where the block should be read
 * \param  pu8Buffer: pointer to the output buffer
 * \param  u32BufferSize: the size of the buffer in bytes
 * \return -
 *********************************************************************/
static void ReadFSInfo( U32 u32ByteOffset, U8* pu8Buffer, U32 u32BufferSize )
{
  const U8 cau8LeadSig[ 4u ] = {0x52u, 0x52u, 0x61u, 0x41u};  // FAT32 lead signature (0x41615252)
  const U8 cau8StrucSig[ 4u ] = {0x72u, 0x72u, 0x41u, 0x61u};  // FAT32 structure signature (0x61417272)
  const U8 cau8TrailSig[ 4u ] = {0x00u, 0x00u, 0x55u, 0xAAu};  // FAT32 trail signature (0xAA550000)
  U32 u32Index;
  U32 u32BufferIndex = 0u;
  
  for( u32Index = u32ByteOffset; u32Index < (u32ByteOffset+u32BufferSize); u32Index++ )
  {
    if( 4u > u32Index )  // Lead signature
    {
      pu8Buffer[ u32BufferIndex ] = cau8LeadSig[ u32Index ];
    }
    else if( 484u > u32Index )  // Reserved, should be initialized to 0.
    {
      pu8Buffer[ u32BufferIndex ] = 0x00u;
    }
    else if( 488u > u32Index )  // Structure signature
    {
      pu8Buffer[ u32BufferIndex ] = cau8StrucSig[ u32Index - 484u ];
    }
    else if( 492u > u32Index )  // Last known free sector
    {
      pu8Buffer[ u32BufferIndex ] = 0xFFu;  // Unknown, the OS should calculate it
    }
    else if( 496u > u32Index )  // Hint for searching for the next free sector
    {
      pu8Buffer[ u32BufferIndex ] = 0xFFu;  // No hint, the OS should start searching from 2.
    }
    else if( 508u > u32Index )  // Reserved, should be initialized to 0.
    {
      pu8Buffer[ u32BufferIndex ] = 0x00u;
    }
    else if( 512u > u32Index )  // Trail signature
    {
      pu8Buffer[ u32BufferIndex ] = cau8TrailSig[ u32Index - 508u ];
    }
    else  // If the sector is larger than 512, the rest of the sector should be 0.
    {
      pu8Buffer[ u32BufferIndex ] = 0x00u;
    }    
    u32BufferIndex++;
  }
}

 /*! *******************************************************************
 * \brief  Copies the file allocation table to a given buffer
 * \param  u32BlockOffset: which block of the FAT should be read
 * \param  pu8Buffer: pointer to the output buffer
 * \param  u32BufferSize: the size of the buffer in bytes
 * \return -
 *********************************************************************/
static void ReadFAT( U32 u32BlockOffset, U8* pu8Buffer, U32 u32BufferSize )
{
  U32 u32Index, u32ByteAddress;
  U32 u32BufferIndex = 0u;
  U32 u32FATEntry;

  U8  u8FileIndex;
  U32 u32FileEnd = FIRST_FILE_BLOCK*MASS_BLOCK_SIZE;
  U32 u32FileSize;
  U32 u32CurrentCluster;
  U32 u32Clusters;
  
  for( u32Index = 0u; u32Index < u32BufferSize; u32Index++ )
  {
    u32ByteAddress = u32BlockOffset*MASS_BLOCK_SIZE + u32Index;
    u32FATEntry = 0xFFFFFFF7u;  // default: bad cluster

    if( u32ByteAddress < 4u )  // First entry: root descriptor
    {
      u32FATEntry = 0xFFFFFF00u | cau8BootBlockData[ 21u ];  // should contain the media type
    }
    else  // file entries
    {
      u32CurrentCluster = MASS_BLOCK_SIZE*((u32ByteAddress-4u)/4u);
      for( u8FileIndex = 0u; u8FileIndex < gcu8NumFilesOnDrive; u8FileIndex++ )
      {
        u32FileSize = gcsFilesOnDrive[ u8FileIndex ].u32FileSize;
        u32Clusters = (u32FileSize + MASS_BLOCK_SIZE - 1u) / MASS_BLOCK_COUNT;
        u32FileEnd += u32Clusters;
        if( u32CurrentCluster < u32FileEnd )
        {
          break;  // we have found the file
        }
      }
      if( u8FileIndex >= gcu8NumFilesOnDrive )  // no file
      {
        u32FATEntry = 0xFFFFFFF7u;  // bad cluster
      }
      else  // belongs to a file
      {
        u32FATEntry = 0xFFFFFFF8u;  // default: end of file
        if( u32CurrentCluster + 1u < u32FileEnd )
        {
          u32FATEntry = u32CurrentCluster + 1u;
        }
      }
    }
    
    pu8Buffer[ u32BufferIndex ] = 0xFFu & (u32FATEntry>>(8u*(3u - (u32ByteAddress & 0x3u))));  // little endian...
    u32BufferIndex++;
  }
}

 /*! *******************************************************************
 * \brief  Reads the root directory including the volume descriptor
 * \param  u32BlockOffset: which block of the descriptor should be read
 * \param  pu8Buffer: pointer to the output buffer
 * \param  u32BufferSize: the size of the buffer in bytes
 * \return -
 *********************************************************************/
static void ReadRootDir( U32 u32BlockOffset, U8* pu8Buffer, U32 u32BufferSize )
{
  U32 u32Index, u32ByteAddress;
  U32 u32BufferIndex = 0u;
  U8  u8Ret = 0u;
  U8  u8FileIndex;
  U8  u8OffsetIndex;
  U32 u32StartCluster;

  for( u32Index = 0u; u32Index < u32BufferSize; u32Index++ )
  {
    u32ByteAddress = u32BlockOffset*MASS_BLOCK_SIZE + u32Index;
    
    if( 32u > u32ByteAddress )  // volume descriptor
    {
      u8Ret = cau8VolumeLabel[ u32ByteAddress ];
    }
    else if( 32u*(1u + gcu8NumFilesOnDrive ) > u32ByteAddress )  // file entry
    {
      u32StartCluster = FIRST_FILE_BLOCK;
      for( u8FileIndex = 0u; u8FileIndex < ( u32ByteAddress / 32u ) - 1u; u8FileIndex++ )
      {
        u32StartCluster += ( gcsFilesOnDrive[ u8FileIndex ].u32FileSize + MASS_BLOCK_SIZE - 1u) / MASS_BLOCK_SIZE;
      }
      u8FileIndex = ( u32ByteAddress / 32u ) - 1u;
      u8OffsetIndex = u32ByteAddress % 32u;
      
      if( 11u > u8OffsetIndex )  // File name
      {
        u8Ret = gcsFilesOnDrive[ u8FileIndex ].au8FileName[ u8OffsetIndex ];
      }
      else if( 11u == u8OffsetIndex )  // File attributes
      {
        u8Ret = 0x00u;  // Read/write access
        if( NULL == gcsFilesOnDrive[ u8FileIndex ].pWriteHandler )
        {
          u8Ret = 0x01u;  // Read only
        }
      }
      else if( 12u == u8OffsetIndex )  // Optional flags
      {
        u8Ret = 0x00u;
      }
      else if( 13u == u8OffsetIndex )  // Creation time: 10 ms units
      {
        u8Ret = 0x00u;
      }
      else if( 16u > u8OffsetIndex )  // Creation time: hours, minutes and seconds
      {
        u8Ret = gcsFilesOnDrive[ u8FileIndex ].au8FileTime[ u8OffsetIndex - 14u ];
      }
      else if( 18u > u8OffsetIndex )  // Creation date: year, month and day
      {
        u8Ret = gcsFilesOnDrive[ u8FileIndex ].au8FileDate[ u8OffsetIndex - 16u ];
      }
      else if( 20u > u8OffsetIndex )  // Last access date: year, month and day
      {
        u8Ret = gcsFilesOnDrive[ u8FileIndex ].au8FileDate[ u8OffsetIndex - 18u ];
      }
      else if( 22u > u8OffsetIndex )  // Upper two bytes of first cluster number
      {
        u8Ret = 0xFFu & ( u32StartCluster>>( 16u + 8u*(u8OffsetIndex & 0x1u) ) );
      }
      else if( 24u > u8OffsetIndex )  // Last modified time: hours, minutes and seconds
      {
        u8Ret = gcsFilesOnDrive[ u8FileIndex ].au8FileTime[ u8OffsetIndex - 22u ];
      }
      else if( 26u > u8OffsetIndex )  // Last modified date: year, month and day
      {
        u8Ret = gcsFilesOnDrive[ u8FileIndex ].au8FileDate[ u8OffsetIndex - 24u ];
      }
      else if( 28u > u8OffsetIndex )  // Lower two bytes of first cluster number
      {
        u8Ret = 0xFFu & ( u32StartCluster>>( 0u + 8u*(u8OffsetIndex & 0x1u) ) );
      }
      else  // file size
      {
        u8Ret = 0xFFu & ( gcsFilesOnDrive[ u8FileIndex ].u32FileSize>>( 8u*(u8OffsetIndex & 0x3u) ) );
      }
    }
    else  // empty area
    {
      u8Ret = 0u;
    }

    pu8Buffer[ u32BufferIndex ] = u8Ret;
    u32BufferIndex++;
  }
}

 /*! *******************************************************************
 * \brief  Reads the contents of files
 * \param  u32BlockOffset: which sector of all the files should be read
 * \param  pu8Buffer: pointer to the output buffer
 * \param  u32BufferSize: the size of the buffer in bytes
 * \return -
 *********************************************************************/
static void ReadFile( U32 u32BlockOffset, U8* pu8Buffer, U32 u32BufferSize )
{
  U8  u8FileIndex;
  U32 u32FileEnd = 0u;
  U32 u32FileSize;
  U32 u32FileOffset;
  U32 u32Index;
  U32 u32BufferIndex = 0u;
  
  for( u8FileIndex = 0u; u8FileIndex < gcu8NumFilesOnDrive; u8FileIndex++ )
  {
    u32FileSize = gcsFilesOnDrive[ u8FileIndex ].u32FileSize;
    u32FileEnd += (u32FileSize + MASS_BLOCK_SIZE - 1u) / MASS_BLOCK_COUNT;
    if( u32BlockOffset < u32FileEnd )
    {
      break;  // we have found the file
    }
  }
  if( u8FileIndex >= gcu8NumFilesOnDrive )  // no file
  {
    return;
  }
  u32FileOffset = u32FileEnd - u32BlockOffset;
  
  if( CONST_DATA_FILE == gcsFilesOnDrive[ u8FileIndex ].eReadCallbackType )
  {
    for( u32Index = 0u; u32Index < u32BufferSize; u32Index++ )
    {
      pu8Buffer[ u32BufferIndex ] = gcsFilesOnDrive[ u8FileIndex ].uReadHandler.pu8ConstantArray[ u32FileOffset + u32Index ];
      u32BufferIndex++;
    }
  }
  else
  {
    if( NULL != gcsFilesOnDrive[ u8FileIndex ].uReadHandler.pFuncRead )
    {
      gcsFilesOnDrive[ u8FileIndex ].uReadHandler.pFuncRead(u32FileOffset, pu8Buffer, u32BufferSize );
    }
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


//--------------------------------------------------------------------------------------------------------/
// Interface functions
//--------------------------------------------------------------------------------------------------------/
/*! *******************************************************************
 * \brief  Reads blocks from the virtual media
 * \param  u32StartBlock: starts the read from this block
 * \param  pu8Buffer: the output is written here
 * \param  u32BlockNum: number of blocks to be read
 * \return -
 *********************************************************************/
void USBMediumAccess_Read( U32 u32StartBlock, U8* pu8Buffer, U32 u32BlockNum )
{
  U32 u32Index, u32CurrentBlock;
  
  for( u32Index = 0u; u32Index < u32BlockNum; u32Index++ )
  {
    u32CurrentBlock = u32StartBlock + u32Index;
    if( ( 0u == u32CurrentBlock) || ( 6u == u32CurrentBlock ) )  // boot block, backup boot block
    {
      ReadBootBlock( 0u, &(pu8Buffer[ u32Index ]), MASS_BLOCK_SIZE );
    }
    else if( ( 1u == u32CurrentBlock) || ( 7u == u32CurrentBlock ) )  // FSInfo block, backup FSInfo block
    {
      ReadFSInfo( 0u, &(pu8Buffer[ u32Index ]), MASS_BLOCK_SIZE );
    }
    else if( FAT_BLOCKS_START+2u*NUM_FAT_BLOCKS > u32CurrentBlock )  // FAT
    {
      if( 65u >= u32CurrentBlock )  // FAT2
      {
        ReadFAT( u32CurrentBlock - 65u, &(pu8Buffer[ u32Index ]), MASS_BLOCK_SIZE );
      }
      else  // FAT1
      {
        ReadFAT( u32CurrentBlock, &(pu8Buffer[ u32Index ]), MASS_BLOCK_SIZE );
      }
    }
    else if( FIRST_FILE_BLOCK > u32CurrentBlock )  // Root directory
    {
      ReadRootDir( u32CurrentBlock - 128u, &(pu8Buffer[ u32Index ]), MASS_BLOCK_SIZE );
    }
    else  // Files or no data
    {
      memset( &(pu8Buffer[ u32Index ]), 0, MASS_BLOCK_SIZE );
      ReadFile( u32CurrentBlock - FIRST_FILE_BLOCK, &(pu8Buffer[ u32Index ]), MASS_BLOCK_SIZE );
    }
  }
}

 /*! *******************************************************************
 * \brief
 * \param
 * \return
 *********************************************************************/



//--------------------------------------------------------------------------------------------------------/
//--------------------------------------------------------------------------------------------------------/
//--------------------------------------------------------------------------------------------------------/
//--------------------------------------------------------------------------------------------------------/
//--------------------------------------------------------------------------------------------------------/
//--------------------------------------------------------------------------------------------------------/
//-------------------------------------< Virtual disk contents >------------------------------------------/
//--------------------------------------------------------------------------------------------------------/
//--------------------------------------------------------------------------------------------------------/
//--------------------------------------------------------------------------------------------------------/
//--------------------------------------------------------------------------------------------------------/
//--------------------------------------------------------------------------------------------------------/
//--------------------------------------------------------------------------------------------------------/
static const U8 cau8ReadmeFile[] =
"This is a mass storage device example on the ,,Ketyere'' hardware. \r\n\
It support multiple files defined in the \"filesOnDrive\" array, with file content either hardcoded in an array, or generated by a callback function.\r\n\
To make the FAT table simple, all unused clusters are marked as bad to prevent new file creation or file movement.\r\n\
But the existing file can be modified to send data back to the hardware.\r\n\
";

static const U8 cau8LEDControlFile[] =
"1\r\nThe first byte in this file controls the LCD backlight on board. Use a texteditor without autosave to avoid the disk full error.\r\n";

void LEDControlCallback(U32 offset, U8* buf, U32 size)
{
  for (U32 i = 0; i < size; i++)
  {
    uint8_t inputVal = buf[i];
    if (offset == 0)
    {
      if (inputVal & 1)  //odd value, such as 1 or '1'
      {
        HAL_GPIO_WritePin( LCD_BACKLIGHT_GPIO_Port, LCD_BACKLIGHT_Pin, GPIO_PIN_SET );
      } 
      else  //even value, such as 0 or '0'
      {
        HAL_GPIO_WritePin( LCD_BACKLIGHT_GPIO_Port, LCD_BACKLIGHT_Pin, GPIO_PIN_RESET );
      }
    }
    offset++;
  }
}

void dataflashReadCallback(U32 offset, U8* buf, U32 size) 
{
  for (uint32_t i = 0; i < size; i++) 
  {
    uint8_t returnVal = 0;
    if (offset < 128) 
    {
      //returnVal = eeprom_read_byte(((uint8_t)offset));
    }
    buf[i] = returnVal;
    offset++;
  }
}

void dataflashWriteCallback(U32 offset, U8* buf, U32 size) 
{
  for (uint32_t i = 0; i < size; i++) 
  {
    uint8_t inputVal = buf[i];
    if (offset < 128) 
    {
      uint8_t eepromData;// = eeprom_read_byte(((uint8_t)offset));
      if (eepromData != inputVal) 
      {
        //eeprom_write_byte(((uint8_t)offset), inputVal);
      }
    }
    offset++;
  }
}

void longfileCallback(U32 offset, U8* buf, U32 size) 
{
  for (uint32_t i = 0; i < size; i++) 
  {
    uint8_t returnVal = 0;
    if (offset < LONGFILE_SIZE) 
    {
      //make each line 32 charactors.
      uint16_t lineCount = offset / 32;
      uint8_t lineOffset = offset % 32;
      if (lineOffset == 30) 
      {
        returnVal = '\r';
      } 
      else if (lineOffset == 31) 
      {
        returnVal = '\n';
      }
      else if (lineOffset == (lineCount % 32)) 
      {
        returnVal = '*';
      } 
      else 
      {
        returnVal = ' ';
      }
    }
    buf[i] = returnVal;
    offset++;
  }
}

//! \brief Files on the virtual drive
const S_FILE_DESCRIPTOR gcsFilesOnDrive[] = 
{
  {
    .au8FileName = {'R', 'E', 'A', 'D', 'M', 'E', ' ', ' ', 'T', 'X', 'T'},
    .au8FileTime = {TIME_LOW(12, 34, 56), TIME_HIGH(12, 34, 56)},
    .au8FileDate = {DATE_LOW(2021, 1, 2), DATE_HIGH(2021, 1, 2)},
    .u32FileSize = sizeof(cau8ReadmeFile),
    .eReadCallbackType = CONST_DATA_FILE,
    .uReadHandler = {.pu8ConstantArray = (U8*)cau8ReadmeFile},
    .pWriteHandler = NULL,
  },
  {
    .au8FileName = {'L', 'O', 'N', 'G', 'F', 'I', 'L', 'E', 'T', 'X', 'T'},
    .au8FileTime = {TIME_LOW(12, 34, 56), TIME_HIGH(12, 34, 56)},
    .au8FileDate = {DATE_LOW(2021, 1, 2), DATE_HIGH(2021, 1, 2)},
    .u32FileSize = LONGFILE_SIZE,
    .eReadCallbackType = FUNCTION_CALLBACK_FILE,
    .uReadHandler = {.pFuncRead = longfileCallback},
    .pWriteHandler = NULL,
  },
  {
    .au8FileName = {'L', 'E', 'D', '_', 'C', 'T', 'R', 'L', 'T', 'X', 'T'},
    .au8FileTime = {TIME_LOW(12, 34, 56), TIME_HIGH(12, 34, 56)},
    .au8FileDate = {DATE_LOW(2021, 1, 2), DATE_HIGH(2021, 1, 2)},
    .u32FileSize = sizeof(cau8LEDControlFile),
    .eReadCallbackType = CONST_DATA_FILE,
    .uReadHandler = {.pu8ConstantArray = (U8*)cau8LEDControlFile},
    .pWriteHandler = LEDControlCallback,
  },
  {
    .au8FileName = {'D', 'A', 'T', 'F', 'L', 'A', 'S', 'H', 'B', 'I', 'N'},
    .au8FileTime = {TIME_LOW(12, 34, 56), TIME_HIGH(12, 34, 56)},
    .au8FileDate = {DATE_LOW(2021, 1, 2), DATE_HIGH(2021, 1, 2)},
    .u32FileSize = 128,
    .eReadCallbackType = FUNCTION_CALLBACK_FILE,
    .uReadHandler = {.pFuncRead = dataflashReadCallback},
    .pWriteHandler = dataflashWriteCallback,
  }
};

const U8 gcu8NumFilesOnDrive = (sizeof(gcsFilesOnDrive)/sizeof(S_FILE_DESCRIPTOR) );


//-----------------------------------------------< EOF >--------------------------------------------------/



















































/* External variables --------------------------------------------------------*/
// Defined in USBMassStorage.c
//extern Bulk_Only_CBW CBW;

//FAT info: http://www.c-jump.com/CIS24/Slides/FAT/lecture.html
//http://elm-chan.org/docs/fat_e.html
//FAT12<4087 clusters. FAT16>4087 <65526clusters
//FAT16 FAT is easier to generate

uint8_t emuDisk_Status = MAL_OK;

const uint8_t DBR_data[62]={
    0xeb, 0x3e, 0x90,          //Instructions to jump to boot code
    'M','S','D','O','S','5','.','0', //Name string (MSDOS5.0)
    0x00, 0x02,                //Bytes/sector (0x0200 = 512)
    0x01,                      //Sectors/cluster (1)
    0x01, 0x00,                //Size of reserved area (1 sector)
    0x02,                      //Number of FATs (2)
    0x00, 0x02,                //Max. number of root directory entries (0x0200 = 512)
    0x00, 0x00,                //Total number of sectors (0, not used)
    0xF8,                      //Media type (hard disk)
    0x40, 0x00,                //FAT size (0x0040 = 64 sectors)
    0x20, 0x00,                //Sectors/track (0x0020 = 32)
    0x01, 0x00,                //Number of heads (0x0001 = 1)
    0x00, 0x00, 0x00, 0x00,    //Number of sector before partition (0)
    0xFFu&(MASS_BLOCK_COUNT>>0u), 0xFFu&(MASS_BLOCK_COUNT>>8u), 0xFFu&(MASS_BLOCK_COUNT>>16u), 0xFFu&(MASS_BLOCK_COUNT>>24u),    //Total number of sectors. 8M is 0x4000, 16384 clusters, FAT16 is more than 4087 clusters
    0x80,                      //Drive number (hard disk)
    0x00,                      //Reserved
    0x29,                      //Extended boot signature
    0x00, 0x00, 0x00, 0x00,    //Volume serial number
    
    //Volume label
    'K', 'e', 't', 'y', 'e', 'r', 'e', ' ', 'M', 'S', 'D',
    
    //File system type label
    'F', 'A', 'T', '1', '6', 0x20,0x20, 0x20,
 
};

//extern File_Entry filesOnDrive[];    //refer to main file
//extern uint8_t filesOnDriveCount;    //refer to main file

/*
//Root directory
const uint8_t RootDir[32]={
    //label, match DBR
    'K', 'e', 't', 'y', 'e', 'r', 'e', ' ', 'M', 'S', 'D',
    0x08,                  //File Attributes, Volume Label (0x08).
    0x00,                  //Reserved
    0x00,                  //Create time, fine resolution: 10 ms units, Byte 13
    
    //Create time
    TIME_LB(12, 34, 56), TIME_HB(12, 34, 56),
    
    //Create date
    DATE_LB(2021, 1, 2), DATE_HB(2021, 1, 2),
    
    //Last access date
    DATE_LB(2021, 1, 2), DATE_HB(2021, 1, 2),
    
    0x00, 0x00,            //High two bytes of first cluster number, keep 0 for FAT12/16
    
    //Last modified time
    TIME_LB(12, 34, 56), TIME_HB(12, 34, 56),
    
    //Last modified date
    DATE_LB(2021, 1, 2), DATE_HB(2021, 1, 2),
    
    0x00, 0x00,            //Start of file in clusters in FAT12 and FAT16.
    0x00, 0x00, 0x00, 0x00,   //file size
};
*/
uint8_t LUN_GetStatus () {
    return emuDisk_Status;
}

void LUN_Read_func_DBR(uint16_t DBR_data_index, uint8_t* buf, uint32_t size){    //separate funcs relieve the register usage
    for (uint32_t i=0;i<size*MASS_BLOCK_SIZE;i++){
        if (DBR_data_index<62){
            buf[i] = DBR_data[DBR_data_index];
        }else if (DBR_data_index==510){
            buf[i] = 0x55;
        }else if (DBR_data_index==511){
            buf[i] = 0xAA;
        }else{
            buf[i] = 0x90;   //nop for boot code
        }
        DBR_data_index++;
    }
}

/*
//1 cluster = 1 sectors = 0.5KB
//emulated file allocation table
//little-endian
//Unused (0x0000) Cluster in use by a file(Next cluster) Bad cluster (0xFFF7) Last cluster in a file (0xFFF8-0xFFFF).
void LUN_Read_func_FAT(uint16_t FAT_data_index, uint8_t* buf, uint32_t size){    //separate funcs relieve the register usage
    for (uint32_t i=0;i<size*MASS_BLOCK_SIZE;i++){
        uint8_t sendVal=0;
        //uint16_t fatEntry = 0x0000; //Free
        uint16_t fatEntry = 0xFFF7; //Bad cluster
        if (FAT_data_index<4){  //FAT16: FAT[0] = 0xFF??; FAT[1] = 0xFFFF; ?? in the value of FAT[0] is the same value of BPB_Media
            if (FAT_data_index<2){
                fatEntry = 0xFF00|DBR_data[21];
            }else{
                fatEntry = 0xFFFF;
            }
        }else{
            uint8_t fileIndex = ((FAT_data_index-4)/2)/64;  //64 sector reserved per file
            uint8_t fileOffset = ((FAT_data_index-4)/2)%64;
            uint8_t clusterUsage = (filesOnDrive[fileIndex].filesize+511)/512;
            if (fileIndex<filesOnDriveCount){
                if (fileOffset<clusterUsage){
                    fatEntry = 0xFFF8; //end of file
                    if ((fileOffset+1)<clusterUsage){
                        fatEntry = 64*fileIndex + fileOffset + 3;   //make FAT link to next cluster.
                    }
                }
            }
        }
        if (FAT_data_index&1){
            sendVal = fatEntry>>8;
        }else{
            sendVal = fatEntry;
        }
        buf[i] = sendVal;
        FAT_data_index++;
    }
}
*/

/*
void LUN_Read_func_Root_DIR(uint16_t rootAddrIndex, uint8_t* buf, uint32_t size){    //separate funcs relieve the register usage
    for (uint32_t i=0;i<size*MASS_BLOCK_SIZE;i++){
        uint8_t sendVal=0;
        if (rootAddrIndex<32){  // RootDir entry
            sendVal = RootDir[rootAddrIndex];
        }else if (rootAddrIndex< (32*(1+filesOnDriveCount)) ){
            uint8_t fileIndex = (rootAddrIndex/32)-1;
            uint8_t offsetIndex = rootAddrIndex%32;
            if (offsetIndex<11){
                sendVal = filesOnDrive[fileIndex].filename[offsetIndex];
            }else if (offsetIndex==11){ //File Attributes
                if (filesOnDrive[fileIndex].fileWriteHandler!=0){
                    sendVal = 0x00; //Normal
                }else{
                    sendVal = 0x01; //Read Only
                }
            }else if (offsetIndex==12){ //Reserved
                sendVal = 0x00;
            }else if (offsetIndex==13){ //Create time, fine resolution: 10 ms units
                sendVal = 0x00;
            }else if (offsetIndex<16){ //Create time
                sendVal = filesOnDrive[fileIndex].filetime[offsetIndex-14];
            }else if (offsetIndex<18){ //Create date
                sendVal = filesOnDrive[fileIndex].filedate[offsetIndex-16];
            }else if (offsetIndex<20){ //Last access date
                sendVal = filesOnDrive[fileIndex].filedate[offsetIndex-18];
            }else if (offsetIndex<22){ //High two bytes of first cluster number, keep 0 for FAT12/16
                sendVal = 0;
            }else if (offsetIndex<24){ //Last modified time
                sendVal = filesOnDrive[fileIndex].filetime[offsetIndex-22];
            }else if (offsetIndex<26){ //Last modified date
                sendVal = filesOnDrive[fileIndex].filedate[offsetIndex-24];
            }else if (offsetIndex<28){ //Start of file in clusters in FAT12 and FAT16.
                uint16_t startClusters = 2 + fileIndex*64;  //make file 64 clusters, 32K apart. Should be enough.
                if (offsetIndex == 26){
                    sendVal = startClusters & 0xFF;
                }else{
                    sendVal = (startClusters>>8) & 0xFF;
                }
            }else if (offsetIndex<30){ //Low 2 byte of file size
                uint16_t filesize = filesOnDrive[fileIndex].filesize;
                if (offsetIndex == 28){
                    sendVal = filesize & 0xFF;
                }else{
                    sendVal = (filesize>>8) & 0xFF;
                }
            }else{
            }
        }else{
        }
        buf[i] = sendVal;
        rootAddrIndex++;
    }
}

void LUN_Read_func_Files(uint32_t file_data_index, uint8_t* buf, uint32_t size){    //separate funcs relieve the register usage
    uint8_t fileIndex = file_data_index/32768;
    uint16_t fileOffset = file_data_index%32768;
    uint16_t fileSize = filesOnDrive[fileIndex].filesize;
    if (fileIndex<filesOnDriveCount && fileOffset<fileSize){
        //check file response type
        uint8_t responseType = filesOnDrive[fileIndex].fileCallBackType;
        if (responseType == CONST_DATA_FILE){
            const uint8_t *dateArray = filesOnDrive[fileIndex].fileReadHandler.constPtr;
            for (uint32_t i=0;i<size*MASS_BLOCK_SIZE;i++){
                buf[i] = (fileOffset<fileSize)?dateArray[fileOffset]:0;
                fileOffset++;
            }
        }else{
            pFileCBFn readFuncPtr = filesOnDrive[fileIndex].fileReadHandler.funcPtr;
            if( 0u != readFuncPtr )
            {
              readFuncPtr(fileOffset, buf, size*MASS_BLOCK_SIZE);
            }
        }
    }else{
        for (uint32_t i=0;i<size*MASS_BLOCK_SIZE;i++){
            buf[i] = 0x0;
        }
    }
}

// Read BULK_MAX_PACKET_SIZE bytes of data from device to BOT_Tx_Buf
void LUN_Read(uint32_t curAddr, uint8_t* buf, uint32_t blocknum) {
    for( uint32_t index = 0u; index < blocknum; index++ )
    {
      if ( curAddr<512 ){  //Boot Sector
          LUN_Read_func_DBR(curAddr, buf, 1u);
      }else if ( curAddr<(512+512*128L) ){  //0x200 0x8200, Each FAT is 32 sectors
          if (curAddr>=512*65L){  //FAT2
              LUN_Read_func_FAT(curAddr-512*65L, buf, 1u);
          }else{                  //FAT1
              LUN_Read_func_FAT(curAddr-512, buf, 1u);
          }
      }else if ( (curAddr<512*161L) ){ //0x10200    Root directory, 512 items, 32 bytes each
          LUN_Read_func_Root_DIR(curAddr - 512*129L, buf, 1u);
      }else {  //0x14200 1st usable cluster, with index 2.
          LUN_Read_func_Files(curAddr - 512*161L, buf, 1u);
      }
      curAddr += MASS_BLOCK_SIZE;
    }
}
*/

/*
void LUN_Write_func_Files(uint32_t file_data_index, uint8_t* buf, uint32_t size){    //separate funcs relieve the register usage
    uint8_t fileIndex = file_data_index/32768;
    uint16_t fileOffset = file_data_index%32768;
    uint16_t fileSize = filesOnDrive[fileIndex].filesize;

    if (fileIndex<filesOnDriveCount && fileOffset<fileSize){
        pFileCBFn writeFuncPtr = filesOnDrive[fileIndex].fileWriteHandler;
        if (writeFuncPtr != 0){
            writeFuncPtr(fileOffset, buf, size*MASS_BLOCK_SIZE);
        }
    }
}

// Write BULK_MAX_PACKET_SIZE bytes of data from BOT_Rx_Buf to device
void LUN_Write (uint32_t curAddr, uint8_t* buf, uint32_t blocknum) {
    for( uint32_t index = 0u; index < blocknum; index++ )
    {
      if ( (curAddr<512*161L) ){ //0x10200    Root directory, 512 items, 32 bytes each
          //don't care Root directory write, FAT write or Boot Sector write
      }else {  //0x14200 1st usable cluster, with index 2.
          LUN_Write_func_Files(curAddr - 512*161L, buf, 1u);
      }
      curAddr += MASS_BLOCK_SIZE;
    }
}
*/

void LUN_Eject () {
    emuDisk_Status = MAL_FAIL;
}










