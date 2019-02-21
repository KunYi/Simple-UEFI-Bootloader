//==================================================================================================================================
//  Simple UEFI Bootloader: Graphics Functions
//==================================================================================================================================
//
// Version 1.3
//
// Author:
//  KNNSpeed
//
// Source Code:
//  https://github.com/KNNSpeed/Simple-UEFI-Bootloader
//
// This file contains the graphics setup code.
//

#include "Bootloader.h"

//==================================================================================================================================
//  InitUEFI_GOP: Graphics Initialization
//==================================================================================================================================
//
// Determine the UEFI-provided graphical capabilities of the machine and set the desired output mode (default is 0, usually native resolution).
// Writes a GPU_CONFIG structure, which contains an array of EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE structures:
/*
  typedef struct {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE  *GPUArray;             // An array of EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE structs defining each available framebuffer
    UINT64                              NumberOfFrameBuffers; // The number of structs in the array (== the number of available framebuffers)
  } GPU_CONFIG;
*/
// The GPUArray pointer points to the base address of an array of these structures:
/*
  typedef struct {
    UINT32                                 MaxMode;
    UINT32                                 Mode;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION   *Info;
    UINTN                                  SizeOfInfo;
    EFI_PHYSICAL_ADDRESS                   FrameBufferBase;
    UINTN                                  FrameBufferSize;
  } EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;
*/
// Various parameters within this structure are defined like so:
/*
  typedef struct {
    UINT32                     Version;
    UINT32                     HorizontalResolution;
    UINT32                     VerticalResolution;
    EFI_GRAPHICS_PIXEL_FORMAT  PixelFormat;            // 0 - 4
    EFI_PIXEL_BITMASK          PixelInformation;
    UINT32                     PixelsPerScanLine;
  } EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

  typedef enum {
    PixelRedGreenBlueReserved8BitPerColor,   // 0
    PixelBlueGreenRedReserved8BitPerColor,   // 1
    PixelBitMask,                            // 2
    PixelBltOnly,                            // 3
    PixelFormatMax                           // 4
  } EFI_GRAPHICS_PIXEL_FORMAT;

  typedef struct {
    UINT32            RedMask;
    UINT32            GreenMask;
    UINT32            BlueMask;
    UINT32            ReservedMask;
  } EFI_PIXEL_BITMASK;
*/
//

EFI_STATUS InitUEFI_GOP(EFI_HANDLE ImageHandle, GPU_CONFIG * Graphics)
{ // Declaring a pointer only allocates 8 bytes (x64) for that pointer. Every pointer's destination must be manually allocated memory via AllocatePool and then freed with FreePool when done with.

  Graphics->NumberOfFrameBuffers = 0;

  EFI_STATUS GOPStatus;

  UINT64 GOPInfoSize;
  UINT32 mode;
  UINTN NumHandlesInHandleBuffer = 0; // Number of discovered graphics handles (GPUs)
  UINT64 DevNum = 0;
  EFI_INPUT_KEY Key;

  CHAR16 * PxFormats[] = {
      L"RGBReserved 8Bpp",
      L"BGRReserved 8Bpp",
      L"PixelBitMask",
      L"PixelBltOnly",
      L"PixelFormatMax"
  };

  Key.UnicodeChar = 0;

// Commented out on account of GPU name search's not working right
/*
  UINTN Name2HandlesInHandleBuffer2 = 0; // Number of discovered handles (devices)
  // There was only supposed to be one of these, but for some reason vendors go all over the place...
  CHAR8 LanguageToUse[6] = {'e','n','-','U','S','\0'};
  CHAR8 LanguageToUse2[3] = {'e','n','\0'};
  CHAR8 LanguageToUse3[4] = {'e','n','g','\0'};

  CHAR16 *DriverDisplayName = L"No Driver Name";
  CHAR16 *ControllerDisplayName = L"No Controller Name";
*/

  // We can pick which graphics output device we want (handy for multi-GPU setups)...
  EFI_HANDLE *GraphicsHandles; // Array of discovered graphics handles that support the Graphics Output Protocol
  GOPStatus = BS->LocateHandleBuffer(ByProtocol, &GraphicsOutputProtocol, NULL, &NumHandlesInHandleBuffer, &GraphicsHandles); // This automatically allocates pool for GraphicsHandles
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"GraphicsTable LocateHandleBuffer error. 0x%llx\r\n", GOPStatus);
    return GOPStatus;
  }

  Print(L"\r\n");
  if(NumHandlesInHandleBuffer == 1) // Grammar
  {
    Print(L"There is %llu UEFI graphics device:\r\n", NumHandlesInHandleBuffer);
  }
  else
  {
    Print(L"There are %llu UEFI graphics devices:\r\n", NumHandlesInHandleBuffer);
  }

