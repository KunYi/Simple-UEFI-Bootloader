#include "Bootloader.h"

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  //ImageHandle is this program's own EFI_HANDLE
  InitializeLib(ImageHandle, SystemTable);
  /*
  From InitializeLib:

  ST = SystemTable;
  BS = SystemTable->BootServices;
  RT = SystemTable->RuntimeServices;

  */
  EFI_STATUS Status;

  //Comment out when done
  Status = BS->SetWatchdogTimer (0, 0, 0, NULL); // Disable watchdog timer for debugging
  if(EFI_ERROR(Status))
  {
    Print(L"Error stopping watchdog, timeout still be counting down...\r\n");
  }

  EFI_TIME Now;
  Status = RT->GetTime(&Now, NULL);
  if(EFI_ERROR(Status))
  {
    Print(L"Error getting time...\r\n");
    return Status;
  }
  Print(L"%02hhu/%02hhu/%04hu - %02hhu:%02hhu:%02hhu.%u\r\n\n", Now.Month, Now.Day, Now.Year, Now.Hour, Now.Minute, Now.Second, Now.Nanosecond);
  Print(L"System Table Header Info\r\nSignature: 0x%lx\r\nRevision: 0x%08x\r\nHeaderSize: %u Bytes\r\nCRC32: 0x%08x\r\nReserved: 0x%x\r\n\n", ST->Hdr.Signature, ST->Hdr.Revision, ST->Hdr.HeaderSize, ST->Hdr.CRC32, ST->Hdr.Reserved);
  Print(L"Firmware Vendor: %s\r\nFirmware Revision: 0x%08x\r\n\n", ST->FirmwareVendor, ST->FirmwareRevision);
  Print(L"Configuration Table Info\r\n");
  Print(L"Number of tables: %lu\r\n", ST->NumberOfTableEntries);

  UINT8 RSDPfound = 0;
  UINT8 RSDP_index = 0;

  for(int i=0; i < ST->NumberOfTableEntries; i++)
  {
    Print(L"Table %d GUID: %08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x\r\n", i,
            ST->ConfigurationTable[i].VendorGuid.Data1,
            ST->ConfigurationTable[i].VendorGuid.Data2,
            ST->ConfigurationTable[i].VendorGuid.Data3,
            ST->ConfigurationTable[i].VendorGuid.Data4[0],
            ST->ConfigurationTable[i].VendorGuid.Data4[1],
            ST->ConfigurationTable[i].VendorGuid.Data4[2],
            ST->ConfigurationTable[i].VendorGuid.Data4[3],
            ST->ConfigurationTable[i].VendorGuid.Data4[4],
            ST->ConfigurationTable[i].VendorGuid.Data4[5],
            ST->ConfigurationTable[i].VendorGuid.Data4[6],
            ST->ConfigurationTable[i].VendorGuid.Data4[7]);

    if (compare(&ST->ConfigurationTable[i].VendorGuid, &Acpi20TableGuid, 16))
    {
      Print(L"RSDP 2.0 found!\r\n");
      RSDP_index = i;
      RSDPfound = 2;
    }
  }
// If no RSDP 2.0, check for 1.0
  if(!RSDPfound)
  {
    for(int i=0; i < ST->NumberOfTableEntries; i++)
    {
      if (compare(&ST->ConfigurationTable[i].VendorGuid, &AcpiTableGuid, 16))
      {
        Print(L"RSDP 1.0 found!\r\n");
        RSDP_index = i;
        RSDPfound = 1;
      }
    }
  }

  if(!RSDPfound)
  {
    Print(L"Invalid system: no RSDP.\r\n");
    return EFI_INVALID_PARAMETER;
  }

// From http://wiki.osdev.org/UEFI_Bare_Bones

    EFI_INPUT_KEY Key;

    /* Say hi */
    Status = ST->ConOut->OutputString(ST->ConOut, L"Press any key to continue...");
    if (EFI_ERROR(Status))
        return Status;

    /* Now wait for a keystroke before continuing, otherwise your
       message will flash off the screen before you see it.

       First, we need to empty the console input buffer to flush
       out any keystrokes entered before this point */
    Status = ST->ConIn->Reset(ST->ConIn, FALSE);
    if (EFI_ERROR(Status))
        return Status;

    /* Now wait until a key becomes available.  This is a simple
       polling implementation.  You could try and use the WaitForKey
       event instead if you like */
    while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);
// End from  http://wiki.osdev.org/UEFI_Bare_Bones
    Print(L"\r\n");

// Next step: Load a C program and exit boot services
// (will use BS-> stuff)
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE Graphics;
    Graphics = InitUEFI_GOP(ImageHandle);
    Keywait(L"InitUEFI_GOP finished.\r\n");
    Print(L"RSDP address: 0x%llx\r\n", ST->ConfigurationTable[RSDP_index].VendorTable);
    Print(L"Data at RSDP (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(ST->ConfigurationTable[RSDP_index].VendorTable + 8), *(EFI_PHYSICAL_ADDRESS*)ST->ConfigurationTable[RSDP_index].VendorTable);

    Status = GoTime(ImageHandle, Graphics, ST->ConfigurationTable[RSDP_index].VendorTable);

// To evaluate any errors
    Keywait(L"GoTime returned...\r\n");
    return Status;
}

EFI_STATUS EFIAPI Keywait(CHAR16 *String)
{
  EFI_STATUS Status;
  EFI_INPUT_KEY Key;
  Print(String);
  Status = ST->ConOut->OutputString(ST->ConOut, L"Press any key to continue...");
  if (EFI_ERROR(Status))
      return Status;

  Status = ST->ConIn->Reset(ST->ConIn, FALSE);
  if (EFI_ERROR(Status))
      return Status;

  while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);
  Print(L"\r\n");

  return Status;
}

int EFIAPI compare(const void* firstitem, const void* seconditem, size_t comparelength)
{
  const unsigned char *one = firstitem, *two = seconditem;
  for (size_t i = 0; i < comparelength; i++)
  {
    if(one[i] != two[i])
    {
      return 0;
    }
  }
  return 1;
}

//==================================================================================================================================
//  InitUEFI_GOP: Graphics Initialization
//==================================================================================================================================

EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE EFIAPI InitUEFI_GOP(EFI_HANDLE ImageHandle)
{
  // Insert graphics framebuffer init here
  EFI_STATUS GOPStatus;

  EFI_HANDLE *GraphicsHandles; // Array of discovered graphics handles
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *GOPInfo;
  UINT64 GOPInfoSize;
  UINT32 mode;
  UINTN NumHandlesInHandleBuffer = 0;
  UINT64 DevNum = 0;
  EFI_INPUT_KEY Key;

  Keywait(L"Allocating pools...\r\n");
  EFI_GRAPHICS_OUTPUT_PROTOCOL *GOPTable;
  GOPStatus = ST->BootServices->AllocatePool(EfiLoaderData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&GOPTable->Mode); // Reserve memory for mode to preserve it
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"GOP Mode AllocatePool error. 0x%llx\r\n", GOPStatus);
    return *(GOPTable->Mode);
  }

  // Allocate pool for information storage
  GOPStatus = ST->BootServices->AllocatePool(EfiLoaderData, GOPTable->Mode->SizeOfInfo, (void**)&GOPTable->Mode->Info); // Reserve memory for mode to preserve it
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"GOP Mode->Info AllocatePool error. 0x%llx\r\n", GOPStatus);
    return *(GOPTable->Mode);
  }

  Key.UnicodeChar = 0;
  Keywait(L"Allocated.\r\n");
