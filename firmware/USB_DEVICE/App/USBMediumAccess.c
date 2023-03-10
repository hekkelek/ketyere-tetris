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
#include "spi_flash.h"
// Own include
#include "USBMediumAccess.h"


//--------------------------------------------------------------------------------------------------------/
// Definitions
//--------------------------------------------------------------------------------------------------------/
// Virtual file system parameters
#define FAT_SECTORSIZE         (512u)   //!< Sector size in bytes used by FAT filesystem
#define SECTORS_PER_CLUSTER   (0x01u)   //!< Number of sectors per cluster
#define FAT_BLOCKS_START         (1u)   //!< Sector with the first FAT
#define NUM_FAT_BLOCKS         (256u)   //!< Number of FAT blocks. Has an effect on the maximum volume size!
#define MAX_NUM_FILES_ROOT     (512u)   //!< Maximum number of files in the drive root

// File parameters
#define LONGFILE_SIZE       (200000u)   //!< Size of the generative file

// Derived parameters
#define ROOT_DIRECTORY_SIZE_BLOCKS                          (MAX_NUM_FILES_ROOT*32u/FAT_SECTORSIZE)   //!< Each file descriptor is 32 bytes long
#define FIRST_FILE_BLOCK            (FAT_BLOCKS_START+2u*NUM_FAT_BLOCKS+ROOT_DIRECTORY_SIZE_BLOCKS)   //!< Block index of the first file


//--------------------------------------------------------------------------------------------------------/
// Macros
//--------------------------------------------------------------------------------------------------------/
//Time format (16Bits):
// Bits15~11 Hour, ranging from 0~23
// Bits10~5 Minute, ranging from 0~59
// Bits4~0 Second, ranging from 0~29. Each unit is 2 seconds.

//Get high byte of time format in 16bits format
#define TIME_HIGH(H,M,S) ((U8)(((((H)<<3))|((M)>>3))))
//Get low byte of time format in 16bits format
#define TIME_LOW(H,M,S) ((U8)(((0))|((M)<<5)|(S)))

//Date format (16Bits):
// Bits15~9 Year, ranging from 0~127. Represent difference from 1980.
// Bits8~5 Month, ranging from 1~12
// Bits4~0 Day, ranging from 1~31

//Get high byte of date format in 16bits format
#define DATE_HIGH(Y,M,D) ((U8)(((((Y)-1980)<<1)|((M)>>3))))
//Get low byte of date format in 16bits format
#define DATE_LOW(Y,M,D) ((U8)((0)|((M)<<5)|(D)))


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
    0xEBu, 0x3Eu, 0x90u,                                             // Jump instruction for bootstrap routine
    'M','S','D','O','S','5','.','0',                                 // OEM Name string (MSDOS5.0)
    0xFFu&(FAT_SECTORSIZE>>0u), 0xFFu&(FAT_SECTORSIZE>>8u),          // Sector size in bytes
    SECTORS_PER_CLUSTER,                                             // Number of sectors per cluster
    0xFFu&(FAT_BLOCKS_START>>0u), 0xFFu&(FAT_BLOCKS_START>>8u),      // Number of reserved sectors before the FATs, including the boot block. Must be higher than 0.
    0x02u,                                                           // Number of FATs. For compatibility reasons, should not be other than 2.
    0xFFu&(MAX_NUM_FILES_ROOT>>0u), 0xFFu&(MAX_NUM_FILES_ROOT>>8u),  // Maximum number of root directory entries. For FAT16, this should be 512 for compatibility reasons, but for FAT32, this must be 0.
    0x00u, 0x00u,                                                    // Total number of sectors if it's less than 0x10000 for FAT16. For FAT32, this should be 0. 
    0xF0u,                                                           // Media type: removable disk. Used only by MS-DOS 1.0
    0xFFu&(NUM_FAT_BLOCKS>>0u), 0xFFu&(NUM_FAT_BLOCKS>>8u),          // Number of sectors occupied by a FAT
    0x20u, 0x00u,                                                    // Number of sectors per track (32 in this case). Used only by BIOS.
    0x01u, 0x00u,                                                    // Number of heads (1 in this case). Used only by BIOS.
    0x00, 0x00, 0x00, 0x00,                                          // Number of hidden physical sectors preceding the FAT volume. Should be 0 for non-partitioned disks.
    0xFFu&(MASS_BLOCK_COUNT>>0u), 0xFFu&(MASS_BLOCK_COUNT>>8u), 0xFFu&(MASS_BLOCK_COUNT>>16u), 0xFFu&(MASS_BLOCK_COUNT>>24u),  // Total number of sectors of the FAT volume. For FAT16 it is only used if it doesn't fit into 16 bits. Always valid for FAT32.
    // From now on, FAT16 specific data is stored
    0x80u,                                                           // Drive number (fixed disk)
    0x00u,                                                           // Reserved
    0x29u,                                                           // Extended boot signature
    0x00u, 0x00u, 0x00u, 0x00u,                                      // Volume serial number
    'K', 'e', 't', 'y', 'e', 'r', 'e', ' ', 'M', 'S', 'D',           // Volume label
    'F', 'A', 'T', '1', '6', ' ', ' ', ' ',                          // File system type label
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