// NOTE: This commented out section is an attempt at getting the GPU names. For some reason, it just doesn't always work.
// Maybe it's just vendors not properly supporting the spec, if it's not a semantic bug?
// If someone comes across this one day and can figure it out, it'll get implemented, but it's not particularly important.
/*
  // List GPUs (if they don't have Name2 protocol support, they'll be named "-1")
  // List all Name2Handles in system because we need them
  EFI_HANDLE *Name2Handles; // Array of discovered handles that support the NAME2 protocol
  GOPStatus = BS->LocateHandleBuffer(ByProtocol, &ComponentName2Protocol, NULL, &Name2HandlesInHandleBuffer2, &Name2Handles); // This automatically allocates pool for GraphicsHandles
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"Name2Handles LocateHandleBuffer error. 0x%llx\r\n", GOPStatus);
    return GOPStatus;
  }

#ifdef GOP_NAMING_DEBUG_ENABLED
  Print(L"Number of Name2Handles: %llu\r\n", Name2HandlesInHandleBuffer2);
#endif

  for(DevNum = 0; DevNum < NumHandlesInHandleBuffer; DevNum++)
  {
    DriverDisplayName = L"No Driver Name";
    ControllerDisplayName = L"No Controller Name";

    for(UINT64 k = 0; k < Name2HandlesInHandleBuffer2; k++)
    {
      EFI_COMPONENT_NAME2_PROTOCOL *DeviceName;
//
//      GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_COMPONENT_NAME2_PROTOCOL), (void**)&DeviceName); // All EfiBootServicesData get freed on ExitBootServices()
//      if(EFI_ERROR(GOPStatus))
//      {
//        Print(L"DeviceName AllocatePool error. 0x%llx\r\n", GOPStatus);
//        return GOPStatus;
//      }

      GOPStatus = BS->OpenProtocol(Name2Handles[k], &ComponentName2Protocol, (void**)&DeviceName, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"DeviceName OpenProtocol error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

      GOPStatus = DeviceName->GetDriverName(DeviceName, LanguageToUse, &DriverDisplayName);
      if(GOPStatus == EFI_UNSUPPORTED)
      {
        GOPStatus = DeviceName->GetDriverName(DeviceName, LanguageToUse2, &DriverDisplayName);
        if(GOPStatus == EFI_UNSUPPORTED)
        {
          GOPStatus = DeviceName->GetDriverName(DeviceName, LanguageToUse3, &DriverDisplayName);
        }
      }
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"DeviceName GetDriverName error. 0x%llx\r\n", GOPStatus);
        if(GOPStatus == EFI_UNSUPPORTED)
        {
          // This is the format of the language, since it isn't "en-US" or "en"
          // It will need to be implemented like the above arrays
          Print(L"First 10 language characters look like this:\r\n");
          for(UINT32 p = 0; p < 10; p++)
          {
            Print(L"%c", DeviceName->SupportedLanguages[p]);
          }
          Print(L"\r\n");
        }
        return GOPStatus;
      }

      GOPStatus = DeviceName->GetControllerName(DeviceName, Name2Handles[k], GraphicsHandles[DevNum], LanguageToUse, &ControllerDisplayName);
      if(GOPStatus == EFI_UNSUPPORTED)
      {
        GOPStatus = DeviceName->GetControllerName(DeviceName, Name2Handles[k], GraphicsHandles[DevNum], LanguageToUse2, &ControllerDisplayName);
        if(GOPStatus == EFI_UNSUPPORTED)
        {
          GOPStatus = DeviceName->GetControllerName(DeviceName, Name2Handles[k], GraphicsHandles[DevNum], LanguageToUse3, &ControllerDisplayName);
        }
      }
#ifdef GOP_NAMING_DEBUG_ENABLED
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"DeviceName GetControllerName error. 0x%llx\r\n", GOPStatus);
      }
      Print(L"%llu. 0x%llx, 0x%llx - %s: %s\r\n", k, GraphicsHandles[DevNum], Name2Handles[k], DriverDisplayName, ControllerDisplayName);
      Keywait(L"\0");
#endif
      if(GOPStatus == EFI_SUCCESS)
      {
        // Found the name and it has been assigned. No need to keep looking.
        break;
      }
      // Did not find a name for some reason
      else if((k == Name2HandlesInHandleBuffer2 - 1) && (GOPStatus != EFI_SUCCESS))
      {
        // You know, we have specifications for a reason.
        // Those who refuse to follow them get this.
        DriverDisplayName = L"No Driver Name";
        ControllerDisplayName = L"No Controller Name";
      }
    }
    Print(L"%c. 0x%llx - %s: %s\r\n", DevNum + 0x30, GraphicsHandles[DevNum], DriverDisplayName, ControllerDisplayName);
  }
  Print(L"\r\n");

  // Done with this massive array
  GOPStatus = BS->FreePool(Name2Handles);
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"Name2Handles FreePool error. 0x%llx\r\n", GOPStatus);
    return GOPStatus;
  }
*/

  for(DevNum = 0; DevNum < NumHandlesInHandleBuffer; DevNum++)
  {
    Print(L"%c. GPU #%c: 0x%llx\r\n", DevNum + 0x30, DevNum + 0x30, GraphicsHandles[DevNum]); // Memory address of GPU handle
  }
  Print(L"\r\n");

  // If applicable, select a GPU. Otherwise, skip all the way to single GPU configuration.
  if(NumHandlesInHandleBuffer > 1)
  {
    Print(L"Multiple GPUs detected:\r\n");

    // Using this as the choice holder
    // This sets the default option... Which doesn't really matter since there's no timer on this menu :/
    DevNum = 2;

    // User selection
    while(0x30 > Key.UnicodeChar || Key.UnicodeChar > 0x33)
    {
      Print(L"Configure all or configure one?\r\n");
      Print(L"0. Configure all individually\r\n");
      Print(L"1. Configure one\r\n");
      Print(L"2. Configure all to use default resolution of connected active display (usually native)\r\n");
      Print(L"3. Configure all to use 1024x768\r\n");
      Print(L"Note: The \"active display(s)\" on a GPU are determined by the GPU's firmware\r\n");
      Print(L"\r\nPlease select an option.\r\n");
      while ((GOPStatus = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);
      Print(L"Option %c selected.\r\n\n", Key.UnicodeChar);
    }
    DevNum = (UINT64)(Key.UnicodeChar - 0x30); // Convert user input character from UTF-16 to number
    Key.UnicodeChar = 0; // Reset input
  }

  if((NumHandlesInHandleBuffer > 1) && (DevNum == 0))
  {
    // Configure all individually

    // Setup
    Graphics->NumberOfFrameBuffers = NumHandlesInHandleBuffer;
    GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, Graphics->NumberOfFrameBuffers*sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&Graphics->GPUArray);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GPUArray AllocatePool error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

    // Configure
    for(DevNum = 0; DevNum < NumHandlesInHandleBuffer; DevNum++)
    {
      Print(L"%c. GPU #%c: 0x%llx\r\n", DevNum + 0x30, DevNum + 0x30, GraphicsHandles[DevNum]); // Memory address of GPU handle

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"Allocating GOP pools...\r\n");
#endif

      EFI_GRAPHICS_OUTPUT_PROTOCOL *GOPTable;
      // Reserve memory for graphics output structure

      GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL), (void**)&GOPTable); // All EfiBootServicesData get freed on ExitBootServices()
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GOPTable AllocatePool error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

    /*
      // These are all the same
      Print(L"sizeof(struct _EFI_GRAPHICS_OUTPUT_PROTOCOL): %llu\r\n", sizeof(struct _EFI_GRAPHICS_OUTPUT_PROTOCOL));
      Print(L"sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL): %llu\r\n", sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL));
      Print(L"sizeof(*GOPTable): %llu\r\n", sizeof(*GOPTable));
      Print(L"sizeof(GOPTable[0]): %llu\r\n", sizeof(GOPTable[0]));
    */

      // Reserve memory for graphics output mode to preserve it
      GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&GOPTable->Mode);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GOP Mode AllocatePool error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }
      // Mode->Info gets reserved once SizeOfInfo is determined (via QueryMode()).

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"GOPTable and Mode pools allocated....\r\n");
#endif

      GOPStatus = BS->OpenProtocol(GraphicsHandles[DevNum], &GraphicsOutputProtocol, (void**)&GOPTable, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GraphicsTable OpenProtocol error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"OpenProtocol passed.\r\n");

      Print(L"Current GOP Mode Info:\r\n");
      Print(L"Max Mode supported: %u, Current Mode: %u\r\nSize of Mode Info Structure: %llu Bytes\r\n", GOPTable->Mode->MaxMode - 1, GOPTable->Mode->Mode, GOPTable->Mode->SizeOfInfo);
      Print(L"FrameBufferBase: 0x%016llx, FrameBufferSize: 0x%llx\r\n", GOPTable->Mode->FrameBufferBase, GOPTable->Mode->FrameBufferSize); // Per spec, the FrameBufferBase might be 0 until SetMode is called

      Keywait(L"\0");

      EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *GOPInfo; // Querymode allocates GOPInfo
      // Get detailed info about supported graphics modes
      for(mode = 0; mode < GOPTable->Mode->MaxMode; mode++) // Valid modes are from 0 to MaxMode - 1
      {
        GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPInfo); // IN IN OUT OUT
        if(EFI_ERROR(GOPStatus))
        {
          Print(L"GraphicsTable QueryMode error. 0x%llx\r\n", GOPStatus);
          return GOPStatus;
        }
        Print(L"Mode %u of %u (%llu Bytes):\r\n Ver: 0x%x, Res: %ux%u\r\n", mode, GOPTable->Mode->MaxMode - 1, GOPInfoSize, GOPInfo->Version, GOPInfo->HorizontalResolution, GOPInfo->VerticalResolution);
        Print(L"PxPerScanLine: %u\r\n", GOPInfo->PixelsPerScanLine);
        Print(L"PxFormat: 0x%x, PxInfo (R,G,B,Rsvd Masks): 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n", GOPInfo->PixelFormat, GOPInfo->PixelInformation.RedMask, GOPInfo->PixelInformation.GreenMask, GOPInfo->PixelInformation.BlueMask, GOPInfo->PixelInformation.ReservedMask);
        Keywait(L"\0");
      }

      // Don't need GOPInfo anymore
      GOPStatus = BS->FreePool(GOPInfo);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPInfo pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }
#endif

      if(GOPTable->Mode->MaxMode == 1) // Grammar
      {
        Print(L"%u available graphics mode found.\r\n", GOPTable->Mode->MaxMode);
      }
      else
      {
        Print(L"%u available graphics modes found.\r\n", GOPTable->Mode->MaxMode);
      }

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"Getting list of supported modes...\r\n");
#endif

      // Get supported graphics modes
      EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *GOPInfo2; // Querymode allocates GOPInfo
      while(0x30 > Key.UnicodeChar || Key.UnicodeChar > (0x30 + GOPTable->Mode->MaxMode - 1))
      {
        Print(L"Current Mode: %u\r\n", GOPTable->Mode->Mode);
        for(mode = 0; mode < GOPTable->Mode->MaxMode; mode++) // Valid modes are from 0 to MaxMode - 1
        {
          GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPInfo2); // IN IN OUT OUT
          if(EFI_ERROR(GOPStatus))
          {
            Print(L"GraphicsTable QueryMode error. 0x%llx\r\n", GOPStatus);
            return GOPStatus;
          }
          Print(L"%c. %ux%u, PxPerScanLine: %u, PxFormat: %s\r\n", mode + 0x30, GOPInfo2->HorizontalResolution, GOPInfo2->VerticalResolution, GOPInfo2->PixelsPerScanLine, PxFormats[GOPInfo2->PixelFormat]);
        }

        Print(L"Please select a graphics mode. (0 - %u)\r\n", GOPTable->Mode->MaxMode - 1);
        while ((GOPStatus = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);
        Print(L"Selected graphics mode %c.\r\n\n", Key.UnicodeChar);
      }
      mode = (UINT32)(Key.UnicodeChar - 0x30);
      Key.UnicodeChar = 0;

      // Don't need GOPInfo2 anymore
      GOPStatus = BS->FreePool(GOPInfo2);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPInfo2 pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }

      Print(L"Setting graphics mode %u of %u.\r\n", mode, GOPTable->Mode->MaxMode - 1);

      // Query current mode to get size and info
      // QueryMode allocates EfiBootServicesData
      GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPTable->Mode->Info); // IN IN OUT OUT
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GraphicsTable QueryMode error 2. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

#ifdef GOP_DEBUG_ENABLED
      Print(L"GOPInfoSize: %llu\r\n", GOPInfoSize);
#endif

      // Allocate graphics mode info
      GOPStatus = ST->BootServices->AllocatePool(EfiLoaderData, GOPInfoSize, (void**)&Graphics->GPUArray[DevNum].Info);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GOP Mode->Info AllocatePool error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"Current mode info allocated.\r\n");
#endif

      // Clear screen to reset cursor position
      ST->ConOut->ClearScreen(ST->ConOut);

      // Set mode
      GOPStatus = GOPTable->SetMode(GOPTable, mode);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GraphicsTable SetMode error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

      // Store graphics mode info
      // Can't blanketly store Mode struct because Mode->Info pointer in array will get overwritten
      Graphics->GPUArray[DevNum].MaxMode = GOPTable->Mode->MaxMode;
      Graphics->GPUArray[DevNum].Mode = GOPTable->Mode->Mode;
      Graphics->GPUArray[DevNum].SizeOfInfo = GOPTable->Mode->SizeOfInfo;
      Graphics->GPUArray[DevNum].FrameBufferBase = GOPTable->Mode->FrameBufferBase;
      Graphics->GPUArray[DevNum].FrameBufferSize = GOPTable->Mode->FrameBufferSize;
      // Can blanketly override Info struct, though (no pointers in it, just raw data)
      *(Graphics->GPUArray[DevNum].Info) = *(GOPTable->Mode->Info);

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"Current mode info assigned.\r\n");
#endif