/*
  GOPStatus = BS->LocateProtocol(&GraphicsOutputProtocol, NULL, (void **)&GOPTable);
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"GraphicsTable LocateProtocol error. 0x%llx\r\n", GOPStatus);
    return GOPStatus;
  }
  Keywait(L"Protocol found!\r\n");
*/
// LocateProtocol gives us the first device it finds.
// Alternatively, we can pick which GOP we want...
  GOPStatus = BS->LocateHandleBuffer(ByProtocol, &GraphicsOutputProtocol, NULL, &NumHandlesInHandleBuffer, &GraphicsHandles);
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"GraphicsTable LocateHandleBuffer error. 0x%llx\r\n", GOPStatus);
    return *(GOPTable->Mode);
  }
  Print(L"There are %llu GOP-supporting devices.\r\n", NumHandlesInHandleBuffer);

  while(0x30 > Key.UnicodeChar || Key.UnicodeChar > (0x30 + NumHandlesInHandleBuffer - 1))
  {
    for(DevNum = 0; DevNum < NumHandlesInHandleBuffer; DevNum++)
    {
      Print(L"%c. 0x%llx\r\n", DevNum + 0x30, GraphicsHandles[DevNum]);
    }
    Print(L"Select an output device. (0 - %llu)\r\n", NumHandlesInHandleBuffer - 1);
    while ((GOPStatus = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);
    Print(L"Device %c selected.\r\n", Key.UnicodeChar);
  }
  DevNum = (UINT64)(Key.UnicodeChar - 0x30);
  Key.UnicodeChar = 0; // Reset input

  Print(L"Using handle %llu...\r\n", DevNum);
  GOPStatus = BS->OpenProtocol(GraphicsHandles[DevNum], &GraphicsOutputProtocol, (void**)&GOPTable, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"GraphicsTable OpenProtocol error. 0x%llx\r\n", GOPStatus);
    return *(GOPTable->Mode);
  }
  Keywait(L"OpenProtocol passed.\r\n");

  Print(L"Current GOP Mode Info:\r\n");
  Print(L"Max Mode supported: %u, Current Mode: %u\r\nSize of Mode Info Structure: %llu Bytes\r\n", GOPTable->Mode->MaxMode - 1, GOPTable->Mode->Mode, GOPTable->Mode->SizeOfInfo);
  Print(L"FrameBufferBase: 0x%016llx, FrameBufferSize: 0x%llx\r\n", GOPTable->Mode->FrameBufferBase, GOPTable->Mode->FrameBufferSize);
  Keywait(L"\0");
  // This is how to get supported graphics modes
  for(mode = 0; mode < GOPTable->Mode->MaxMode; mode++) // MaxMode does not include 0, so valid modes are from 0 to MaxMode - 1
  {
    GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPInfo); // IN IN OUT OUT
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GraphicsTable QueryMode error. 0x%llx\r\n", GOPStatus);
      return *(GOPTable->Mode);
    }
    Print(L"Mode %u of %u (%llu Bytes):\r\n Ver: 0x%x, Res: %ux%u\r\n", mode, GOPTable->Mode->MaxMode - 1, GOPInfoSize, GOPInfo->Version, GOPInfo->HorizontalResolution, GOPInfo->VerticalResolution);
    Print(L"PxPerScanLine: %u\r\n", GOPInfo->PixelsPerScanLine);
    Print(L"PxFormat: 0x%x, PxInfo (R,G,B,Rsvd Masks): 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n", GOPInfo->PixelFormat, GOPInfo->PixelInformation.RedMask, GOPInfo->PixelInformation.GreenMask, GOPInfo->PixelInformation.BlueMask, GOPInfo->PixelInformation.ReservedMask);
    Keywait(L"\0");
  }
  Keywait(L"\r\nGetting list of supported modes...\r\n");

  Print(L"%u available graphics modes found.\r\n", GOPTable->Mode->MaxMode);
  while(0x30 > Key.UnicodeChar || Key.UnicodeChar > (0x30 + GOPTable->Mode->MaxMode - 1))
  {
    for(mode = 0; mode < GOPTable->Mode->MaxMode; mode++) // MaxMode does not include 0, so valid modes are from 0 to MaxMode - 1
    {
      GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPInfo); // IN IN OUT OUT
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GraphicsTable QueryMode error. 0x%llx\r\n", GOPStatus);
        return *(GOPTable->Mode);
      }
      Print(L"%c. %ux%u\r\n", mode + 0x30, GOPInfo->HorizontalResolution, GOPInfo->VerticalResolution);
    }

    Print(L"Select a graphics mode. (0 - %u)\r\n", GOPTable->Mode->MaxMode - 1);
    while ((GOPStatus = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);
    Print(L"Selected graphics mode %c.\r\n", Key.UnicodeChar);
  }
  //
  // Allow user choice
  //
  mode = (UINT32)(Key.UnicodeChar - 0x30);
  Key.UnicodeChar = 0;
// Query mode to get size
/*
  GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPInfo); // IN IN OUT OUT
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"GraphicsTable QueryMode error 2. 0x%llx\r\n", GOPStatus);
    return *(GOPTable->Mode);
  }
*/
  Print(L"Setting graphics mode %u of %u.\r\n", mode, GOPTable->Mode->MaxMode - 1);

  // Clear screen to reset cursor position
  ST->ConOut->ClearScreen(ST->ConOut);

  // Set mode
  GOPStatus = GOPTable->SetMode(GOPTable, mode);
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"GraphicsTable SetMode error. 0x%llx\r\n", GOPStatus);
    return *(GOPTable->Mode);
  }

  Print(L"\r\nCurrent GOP Mode Info:\r\n");
  Print(L"Max Mode supported: %u, Current Mode: %u\r\nSize of Mode Info Structure: %llu Bytes\r\n", GOPTable->Mode->MaxMode - 1, GOPTable->Mode->Mode, GOPTable->Mode->SizeOfInfo);
  Print(L"FrameBufferBase: 0x%016llx, FrameBufferSize: 0x%llx\r\n", GOPTable->Mode->FrameBufferBase, GOPTable->Mode->FrameBufferSize);

  Print(L"Mode %u of %u (%llu Bytes):\r\n Ver: 0x%x, Res: %ux%u\r\n", mode, GOPTable->Mode->MaxMode - 1, GOPTable->Mode->SizeOfInfo, GOPTable->Mode->Info->Version, GOPTable->Mode->Info->HorizontalResolution, GOPTable->Mode->Info->VerticalResolution);
  Print(L"PxPerScanLine: %u\r\n", GOPTable->Mode->Info->PixelsPerScanLine);
  Print(L"PxFormat: 0x%x, PxInfo (R,G,B,Rsvd Masks): 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n", GOPTable->Mode->Info->PixelFormat, GOPTable->Mode->Info->PixelInformation.RedMask, GOPTable->Mode->Info->PixelInformation.GreenMask, GOPTable->Mode->Info->PixelInformation.BlueMask, GOPTable->Mode->Info->PixelInformation.ReservedMask);
  Keywait(L"\0");

  return *(GOPTable->Mode);
}
/*
struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
  EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE  QueryMode; // function
  EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE    SetMode; // function
  EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT         Blt; // function
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE        *Mode; // structure
};



typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE) (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
  IN  UINT32                                ModeNumber,
  OUT UINTN                                 *SizeOfInfo,
  OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info
  )
;

typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE) (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL *This,
  IN  UINT32                       ModeNumber
  );

typedef
EFI_STATUS
(EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_BLT) (
  IN  EFI_GRAPHICS_OUTPUT_PROTOCOL            *This,
  IN  EFI_GRAPHICS_OUTPUT_BLT_PIXEL           *BltBuffer,   OPTIONAL
  IN  EFI_GRAPHICS_OUTPUT_BLT_OPERATION       BltOperation,
  IN  UINTN                                   SourceX,
  IN  UINTN                                   SourceY,
  IN  UINTN                                   DestinationX,
  IN  UINTN                                   DestinationY,
  IN  UINTN                                   Width,
  IN  UINTN                                   Height,
  IN  UINTN                                   Delta         OPTIONAL
  );

typedef struct {
  UINT32                                 MaxMode;
  UINT32                                 Mode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION   *Info;
  UINTN                                  SizeOfInfo;
  EFI_PHYSICAL_ADDRESS                   FrameBufferBase;
  UINTN                                  FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;




typedef struct {
  UINT32                     Version;
  UINT32                     HorizontalResolution;
  UINT32                     VerticalResolution;
  EFI_GRAPHICS_PIXEL_FORMAT  PixelFormat; // 0 - 4
  EFI_PIXEL_BITMASK          PixelInformation;
  UINT32                     PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef enum {
  PixelRedGreenBlueReserved8BitPerColor, // 0
  PixelBlueGreenRedReserved8BitPerColor, // 1
  PixelBitMask, // 2
  PixelBltOnly, // 3
  PixelFormatMax // 4
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
  UINT32            RedMask;
  UINT32            GreenMask;
  UINT32            BlueMask;
  UINT32            ReservedMask;
} EFI_PIXEL_BITMASK;




typedef enum {
  EfiBltVideoFill, // 0
  EfiBltVideoToBltBuffer, // 1
  EfiBltBufferToVideo, // 2
  EfiBltVideoToVideo, // 3
  EfiGraphicsOutputBltOperationMax // 4
} EFI_GRAPHICS_OUTPUT_BLT_OPERATION;

// 24-bit RGB, no alpha
typedef struct {
  UINT8 Blue;
  UINT8 Green;
  UINT8 Red;
  UINT8 Reserved;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL;

typedef union {
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL Pixel;
  UINT32                        Raw;
} EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION;

*/

//==================================================================================================================================
//  GoTime: Kernel Loader
//==================================================================================================================================