//! \brief Drive status
static volatile E_USB_MEDIUM_STATE geMediumState = USB_MEDIUM_OK;


//--------------------------------------------------------------------------------------------------------/
// Static function declarations
//--------------------------------------------------------------------------------------------------------/
static void ReadBootBlock( U32 u32ByteOffset, U8* u8Buffer, U32 u32BufferSize );
static void ReadFAT( U32 u32BlockOffset, U8* pu8Buffer, U32 u32BufferSize );
static void ReadRootDir( U32 u32BlockOffset, U8* pu8Buffer, U32 u32BufferSize );
static void ReadFile( U32 u32BlockOffset, U8* pu8Buffer, U32 u32BufferSize );
static void WriteFile( U32 u32BlockOffset, U8* pu8Buffer, U32 u32BufferSize );


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
  U16 u16FATEntry;

  U8  u8FileIndex;
  U32 u32FileEnd;
  U32 u32FileSize;
  U32 u32CurrentCluster;
  U32 u32Clusters;
  
  for( u32Index = 0u; u32Index < u32BufferSize; u32Index++ )
  {
    u32ByteAddress = u32BlockOffset*MASS_BLOCK_SIZE + u32Index;

    if( u32ByteAddress < 2u )  // First entry: volume label
    {
      u16FATEntry = 0xFF00u | cau8BootBlockData[ 21u ];  // should contain the media type
    }
    else if( u32ByteAddress < 4u )  // Root directory and some of the filenames
    {
      u16FATEntry = 0xFFFFu;
    }
    else  // file entries
    {
      u32CurrentCluster = ((u32ByteAddress-4u)/sizeof(U16));
      u32FileEnd = 0u;
      for( u8FileIndex = 0u; u8FileIndex < gcu8NumFilesOnDrive; u8FileIndex++ )
      {
        u32FileSize = gcsFilesOnDrive[ u8FileIndex ].u32FileSize;
        u32Clusters = (u32FileSize + MASS_BLOCK_SIZE - 1u) / MASS_BLOCK_SIZE;
        u32FileEnd += u32Clusters;
        if( u32CurrentCluster < u32FileEnd )
        {
          break;  // we have found the file
        }
      }
      if( u8FileIndex >= gcu8NumFilesOnDrive )  // no file
      {
        u16FATEntry = 0xFFF7u;  // bad cluster
      }
      else  // belongs to a file
      {
        u16FATEntry = 0xFFF8u;  // default: end of file
        if( u32CurrentCluster + 1u < u32FileEnd )
        {
          u16FATEntry = 2u + u32CurrentCluster + 1u;  // The first 2 clusters are reserved
        }
      }
    }
    
    pu8Buffer[ u32BufferIndex ] = 0xFFu & (u16FATEntry>>(8u*(u32ByteAddress & 0x1u)));  // little endian...
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
      u32StartCluster = 2u;  // The first 2 clusters are reserved
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
  U32 u32FileStart = 0u;
  U32 u32FileEnd = 0u;
  U32 u32FileSize;
  U32 u32FileOffset;
  U32 u32Index;
  U32 u32BufferIndex = 0u;
  
  for( u8FileIndex = 0u; u8FileIndex < gcu8NumFilesOnDrive; u8FileIndex++ )
  {
    u32FileSize = gcsFilesOnDrive[ u8FileIndex ].u32FileSize;
    u32FileEnd += (u32FileSize + MASS_BLOCK_SIZE - 1u) / MASS_BLOCK_SIZE;
    if( u32BlockOffset < u32FileEnd )
    {
      break;  // we have found the file
    }
    u32FileStart = u32FileEnd;
  }
  if( u8FileIndex >= gcu8NumFilesOnDrive )  // no file
  {
    return;
  }
  u32FileOffset = u32BlockOffset - u32FileStart;
  
  if( CONST_DATA_FILE == gcsFilesOnDrive[ u8FileIndex ].eReadCallbackType )
  {
    for( u32Index = 0u; u32Index < u32BufferSize; u32Index++ )
    {
      pu8Buffer[ u32BufferIndex ] = gcsFilesOnDrive[ u8FileIndex ].uReadHandler.pu8ConstantArray[ u32FileOffset*MASS_BLOCK_SIZE + u32Index ];
      u32BufferIndex++;
    }
  }
  else
  {
    if( NULL != gcsFilesOnDrive[ u8FileIndex ].uReadHandler.pFuncRead )
    {
      gcsFilesOnDrive[ u8FileIndex ].uReadHandler.pFuncRead(u32FileOffset*MASS_BLOCK_SIZE, pu8Buffer, u32BufferSize );
    }
  }
}

 /*! *******************************************************************
 * \brief  Writes to a file
 * \param  u32BlockOffset: which sector of all the files should be written
 * \param  pu8Buffer: this block content will be written
 * \param  u32BufferSize: the size of the buffer in bytes
 * \return -
 *********************************************************************/