/*
// I'm not sure we even need these, especially if AllocatePool is set to allocate EfiBootServicesData for them
// EfiBootServicesData just gets cleared on ExitBootServices() anyways

      // Free pools
      GOPStatus = BS->FreePool(GOPTable->Mode->Info);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPTable Mode Info pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }

      GOPStatus = BS->FreePool(GOPTable->Mode);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPTable Mode pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }

      GOPStatus = BS->FreePool(GOPTable);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPTable pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }
*/
    } // End for each individual DevNum
  }
  else if((NumHandlesInHandleBuffer > 1) && (DevNum == 1))
  {
    // Configure one

    // Setup
    Graphics->NumberOfFrameBuffers = 1;
    GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, Graphics->NumberOfFrameBuffers*sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&Graphics->GPUArray);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GPUArray AllocatePool error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

    // Configure

    // User selection of GOP-supporting device
    while(0x30 > Key.UnicodeChar || Key.UnicodeChar > (0x30 + NumHandlesInHandleBuffer - 1))
    {
      for(DevNum = 0; DevNum < NumHandlesInHandleBuffer; DevNum++)
      {
        Print(L"%c. GPU #%c: 0x%llx\r\n", DevNum + 0x30, DevNum + 0x30, GraphicsHandles[DevNum]); // Memory address of GPU handle
      }
      Print(L"Please select an output device. (0 - %llu)\r\n", NumHandlesInHandleBuffer - 1);
      while ((GOPStatus = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);
      Print(L"Device %c selected.\r\n", Key.UnicodeChar);
    }
    DevNum = (UINT64)(Key.UnicodeChar - 0x30); // Convert user input character from UTF-16 to number
    Key.UnicodeChar = 0; // Reset input

#ifdef GOP_DEBUG_ENABLED
    Print(L"Using handle %llu...\r\n", DevNum);

    Keywait(L"Allocating GOP pools...\r\n");
#endif

    EFI_GRAPHICS_OUTPUT_PROTOCOL *GOPTable;
    // Reserve memory for graphics output structure

    GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL), (void**)&GOPTable); // All EfiBootServicesData get freed on ExitBootServices()
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GOPTable AllocatePool error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

    // Reserve memory for graphics output mode to preserve it
    GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&GOPTable->Mode);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GOP Mode AllocatePool error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }
    // Mode->Info gets reserved once SizeOfInfo is determined.

#ifdef GOP_DEBUG_ENABLED
    Keywait(L"GOPTable and Mode pools allocated....\r\n");
#endif

    GOPStatus = BS->OpenProtocol(GraphicsHandles[DevNum], &GraphicsOutputProtocol, (void**)&GOPTable, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GraphicsTable OpenProtocol error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

#ifdef GOP_DEBUG_ENABLED
    Keywait(L"OpenProtocol passed.\r\n");

    Print(L"Current GOP Mode Info:\r\n");
    Print(L"Max Mode supported: %u, Current Mode: %u\r\nSize of Mode Info Structure: %llu Bytes\r\n", GOPTable->Mode->MaxMode - 1, GOPTable->Mode->Mode, GOPTable->Mode->SizeOfInfo);
    Print(L"FrameBufferBase: 0x%016llx, FrameBufferSize: 0x%llx\r\n", GOPTable->Mode->FrameBufferBase, GOPTable->Mode->FrameBufferSize); // Per spec, the FrameBufferBase might be 0 until SetMode is called

    Keywait(L"\0");

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *GOPInfo; // Querymode allocates GOPInfo
    // Get detailed info about supported graphics modes
    for(mode = 0; mode < GOPTable->Mode->MaxMode; mode++) // Valid modes are from 0 to MaxMode - 1
    {
      GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPInfo); // IN IN OUT OUT
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GraphicsTable QueryMode error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }
      Print(L"Mode %u of %u (%llu Bytes):\r\n Ver: 0x%x, Res: %ux%u\r\n", mode, GOPTable->Mode->MaxMode - 1, GOPInfoSize, GOPInfo->Version, GOPInfo->HorizontalResolution, GOPInfo->VerticalResolution);
      Print(L"PxPerScanLine: %u\r\n", GOPInfo->PixelsPerScanLine);
      Print(L"PxFormat: 0x%x, PxInfo (R,G,B,Rsvd Masks): 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n", GOPInfo->PixelFormat, GOPInfo->PixelInformation.RedMask, GOPInfo->PixelInformation.GreenMask, GOPInfo->PixelInformation.BlueMask, GOPInfo->PixelInformation.ReservedMask);
      Keywait(L"\0");
    }

    // Don't need GOPInfo anymore
    GOPStatus = BS->FreePool(GOPInfo);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"Error freeing GOPInfo pool. 0x%llx\r\n", GOPStatus);
      Keywait(L"\0");
    }
#endif

    if(GOPTable->Mode->MaxMode == 1) // Grammar
    {
      Print(L"%u available graphics mode found.\r\n", GOPTable->Mode->MaxMode);
    }
    else
    {
      Print(L"%u available graphics modes found.\r\n", GOPTable->Mode->MaxMode);
    }

#ifdef GOP_DEBUG_ENABLED
    Keywait(L"\r\nGetting list of supported modes...\r\n");
#endif

    // Get supported graphics modes
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *GOPInfo2; // Querymode allocates GOPInfo
    while(0x30 > Key.UnicodeChar || Key.UnicodeChar > (0x30 + GOPTable->Mode->MaxMode - 1))
    {
      Print(L"Current Mode: %u\r\n", GOPTable->Mode->Mode);
      for(mode = 0; mode < GOPTable->Mode->MaxMode; mode++) // Valid modes are from 0 to MaxMode - 1
      {
        GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPInfo2); // IN IN OUT OUT
        if(EFI_ERROR(GOPStatus))
        {
          Print(L"GraphicsTable QueryMode error. 0x%llx\r\n", GOPStatus);
          return GOPStatus;
        }
        Print(L"%c. %ux%u, PxPerScanLine: %u, PxFormat: %s\r\n", mode + 0x30, GOPInfo2->HorizontalResolution, GOPInfo2->VerticalResolution, GOPInfo2->PixelsPerScanLine, PxFormats[GOPInfo2->PixelFormat]);
      }

      Print(L"Please select a graphics mode. (0 - %u)\r\n", GOPTable->Mode->MaxMode - 1);
      while ((GOPStatus = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);
      Print(L"Selected graphics mode %c.\r\n\n", Key.UnicodeChar);
    }
    mode = (UINT32)(Key.UnicodeChar - 0x30);
    Key.UnicodeChar = 0;

    // Don't need GOPInfo2 anymore
    GOPStatus = BS->FreePool(GOPInfo2);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"Error freeing GOPInfo2 pool. 0x%llx\r\n", GOPStatus);
      Keywait(L"\0");
    }

    Print(L"Setting graphics mode %u of %u.\r\n", mode, GOPTable->Mode->MaxMode - 1);

    // Query current mode to get size and info
    // QueryMode allocates EfiBootServicesData
    GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPTable->Mode->Info); // IN IN OUT OUT
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GraphicsTable QueryMode error 2. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