EFI_STATUS EFIAPI GoTime(EFI_HANDLE ImageHandle, EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE Graphics, void *RSDPTable)
{
// Integrity check
  Print(L"GPU Mode: %u of %u\r\n", Graphics.Mode, Graphics.MaxMode - 1);
  Print(L"GPU FB: 0x%016llx\r\n", Graphics.FrameBufferBase);
  Print(L"GPU FB Size: 0x%016llx\r\n", Graphics.FrameBufferSize);
  Print(L"GPU SizeOfInfo: %u Bytes\r\n", Graphics.SizeOfInfo);
  Print(L"GPU Info Ver: 0x%x\r\n", Graphics.Info->Version);
  Print(L"GPU Info Res: %ux%u\r\n", Graphics.Info->HorizontalResolution, Graphics.Info->VerticalResolution);
  Print(L"GPU Info PxFormat: 0x%x\r\n", Graphics.Info->PixelFormat);
  Print(L"GPU Info PxInfo (R,G,B,Rsvd Masks): 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n", Graphics.Info->PixelInformation.RedMask, Graphics.Info->PixelInformation.GreenMask, Graphics.Info->PixelInformation.BlueMask, Graphics.Info->PixelInformation.ReservedMask);
  Print(L"GPU Info PxPerScanLine: %u\r\n", Graphics.Info->PixelsPerScanLine);

  Print(L"GO GO GO!!!\r\n");
  EFI_STATUS GoTimeStatus;

//Load file called Kernel64 from this drive's root

	EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
/*  GoTimeStatus = ST->BootServices->AllocatePool(EfiLoaderData, sizeof(EFI_LOADED_IMAGE_PROTOCOL), (void**)&LoadedImage); // Reserve memory for mode to preserve it
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"LoadedImage AllocatePool error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }
*/
  // Get a pointer to the (loaded image) pointer of BOOTX64.EFI
  // Pointer 1 -> Pointer 2 -> BOOTX64.EFI
  // OpenProtocol wants Pointer 1 as input to give you Pointer 2.
	GoTimeStatus = ST->BootServices->OpenProtocol(ImageHandle, &LoadedImageProtocol, (void**)&LoadedImage, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"LoadedImage OpenProtocol error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }

  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *FileSystem;
/*  GoTimeStatus = ST->BootServices->AllocatePool(EfiLoaderData, sizeof(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL), (void**)&FileSystem); // Reserve memory for mode to preserve it
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"FileSystem AllocatePool error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }
*/
  // Parent device of BOOTX64.EFI (the imagehandle passed in originally is this very file)
  // Loadedimage is an EFI_LOADED_IMAGE_PROTOCOL pointer that points to BOOTX64.EFI
  GoTimeStatus = ST->BootServices->OpenProtocol(LoadedImage->DeviceHandle, &FileSystemProtocol, (void**)&FileSystem, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"FileSystem OpenProtocol error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }

  EFI_FILE *CurrentDriveRoot;
  GoTimeStatus = FileSystem->OpenVolume(FileSystem, &CurrentDriveRoot);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"OpenVolume error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }

  EFI_FILE *KernelFile;
  // Open the kernel file from current drive root and point to it with KernelFile
	GoTimeStatus = CurrentDriveRoot->Open(CurrentDriveRoot, &KernelFile, L"Kernel64", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	if (EFI_ERROR(GoTimeStatus))
  {
		ST->ConOut->OutputString(ST->ConOut, L"Kernel file is missing\r\n");
		return GoTimeStatus;
	}
  Keywait(L"Kernel file opened.\r\n");

  // Get address of start of file
//  UINT64 FileStartPosition;
//  KernelFile->GetPosition(KernelFile, &FileStartPosition);
//  Keywait(L"Kernel file start position acquired.\r\n");

// Default image base
  EFI_PHYSICAL_ADDRESS Header_memory = 0x400000; // Default ImageBase for 64-bit PE DLLs

  UINTN FileInfoSize;

  GoTimeStatus = KernelFile->GetInfo(KernelFile, &gEfiFileInfoGuid, &FileInfoSize, NULL);
  // This will intentionally error out and provide the correct fileinfosize value
  Print(L"FileInfoSize: %llu Bytes\r\n", FileInfoSize);
  EFI_FILE_INFO *FileInfo;
  GoTimeStatus = ST->BootServices->AllocatePool(EfiLoaderData, FileInfoSize, (void**)&FileInfo); // Reserve memory for file info/attributes and such, to prevent it from getting run over
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"FileInfo AllocatePool error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }

  GoTimeStatus = KernelFile->GetInfo(KernelFile, &gEfiFileInfoGuid, &FileInfoSize, FileInfo);
  Print(L"FileInfoSize 2: %llu Bytes\r\n", FileInfoSize);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"GetInfo error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }

  Print(L"FileName: %s\r\n", FileInfo->FileName);
  Print(L"Size: %llu\r\n", FileInfo->Size);
  Print(L"FileSize: %llu\r\n", FileInfo->FileSize);
  Print(L"PhysicalSize: %llu\r\n", FileInfo->PhysicalSize);
  Print(L"Attribute: %llx\r\n", FileInfo->Attribute);
  /*
  Attributes:

  #define EFI_FILE_READ_ONLY 0x0000000000000001
  #define EFI_FILE_HIDDEN 0x0000000000000002
  #define EFI_FILE_SYSTEM 0x0000000000000004
  #define EFI_FILE_RESERVED 0x0000000000000008
  #define EFI_FILE_DIRECTORY 0x0000000000000010
  #define EFI_FILE_ARCHIVE 0x0000000000000020
  #define EFI_FILE_VALID_ATTR 0x0000000000000037

  */
  Print(L"Created: %02hhu/%02hhu/%04hu - %02hhu:%02hhu:%02hhu.%u\r\n", FileInfo->CreateTime.Month, FileInfo->CreateTime.Day, FileInfo->CreateTime.Year, FileInfo->CreateTime.Hour, FileInfo->CreateTime.Minute, FileInfo->CreateTime.Second, FileInfo->CreateTime.Nanosecond);
  Print(L"Last Modified: %02hhu/%02hhu/%04hu - %02hhu:%02hhu:%02hhu.%u\r\n", FileInfo->ModificationTime.Month, FileInfo->ModificationTime.Day, FileInfo->ModificationTime.Year, FileInfo->ModificationTime.Hour, FileInfo->ModificationTime.Minute, FileInfo->ModificationTime.Second, FileInfo->ModificationTime.Nanosecond);



  Keywait(L"GetInfo memory allocated and populated.\r\n");

  // read file header
//  UINTN size = 0x40; // Size of DOS header
  UINTN size = sizeof(IMAGE_DOS_HEADER);
  IMAGE_DOS_HEADER DOSheader;
/*
  GoTimeStatus = ST->BootServices->AllocatePool(EfiLoaderData, size, (void**)&DOSheader);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"DOS Header AllocatePool error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }
*/
  GoTimeStatus = KernelFile->Read(KernelFile, &size, &DOSheader);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"DOSheader read error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }

  Keywait(L"DOS Header read from file.\r\n");

//Check if PE or ELF header

  if(DOSheader.e_magic == IMAGE_DOS_SIGNATURE) // Check out pe.h in gnu-efi for help with data types
  {
    //MZ
    Keywait(L"DOS header passed.\r\n");
    Print(L"e_lfanew: 0x%x\r\n", DOSheader.e_lfanew);
//    Print(L"FileStart: 0x%llx\r\n", &FileStartPosition);
//    Print(L"Corrected filestart: 0x%llx\r\n", &FileStartPosition + (UINT64)DOSheader.e_lfanew);

// Get to the PE section of the header (Use the pointer at offset 0x3C, which contains the offset of the PE header relative to the start of the file)
    GoTimeStatus = KernelFile->SetPosition(KernelFile, (UINT64)DOSheader.e_lfanew); // Go to PE Header
    if(EFI_ERROR(GoTimeStatus))
    {
      Print(L"SetPosition error. 0x%llx\r\n", GoTimeStatus);
      return GoTimeStatus;
    }

//    size = 4 + IMAGE_SIZEOF_FILE_HEADER + IMAGE_SIZEOF_NT_OPTIONAL64_HEADER; // 264 bytes
    size = sizeof(IMAGE_NT_HEADERS64);
    IMAGE_NT_HEADERS64 PEHeader;
/*
    GoTimeStatus = ST->BootServices->AllocatePool(EfiLoaderData, size, (void**)&PEHeader);
    if(EFI_ERROR(GoTimeStatus))
    {
      Print(L"PE Header AllocatePool error. 0x%llx\r\n", GoTimeStatus);
      return GoTimeStatus;
    }
*/

    GoTimeStatus = KernelFile->Read(KernelFile, &size, &PEHeader);
    if(EFI_ERROR(GoTimeStatus))
    {
      Print(L"PE header read error. 0x%llx\r\n", GoTimeStatus);
      return GoTimeStatus;
    }

    Print(L"PE Header Signature: 0x%x\r\n", PEHeader.Signature);

    //NOTE: SetPosition is RELATIVE TO THE FILE. Phoenix wiki is badly worded:
    // the position is not "absolute" (implying absolute memory location with
    // respect to system memory address 0x0), it's "absolute relative to the
    // file start"
    if(PEHeader.Signature == IMAGE_NT_SIGNATURE)
    {
      //PE
      Keywait(L"PE header passed.\r\n");

      if(PEHeader.FileHeader.Machine == IMAGE_FILE_MACHINE_X64 && PEHeader.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) // PE Headers have Signature, FileHeader, and OptionalHeader
      {
        //PE32+
        Keywait(L"PE32+ header passed.\r\n");

        if (PEHeader.OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_EFI_APPLICATION) // Was it compiled with -Wl,--subsystem,10 (MINGW-W64 GCC)?
        {
          GoTimeStatus = EFI_INVALID_PARAMETER;
          Print(L"Not a UEFI PE32+ application...\r\n");
          Print(L"Subsystem: %hu\r\n", PEHeader.OptionalHeader.Subsystem); // If it's 3, it was compiled as a Windows CUI (command line) program. Just needs to be linked with the above GCC flag.
          return GoTimeStatus;
        }
        Keywait(L"UEFI PE32+ header passed.\r\n");

        UINT64 i;
        UINT64 virt_size = 0; // Size of all the data sections combined, which we need to know in order to allocate the right number of pages
        UINT64 Numofsections = (UINT64)PEHeader.FileHeader.NumberOfSections; // Number of sections described at end of PE headers
        IMAGE_SECTION_HEADER section_headers_table[Numofsections]; // This table is an array of section headers
        size = IMAGE_SIZEOF_SECTION_HEADER*Numofsections; //size of section header table in file... Hm...

//        IMAGE_SECTION_HEADER *section_headers_table_pointer = section_headers_table; // Pointer to the first section header is the same as a pointer to the table

        Print(L"Numofsections: %llu, size: %llu\r\n", Numofsections, size);
//        Print(L"section_headers_table_pointer: 0x%llx\r\n&section_headers_table[0]: 0x%llx\r\n", section_headers_table_pointer, &section_headers_table[0]);
        Keywait(L"\0");
/*
        GoTimeStatus = ST->BootServices->AllocatePool(EfiLoaderData, size, (void**)&section_headers_table_pointer);
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Section headers table AllocatePool error. 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }
*/
        // Cursor is already at the end of the PE Header
        GoTimeStatus = KernelFile->Read(KernelFile, &size, &section_headers_table[0]); // Run right over the section table, it should be exactly the size to hold this data
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Section headers table read error. 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }

        // Section headers table comes right after the PE Optional Header
        for(i = 0; i < Numofsections; i++) // Go through each section of the "sections" section to get the address boundary of the last section
        {
          IMAGE_SECTION_HEADER *specific_section_header = &section_headers_table[i];
          Print(L"current section address: 0x%x, size: 0x%x\r\n", specific_section_header->VirtualAddress, specific_section_header->Misc.VirtualSize);
          Print(L"current section address + size 0x%x\r\n", specific_section_header->VirtualAddress + specific_section_header->Misc.VirtualSize);
          virt_size = (virt_size > (UINT64)(specific_section_header->VirtualAddress + specific_section_header->Misc.VirtualSize) ? virt_size: (UINT64)(specific_section_header->VirtualAddress + specific_section_header->Misc.VirtualSize));
        }
        Print(L"virt_size: 0x%llx\r\n", virt_size);
        Keywait(L"Section Headers table passed.\r\n");

        UINTN Total_image_size = (UINTN)PEHeader.OptionalHeader.SizeOfImage;
        UINTN Header_size = (UINTN)PEHeader.OptionalHeader.SizeOfHeaders;
        Print(L"Total image size: %llu Bytes\r\nHeaders total size: %llu Bytes\r\n", Total_image_size, Header_size);
        //Note: This implies the max file size for a PE executable is 4GB (SizeOfImage is a UINT32).
        // In any event, this has to be loaded from FAT32. You can't have a file larger than 4GB (32-bit max) on FAT32 anyways.
        // A 4GB bootloader, or even kernel, would be insane. You'd need to use 64-bit linux ELF for those.

        UINT64 pages = (virt_size + EFI_PAGE_MASK) >> EFI_PAGE_SHIFT; //To get number of pages (typically 4KB per), rounded up

        Print(L"pages: %llu\r\n", pages);
        Print(L"Expected ImageBase: 0x%llx\r\n", PEHeader.OptionalHeader.ImageBase);
        EFI_PHYSICAL_ADDRESS AllocatedMemory = PEHeader.OptionalHeader.ImageBase;
//        EFI_PHYSICAL_ADDRESS AllocatedMemory = 0x400000;
        Print(L"Address of AllocatedMemory: 0x%llx\r\n", &AllocatedMemory);
        GoTimeStatus = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &AllocatedMemory);
//        GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &AllocatedMemory);

        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Could not allocate pages for PE32+ sections. Error code: %lld\r\n", GoTimeStatus);
          return GoTimeStatus;
        }
        Print(L"AllocatedMemory location: 0x%llx\r\n", AllocatedMemory);
        Keywait(L"Allocate Pages passed.\r\n");

        // Map headers
        Print(L"\nLoading Headers:\r\n");
        Print(L"Check:\r\nSectionAddress: 0x%llx\r\nData there: 0x%016llx%016llx (Should be 0)\r\n", AllocatedMemory, *(EFI_PHYSICAL_ADDRESS*)(AllocatedMemory + 8), *(EFI_PHYSICAL_ADDRESS*)AllocatedMemory); // print the first 64 bits of that address to compare
        GoTimeStatus = KernelFile->SetPosition(KernelFile, 0);
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Error setting file position for mapping. 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }
        GoTimeStatus = KernelFile->Read(KernelFile, &Header_size, (EFI_PHYSICAL_ADDRESS*)AllocatedMemory); // Should go right up to the page boundary of the first main section
        // In other words the boundary of the first specific section header's virtual address.
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Error reading header data for mapping. 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }
        // little endian; print various 16 bytes to make sure data's in memory OK
        Print(L"\r\nVerify:\r\nSectionAddress: 0x%llx\r\nData there (first 16 bytes): 0x%016llx%016llx\r\n", AllocatedMemory, *(EFI_PHYSICAL_ADDRESS*)(AllocatedMemory + 8), *(EFI_PHYSICAL_ADDRESS*)AllocatedMemory);
        Print(L"Last 16 bytes: 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS *)(AllocatedMemory + Header_size - 8), *(EFI_PHYSICAL_ADDRESS *)(AllocatedMemory + Header_size - 16));
        Print(L"Next 16 bytes: 0x%016llx%016llx (should be 0)\r\n", *(EFI_PHYSICAL_ADDRESS *)(AllocatedMemory + Header_size + 8), *(EFI_PHYSICAL_ADDRESS *)(AllocatedMemory + Header_size));
        Keywait(L"\0");

        for(i = 0; i < Numofsections; i++) // Load sections into memory
        {
          IMAGE_SECTION_HEADER *specific_section_header = &section_headers_table[i];
          UINTN RawDataSize = (UINTN)specific_section_header->SizeOfRawData;
          EFI_PHYSICAL_ADDRESS SectionAddress = AllocatedMemory + (UINT64)specific_section_header->VirtualAddress;

          Print(L"\n%llu. current section address: 0x%x, RawDataSize: 0x%llx\r\n", i+1, specific_section_header->VirtualAddress, RawDataSize);
          Print(L"current destination address: 0x%llx, AllocatedMemory base: 0x%llx\r\n", SectionAddress, AllocatedMemory);
          Print(L"PointerToRawData: 0x%llx\r\n",  (UINT64)specific_section_header->PointerToRawData);
          Print(L"Check:\r\nSectionAddress: 0x%llx\r\nData there: 0x%016llx%016llx (should be 0)\r\n", SectionAddress, *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + 8), *(EFI_PHYSICAL_ADDRESS *)SectionAddress); // print the first 64 bits of that address to compare
          Print(L"About to load section %llu of %llu...\r\n", i + 1, Numofsections);
          Keywait(L"\0");

          GoTimeStatus = KernelFile->SetPosition(KernelFile, (UINT64)specific_section_header->PointerToRawData);
          if(EFI_ERROR(GoTimeStatus))
          {
            Print(L"Section SetPosition error. 0x%llx\r\n", GoTimeStatus);
            return GoTimeStatus;
          }

          GoTimeStatus = KernelFile->Read(KernelFile, &RawDataSize, (EFI_PHYSICAL_ADDRESS*)SectionAddress); // (void*)SectionAddress
          if(EFI_ERROR(GoTimeStatus))
          {
            Print(L"Section read error. 0x%llx\r\n", GoTimeStatus);
            return GoTimeStatus;
          }

          Print(L"\r\nVerify:\r\nSectionAddress: 0x%llx\r\nData there (first 16 bytes): 0x%016llx%016llx\r\n", SectionAddress, *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + 8), *(EFI_PHYSICAL_ADDRESS*)SectionAddress); // print the first 64 bits of that address to compare
          Print(L"Last 16 bytes: 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize - 8), *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize - 16));
          Print(L"Next 16 bytes: 0x%016llx%016llx (0 unless last section)\r\n", *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize + 8), *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize));
          // "Next 16 bytes" should be 0 unless last section
        }

        Keywait(L"\nLoad file sections into allocated pages passed.\r\n");

        // Apply relocation fixes, if necessary
        if(AllocatedMemory != PEHeader.OptionalHeader.ImageBase && PEHeader.OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC) // Need to perform relocations
        {
          IMAGE_BASE_RELOCATION * Relocation_Directory_Base;
          IMAGE_BASE_RELOCATION * RelocTableEnd;

          Relocation_Directory_Base = (IMAGE_BASE_RELOCATION*)(AllocatedMemory + (UINT64)PEHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
          RelocTableEnd = (IMAGE_BASE_RELOCATION*)(AllocatedMemory + (UINT64)PEHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size + (UINT64)PEHeader.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);

          // Determine by how much we are off the executable's intended ImageBase
          UINT64 delta;
          if(AllocatedMemory > PEHeader.OptionalHeader.ImageBase)
          {
            delta = AllocatedMemory - PEHeader.OptionalHeader.ImageBase; // Determine by how much we are off the executable's intended ImageBase
            Print(L"AllocatedMemory: 0x%llx, ImageBase: 0x%llx, Delta: 0x%llx\r\n", AllocatedMemory, PEHeader.OptionalHeader.ImageBase, delta);
          }
          else
          {
            delta = PEHeader.OptionalHeader.ImageBase - AllocatedMemory;
            Print(L"AllocatedMemory: 0x%llx, ImageBase: 0x%llx, Delta: -0x%llx\r\n", AllocatedMemory, PEHeader.OptionalHeader.ImageBase, delta);
          }
          UINT64 NumRelocationsPerChunk;

          for(;(Relocation_Directory_Base->SizeOfBlock) && (Relocation_Directory_Base < RelocTableEnd);)
          {
            Print(L"\nSizeOfBlock: %u Bytes\r\n", Relocation_Directory_Base->SizeOfBlock);
            Print(L"Rel_dir_base: 0x%llx, RelTableEnd: 0x%llx\r\n", Relocation_Directory_Base, RelocTableEnd);

            EFI_PHYSICAL_ADDRESS page = AllocatedMemory + (UINT64)Relocation_Directory_Base->VirtualAddress; //This virtual address is page-specific, and needs to be offset by Header_memory
            UINT16* DataToFix = (UINT16*)((UINT8*)Relocation_Directory_Base + IMAGE_SIZEOF_BASE_RELOCATION); // The base relocation size is 8 bytes (64 bits)
            NumRelocationsPerChunk = (Relocation_Directory_Base->SizeOfBlock - IMAGE_SIZEOF_BASE_RELOCATION)/sizeof(UINT16);

            Print(L"DataToFix: 0x%hx, Base page: 0x%llx\r\n", DataToFix, page);
            Print(L"NumRelocations in this chunk: %llu\r\n", NumRelocationsPerChunk);
            Keywait(L"About to relocate this chunk...\r\n");
            for(i = 0; i < NumRelocationsPerChunk; i++)
            {
              if(DataToFix[i] >> EFI_PAGE_SHIFT == 0)
              {
                // Nothing, this is a padding area
                Print(L"%llu of %llu -- Padding Area\r\n", i+1, NumRelocationsPerChunk);
              }
              else if (DataToFix[i] >> EFI_PAGE_SHIFT == 10) // 64-bit offset relocation only, check uppper 4 bits of each DataToFix entry
              {
                Print(L"%llu of %llu, DataToFix[%llu]: 0x%hx\r\n", i+1, NumRelocationsPerChunk, i, DataToFix[i]);
                if(AllocatedMemory > PEHeader.OptionalHeader.ImageBase)
                {
                  Print(L"Page: 0x%llx, Current Address: 0x%llx, Data there: 0x%llx\r\n", page, (UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK)), *((UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK))));
                  *((UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK))) += delta;
                  Print(L"Delta: 0x%llx, Corrected Data there: 0x%llx\r\n", delta, *((UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK))));
                  // Lower 12 bits of each DataToFix entry (each entry is of size word) need to be added to virtual address of the block
                  // The real locaiton of the virtual address in memory is stored in "page"
                  // This is an "on-the-fly" RAM patch
                }
                else
                {
                  Print(L"Page: 0x%llx, Current Address: 0x%llx, Data there: 0x%llx\r\n", page, (UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK)), *((UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK))));
                  *((UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK))) -= delta;
                  Print(L"Delta: -0x%llx, Corrected Data there: 0x%llx\r\n", delta, *((UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK))));
                }
              }
              else
              {
                GoTimeStatus = EFI_INVALID_PARAMETER;
                Print(L"Something happened whilst relocating. i: %llu, Relocation_Directory_Base: 0x%llx \r\n", i, Relocation_Directory_Base);
              }
            }
            // Next chunk
            Relocation_Directory_Base = (IMAGE_BASE_RELOCATION*)((UINT8*)Relocation_Directory_Base + Relocation_Directory_Base->SizeOfBlock);
          }
        }
        else
        {
          Print(L"Well that's convenient. No relocation necessary.\r\n");
        }


        if(GoTimeStatus == EFI_INVALID_PARAMETER)
        {
          GoTimeStatus = BS->FreePages(AllocatedMemory, pages);
          if(EFI_ERROR(GoTimeStatus))
          {
            Print(L"Error freeing pages. Error: 0x%llx\r\n", GoTimeStatus);
          }
          GoTimeStatus = EFI_INVALID_PARAMETER;
          Print(L"Relocation failed\r\n");
          return GoTimeStatus;
        }

        //AddressOfEntryPoint should be a 32-bit relative mem address of the entry point of the kernel
        Header_memory = AllocatedMemory + (UINT64)PEHeader.OptionalHeader.AddressOfEntryPoint;
        Print(L"Header_memory: 0x%llx, AllocatedMemory: 0x%llx, EntryPoint: 0x%x\r\n", Header_memory, AllocatedMemory, PEHeader.OptionalHeader.AddressOfEntryPoint);
        Print(L"Data at Header_memory (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(Header_memory + 8), *(EFI_PHYSICAL_ADDRESS*)Header_memory);
      // Loaded! On to memorymap and exitbootservices...
      // Note: Executable entry point is now defined in Header_memory's contained address, which is AllocatedMemory + OptionalHeader.AddressOfEntryPoint

      }
      else
      {
        GoTimeStatus = EFI_INVALID_PARAMETER;
        Print(L"Hey! 64-bit (x86_64) only. Get yo' 32-bit outta here!\r\n");
        return GoTimeStatus;
      }
    }
    else
    {
      Keywait(L"Seems like a 16-bit MS-DOS executable to me...\r\n");

      // :)
      Keywait(L"Well, if you insist...\r\n");

      size = (UINT64)(512*DOSheader.e_cp + DOSheader.e_cblp - 16*DOSheader.e_cparhdr); // Bytes of MZ load module
      UINT64 pages = (size + EFI_PAGE_MASK) >> EFI_PAGE_SHIFT;
      Print(L"e_cp: %hu, e_cblp: %hu, e_cparhdr: %hu\r\n", DOSheader.e_cp, DOSheader.e_cblp, DOSheader.e_cparhdr);
      Print(L"file size: %llu, load module size: %llu, pages: %llu\r\n", size + 16*(UINT64)DOSheader.e_cparhdr, size, pages);

      EFI_PHYSICAL_ADDRESS DOSMem = 0x100;
      GoTimeStatus = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &DOSMem);
      if(EFI_ERROR(GoTimeStatus))
      {
        Print(L"Could not allocate pages for MZ load module. Error code: %lld\r\n", GoTimeStatus);
        return GoTimeStatus;
      }
      Print(L"DOSMem location: 0x%llx\r\n", DOSMem);
      Keywait(L"Allocate Pages passed.\r\n");

      GoTimeStatus = KernelFile->SetPosition(KernelFile, (UINT64)DOSheader.e_cparhdr*16); // Load module is right after the header
      if(EFI_ERROR(GoTimeStatus))
      {
        Print(L"Reset SetPosition error (MZ). 0x%llx\r\n", GoTimeStatus);
        return GoTimeStatus;
      }

      Print(L"current destination address: 0x%llx, DOSMem base: 0x%llx, size: 0x%llx\r\n", DOSMem, DOSMem, size);
      Print(L"Check:\r\nDOSMem: 0x%llx\r\nData there: 0x%016llx%016llx (should be 0)\r\n", DOSMem, *(EFI_PHYSICAL_ADDRESS*)(DOSMem + 8), *(EFI_PHYSICAL_ADDRESS *)DOSMem); // print the first 64 bits of that address to compare
      Keywait(L"\0");
      GoTimeStatus = KernelFile->Read(KernelFile, &size, (EFI_PHYSICAL_ADDRESS*)DOSMem); // Read load module into DOSMem
      if(EFI_ERROR(GoTimeStatus))
      {
        Print(L"Load module read error (MZ). 0x%llx\r\n", GoTimeStatus);
        return GoTimeStatus;
      }

      // mov relocated e_sp to %rsp
      // Normally, entry point is in e_ip...
      Print(L"\r\nVerify:\r\nDOSMem: 0x%llx\r\nData there (first 16 bytes): 0x%016llx%016llx\r\n", DOSMem, *(EFI_PHYSICAL_ADDRESS*)(DOSMem + 8), *(EFI_PHYSICAL_ADDRESS*)DOSMem); // print the first 64 bits of that address to compare
      Print(L"Last 16 bytes: 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(DOSMem + size - 8), *(EFI_PHYSICAL_ADDRESS*)(DOSMem + size - 16));
      Print(L"Next 16 bytes: 0x%016llx%016llx (0 unless last section)\r\n", *(EFI_PHYSICAL_ADDRESS*)(DOSMem + size + 8), *(EFI_PHYSICAL_ADDRESS*)(DOSMem + size));
      Keywait(L"\0");

      Header_memory = DOSMem + (UINT64)DOSheader.e_ip*16;
      Print(L"\r\nHeader_memory: 0x%llx, DOSMem: 0x%llx, EntryPoint: 0x%llx\r\n", Header_memory, DOSMem, (UINT64)DOSheader.e_ip*16);
      Print(L"Data at Header_memory (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(Header_memory + 8), *(EFI_PHYSICAL_ADDRESS*)Header_memory);
      // Loaded! On to memorymap and exitbootservices...
      Keywait(L"\0");

// No idea what will happen if these three lines get commented and the program tries to run. It might actually work if a 64-bit program got packed in there with a correctly-formatted header.
      GoTimeStatus = DOS_EXECUTABLE;
      Print(L"\r\nError Code: 0x%llx\r\nThis program cannot be run in UEFI mode.\r\n:P\r\n", GoTimeStatus);
      return GoTimeStatus;
    }
  }
  else // Check if it's an ELF if it's not a PE32+
  {
    //Slightly less terrible way of doing this; just a placeholder.
    GoTimeStatus = KernelFile->SetPosition(KernelFile, 0);
    if(EFI_ERROR(GoTimeStatus))
    {
      Print(L"Reset SetPosition error (ELF). 0x%llx\r\n", GoTimeStatus);
      return GoTimeStatus;
    }

    Elf64_Ehdr ELF64header;
    UINTN size = sizeof(ELF64header); // This works because it's not a pointer

    GoTimeStatus = KernelFile->Read(KernelFile, &size, &ELF64header);
    if(EFI_ERROR(GoTimeStatus))
    {
      Print(L"Header read error (ELF). 0x%llx\r\n", GoTimeStatus);
      return GoTimeStatus;
    }

    Keywait(L"ELF header read from file.\r\n");

    if(compare(&ELF64header.e_ident[EI_MAG0], ELFMAG, SELFMAG)) // Check for \177ELF (hex: \xfELF)
    {
      //ELF!
      Keywait(L"ELF header passed.\r\n");
      // Check if 64-bit
      if(ELF64header.e_ident[EI_CLASS] == ELFCLASS64 && ELF64header.e_machine == EM_X86_64)
      {
        Keywait(L"ELF64 header passed.\r\n");

        if (ELF64header.e_type != ET_DYN)
        {
          GoTimeStatus = EFI_INVALID_PARAMETER;
          Print(L"Not a position-independent, executable ELF64 application...\r\n");
          Print(L"e_type: 0x%hx\r\n", ELF64header.e_type); // If it's 3, we're good and won't see this. Hopefully the image was compiled with -fpic and linked with -shared and -Bsymbolic
          return GoTimeStatus;
        }
        Keywait(L"Executable ELF64 header passed.\r\n");

        UINT64 i;
        UINT64 virt_size = 0;
        UINT64 virt_min = -1; // wraps around to max 64-bit number
        UINT64 Numofprogheaders = (UINT64)ELF64header.e_phnum;
        Elf64_Phdr program_headers_table[Numofprogheaders];
        size = Numofprogheaders*(UINT64)ELF64header.e_phentsize; // Size of all program headers combined

        GoTimeStatus = KernelFile->SetPosition(KernelFile, ELF64header.e_phoff); // Go to program headers
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Error setting file position for mapping (ELF). 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }
        GoTimeStatus = KernelFile->Read(KernelFile, &size, &program_headers_table[0]); // Run right over the section table, it should be exactly the size to hold this data
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Error reading program headers (ELF). 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }

        // Only want to include PT_LOAD segments
        for(i = 0; i < Numofprogheaders; i++) // Go through each section of the "sections" section to get the address boundary of the last section
        {
          Elf64_Phdr *specific_program_header = &program_headers_table[i];
          if(specific_program_header->p_type == PT_LOAD)
          {
            Print(L"current program address: 0x%x, size: 0x%x\r\n", specific_program_header->p_vaddr, specific_program_header->p_memsz);
            Print(L"current program address + size 0x%x\r\n", specific_program_header->p_vaddr + specific_program_header->p_memsz);
            virt_size = (virt_size > (specific_program_header->p_vaddr + specific_program_header->p_memsz) ? virt_size: (specific_program_header->p_vaddr + specific_program_header->p_memsz));
            virt_min = (virt_min < (specific_program_header->p_vaddr) ? virt_min: (specific_program_header->p_vaddr));
          }
        }
        Print(L"virt_size: 0x%llx, virt_min: 0x%llx, difference: 0x%llx\r\n", virt_size, virt_min, virt_size - virt_min);
        Keywait(L"Program Headers table passed.\r\n");
        // Virt_min is technically also the base address of the loadable segments

        UINT64 pages = (virt_size - virt_min + EFI_PAGE_MASK) >> EFI_PAGE_SHIFT; //To get number of pages (typically 4KB per), rounded up
        Print(L"pages: %llu\r\n", pages);

        EFI_PHYSICAL_ADDRESS AllocatedMemory = 0x400000; // Default for ELF
        Print(L"Address of AllocatedMemory: 0x%llx\r\n", &AllocatedMemory);

        GoTimeStatus = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &AllocatedMemory);
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Could not allocate pages for ELF program segments. Error code: %lld\r\n", GoTimeStatus);
          return GoTimeStatus;
        }

        Print(L"AllocatedMemory location: 0x%llx\r\n", AllocatedMemory);
        Keywait(L"Allocate Pages passed.\r\n");

        // No need to copy headers to memory for ELFs, just the program itself
        // Only want to include PT_LOAD segments
        for(i = 0; i < Numofprogheaders; i++) // Load sections into memory
        {
          Elf64_Phdr *specific_program_header = &program_headers_table[i];
          UINTN RawDataSize = specific_program_header->p_filesz; // 64-bit ELFs can have 64-bit file sizes!
          EFI_PHYSICAL_ADDRESS SectionAddress = AllocatedMemory + specific_program_header->p_vaddr; // 64-bit ELFs use 64-bit addressing!

          Print(L"\n%llu. current section address: 0x%x, RawDataSize: 0x%llx\r\n", i+1, specific_program_header->p_vaddr, RawDataSize);
          if(specific_program_header->p_type == PT_LOAD)
          {
            Print(L"current destination address: 0x%llx, AllocatedMemory base: 0x%llx\r\n", SectionAddress, AllocatedMemory);
            Print(L"PointerToRawData: 0x%llx\r\n", specific_program_header->p_offset);
            Print(L"Check:\r\nSectionAddress: 0x%llx\r\nData there: 0x%016llx%016llx (should be 0)\r\n", SectionAddress, *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + 8), *(EFI_PHYSICAL_ADDRESS *)SectionAddress); // print the first 64 bits of that address to compare
            Print(L"About to load section %llu of %llu...\r\n", i + 1, Numofprogheaders);
            Keywait(L"\0");

            GoTimeStatus = KernelFile->SetPosition(KernelFile, specific_program_header->p_offset); // p_offset is a UINT64 relative to the beginning of the file, just like Read() expects!
            if(EFI_ERROR(GoTimeStatus))
            {
              Print(L"Program segment SetPosition error (ELF). 0x%llx\r\n", GoTimeStatus);
              return GoTimeStatus;
            }

            GoTimeStatus = KernelFile->Read(KernelFile, &RawDataSize, (EFI_PHYSICAL_ADDRESS*)SectionAddress); // (void*)SectionAddress
            if(EFI_ERROR(GoTimeStatus))
            {
              Print(L"Program segment read error (ELF). 0x%llx\r\n", GoTimeStatus);
              return GoTimeStatus;
            }

            Print(L"\r\nVerify:\r\nSectionAddress: 0x%llx\r\nData there (first 16 bytes): 0x%016llx%016llx\r\n", SectionAddress, *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + 8), *(EFI_PHYSICAL_ADDRESS*)SectionAddress); // print the first 64 bits of that address to compare
            Print(L"Last 16 bytes: 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize - 8), *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize - 16));
            Print(L"Next 16 bytes: 0x%016llx%016llx (0 unless last section)\r\n", *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize + 8), *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize));
            // "Next 16 bytes" should be 0 unless last section
          }
          else
          {
            Print(L"Not a PT_LOAD section. Type: 0x%x\r\n", specific_program_header->p_type);
          }
        }

        Keywait(L"\nLoad file sections into allocated pages passed.\r\n");

        // Link kernel with -Wl,-Bsymbolic and there's no need for relocations beyond the base-relative ones just done. YUS!
        // Also, using -shared for linking and -fpic for compiling make position independent code.

        //e_entry should be a 64-bit relative mem address of the entry point of the kernel
        Header_memory = AllocatedMemory + ELF64header.e_entry;
        Print(L"Header_memory: 0x%llx, AllocatedMemory: 0x%llx, EntryPoint: 0x%x\r\n", Header_memory, AllocatedMemory, ELF64header.e_entry);
        Print(L"Data at Header_memory (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(Header_memory + 8), *(EFI_PHYSICAL_ADDRESS*)Header_memory);
        // Loaded! On to memorymap and exitbootservices...
        // Note: Executable entry point is now defined in Header_memory's contained address, which is AllocatedMemory + ELF64header.e_entry

      }
      else
      {
        GoTimeStatus = EFI_INVALID_PARAMETER;
        Print(L"Hey! 64-bit (x86_64) ELFs only. Get yo' 32-bit outta here!\r\n");
        return GoTimeStatus;
      }
    }
    else
    {
      //Slightly less terrible way of doing this; just a placeholder.
      GoTimeStatus = KernelFile->SetPosition(KernelFile, 0);
      if(EFI_ERROR(GoTimeStatus))
      {
        Print(L"Reset SetPosition error (Mach). 0x%llx\r\n", GoTimeStatus);
        return GoTimeStatus;
      }

      struct mach_header_64 MACheader;
      UINTN size = sizeof(MACheader); // This works because it's not a pointer

      GoTimeStatus = KernelFile->Read(KernelFile, &size, &MACheader);
      if(EFI_ERROR(GoTimeStatus))
      {
        Print(L"Header read error (Mach). 0x%llx\r\n", GoTimeStatus);
        return GoTimeStatus;
      }

      Keywait(L"Mach header read from file.\r\n");

      if(MACheader.magic == MH_MAGIC_64 && MACheader.cputype == CPU_TYPE_X86_64) // Big endian: 0xfeedfacf && little endian: 0x07000001
      {
        Keywait(L"Mach64 header passed.\r\n");

        if (MACheader.filetype != MH_EXECUTE)
        {
          GoTimeStatus = EFI_INVALID_PARAMETER;
          Print(L"Not an executable Mach64 application...\r\n");
          Print(L"filetype: 0x%x\r\n", MACheader.filetype); // If it's 0x5, we're good and won't see this.
          return GoTimeStatus;
        }
        Keywait(L"Executable Mach64 header passed.\r\n");

        UINT64 i;
        UINT64 virt_size = 0;
        UINT64 virt_min = -1; // wraps around to max 64-bit number
        UINT64 Numofcommands = (UINT64)MACheader.ncmds;
        //load_command load_commands_table[Numofcommands]; // commands are variably sized. Dammit Apple!
        size = (UINT64)MACheader.sizeofcmds; // Size of all commands combined. Well that's convenient!
        char commands_buffer[size]; // *grumble grumble*
        UINT64 current_spot = 0;

// Go to load commands, which is right after the mach_header
/*
        GoTimeStatus = KernelFile->SetPosition(KernelFile, sizeof(MACheader));
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Error setting file position for mapping (ELF). 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }
*/
        GoTimeStatus = KernelFile->Read(KernelFile, &size, &commands_buffer[0]); // Run right over the commands buffer region; it should be exactly the size to hold this data
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Error reading load commands (Mach64). 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }

        for(i = 0; i < Numofcommands; i++) // Figure out how much data needs to be allocated pages
        {
          struct load_command *specific_load_command = (struct load_command*)&commands_buffer[current_spot];
          if(specific_load_command->cmd == LC_SEGMENT_64) // vmaddr and vmsize are 64-bit!!
          {
            struct segment_command_64 *specific_segment_command = (struct segment_command_64 *)specific_load_command;
            Print(L"current segment address: 0x%x, size: 0x%x\r\n", specific_segment_command->vmaddr, specific_segment_command->vmsize);
            Print(L"current segment address + size 0x%x\r\n", specific_segment_command->vmaddr + specific_segment_command->vmsize);
            virt_size = (virt_size > (specific_segment_command->vmaddr + specific_segment_command->vmsize) ? virt_size: (specific_segment_command->vmaddr + specific_segment_command->vmsize));
            virt_min = (virt_min < (specific_segment_command->vmaddr) ? virt_min: (specific_segment_command->vmaddr));
          }
          current_spot += (UINT64)specific_load_command->cmdsize;
        }
        Print(L"virt_size: 0x%llx, virt_min: 0x%llx, difference: 0x%llx\r\n", virt_size, virt_min, virt_size - virt_min);
        if(current_spot != size)
        {
          GoTimeStatus = EFI_INVALID_PARAMETER;
          Print(L"Hmmm... current_spot: %llu != total cmd size: %llu\r\n", current_spot, size);
          return GoTimeStatus;
        }
        Print(L"current_spot: %llu == total cmd size: %llu\r\n", current_spot, size);
        Keywait(L"Load commands buffer passed.\r\n");

        UINT64 pages = (virt_size - virt_min + EFI_PAGE_MASK) >> EFI_PAGE_SHIFT; //To get number of pages (typically 4KB per), rounded up
        Print(L"pages: %llu\r\n", pages);


        EFI_PHYSICAL_ADDRESS AllocatedMemory = 0x100000000; // Default for non-pagezero Mach-O with a 4GB pagezero
        Print(L"Address of AllocatedMemory: 0x%llx\r\n", &AllocatedMemory);

        GoTimeStatus = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &AllocatedMemory);
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Could not allocate pages for Mach64 segment sections. Error code: %lld\r\n", GoTimeStatus);
          return GoTimeStatus;
        }

        Print(L"AllocatedMemory location: 0x%llx\r\n", AllocatedMemory);
        Keywait(L"Allocate Pages passed.\r\n");

        current_spot = 0;
        UINT64 entrypointoffset = 0;
        // Only want to include LC_SEGMENT_64 & LC_UNIXTHREAD segments
        for(i = 0; i < Numofcommands; i++) // Load segments into memory
        {
          struct load_command *specific_load_command = (struct load_command*)&commands_buffer[current_spot];
          if(specific_load_command->cmd == LC_SEGMENT_64)
          {
            struct segment_command_64 *specific_segment_command = (struct segment_command_64 *)specific_load_command;
            UINTN RawDataSize = specific_segment_command->filesize; // 64-bit Mach-Os can have 64-bit file sizes!
            EFI_PHYSICAL_ADDRESS SectionAddress = AllocatedMemory + specific_segment_command->vmaddr; // 64-bit Mach-Os use 64-bit addressing!

            Print(L"\n%llu. current section address: 0x%x, RawDataSize: 0x%llx\r\n", i+1, specific_segment_command->vmaddr, RawDataSize);
            Print(L"current destination address: 0x%llx, AllocatedMemory base: 0x%llx\r\n", SectionAddress, AllocatedMemory);
            Print(L"PointerToRawData: 0x%llx\r\n", specific_segment_command->fileoff);
            Print(L"Check:\r\nSectionAddress: 0x%llx\r\nData there: 0x%016llx%016llx (should be 0)\r\n", SectionAddress, *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + 8), *(EFI_PHYSICAL_ADDRESS *)SectionAddress); // print the first 64 bits of that address to compare
            Print(L"About to load section %llu of %llu...\r\n", i + 1, Numofcommands);
            Keywait(L"\0");

            GoTimeStatus = KernelFile->SetPosition(KernelFile, specific_segment_command->fileoff); // p_offset is a UINT64 relative to the beginning of the file, just like Read() expects!
            if(EFI_ERROR(GoTimeStatus))
            {
              Print(L"Program segment SetPosition error (Mach64). 0x%llx\r\n", GoTimeStatus);
              return GoTimeStatus;
            }

            GoTimeStatus = KernelFile->Read(KernelFile, &RawDataSize, (EFI_PHYSICAL_ADDRESS*)SectionAddress); // (void*)SectionAddress
            if(EFI_ERROR(GoTimeStatus))
            {
              Print(L"Program segment read error (Mach64). 0x%llx\r\n", GoTimeStatus);
              return GoTimeStatus;
            }

            Print(L"\r\nVerify:\r\nSectionAddress: 0x%llx\r\nData there (first 16 bytes): 0x%016llx%016llx\r\n", SectionAddress, *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + 8), *(EFI_PHYSICAL_ADDRESS*)SectionAddress); // print the first 64 bits of that address to compare
            Print(L"Last 16 bytes: 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize - 8), *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize - 16));
            Print(L"Next 16 bytes: 0x%016llx%016llx (0 unless last section)\r\n", *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize + 8), *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize));
            // "Next 16 bytes" should be 0 unless last section
          }
          else if(specific_load_command->cmd == LC_UNIXTHREAD)
          {
            //Get the entry point
            UINT64 *unixthread_data = (UINT64*)specific_load_command; // Or use an array of 64-bit UINTs and grab RIP. The other registers should be zero.
            entrypointoffset = unixthread_data[18]; // Get entrypoint from RIP register initialization
            Print(L"Entry Point: 0x%llx\r\n", entrypointoffset);
          }
          else if(specific_load_command->cmd == LC_MAIN)
          {
            Print(L"LC_MAIN is not supported, as it requires DYLD, which requires an OS.\r\nPlease relink as static for LC_UNIXTHREAD.\r\n");
            GoTimeStatus = BS->FreePages(AllocatedMemory, pages);
            if(EFI_ERROR(GoTimeStatus))
            {
              Print(L"Error freeing pages. Error: 0x%llx\r\n", GoTimeStatus);
            }
            GoTimeStatus = EFI_INVALID_PARAMETER;
            return GoTimeStatus;
          }
          else
          {
            Print(L"Not a LC_SEGMENT_64 section. Type: 0x%x\r\n", specific_load_command->cmd);
          }
          current_spot += (UINT64)specific_load_command->cmdsize;
        }

        Keywait(L"\nLoad file sections into allocated pages passed.\r\n");


        //entrypointoffset should be a 64-bit relative mem address of the entry point of the kernel
        Header_memory = AllocatedMemory + entrypointoffset;
        Print(L"Header_memory: 0x%llx, AllocatedMemory: 0x%llx, EntryPoint: 0x%x\r\n", Header_memory, AllocatedMemory, entrypointoffset);
        Print(L"Data at Header_memory (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(Header_memory + 8), *(EFI_PHYSICAL_ADDRESS*)Header_memory);
        // Loaded! On to memorymap and exitbootservices...
        // Note: Executable entry point is now defined in Header_memory's contained address, which is AllocatedMemory + entrypointoffset
      }
      else if(MACheader.magic == FAT_MAGIC) // Big endian: 0xcafebabe
      {
        GoTimeStatus = EFI_INVALID_PARAMETER;
        Print(L"A universal binary?? What?? O_o\r\nx86-64 Mach-O only please.\r\n");
        return GoTimeStatus;
      }
      else if(MACheader.magic == MH_MAGIC) // Big endian: 0xfeedface
      {
        GoTimeStatus = EFI_INVALID_PARAMETER;
        Print(L"Hey! 64-bit (x86_64) Mach-Os only. Get yo' 32-bit outta here!\r\n");
        return GoTimeStatus;
      }
      else
      {
        GoTimeStatus = EFI_INVALID_PARAMETER;
        Print(L"Neither PE32+, ELF, nor Mach-O image supplied as Kernel64. Check the binary.\r\n");
        return GoTimeStatus;
      }
    }
  }

  Print(L"Header_memory: 0x%llx\r\n", Header_memory);
  Print(L"Data at Header_memory (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(Header_memory + 8), *(EFI_PHYSICAL_ADDRESS*)Header_memory);

  Keywait(L"\0");

// Integrity check
  Print(L"GPU Mode: %u of %u\r\n", Graphics.Mode, Graphics.MaxMode - 1);
  Print(L"GPU FB: 0x%016llx\r\n", Graphics.FrameBufferBase);
  Print(L"GPU FB Size: 0x%016llx\r\n", Graphics.FrameBufferSize);
  Print(L"GPU SizeOfInfo: %u Bytes\r\n", Graphics.SizeOfInfo);
  Print(L"GPU Info Ver: 0x%x\r\n", Graphics.Info->Version);
  Print(L"GPU Info Res: %ux%u\r\n", Graphics.Info->HorizontalResolution, Graphics.Info->VerticalResolution);
  Print(L"GPU Info PxFormat: 0x%x\r\n", Graphics.Info->PixelFormat);
  Print(L"GPU Info PxInfo (R,G,B,Rsvd Masks): 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n", Graphics.Info->PixelInformation.RedMask, Graphics.Info->PixelInformation.GreenMask, Graphics.Info->PixelInformation.BlueMask, Graphics.Info->PixelInformation.ReservedMask);
  Print(L"GPU Info PxPerScanLine: %u\r\n", Graphics.Info->PixelsPerScanLine);

  Print(L"RSDP address: 0x%llx\r\n", RSDPTable);
  Print(L"Data at RSDP (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(RSDPTable + 8), *(EFI_PHYSICAL_ADDRESS*)RSDPTable);

  LOADER_PARAMS * Loader_block;
  GoTimeStatus = BS->AllocatePool(EfiLoaderData, sizeof(LOADER_PARAMS), (void**)&Loader_block);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"Error allocating loader block pool. Error: 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }

  Print(L"Loader block allocated, size of structure: %llu\r\n", sizeof(LOADER_PARAMS));
  Keywait(L"About to get MemMap and exit boot services...\r\n");

  // Clear screen while we still have EFI services
//  ST->ConOut->ClearScreen(ST->ConOut); // Hm... This appears to also reset the video mode to mode 0...
// This won't matter once debug statements are disabled, anyways, since the only time user input is required is before GOP SetMode

//======================================================================================================================

  //UINTN is the largest uint type supported. For x86_64, this is uint64_t
  UINTN MemMapSize = 0, MemMapKey, MemMapDescriptorSize;
  UINT32 MemMapDescriptorVersion;
  EFI_MEMORY_DESCRIPTOR * MemMap = NULL;
/*
  //Simple version:
  GoTimeStatus = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
  // Will error intentionally
  GoTimeStatus = BS->AllocatePool(EfiBootServicesData, MemMapSize, (void **)&MemMap); // this might be really slow. mapSize += 2 * descriptorSize or mapSize += descriptorSize; might be all that's needed
  if(EFI_ERROR(GoTimeStatus)) // Error! Wouldn't be safe to continue.
    {
      Print(L"MemMap AllocatePool error. 0x%llx\r\n", GoTimeStatus);
      return GoTimeStatus;
    }
  GoTimeStatus = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
  GoTimeStatus = BS->ExitBootServices(ImageHandle, MemMapKey);

*/
// End simple version, below is the more complex version

  // Get memory map and exit boot services
  GoTimeStatus = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
  if(GoTimeStatus == EFI_BUFFER_TOO_SMALL)
  {
    GoTimeStatus = BS->AllocatePool(EfiBootServicesData, MemMapSize, (void **)&MemMap); // Allocate pool for MemMap (it should always be resident in memory)
    if(EFI_ERROR(GoTimeStatus)) // Error! Wouldn't be safe to continue.
    {
      Print(L"MemMap AllocatePool error. 0x%llx\r\n", GoTimeStatus);
      return GoTimeStatus;
    }
    GoTimeStatus = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
  }
  GoTimeStatus = BS->ExitBootServices(ImageHandle, MemMapKey);
  if(EFI_ERROR(GoTimeStatus)) // Error! EFI_INVALID_PARAMETER, MemMapKey is incorrect
  {
    Print(L"ExitBootServices #1 failed. 0x%llx, Trying again...\r\n", GoTimeStatus);
    Keywait(L"\0");
    MemMapSize = 0;
    GoTimeStatus = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
    if(GoTimeStatus == EFI_BUFFER_TOO_SMALL)
    {
      GoTimeStatus = BS->AllocatePool(EfiBootServicesData, MemMapSize, (void **)&MemMap); // this might be really slow. mapSize += 2 * descriptorSize or mapSize += descriptorSize; might be all that's needed
      if(EFI_ERROR(GoTimeStatus)) // Error! Wouldn't be safe to continue.
      {
        Print(L"MemMap AllocatePool error #2. 0x%llx\r\n", GoTimeStatus);
        return GoTimeStatus;
      }
      GoTimeStatus = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
    }
    GoTimeStatus = BS->ExitBootServices(ImageHandle, MemMapKey);
  }

  // This applies to both the simple and larger versions of the above.
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"Could not exit boot services... 0x%llx\r\n", GoTimeStatus);
    GoTimeStatus = BS->FreePool(&MemMap); // This might not be right
    if(EFI_ERROR(GoTimeStatus)) // Error! Wouldn't be safe to continue.
      {
        Print(L"Error freeing pool. 0x%llx\r\n", GoTimeStatus);
      }
    Print(L"MemMapSize: %llx, MemMapKey: %llx\r\n", MemMapSize, MemMapKey);
    Print(L"DescriptorSize: %llx, DescriptorVersion: %x\r\n", MemMapDescriptorSize, MemMapDescriptorVersion);
    return GoTimeStatus;
  }