static void WriteFile( U32 u32BlockOffset, U8* pu8Buffer, U32 u32BufferSize )
{
  U8  u8FileIndex;
  U32 u32FileStart = 0u;
  U32 u32FileEnd = 0u;
  U32 u32FileSize;
  U32 u32FileOffset;
  
  for( u8FileIndex = 0u; u8FileIndex < gcu8NumFilesOnDrive; u8FileIndex++ )
  {
    u32FileSize = gcsFilesOnDrive[ u8FileIndex ].u32FileSize;
    u32FileEnd += (u32FileSize + MASS_BLOCK_SIZE - 1u) / MASS_BLOCK_SIZE;
    if( u32BlockOffset < u32FileEnd )
    {
      break;  // we have found the file
    }
    u32FileStart = u32FileEnd;
  }
  if( u8FileIndex >= gcu8NumFilesOnDrive )  // no file
  {
    return;
  }
  u32FileOffset = u32BlockOffset - u32FileStart;
  if( NULL != gcsFilesOnDrive[ u8FileIndex ].pWriteHandler )
  {
    gcsFilesOnDrive[ u8FileIndex ].pWriteHandler(u32FileOffset*MASS_BLOCK_SIZE, pu8Buffer, u32BufferSize );
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
 * \brief  Initializes the medium
 * \param  -
 * \return -
 *********************************************************************/
void USBMediumAccess_Init( void )
{
  geMediumState = USB_MEDIUM_OK;
}

 /*! *******************************************************************
 * \brief  Returns the status of the medium
 * \param  -
 * \return Virtual drive status
 *********************************************************************/
E_USB_MEDIUM_STATE USBMediumAccess_GetStatus( void )
{
  return geMediumState;
}

 /*! *******************************************************************
 * \brief  Ejects the medium
 * \param  -
 * \return -
 *********************************************************************/
void USBMediumAccess_Eject( void )
{
  geMediumState = USB_MEDIUM_FAIL;
}

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
    if( 0u == u32CurrentBlock)  // boot block
    {
      ReadBootBlock( 0u, &(pu8Buffer[ u32Index ]), MASS_BLOCK_SIZE );
    }
    else if( FAT_BLOCKS_START+2u*NUM_FAT_BLOCKS > u32CurrentBlock )  // FAT
    {
      if( FAT_BLOCKS_START+NUM_FAT_BLOCKS <= u32CurrentBlock )  // FAT2
      {
        ReadFAT( u32CurrentBlock - FAT_BLOCKS_START - NUM_FAT_BLOCKS, &(pu8Buffer[ u32Index ]), MASS_BLOCK_SIZE );
      }
      else  // FAT1
      {
        ReadFAT( u32CurrentBlock - FAT_BLOCKS_START, &(pu8Buffer[ u32Index ]), MASS_BLOCK_SIZE );
      }
    }
    else if( FIRST_FILE_BLOCK > u32CurrentBlock )  // Root directory
    {
      ReadRootDir( u32CurrentBlock - (FAT_BLOCKS_START+2u*NUM_FAT_BLOCKS), &(pu8Buffer[ u32Index ]), MASS_BLOCK_SIZE );
    }
    else  // Files or no data
    {
      memset( &(pu8Buffer[ u32Index ]), 0, MASS_BLOCK_SIZE );
      ReadFile( u32CurrentBlock - FIRST_FILE_BLOCK, &(pu8Buffer[ u32Index ]), MASS_BLOCK_SIZE );
    }
  }
}