#ifdef GOP_DEBUG_ENABLED
    Print(L"GOPInfoSize: %llu\r\n", GOPInfoSize);
#endif

    DevNum = 0; // There's only one item in the array
    // Allocate graphics mode info
    GOPStatus = ST->BootServices->AllocatePool(EfiLoaderData, GOPInfoSize, (void**)&Graphics->GPUArray[DevNum].Info);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GOP Mode->Info AllocatePool error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

#ifdef GOP_DEBUG_ENABLED
    Keywait(L"Current mode info allocated.\r\n");
#endif

    // Clear screen to reset cursor position
    ST->ConOut->ClearScreen(ST->ConOut);

    // Set mode
    GOPStatus = GOPTable->SetMode(GOPTable, mode);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GraphicsTable SetMode error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

    // Store graphics mode info
    // Can't blanketly store Mode struct because Mode->Info pointer in array will get overwritten
    Graphics->GPUArray[DevNum].MaxMode = GOPTable->Mode->MaxMode;
    Graphics->GPUArray[DevNum].Mode = GOPTable->Mode->Mode;
    Graphics->GPUArray[DevNum].SizeOfInfo = GOPTable->Mode->SizeOfInfo;
    Graphics->GPUArray[DevNum].FrameBufferBase = GOPTable->Mode->FrameBufferBase;
    Graphics->GPUArray[DevNum].FrameBufferSize = GOPTable->Mode->FrameBufferSize;
    // Can blanketly override Info struct, though (no pointers in it, just raw data)
    *(Graphics->GPUArray[DevNum].Info) = *(GOPTable->Mode->Info);

#ifdef GOP_DEBUG_ENABLED
    Keywait(L"Current mode info assigned.\r\n");
#endif

/*
// I'm not sure we even need these, especially if AllocatePool is set to allocate EfiBootServicesData for them
// EfiBootServicesData just gets cleared on ExitBootServices() anyways

    // Free pools
    GOPStatus = BS->FreePool(GOPTable->Mode->Info);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"Error freeing GOPTable Mode Info pool. 0x%llx\r\n", GOPStatus);
      Keywait(L"\0");
    }

    GOPStatus = BS->FreePool(GOPTable->Mode);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"Error freeing GOPTable Mode pool. 0x%llx\r\n", GOPStatus);
      Keywait(L"\0");
    }

    GOPStatus = BS->FreePool(GOPTable);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"Error freeing GOPTable pool. 0x%llx\r\n", GOPStatus);
      Keywait(L"\0");
    }
*/
  // End configure one only
  }
  else if((NumHandlesInHandleBuffer > 1) && (DevNum == 2))
  {
    // Configure each device to use the default resolutions of each connected display (usually native)

    // Setup
    Graphics->NumberOfFrameBuffers = NumHandlesInHandleBuffer;
    GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, Graphics->NumberOfFrameBuffers*sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&Graphics->GPUArray);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GPUArray AllocatePool error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

    // Configure
    for(DevNum = 0; DevNum < NumHandlesInHandleBuffer; DevNum++)
    {
      Print(L"%c. GPU #%c: 0x%llx\r\n", DevNum + 0x30, DevNum + 0x30, GraphicsHandles[DevNum]); // Memory address of GPU handle

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"Allocating GOP pools...\r\n");
#endif

      EFI_GRAPHICS_OUTPUT_PROTOCOL *GOPTable;
      // Reserve memory for graphics output structure

      GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL), (void**)&GOPTable); // All EfiBootServicesData get freed on ExitBootServices()
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GOPTable AllocatePool error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

      // Reserve memory for graphics output mode to preserve it
      GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&GOPTable->Mode);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GOP Mode AllocatePool error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }
      // Mode->Info gets reserved once SizeOfInfo is determined.

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"GOPTable and Mode pools allocated....\r\n");
#endif

      GOPStatus = BS->OpenProtocol(GraphicsHandles[DevNum], &GraphicsOutputProtocol, (void**)&GOPTable, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GraphicsTable OpenProtocol error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"OpenProtocol passed.\r\n");

      Print(L"Current GOP Mode Info:\r\n");
      Print(L"Max Mode supported: %u, Current Mode: %u\r\nSize of Mode Info Structure: %llu Bytes\r\n", GOPTable->Mode->MaxMode - 1, GOPTable->Mode->Mode, GOPTable->Mode->SizeOfInfo);
      Print(L"FrameBufferBase: 0x%016llx, FrameBufferSize: 0x%llx\r\n", GOPTable->Mode->FrameBufferBase, GOPTable->Mode->FrameBufferSize); // Per spec, the FrameBufferBase might be 0 until SetMode is called

      Keywait(L"\0");
#endif

      // Set mode 0
      mode = 0;

      Print(L"Setting graphics mode %u of %u.\r\n", mode, GOPTable->Mode->MaxMode - 1);

      // Query current mode to get size and info
      // QueryMode allocates EfiBootServicesData
      GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPTable->Mode->Info); // IN IN OUT OUT
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GraphicsTable QueryMode error 2. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

#ifdef GOP_DEBUG_ENABLED
      Print(L"Mode %u of %u (%llu Bytes):\r\n Ver: 0x%x, Res: %ux%u\r\n", mode, GOPTable->Mode->MaxMode - 1, GOPInfoSize, GOPTable->Mode->Info->Version, GOPTable->Mode->Info->HorizontalResolution, GOPTable->Mode->Info->VerticalResolution);
      Print(L"PxPerScanLine: %u\r\n", GOPTable->Mode->Info->PixelsPerScanLine);
      Print(L"PxFormat: 0x%x, PxInfo (R,G,B,Rsvd Masks): 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n", GOPTable->Mode->Info->PixelFormat, GOPTable->Mode->Info->PixelInformation.RedMask, GOPTable->Mode->Info->PixelInformation.GreenMask, GOPTable->Mode->Info->PixelInformation.BlueMask, GOPTable->Mode->Info->PixelInformation.ReservedMask);
      Keywait(L"\0");