//======================================================================================================================

// Jump to entry point, and WE ARE LIVE, BABY!!
  // ((EFIAPI void(*)(EFI_RUNTIME_SERVICES*)) Header_memory)(RT);
//  ((EFIAPI void(*)(void)) Header_memory)();
  // A void-returning function that takes no arguments. Neat!
/*
  typedef void (EFIAPI *EntryPointFunction)();
  EntryPointFunction EntryPointPlaceholder = (EntryPointFunction)(Header_memory);
//  Print(L"EntryPointPlaceholder: %llx\r\n", (UINT64)EntryPointPlaceholder);
  EntryPointPlaceholder();
*/

/* // No loader block version
  typedef void (EFIAPI *EntryPointFunction)(EFI_MEMORY_DESCRIPTOR * Memory_Map, EFI_RUNTIME_SERVICES* RTServices, EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE* GPU_Mode, EFI_FILE_INFO* FileMeta, void * RSDP); // Placeholder names for jump
  EntryPointFunction EntryPointPlaceholder = (EntryPointFunction)(Header_memory);
  EntryPointPlaceholder(MemMap, RT, &Graphics, FileInfo, RSDPTable);
*/

/*
  typedef struct PARAMETER_BLOCK {
    EFI_MEMORY_DESCRIPTOR               *Memory_Map;
    EFI_RUNTIME_SERVICES                *RTServices;
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE   *GPU_Mode;
    EFI_FILE_INFO                       *FileMeta;
    void                                *RSDP;
  } LOADER_PARAMS;
*/