/*! *******************************************************************
 * \brief  Writes blocks to the virtual media
 * \param  u32StartBlock: starts writing from this block
 * \param  pu8Buffer: these blocks are written
 * \param  u32BlockNum: number of blocks to be written
 * \return -
 *********************************************************************/
void USBMediumAccess_Write( U32 u32StartBlock, U8* pu8Buffer, U32 u32BlockNum )
{
  U32 u32Index, u32CurrentBlock;
  
  for( u32Index = 0u; u32Index < u32BlockNum; u32Index++ )
  {
    u32CurrentBlock = u32StartBlock + u32Index;
    // Boot block, FAT and root directory writes are discarded
    if( FIRST_FILE_BLOCK <= u32StartBlock )
    {
      WriteFile( u32CurrentBlock - FIRST_FILE_BLOCK, &(pu8Buffer[ u32Index ]), MASS_BLOCK_SIZE );
    }
  }
}


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

void LEDControlCallback( U32 u32FileOffset, U8* pu8Buffer, U32 u32Size )
{
  for( U32 u32BufferIndex = 0u; u32BufferIndex < u32Size; u32BufferIndex++ )
  {
    if( 0u == u32FileOffset )  // We're only interested in the first byte of the file
    {
      if( pu8Buffer[ u32BufferIndex ] & 0x01u )  // Odd value, such as 1 or '1'
      {
        HAL_GPIO_WritePin( LCD_BACKLIGHT_GPIO_Port, LCD_BACKLIGHT_Pin, GPIO_PIN_SET );
      } 
      else  // Even value, such as 0 or '0'
      {
        HAL_GPIO_WritePin( LCD_BACKLIGHT_GPIO_Port, LCD_BACKLIGHT_Pin, GPIO_PIN_RESET );
      }
    }
    break;
  }
}

void FlashReadCallback( U32 u32FileOffset, U8* pu8Buffer, U32 u32Size ) 
{
  SPIFlash_Read_Polling( u32FileOffset, pu8Buffer, u32Size );
}

void FlashWriteCallback( U32 u32FileOffset, U8* pu8Buffer, U32 u32Size ) 
{
  /*
  // If this is the first block of the file, then erase the flash
  if( 0u == u32FileOffset )
  {
    SPIFlash_ChipErase_Polling();
  }
  */

  // Full chip erase is slow at high capacity NOR flash, so we use sector erase
  if( 0u == (u32FileOffset % 65536u) )  // 64kB boundary
  {
    SPIFlash_EraseSector_Polling( u32FileOffset );
  }
  
  SPIFlash_Write_Polling( u32FileOffset, pu8Buffer, u32Size );
}