#endif

      // Allocate graphics mode info
      GOPStatus = ST->BootServices->AllocatePool(EfiLoaderData, GOPInfoSize, (void**)&Graphics->GPUArray[DevNum].Info);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GOP Mode->Info AllocatePool error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"Current mode info allocated.\r\n");
#endif

      // Clear screen to reset cursor position
      ST->ConOut->ClearScreen(ST->ConOut);

      // Set mode
      GOPStatus = GOPTable->SetMode(GOPTable, mode);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GraphicsTable SetMode error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

      // Store graphics mode info
      // Can't blanketly store Mode struct because Mode->Info pointer in array will get overwritten
      Graphics->GPUArray[DevNum].MaxMode = GOPTable->Mode->MaxMode;
      Graphics->GPUArray[DevNum].Mode = GOPTable->Mode->Mode;
      Graphics->GPUArray[DevNum].SizeOfInfo = GOPTable->Mode->SizeOfInfo;
      Graphics->GPUArray[DevNum].FrameBufferBase = GOPTable->Mode->FrameBufferBase;
      Graphics->GPUArray[DevNum].FrameBufferSize = GOPTable->Mode->FrameBufferSize;
      // Can blanketly override Info struct, though (no pointers in it, just raw data)
      *(Graphics->GPUArray[DevNum].Info) = *(GOPTable->Mode->Info);

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"Current mode info assigned.\r\n");
#endif

/*
// I'm not sure we even need these, especially if AllocatePool is set to allocate EfiBootServicesData for them
// EfiBootServicesData just gets cleared on ExitBootServices() anyways

      // Free pools
      GOPStatus = BS->FreePool(GOPTable->Mode->Info);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPTable Mode Info pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }

      GOPStatus = BS->FreePool(GOPTable->Mode);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPTable Mode pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }

      GOPStatus = BS->FreePool(GOPTable);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPTable pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }
*/
    } // End for each individual DevNum
    // End default res for each
  }
  else if((NumHandlesInHandleBuffer > 1) && (DevNum == 3))
  {
    // Configure all to use 1024x768
    // Despite the UEFI spec's mandating only 640x480 and 800x600, everyone who supports Windows must also support 1024x768

    // Setup
    Graphics->NumberOfFrameBuffers = NumHandlesInHandleBuffer;
    GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, Graphics->NumberOfFrameBuffers*sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&Graphics->GPUArray);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GPUArray AllocatePool error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

    // Configure
    for(DevNum = 0; DevNum < NumHandlesInHandleBuffer; DevNum++)
    {
      Print(L"%c. GPU #%c: 0x%llx\r\n", DevNum + 0x30, DevNum + 0x30, GraphicsHandles[DevNum]); // Memory address of GPU handle

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"Allocating GOP pools...\r\n");
#endif

      EFI_GRAPHICS_OUTPUT_PROTOCOL *GOPTable;
      // Reserve memory for graphics output structure

      GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL), (void**)&GOPTable); // All EfiBootServicesData get freed on ExitBootServices()
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GOPTable AllocatePool error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

      // Reserve memory for graphics output mode to preserve it
      GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&GOPTable->Mode);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GOP Mode AllocatePool error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }
      // Mode->Info gets reserved once SizeOfInfo is determined.

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"GOPTable and Mode pools allocated....\r\n");
#endif

      GOPStatus = BS->OpenProtocol(GraphicsHandles[DevNum], &GraphicsOutputProtocol, (void**)&GOPTable, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GraphicsTable OpenProtocol error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"OpenProtocol passed.\r\n");

      Print(L"Current GOP Mode Info:\r\n");
      Print(L"Max Mode supported: %u, Current Mode: %u\r\nSize of Mode Info Structure: %llu Bytes\r\n", GOPTable->Mode->MaxMode - 1, GOPTable->Mode->Mode, GOPTable->Mode->SizeOfInfo);
      Print(L"FrameBufferBase: 0x%016llx, FrameBufferSize: 0x%llx\r\n", GOPTable->Mode->FrameBufferBase, GOPTable->Mode->FrameBufferSize); // Per spec, the FrameBufferBase might be 0 until SetMode is called

      Keywait(L"\0");
#endif

      // Get supported graphics modes
      EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *GOPInfo2; // Querymode allocates GOPInfo
      for(mode = 0; mode < GOPTable->Mode->MaxMode; mode++) // Valid modes are from 0 to MaxMode - 1
      {
        GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPInfo2); // IN IN OUT OUT
        if(EFI_ERROR(GOPStatus))
        {
          Print(L"GraphicsTable QueryMode error. 0x%llx\r\n", GOPStatus);
          return GOPStatus;
        }
        if((GOPInfo2->HorizontalResolution == 1024) && (GOPInfo2->VerticalResolution == 768))
        {
          break; // Use this mode
        }
      }
      if((mode == GOPTable->Mode->MaxMode) && (mode != 0)) // Hyper-V only has a 1024x768 mode, and it's mode 0
      {
        Print(L"Odd. No 1024x768 mode found. Using mode 0...\r\n");
        mode = 0;
      }

      // Don't need GOPInfo2 anymore
      GOPStatus = BS->FreePool(GOPInfo2);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPInfo2 pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }

      Print(L"Setting graphics mode %u of %u.\r\n", mode, GOPTable->Mode->MaxMode - 1);

      // Query current mode to get size and info
      // QueryMode allocates EfiBootServicesData
      GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPTable->Mode->Info); // IN IN OUT OUT
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GraphicsTable QueryMode error 2. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

#ifdef GOP_DEBUG_ENABLED
      Print(L"Mode %u of %u (%llu Bytes):\r\n Ver: 0x%x, Res: %ux%u\r\n", mode, GOPTable->Mode->MaxMode - 1, GOPInfoSize, GOPTable->Mode->Info->Version, GOPTable->Mode->Info->HorizontalResolution, GOPTable->Mode->Info->VerticalResolution);
      Print(L"PxPerScanLine: %u\r\n", GOPTable->Mode->Info->PixelsPerScanLine);
      Print(L"PxFormat: 0x%x, PxInfo (R,G,B,Rsvd Masks): 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n", GOPTable->Mode->Info->PixelFormat, GOPTable->Mode->Info->PixelInformation.RedMask, GOPTable->Mode->Info->PixelInformation.GreenMask, GOPTable->Mode->Info->PixelInformation.BlueMask, GOPTable->Mode->Info->PixelInformation.ReservedMask);
      Keywait(L"\0");