// This shouldn't modify the memory map.
  Loader_block->Memory_Map = MemMap;
  Loader_block->RTServices = RT;
  Loader_block->GPU_Mode = &Graphics;
  Loader_block->FileMeta = FileInfo;
  Loader_block->RSDP = RSDPTable;

  typedef void (EFIAPI *EntryPointFunction)(LOADER_PARAMS * LP); // Placeholder names for jump
  EntryPointFunction EntryPointPlaceholder = (EntryPointFunction)(Header_memory);
  EntryPointPlaceholder(Loader_block);

  //Should never get here
  return GoTimeStatus;
}

/*
Note:

typedef enum {
  EfiResetCold, // Power cycle (hard off/on)
  EfiResetWarm, //
  EfiResetShutdown // Uh, normal shutdown.
} EFI_RESET_TYPE;

typedef
VOID
ResetSystem (IN EFI_RESET_TYPE ResetType, IN EFI_STATUS ResetStatus, IN UINTN DataSize, IN VOID *ResetData OPTIONAL);

RT->ResetSystem (EfiResetShutdown, EFI_SUCCESS, 0, NULL); // Shutdown the system
RT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL); // Hard reboot the system
RT->ResetSystem (EfiResetWarm, EFI_SUCCESS, 0, NULL); // Soft reboot the system

There is also EfiResetPlatformSpecific, but that's not really important. (Takes a standard 128-bit EFI_GUID as ResetData for a custom reset type)
*/