void LongfileCallback( U32 u32FileOffset, U8* pu8Buffer, U32 u32Size ) 
{
  for( U32 u32BufferIndex = 0u; u32BufferIndex < u32Size; u32BufferIndex++ )
  {
    U8 u8ReturnVal = 0u;
    if( u32FileOffset < LONGFILE_SIZE ) 
    {
      //make each line 32 charactors.
      U16 u16LineCount = u32FileOffset / 32u;
      U8 u8LineOffset = u32FileOffset % 32u;
      if( 30u == u8LineOffset ) 
      {
        u8ReturnVal = '\r';
      } 
      else if( 31u == u8LineOffset ) 
      {
        u8ReturnVal = '\n';
      }
      else if( (u16LineCount % 32u) == u8LineOffset ) 
      {
        u8ReturnVal = '*';
      } 
      else 
      {
        u8ReturnVal = ' ';
      }
    }
    pu8Buffer[ u32BufferIndex ] = u8ReturnVal;
    u32FileOffset++;
  }
}

//! \brief Files on the virtual drive
const S_FILE_DESCRIPTOR gcsFilesOnDrive[] = 
{
  {
    .au8FileName = {'R', 'E', 'A', 'D', 'M', 'E', ' ', ' ', 'T', 'X', 'T'},
    .au8FileTime = {TIME_LOW(12, 34, 56), TIME_HIGH(12, 34, 56)},
    .au8FileDate = {DATE_LOW(2023,1,20), DATE_HIGH(2023,1,20)},
    .u32FileSize = sizeof(cau8ReadmeFile) - 1u,  // We don't want to display the string terminator
    .eReadCallbackType = CONST_DATA_FILE,
    .uReadHandler = {.pu8ConstantArray = (U8*)cau8ReadmeFile},
    .pWriteHandler = NULL,
  },
  {
    .au8FileName = {'L', 'O', 'N', 'G', 'F', 'I', 'L', 'E', 'T', 'X', 'T'},
    .au8FileTime = {TIME_LOW(12, 34, 56), TIME_HIGH(12, 34, 56)},
    .au8FileDate = {DATE_LOW(2023,1,20), DATE_HIGH(2023,1,20)},
    .u32FileSize = LONGFILE_SIZE,
    .eReadCallbackType = FUNCTION_CALLBACK_FILE,
    .uReadHandler = {.pFuncRead = LongfileCallback},
    .pWriteHandler = NULL,
  },
  {
    .au8FileName = {'L', 'E', 'D', '_', 'C', 'T', 'R', 'L', 'T', 'X', 'T'},
    .au8FileTime = {TIME_LOW(12, 34, 56), TIME_HIGH(12, 34, 56)},
    .au8FileDate = {DATE_LOW(2023,1,20), DATE_HIGH(2023,1,20)},
    .u32FileSize = sizeof(cau8LEDControlFile) - 1u,  // We don't want to display the string terminator
    .eReadCallbackType = CONST_DATA_FILE,
    .uReadHandler = {.pu8ConstantArray = (U8*)cau8LEDControlFile},
    .pWriteHandler = LEDControlCallback,
  },
  {
    .au8FileName = {'S', 'P', 'I', 'F', 'L', 'A', 'S', 'H', 'B', 'I', 'N'},
    .au8FileTime = {TIME_LOW(12, 34, 56), TIME_HIGH(12, 34, 56)},
    .au8FileDate = {DATE_LOW(2023,1,20), DATE_HIGH(2023,1,20)},
    .u32FileSize = 16u*1024*1024u,  // 16 Mbytes
    .eReadCallbackType = FUNCTION_CALLBACK_FILE,
    .uReadHandler = {.pFuncRead = FlashReadCallback},
    .pWriteHandler = FlashWriteCallback,
  }
};

//! \brief The number of files constant is calculated here
const U8 gcu8NumFilesOnDrive = (sizeof(gcsFilesOnDrive)/sizeof(S_FILE_DESCRIPTOR) );
// The number of files (including the root directory) should not exceed the maximum
STATIC_ASSERT( (sizeof(gcsFilesOnDrive)/sizeof(S_FILE_DESCRIPTOR) ) < MAX_NUM_FILES_ROOT );


//-----------------------------------------------< EOF >--------------------------------------------------/