#endif

      // Allocate graphics mode info
      GOPStatus = ST->BootServices->AllocatePool(EfiLoaderData, GOPInfoSize, (void**)&Graphics->GPUArray[DevNum].Info);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GOP Mode->Info AllocatePool error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"Current mode info allocated.\r\n");
#endif

      // Clear screen to reset cursor position
      ST->ConOut->ClearScreen(ST->ConOut);

      // Set mode
      GOPStatus = GOPTable->SetMode(GOPTable, mode);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GraphicsTable SetMode error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }

      // Store graphics mode info
      // Can't blanketly store Mode struct because Mode->Info pointer in array will get overwritten
      Graphics->GPUArray[DevNum].MaxMode = GOPTable->Mode->MaxMode;
      Graphics->GPUArray[DevNum].Mode = GOPTable->Mode->Mode;
      Graphics->GPUArray[DevNum].SizeOfInfo = GOPTable->Mode->SizeOfInfo;
      Graphics->GPUArray[DevNum].FrameBufferBase = GOPTable->Mode->FrameBufferBase;
      Graphics->GPUArray[DevNum].FrameBufferSize = GOPTable->Mode->FrameBufferSize;
      // Can blanketly override Info struct, though (no pointers in it, just raw data)
      *(Graphics->GPUArray[DevNum].Info) = *(GOPTable->Mode->Info);

#ifdef GOP_DEBUG_ENABLED
      Keywait(L"Current mode info assigned.\r\n");
#endif

/*
// I'm not sure we even need these, especially if AllocatePool is set to allocate EfiBootServicesData for them
// EfiBootServicesData just gets cleared on ExitBootServices() anyways

      // Free pools
      GOPStatus = BS->FreePool(GOPTable->Mode->Info);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPTable Mode Info pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }

      GOPStatus = BS->FreePool(GOPTable->Mode);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPTable Mode pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }

      GOPStatus = BS->FreePool(GOPTable);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPTable pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }
*/
    } // End for each individual DevNum
    // End 1024x768
  }
  else
  {
    // Single GPU

    // Setup
    Graphics->NumberOfFrameBuffers = 1;
    GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, Graphics->NumberOfFrameBuffers*sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&Graphics->GPUArray);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GPUArray AllocatePool error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

    // Configure
    // Only one device
    DevNum = 0;

#ifdef GOP_DEBUG_ENABLED
    Print(L"One GPU detected.\r\n");

    Keywait(L"Allocating GOP pools...\r\n");
#endif

    EFI_GRAPHICS_OUTPUT_PROTOCOL *GOPTable;
    // Reserve memory for graphics output structure

    GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL), (void**)&GOPTable); // All EfiBootServicesData get freed on ExitBootServices()
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GOPTable AllocatePool error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

    // Reserve memory for graphics output mode to preserve it
    GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&GOPTable->Mode);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GOP Mode AllocatePool error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }
    // Mode->Info gets reserved once SizeOfInfo is determined.

#ifdef GOP_DEBUG_ENABLED
    Keywait(L"GOPTable and Mode pools allocated....\r\n");
#endif

    GOPStatus = BS->OpenProtocol(GraphicsHandles[DevNum], &GraphicsOutputProtocol, (void**)&GOPTable, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GraphicsTable OpenProtocol error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

#ifdef GOP_DEBUG_ENABLED
    Keywait(L"OpenProtocol passed.\r\n");

    Print(L"Current GOP Mode Info:\r\n");
    Print(L"Max Mode supported: %u, Current Mode: %u\r\nSize of Mode Info Structure: %llu Bytes\r\n", GOPTable->Mode->MaxMode - 1, GOPTable->Mode->Mode, GOPTable->Mode->SizeOfInfo);
    Print(L"FrameBufferBase: 0x%016llx, FrameBufferSize: 0x%llx\r\n", GOPTable->Mode->FrameBufferBase, GOPTable->Mode->FrameBufferSize); // Per spec, the FrameBufferBase might be 0 until SetMode is called

    Keywait(L"\0");

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *GOPInfo; // Querymode allocates GOPInfo
    // Get detailed info about supported graphics modes
    for(mode = 0; mode < GOPTable->Mode->MaxMode; mode++) // Valid modes are from 0 to MaxMode - 1
    {
      GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPInfo); // IN IN OUT OUT
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GraphicsTable QueryMode error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }
      Print(L"Mode %u of %u (%llu Bytes):\r\n Ver: 0x%x, Res: %ux%u\r\n", mode, GOPTable->Mode->MaxMode - 1, GOPInfoSize, GOPInfo->Version, GOPInfo->HorizontalResolution, GOPInfo->VerticalResolution);
      Print(L"PxPerScanLine: %u\r\n", GOPInfo->PixelsPerScanLine);
      Print(L"PxFormat: 0x%x, PxInfo (R,G,B,Rsvd Masks): 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n", GOPInfo->PixelFormat, GOPInfo->PixelInformation.RedMask, GOPInfo->PixelInformation.GreenMask, GOPInfo->PixelInformation.BlueMask, GOPInfo->PixelInformation.ReservedMask);
      Keywait(L"\0");
    }

    // Don't need GOPInfo anymore
    GOPStatus = BS->FreePool(GOPInfo);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"Error freeing GOPInfo pool. 0x%llx\r\n", GOPStatus);
      Keywait(L"\0");
    }
#endif

    if(GOPTable->Mode->MaxMode == 1) // Grammar
    {
      Print(L"%u available graphics mode found.\r\n", GOPTable->Mode->MaxMode);
    }
    else
    {
      Print(L"%u available graphics modes found.\r\n", GOPTable->Mode->MaxMode);
    }

#ifdef GOP_DEBUG_ENABLED
    Keywait(L"\r\nGetting list of supported modes...\r\n");
#endif

    // Get supported graphics modes
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *GOPInfo2; // Querymode allocates GOPInfo
    while(0x30 > Key.UnicodeChar || Key.UnicodeChar > (0x30 + GOPTable->Mode->MaxMode - 1))
    {
      Print(L"Current Mode: %u\r\n", GOPTable->Mode->Mode);
      for(mode = 0; mode < GOPTable->Mode->MaxMode; mode++) // Valid modes are from 0 to MaxMode - 1
      {
        GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPInfo2); // IN IN OUT OUT
        if(EFI_ERROR(GOPStatus))
        {
          Print(L"GraphicsTable QueryMode error. 0x%llx\r\n", GOPStatus);
          return GOPStatus;
        }
        Print(L"%c. %ux%u, PxPerScanLine: %u, PxFormat: %s\r\n", mode + 0x30, GOPInfo2->HorizontalResolution, GOPInfo2->VerticalResolution, GOPInfo2->PixelsPerScanLine, PxFormats[GOPInfo2->PixelFormat]);
      }

      Print(L"Please select a graphics mode. (0 - %u)\r\n", GOPTable->Mode->MaxMode - 1);
      while ((GOPStatus = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);
      Print(L"Selected graphics mode %c.\r\n\n", Key.UnicodeChar);
    }
    mode = (UINT32)(Key.UnicodeChar - 0x30);
    Key.UnicodeChar = 0;

    // Don't need GOPInfo2 anymore
    GOPStatus = BS->FreePool(GOPInfo2);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"Error freeing GOPInfo2 pool. 0x%llx\r\n", GOPStatus);
      Keywait(L"\0");
    }

    Print(L"Setting graphics mode %u of %u.\r\n", mode, GOPTable->Mode->MaxMode - 1);

    // Query current mode to get size and info
    // QueryMode allocates EfiBootServicesData
    GOPStatus = GOPTable->QueryMode(GOPTable, mode, &GOPInfoSize, &GOPTable->Mode->Info); // IN IN OUT OUT
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GraphicsTable QueryMode error 2. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

#ifdef GOP_DEBUG_ENABLED
    Print(L"GOPInfoSize: %llu\r\n", GOPInfoSize);
#endif

    // Allocate graphics mode info
    GOPStatus = ST->BootServices->AllocatePool(EfiLoaderData, GOPInfoSize, (void**)&Graphics->GPUArray[DevNum].Info);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GOP Mode->Info AllocatePool error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

#ifdef GOP_DEBUG_ENABLED
    Keywait(L"Current mode info allocated.\r\n");
#endif

    // Clear screen to reset cursor position
    ST->ConOut->ClearScreen(ST->ConOut);

    // Set mode
    GOPStatus = GOPTable->SetMode(GOPTable, mode);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"GraphicsTable SetMode error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

    // Store graphics mode info
    // Can't blanketly store Mode struct because Mode->Info pointer in array will get overwritten
    Graphics->GPUArray[DevNum].MaxMode = GOPTable->Mode->MaxMode;
    Graphics->GPUArray[DevNum].Mode = GOPTable->Mode->Mode;
    Graphics->GPUArray[DevNum].SizeOfInfo = GOPTable->Mode->SizeOfInfo;
    Graphics->GPUArray[DevNum].FrameBufferBase = GOPTable->Mode->FrameBufferBase;
    Graphics->GPUArray[DevNum].FrameBufferSize = GOPTable->Mode->FrameBufferSize;
    // Can blanketly override Info struct, though (no pointers in it, just raw data)
    *(Graphics->GPUArray[DevNum].Info) = *(GOPTable->Mode->Info);

#ifdef GOP_DEBUG_ENABLED
    Keywait(L"Current mode info assigned.\r\n");
#endif

/*
// I'm not sure we even need these, especially if AllocatePool is set to allocate EfiBootServicesData for them
// EfiBootServicesData just gets cleared on ExitBootServices() anyways

    // Free pools
    GOPStatus = BS->FreePool(GOPTable->Mode->Info);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"Error freeing GOPTable Mode Info pool. 0x%llx\r\n", GOPStatus);
      Keywait(L"\0");
    }

    GOPStatus = BS->FreePool(GOPTable->Mode);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"Error freeing GOPTable Mode pool. 0x%llx\r\n", GOPStatus);
      Keywait(L"\0");
    }

    GOPStatus = BS->FreePool(GOPTable);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"Error freeing GOPTable pool. 0x%llx\r\n", GOPStatus);
      Keywait(L"\0");
    }
*/
  // End single GPU
  }

  // Don't need GraphicsHandles anymore
  GOPStatus = BS->FreePool(GraphicsHandles);
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"Error freeing GraphicsHandles pool. 0x%llx\r\n", GOPStatus);
    Keywait(L"\0");
  }

#ifdef GOP_DEBUG_ENABLED
    // Data verification
  for(DevNum = 0; DevNum < Graphics->NumberOfFrameBuffers; DevNum++)
  {
    Print(L"\r\nCurrent GOP Mode Info:\r\n");
    Print(L"Max Mode supported: %u, Current Mode: %u\r\nSize of Mode Info Structure: %llu Bytes\r\n", Graphics->GPUArray[DevNum].MaxMode - 1, Graphics->GPUArray[DevNum].Mode, Graphics->GPUArray[DevNum].SizeOfInfo);
    Print(L"FrameBufferBase: 0x%016llx, FrameBufferSize: 0x%llx\r\n", Graphics->GPUArray[DevNum].FrameBufferBase, Graphics->GPUArray[DevNum].FrameBufferSize);

    Print(L"Mode %u of %u (%llu Bytes):\r\n Ver: 0x%x, Res: %ux%u\r\n", Graphics->GPUArray[DevNum].Mode, Graphics->GPUArray[DevNum].MaxMode - 1, Graphics->GPUArray[DevNum].SizeOfInfo, Graphics->GPUArray[DevNum].Info->Version, Graphics->GPUArray[DevNum].Info->HorizontalResolution, Graphics->GPUArray[DevNum].Info->VerticalResolution);
    Print(L"PxPerScanLine: %u\r\n", Graphics->GPUArray[DevNum].Info->PixelsPerScanLine);
    Print(L"PxFormat: 0x%x, PxInfo (R,G,B,Rsvd Masks): 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n", Graphics->GPUArray[DevNum].Info->PixelFormat, Graphics->GPUArray[DevNum].Info->PixelInformation.RedMask, Graphics->GPUArray[DevNum].Info->PixelInformation.GreenMask, Graphics->GPUArray[DevNum].Info->PixelInformation.BlueMask, Graphics->GPUArray[DevNum].Info->PixelInformation.ReservedMask);
    Keywait(L"\0");
  }
#endif

  return GOPStatus;
}
// For reference, so no one has to go hunting for them
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
