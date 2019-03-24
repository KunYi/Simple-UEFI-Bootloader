//==================================================================================================================================
//  Simple UEFI Bootloader: Graphics Functions
//==================================================================================================================================
//
// Version 1.4
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
#define GOP_NAMING_DEBUG_ENABLED
// TODO: test release mode

#ifdef GOP_NAMING_DEBUG_ENABLED
EFI_STATUS WhatProtocols(EFI_HANDLE * HandleArray, UINTN NumHandlesInHandleArray);
#endif

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

// This array is a global variable so that it can be made static, which helps prevent a stack overflow if it ever needs to lengthen.
STATIC CONST CHAR16 PxFormats[5][17] = {
    L"RGBReserved 8Bpp",
    L"BGRReserved 8Bpp",
    L"PixelBitMask    ",
    L"PixelBltOnly    ",
    L"PixelFormatMax  "
};

EFI_STATUS InitUEFI_GOP(EFI_HANDLE ImageHandle, GPU_CONFIG * Graphics)
{ // Declaring a pointer only allocates 8 bytes (x64) for that pointer. Buffers must be manually allocated memory via AllocatePool and then freed with FreePool when done with.

  Graphics->NumberOfFrameBuffers = 0;

  EFI_STATUS GOPStatus;

  UINT64 GOPInfoSize;
  UINT32 mode;
  UINTN NumHandlesInHandleBuffer = 0; // Number of discovered graphics handles (GPUs)
  UINTN NumName2Handles = 0;
  UINTN NumDriverHandles = 0;
  UINTN NumDevPathHandles = 0;
  UINT64 DevNum = 0;
  EFI_INPUT_KEY Key;

  Key.UnicodeChar = 0;

  // Vendors go all over the place with these...
  CHAR8 LanguageToUse[6] = {'e','n','-','U','S','\0'};
  CHAR8 LanguageToUse2[3] = {'e','n','\0'};
  CHAR8 LanguageToUse3[4] = {'e','n','g','\0'};

  CHAR16 DefaultDriverDisplayName[15] = L"No Driver Name";
  CHAR16 DefaultControllerDisplayName[19] = L"No Controller Name";
  CHAR16 DefaultChildDisplayName[14] = L"No Child Name";

  CHAR16 * DriverDisplayName = DefaultDriverDisplayName;
  CHAR16 * ControllerDisplayName = DefaultControllerDisplayName;
  CHAR16 * ChildDisplayName = DefaultChildDisplayName;

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

  // List all GPUs

  // Get all NAME2-supporting handles
  EFI_HANDLE *Name2Handles;

  GOPStatus = BS->LocateHandleBuffer(ByProtocol, &ComponentName2Protocol, NULL, &NumName2Handles, &Name2Handles); // This automatically allocates pool for Name2Handles
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"Name2Handles LocateHandleBuffer error. 0x%llx\r\n", GOPStatus);
    return GOPStatus;
  }

  EFI_HANDLE *DriverHandles;

  GOPStatus = BS->LocateHandleBuffer(ByProtocol, &DriverBindingProtocol, NULL, &NumDriverHandles, &DriverHandles); // This automatically allocates pool for DriverHandles
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"DriverHandles LocateHandleBuffer error. 0x%llx\r\n", GOPStatus);
    return GOPStatus;
  }

  EFI_HANDLE *DevPathHandles;

  GOPStatus = BS->LocateHandleBuffer(ByProtocol, &DevicePathProtocol, NULL, &NumDevPathHandles, &DevPathHandles); // This automatically allocates pool for DevPathHandles
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"DevPathHandles LocateHandleBuffer error. 0x%llx\r\n", GOPStatus);
    return GOPStatus;
  }

#ifdef GOP_NAMING_DEBUG_ENABLED
  Print(L"Number of Name2Handles: %llu\r\n", NumName2Handles);
  Print(L"Number of DriverHandles: %llu\r\n", NumDriverHandles);
  Print(L"Number of DevPathHandles: %llu\r\n", NumDevPathHandles);

//  WhatProtocols(GraphicsHandles, NumHandlesInHandleBuffer);
//  WhatProtocols(Name2Handles, NumName2Handles);
//  WhatProtocols(DriverHandles, NumDriverHandles);
//  WhatProtocols(DevPathHandles, NumDevPathHandles);
#endif

  for(DevNum = 0; DevNum < NumHandlesInHandleBuffer; DevNum++)
  {
    DriverDisplayName = DefaultDriverDisplayName;
    ControllerDisplayName = DefaultControllerDisplayName;
    ChildDisplayName = DefaultChildDisplayName;

    EFI_DEVICE_PATH *DevicePath_Graphics; // For GraphicsHandles, they'll always have a devpath because they use ACPI _ADR and they describe a physical output device

    GOPStatus = BS->OpenProtocol(GraphicsHandles[DevNum], &DevicePathProtocol, (void**)&DevicePath_Graphics, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL); // Need Device Path Node of GOP device
    if(GOPStatus == EFI_SUCCESS)
    {
      UINTN CntlrPathSize = DevicePathSize(DevicePath_Graphics) - DevicePathNodeLength(DevicePath_Graphics) + 4; // Add 4 bytes to account for the end node

#ifdef GOP_NAMING_DEBUG_ENABLED
//      Print(L"DevPathStructBytes: 0x%x, PathBytes: 0x%016llx%016llx%016llx\r\n", *(DevicePath_Graphics), *(EFI_PHYSICAL_ADDRESS *)((CHAR8*)DevicePath_Graphics + 20), *(EFI_PHYSICAL_ADDRESS *)((CHAR8*)DevicePath_Graphics + 12), *(EFI_PHYSICAL_ADDRESS *)((CHAR8*)DevicePath_Graphics + 4)); // DevicePath struct is 4 bytes
      Print(L"a. DevPathGraphics: %s, CntlrPathSize: %llu\r\n", DevicePathToStr(DevicePath_Graphics), CntlrPathSize); // FYI: This allocates pool for the string //TODO: remove this line
      Keywait(L"\0");
#endif

      // Find the controller that corresponds to the GraphicsHandle's device path

      EFI_DEVICE_PATH *DevicePath_DevPath;
      UINT64 CntlrIndex = 0;

      for(CntlrIndex = 0; CntlrIndex < NumDevPathHandles; CntlrIndex++)
      {
        // Per https://github.com/tianocore/edk2/blob/master/ShellPkg/Library/UefiShellDriver1CommandsLib/DevTree.c
        // Controllers don't have DriverBinding or LoadedImage

#ifdef GOP_NAMING_DEBUG_ENABLED
        Print(L"b. CntlrIndex: %llu\r\n", CntlrIndex);
#endif

        GOPStatus = BS->OpenProtocol(DevPathHandles[CntlrIndex], &DriverBindingProtocol, NULL, NULL, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
        if (!EFI_ERROR (GOPStatus))
        {
          continue;
        }

        GOPStatus = BS->OpenProtocol(DevPathHandles[CntlrIndex], &LoadedImageProtocol, NULL, NULL, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
        if (!EFI_ERROR (GOPStatus))
        {
          continue;
        }

        // The controllers we want don't have SimpFs, which seems to cause crashes if we don't skip them. Apparently if a FAT32 controller handle is paired with an NTFS driver, the system locks up.
        // This would only be a problem with the current method if the graphics output device also happened to be a user-writable storage device. I don't know how the UEFI GOP of a Radeon SSG is set up, and that's the only edge case I can think of.
        // If the SSD were writable, it would probably have a separate controller on a similar path to the GOP device, and better to just reject SimpFS tobe safe.
        GOPStatus = BS->OpenProtocol(DevPathHandles[CntlrIndex], &FileSystemProtocol, NULL, NULL, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
        if (!EFI_ERROR (GOPStatus))
        {
          continue;
        }

#ifdef GOP_NAMING_DEBUG_ENABLED
        Print(L"c. Filtered CntlrIndex: %llu\r\n", CntlrIndex);
#endif

        // Get controller's device path
        GOPStatus = BS->OpenProtocol(DevPathHandles[CntlrIndex], &DevicePathProtocol, (void**)&DevicePath_DevPath, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if(EFI_ERROR(GOPStatus))
        {
          Print(L"DevPathHandles OpenProtocol error. 0x%llx\r\n", GOPStatus);
          return GOPStatus;
        }

        // Match device paths; DevPath is a Multi, Graphics is a Single
        // This will fail on certain kinds of systems, like Hyper-V VMs. This method is made with PCI-Express graphics devices in mind.
        UINTN ThisCntlrPathSize = DevicePathSize(DevicePath_DevPath);

#ifdef GOP_NAMING_DEBUG_ENABLED
        Print(L"d. DevPathDevPath: %s, ThisCntlrPathSize: %llu\r\n", DevicePathToStr(DevicePath_DevPath), ThisCntlrPathSize); // FYI: This allocates pool for the string //TODO: remove this line
        Keywait(L"\0");
#endif

        if(ThisCntlrPathSize != CntlrPathSize)
        { // Might be something like PciRoot(0), which would match DevPath_Graphics for a PCI-E GPU without this check
          continue;
        }

        if(LibMatchDevicePaths(DevicePath_DevPath, DevicePath_Graphics))
        {
          // Found it. The desired controller is DevPathHandles[CntlrIndex]

#ifdef GOP_NAMING_DEBUG_ENABLED
          Print(L"e. Above DevPathDevPath matched DevPathGraphics %llu, CntlrIndex: %llu\r\n", DevNum, CntlrIndex);
#endif

          // Now match controller to its Name2-supporting driver
          for(UINT64 Name2DriverIndex = 0; Name2DriverIndex < NumName2Handles; Name2DriverIndex++)
          {

#ifdef GOP_NAMING_DEBUG_ENABLED
            Print(L"f. Name2DriverIndex: %llu\r\n", Name2DriverIndex);
#endif

            // Check if Name2Handles[Name2DriverIndex] manages the DevPathHandles[CntlrIndex] controller
            // See EfiTestManagedDevice at
            // https://github.com/tianocore-docs/edk2-UefiDriverWritersGuide/blob/master/11_uefi_driver_and_controller_names/113_getcontrollername_implementations/1132_bus_drivers_and_hybrid_drivers.md
            // and the implementation at https://github.com/tianocore/edk2/blob/master/MdePkg/Library/UefiLib/UefiLib.c

            VOID * ManagedInterface; // Need a throwaway pointer for OpenProtocol BY_DRIVER

            GOPStatus = BS->OpenProtocol(DevPathHandles[CntlrIndex], &PciIoProtocol, &ManagedInterface, Name2Handles[Name2DriverIndex], DevPathHandles[CntlrIndex], EFI_OPEN_PROTOCOL_BY_DRIVER);
            if(!EFI_ERROR(GOPStatus))
            {
              GOPStatus = BS->CloseProtocol(DevPathHandles[CntlrIndex], &PciIoProtocol, Name2Handles[Name2DriverIndex], DevPathHandles[CntlrIndex]);
              if(EFI_ERROR(GOPStatus))
              {
                Print(L"DevPathHandles Name2Handles CloseProtocol error. 0x%llx\r\n", GOPStatus);
                return GOPStatus;
              }
              // No match!
              continue;
            }
            else if(GOPStatus != EFI_ALREADY_STARTED)
            {
              // No match!
              continue;
            }
            // Yes, found it! Get names.

#ifdef GOP_NAMING_DEBUG_ENABLED
            Print(L"i. Success! CntlrIndex %llu, Name2DriverIndex: %llu, DevNum: %llu\r\n", CntlrIndex, Name2DriverIndex, DevNum);
#endif

            EFI_COMPONENT_NAME2_PROTOCOL *Name2Device;

            // Open Name2 Protocol
            GOPStatus = BS->OpenProtocol(Name2Handles[Name2DriverIndex], &ComponentName2Protocol, (void**)&Name2Device, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
            if(EFI_ERROR(GOPStatus))
            {
              Print(L"Name2Device OpenProtocol error. 0x%llx\r\n", GOPStatus);
              return GOPStatus;
            }

            // Get driver's name
            GOPStatus = Name2Device->GetDriverName(Name2Device, LanguageToUse, &DriverDisplayName);
            if(GOPStatus == EFI_UNSUPPORTED)
            {
              GOPStatus = Name2Device->GetDriverName(Name2Device, LanguageToUse2, &DriverDisplayName);
              if(GOPStatus == EFI_UNSUPPORTED)
              {
                GOPStatus = Name2Device->GetDriverName(Name2Device, LanguageToUse3, &DriverDisplayName);
              }
            }
            if(EFI_ERROR(GOPStatus))
            {

#ifdef GOP_NAMING_DEBUG_ENABLED
              Print(L"Name2Device GetDriverName error. 0x%llx\r\n", GOPStatus);

              if(GOPStatus == EFI_UNSUPPORTED)
              {
                // This is the format of the language, since it isn't "en-US" or "en"
                // It will need to be implemented like the above arrays
                Print(L"First 10 language characters look like this:\r\n");
                for(UINT32 p = 0; p < 10; p++)
                {
                  Print(L"%c", Name2Device->SupportedLanguages[p]);
                }
                Print(L"\r\n");

                Keywait(L"\0");
              }
#endif
              // You know, we have specifications for a reason.
              // Those who refuse to follow them get this.
              DriverDisplayName = DefaultDriverDisplayName;
            }
            // Got driver's name

#ifdef GOP_NAMING_DEBUG_ENABLED
            Print(L"j. Got driver name\r\n");
#endif

            // Get controller's name
            GOPStatus = Name2Device->GetControllerName(Name2Device, DevPathHandles[CntlrIndex], NULL, LanguageToUse, &ControllerDisplayName); // The child should be NULL to get the controller's name.
            if(GOPStatus == EFI_UNSUPPORTED)
            {
              GOPStatus = Name2Device->GetControllerName(Name2Device, DevPathHandles[CntlrIndex], NULL, LanguageToUse2, &ControllerDisplayName);
              if(GOPStatus == EFI_UNSUPPORTED)
              {
                GOPStatus = Name2Device->GetControllerName(Name2Device, DevPathHandles[CntlrIndex], NULL, LanguageToUse3, &ControllerDisplayName);
              }
            }
            if(EFI_ERROR(GOPStatus))
            {

#ifdef GOP_NAMING_DEBUG_ENABLED
              Print(L"Name2Device GetControllerName error. 0x%llx\r\n", GOPStatus); // Enable this to diagnose crashes due to controller name matching.
#endif
              // You know, we have specifications for a reason.
              // Those who refuse to follow them get this.
              ControllerDisplayName = DefaultControllerDisplayName;
            }
            // Got controller's name

#ifdef GOP_NAMING_DEBUG_ENABLED
            Print(L"k. Got controller name\r\n");
#endif

            // Get child's name
            GOPStatus = Name2Device->GetControllerName(Name2Device, DevPathHandles[CntlrIndex], GraphicsHandles[DevNum], LanguageToUse, &ChildDisplayName);
            if(GOPStatus == EFI_UNSUPPORTED)
            {
              GOPStatus = Name2Device->GetControllerName(Name2Device, DevPathHandles[CntlrIndex], GraphicsHandles[DevNum], LanguageToUse2, &ChildDisplayName);
              if(GOPStatus == EFI_UNSUPPORTED)
              {
                GOPStatus = Name2Device->GetControllerName(Name2Device, DevPathHandles[CntlrIndex], GraphicsHandles[DevNum], LanguageToUse3, &ChildDisplayName);
              }
            }
            if(EFI_ERROR(GOPStatus))
            {

#ifdef GOP_NAMING_DEBUG_ENABLED
              Print(L"Name2Device GetControllerName ChildName error. 0x%llx\r\n", GOPStatus);
#endif
              // You know, we have specifications for a reason.
              // Those who refuse to follow them get this.
              ChildDisplayName = DefaultChildDisplayName;
            }

#ifdef GOP_NAMING_DEBUG_ENABLED
            Print(L"l. Got names\r\n");
#endif

            // Got child's name
            break;

          } // End for Name2DriverIndex
          break;
        } // End if match controller to GraphicsHandle's device path
      } // End for CntlrIndex

      // It's possible that a device is missing a child name, but has the controller and device names. The triple if() statement below covers this.

      // After all that, if still no name, it's probably a VM or something weird that doesn't implement ACPI ADR or PCIe.
      // This means there probably aren't devices like the PCIe root bridge to mess with device path matching.
      // So use a more generic method that's cognizant of this and doesn't enforce device path sizes.

      if((ControllerDisplayName == DefaultControllerDisplayName) && (DriverDisplayName == DefaultDriverDisplayName) && (ChildDisplayName == DefaultChildDisplayName))
      {

#ifdef GOP_NAMING_DEBUG_ENABLED
        Print(L"\r\nFunky graphics device here.\r\n");

  //      Print(L"DevPathStructBytes: 0x%x, PathBytes: 0x%016llx%016llx%016llx\r\n", *(DevicePath_Graphics), *(EFI_PHYSICAL_ADDRESS *)((CHAR8*)DevicePath_Graphics + 20), *(EFI_PHYSICAL_ADDRESS *)((CHAR8*)DevicePath_Graphics + 12), *(EFI_PHYSICAL_ADDRESS *)((CHAR8*)DevicePath_Graphics + 4)); // DevicePath struct is 4 bytes
        Print(L"af. DevPathGraphics: %s\r\n", DevicePathToStr(DevicePath_Graphics)); // FYI: This allocates pool for the string //TODO: remove this line
        Keywait(L"\0");
#endif
        // Find the controller that corresponds to the GraphicsHandle's device path

        // These have already been defined.
        // EFI_DEVICE_PATH *DevicePath_DevPath;
        // UINT64 CntlrIndex = 0;

        for(CntlrIndex = 0; CntlrIndex < NumDevPathHandles; CntlrIndex++)
        {
          // Per https://github.com/tianocore/edk2/blob/master/ShellPkg/Library/UefiShellDriver1CommandsLib/DevTree.c
          // Controllers don't have DriverBinding or LoadedImage

#ifdef GOP_NAMING_DEBUG_ENABLED
          Print(L"bf. CntlrIndex: %llu\r\n", CntlrIndex);
#endif

          GOPStatus = BS->OpenProtocol(DevPathHandles[CntlrIndex], &DriverBindingProtocol, NULL, NULL, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
          if (!EFI_ERROR (GOPStatus))
          {
            continue;
          }

          GOPStatus = BS->OpenProtocol(DevPathHandles[CntlrIndex], &LoadedImageProtocol, NULL, NULL, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
          if (!EFI_ERROR (GOPStatus))
          {
            continue;
          }

          // The controllers we want shouldn't have SimpFs, which seems to cause crashes if we don't skip them. Apparently if a FAT32 controller handle is paired with an NTFS driver, the system locks up.
          // This would only be a problem with the current method if the graphics output device also happened to be a user-writable storage device. I don't know how the UEFI GOP of a Radeon SSG is set up, and that's the only edge case I can think of.
          // If the SSD were writable, it would probably have a separate controller on a similar path to the GOP device, and better to just reject SimpFS tobe safe.
          // However, if this is in a VM, all bets are off and it's better to be safe than sorry.
          GOPStatus = BS->OpenProtocol(DevPathHandles[CntlrIndex], &FileSystemProtocol, NULL, NULL, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
          if (!EFI_ERROR (GOPStatus))
          {
            continue;
          }

          // This is to filter out that ridiculous AMI PS/2 driver. Not a problem on PCIe busses, but definitely could be a problem here.
          GOPStatus = BS->OpenProtocol(DevPathHandles[CntlrIndex], &SioProtocol, NULL, NULL, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
          if (!EFI_ERROR (GOPStatus))
          {
            continue;
          }

#ifdef GOP_NAMING_DEBUG_ENABLED
          Print(L"cf. Filtered CntlrIndex: %llu\r\n", CntlrIndex);
#endif
          // Get controller's device path
          GOPStatus = BS->OpenProtocol(DevPathHandles[CntlrIndex], &DevicePathProtocol, (void**)&DevicePath_DevPath, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
          if(EFI_ERROR(GOPStatus))
          {
            Print(L"Funky DevPathHandles OpenProtocol error. 0x%llx\r\n", GOPStatus);
            return GOPStatus;
          }

          // Match device paths; DevPath is a Multi, Graphics is a Single
          // DevPath could be as generic as VMBus or something, which is fine for this case since it's not PCIe.
#ifdef GOP_NAMING_DEBUG_ENABLED
          Print(L"df. DevPathDevPath: %s\r\n", DevicePathToStr(DevicePath_DevPath)); // FYI: This allocates pool for the string //TODO: remove this line
          Keywait(L"\0");
#endif
          if(LibMatchDevicePaths(DevicePath_DevPath, DevicePath_Graphics))
          {
            // Found something on controller DevPathHandles[CntlrIndex]

#ifdef GOP_NAMING_DEBUG_ENABLED
            Print(L"ef. Above DevPathDevPath matched DevPathGraphics %llu, CntlrIndex: %llu\r\n", DevNum, CntlrIndex);
#endif
            // Now match controller to its Name2-supporting driver
            for(UINT64 Name2DriverIndex = 0; Name2DriverIndex < NumName2Handles; Name2DriverIndex++)
            {

#ifdef GOP_NAMING_DEBUG_ENABLED
              Print(L"ff. Name2DriverIndex: %llu\r\n", Name2DriverIndex);
#endif
              // Check if Name2Handles[Name2DriverIndex] manages the DevPathHandles[CntlrIndex] controller
              // See EfiTestManagedDevice at
              // https://github.com/tianocore-docs/edk2-UefiDriverWritersGuide/blob/master/11_uefi_driver_and_controller_names/113_getcontrollername_implementations/1132_bus_drivers_and_hybrid_drivers.md
              // and the implementation at https://github.com/tianocore/edk2/blob/master/MdePkg/Library/UefiLib/UefiLib.c

// Hyper-V has a guid of 63d25797-59eb-4125-a34e-b2b4a8e1587e that might be its VMBus equivalent of PciIoProtocol, but that's probably not standard enough a GUID to be able to enforce this filter.
// This is meant to be generic.
/*
              VOID * ManagedInterface; // Need a throwaway pointer for OpenProtocol BY_DRIVER

              GOPStatus = BS->OpenProtocol(DevPathHandles[CntlrIndex], &PciIoProtocol, &ManagedInterface, Name2Handles[Name2DriverIndex], DevPathHandles[CntlrIndex], EFI_OPEN_PROTOCOL_BY_DRIVER);
              if(!EFI_ERROR(GOPStatus))
              {
                GOPStatus = BS->CloseProtocol(DevPathHandles[CntlrIndex], &PciIoProtocol, Name2Handles[Name2DriverIndex], DevPathHandles[CntlrIndex]);
                if(EFI_ERROR(GOPStatus))
                {
                  Print(L"DevPathHandles Name2Handles CloseProtocol error. 0x%llx\r\n", GOPStatus);
                  return GOPStatus;
                }
                // No match!
                continue;
              }
              else if(GOPStatus != EFI_ALREADY_STARTED)
              {
                // No match!
                continue;
              }
              // Yes, found it! Get names.

#ifdef GOP_NAMING_DEBUG_ENABLED
              Print(L"if. Success! CntlrIndex %llu, Name2DriverIndex: %llu, DevNum: %llu\r\n", CntlrIndex, Name2DriverIndex, DevNum);
#endif
*/
              EFI_COMPONENT_NAME2_PROTOCOL *Name2Device;

              // Open Name2 Protocol
              GOPStatus = BS->OpenProtocol(Name2Handles[Name2DriverIndex], &ComponentName2Protocol, (void**)&Name2Device, ImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
              if(EFI_ERROR(GOPStatus))
              {
                Print(L"Funky Name2Device OpenProtocol error. 0x%llx\r\n", GOPStatus);
                return GOPStatus;
              }

              // Get driver's name
              GOPStatus = Name2Device->GetDriverName(Name2Device, LanguageToUse, &DriverDisplayName);
              if(GOPStatus == EFI_UNSUPPORTED)
              {
                GOPStatus = Name2Device->GetDriverName(Name2Device, LanguageToUse2, &DriverDisplayName);
                if(GOPStatus == EFI_UNSUPPORTED)
                {
                  GOPStatus = Name2Device->GetDriverName(Name2Device, LanguageToUse3, &DriverDisplayName);
                }
              }
              if(EFI_ERROR(GOPStatus))
              {

#ifdef GOP_NAMING_DEBUG_ENABLED
                Print(L"Funky Name2Device GetDriverName error. 0x%llx\r\n", GOPStatus);

                if(GOPStatus == EFI_UNSUPPORTED)
                {
                  // This is the format of the language, since it isn't "en-US" or "en"
                  // It will need to be implemented like the above arrays
                  Print(L"First 10 language characters look like this:\r\n");
                  for(UINT32 p = 0; p < 10; p++)
                  {
                    Print(L"%c", Name2Device->SupportedLanguages[p]);
                  }
                  Print(L"\r\n");

                  Keywait(L"\0");
                }
#endif
                // You know, we have specifications for a reason.
                // Those who refuse to follow them get this.
                DriverDisplayName = DefaultDriverDisplayName;
              }
              // Got driver's name

#ifdef GOP_NAMING_DEBUG_ENABLED
              Print(L"jf. Got driver name\r\n");
#endif
              // Get controller's name
              GOPStatus = Name2Device->GetControllerName(Name2Device, DevPathHandles[CntlrIndex], NULL, LanguageToUse, &ControllerDisplayName); // The child should be NULL to get the controller's name.
              if(GOPStatus == EFI_UNSUPPORTED)
              {
                GOPStatus = Name2Device->GetControllerName(Name2Device, DevPathHandles[CntlrIndex], NULL, LanguageToUse2, &ControllerDisplayName);
                if(GOPStatus == EFI_UNSUPPORTED)
                {
                  GOPStatus = Name2Device->GetControllerName(Name2Device, DevPathHandles[CntlrIndex], NULL, LanguageToUse3, &ControllerDisplayName);
                }
              }
              if(EFI_ERROR(GOPStatus))
              {

#ifdef GOP_NAMING_DEBUG_ENABLED
                Print(L"Funky Name2Device GetControllerName error. 0x%llx\r\n", GOPStatus); // Enable this to diagnose crashes due to controller name matching.
#endif
                // You know, we have specifications for a reason.
                // Those who refuse to follow them get this.
                ControllerDisplayName = DefaultControllerDisplayName;
              }
              // Got controller's name

#ifdef GOP_NAMING_DEBUG_ENABLED
              Print(L"kf. Got controller name\r\n");
#endif
              // Get child's name
              GOPStatus = Name2Device->GetControllerName(Name2Device, DevPathHandles[CntlrIndex], GraphicsHandles[DevNum], LanguageToUse, &ChildDisplayName);
              if(GOPStatus == EFI_UNSUPPORTED)
              {
                GOPStatus = Name2Device->GetControllerName(Name2Device, DevPathHandles[CntlrIndex], GraphicsHandles[DevNum], LanguageToUse2, &ChildDisplayName);
                if(GOPStatus == EFI_UNSUPPORTED)
                {
                  GOPStatus = Name2Device->GetControllerName(Name2Device, DevPathHandles[CntlrIndex], GraphicsHandles[DevNum], LanguageToUse3, &ChildDisplayName);
                }
              }
              if(EFI_ERROR(GOPStatus))
              {

#ifdef GOP_NAMING_DEBUG_ENABLED
                Print(L"Funky Name2Device GetControllerName ChildName error. 0x%llx\r\n", GOPStatus);
#endif
                // You know, we have specifications for a reason.
                // Those who refuse to follow them get this.
                ChildDisplayName = DefaultChildDisplayName;
              }

#ifdef GOP_NAMING_DEBUG_ENABLED
              Print(L"lf. Got names\r\n");
              Print(L"%s: %s: %s\r\n", ControllerDisplayName, DriverDisplayName, ChildDisplayName);
              Keywait(L"\0");
#endif
              // Got child's name
              // Hopefully this wasn't another case of the "PS/2 driver that tries to claim that all handles are its children" :P
              // There's no way to check without explicitly blacklisting by driver name or filtering by protocol (as was done above).
              // I think Hyper-V's video driver does something similar, though it reports Hyper-V Video Controller as the child of Hyper-V Video Controller,
              // which, in this specific case, is actually OK. Would've nice if they'd implemented a proper ChildHandle, though.
              if(ChildDisplayName != DefaultChildDisplayName)
              {
                break;
              }
              // Nope, try another Name2 driver

            } // End for Name2DriverIndex
            if(ChildDisplayName != DefaultChildDisplayName)
            {
              break;
            }
            // Nope, try and see if another controller matches
          } // End if match controller to GraphicsHandle's device path
        } // End for CntlrIndex
      } // End funky graphics types method

      Print(L"%c. 0x%llx - %s: %s: %s\r\n", DevNum + 0x30, GraphicsHandles[DevNum], ControllerDisplayName, DriverDisplayName, ChildDisplayName);

#ifdef GOP_NAMING_DEBUG_ENABLED
      Keywait(L"\0");
#endif

    }
    else if(GOPStatus == EFI_UNSUPPORTED) // Need to do this because VMs can throw curveballs sometimes, like Hyper-V with RemoteFX on an NVidia Optimus laptop. Yep.
    {
      Print(L"%c. 0x%llx - Weird extra device (is this in a VM?).\r\n", DevNum + 0x30, GraphicsHandles[DevNum]);
    }
    else if(EFI_ERROR(GOPStatus))
    {
      Print(L"GraphicsHandles DevicePath_Graphics OpenProtocol error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

#ifdef GOP_NAMING_DEBUG_ENABLED
    Keywait(L"\0");
#endif

  } // End for(GraphicsHandles[DevNum]...)


  // Done with this massive array of DevPathHandles
  GOPStatus = BS->FreePool(DevPathHandles);
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"DevPathHandles FreePool error. 0x%llx\r\n", GOPStatus);
    return GOPStatus;
  }

  // Done with this massive array of DriverHandles // TODO: Need this anymore?
  GOPStatus = BS->FreePool(DriverHandles);
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"DriverHandles FreePool error. 0x%llx\r\n", GOPStatus);
    return GOPStatus;
  }

  // Done with this massive array of Name2Handles
  GOPStatus = BS->FreePool(Name2Handles);
  if(EFI_ERROR(GOPStatus))
  {
    Print(L"Name2Handles FreePool error. 0x%llx\r\n", GOPStatus);
    return GOPStatus;
  }

  Print(L"\r\n");

/* // Old way, no GPU names
  for(DevNum = 0; DevNum < NumHandlesInHandleBuffer; DevNum++)
  {
    Print(L"%c. GPU #%c: 0x%llx\r\n", DevNum + 0x30, DevNum + 0x30, GraphicsHandles[DevNum]); // Memory address of GPU handle
  }
  Print(L"\r\n");
*/
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
    GOPStatus = ST->BootServices->AllocatePool(EfiLoaderData, Graphics->NumberOfFrameBuffers*sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&Graphics->GPUArray);
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
/*
      GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL), (void**)&GOPTable); // All EfiBootServicesData get freed on ExitBootServices()
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GOPTable AllocatePool error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }
*/
    /*
      // These are all the same
      Print(L"sizeof(struct _EFI_GRAPHICS_OUTPUT_PROTOCOL): %llu\r\n", sizeof(struct _EFI_GRAPHICS_OUTPUT_PROTOCOL));
      Print(L"sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL): %llu\r\n", sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL));
      Print(L"sizeof(*GOPTable): %llu\r\n", sizeof(*GOPTable));
      Print(L"sizeof(GOPTable[0]): %llu\r\n", sizeof(GOPTable[0]));
    */
/*
      // Reserve memory for graphics output mode to preserve it
      GOPStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&GOPTable->Mode);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"GOP Mode AllocatePool error. 0x%llx\r\n", GOPStatus);
        return GOPStatus;
      }
      // Mode->Info gets reserved once SizeOfInfo is determined (via QueryMode()).
*/
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


// I'm not sure we even need these, especially if AllocatePool is set to allocate EfiBootServicesData for them
// EfiBootServicesData just gets cleared on ExitBootServices() anyways

      // Free pools
      GOPStatus = BS->FreePool(GOPTable->Mode->Info);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPTable Mode Info pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }
/*
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
    GOPStatus = ST->BootServices->AllocatePool(EfiLoaderData, Graphics->NumberOfFrameBuffers*sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&Graphics->GPUArray);
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
/*
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
*/
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


// I'm not sure we even need these, especially if AllocatePool is set to allocate EfiBootServicesData for them
// EfiBootServicesData just gets cleared on ExitBootServices() anyways

    // Free pools
    GOPStatus = BS->FreePool(GOPTable->Mode->Info);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"Error freeing GOPTable Mode Info pool. 0x%llx\r\n", GOPStatus);
      Keywait(L"\0");
    }
/*
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
    GOPStatus = ST->BootServices->AllocatePool(EfiLoaderData, Graphics->NumberOfFrameBuffers*sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&Graphics->GPUArray);
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
/*
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
*/
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


// I'm not sure we even need these, especially if AllocatePool is set to allocate EfiBootServicesData for them
// EfiBootServicesData just gets cleared on ExitBootServices() anyways

      // Free pools
      GOPStatus = BS->FreePool(GOPTable->Mode->Info);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPTable Mode Info pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }
/*
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
    GOPStatus = ST->BootServices->AllocatePool(EfiLoaderData, Graphics->NumberOfFrameBuffers*sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&Graphics->GPUArray);
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
/*
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
*/
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


// I'm not sure we even need these, especially if AllocatePool is set to allocate EfiBootServicesData for them
// EfiBootServicesData just gets cleared on ExitBootServices() anyways

      // Free pools
      GOPStatus = BS->FreePool(GOPTable->Mode->Info);
      if(EFI_ERROR(GOPStatus))
      {
        Print(L"Error freeing GOPTable Mode Info pool. 0x%llx\r\n", GOPStatus);
        Keywait(L"\0");
      }
/*
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
    GOPStatus = ST->BootServices->AllocatePool(EfiLoaderData, Graphics->NumberOfFrameBuffers*sizeof(EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE), (void**)&Graphics->GPUArray);
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
/*
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
*/
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


// I'm not sure we even need these, especially if AllocatePool is set to allocate EfiBootServicesData for them
// EfiBootServicesData just gets cleared on ExitBootServices() anyways

    // Free pools
    GOPStatus = BS->FreePool(GOPTable->Mode->Info);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"Error freeing GOPTable Mode Info pool. 0x%llx\r\n", GOPStatus);
      Keywait(L"\0");
    }
/*
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


#ifdef GOP_NAMING_DEBUG_ENABLED

// Want more GUIDs or see some you can't identify?
// Try here: https://github.com/tianocore/edk2/blob/master/MdePkg/MdePkg.dec
// And check out this insane list: https://github.com/snare/ida-efiutils
// If you're using a VM, you might get some unlisted ones that aren't in any public list.

STATIC struct {
    EFI_GUID        *Guid;
    WCHAR           *GuidName;
} KnownGuids[] = {
	{  &NullGuid,                                       L"G0" },
	{  &gEfiGlobalVariableGuid,                         L"EfiVar" },

  {  &gEfiFirmwareVolume2ProtocolGuid,                L"FrmVol2" },
  {  &gEfiFirmwareVolumeBlockProtocolGuid,            L"FrmVolBlk" },

	{  &VariableStoreProtocol,                          L"VarStore" },
	{  &gEfiDevicePathProtocolGuid,                     L"DevPath" },
	{  &gEfiLoadedImageProtocolGuid,                    L"LdImg" },
	{  &gEfiSimpleTextInProtocolGuid,                   L"TxtIn" },
	{  &gEfiSimpleTextOutProtocolGuid,                  L"TxtOut" },
	{  &gEfiBlockIoProtocolGuid,                        L"BlkIo" },
	{  &gEfiBlockIo2ProtocolGuid,                       L"BlkIo2" },
	{  &gEfiDiskIoProtocolGuid,                         L"DskIo" },
	{  &gEfiDiskIo2ProtocolGuid,                        L"DskIo2" },
	{  &gEfiSimpleFileSystemProtocolGuid,               L"SimpFs" },
	{  &gEfiLoadFileProtocolGuid,                       L"LdFile" },
  {  &gEfiLoadFile2ProtocolGuid,                      L"LdFile2" },
	{  &gEfiDeviceIoProtocolGuid,                       L"DevIo" },
	{  &gEfiComponentNameProtocolGuid,                  L"CName" },
	{  &gEfiComponentName2ProtocolGuid,                 L"CName2" },

  {  &gEfiDriverBindingProtocolGuid,                  L"DrvBind" },
  {  &gEfiPciIoProtocolGuid,                          L"PciIo" },
  {  &gEfiPciRootBridgeIoProtocolGuid,                L"PciRtBrdgeIo" },
  {  &gEfiGraphicsOutputProtocolGuid,                 L"GOP" },
  {  &gEfiDevicePathToTextProtocolGuid,               L"DevPathToTxt" },
  {  &gEfiDevicePathFromTextProtocolGuid,             L"DevPathFromTxt" },
  {  &gEfiLoadedImageDevicePathProtocolGuid,          L"LdImgDevPath" },

	{  &gEfiFileInfoGuid,                               L"FileInfo" },
	{  &gEfiFileSystemInfoGuid,                         L"FsInfo" },
	{  &gEfiFileSystemVolumeLabelInfoIdGuid,            L"FsVolInfo" },

	{  &gEfiUnicodeCollationProtocolGuid,               L"Unicode" },
	{  &LegacyBootProtocol,                             L"LegacyBoot" },
	{  &gEfiSerialIoProtocolGuid,                       L"SerIo" },
	{  &VgaClassProtocol,                               L"VgaClass"},
	{  &gEfiSimpleNetworkProtocolGuid,                  L"Net" },
	{  &gEfiNetworkInterfaceIdentifierProtocolGuid,     L"Nii" },
  {  &gEfiNetworkInterfaceIdentifierProtocolGuid_31,  L"Nii31" },
	{  &gEfiPxeBaseCodeProtocolGuid,                    L"Pxe" },
	{  &gEfiPxeBaseCodeCallbackProtocolGuid,            L"PxeCb" },

	{  &TextOutSpliterProtocol,                         L"TxtOutSplit" },
	{  &ErrorOutSpliterProtocol,                        L"ErrOutSplit" },
	{  &TextInSpliterProtocol,                          L"TxtInSplit" },
	{  &gEfiPcAnsiGuid,                                 L"PcAnsi" },
	{  &gEfiVT100Guid,                                  L"Vt100" },
	{  &gEfiVT100PlusGuid,                              L"Vt100Plus" },
	{  &gEfiVTUTF8Guid,                                 L"VtUtf8" },
	{  &UnknownDevice,                                  L"UnknownDev" },

  {  &gEfiSimpleTextInExProtocolGuid,                 L"TxtInEx" },
  {  &gEfiConsoleInDeviceGuid,                        L"ConInDevice" },
  {  &gEfiConsoleOutDeviceGuid,                       L"ConOutDevice" },
  {  &gEfiStandardErrorDeviceGuid,                    L"StdErrDevice" },
  {  &gEfiUgaDrawProtocolGuid,                        L"UGADraw" },

  {  &gEfiConsoleInDevicesStartedGuid,                L"ConInDevStrt" },
  {  &gEfiConsoleOutDevicesStartedGuid,               L"ConOutDevStrt" },

  {  &gEfiEdidDiscoveredProtocolGuid,                 L"EdidDiscovered" },
  {  &gEfiEdidActiveProtocolGuid,                     L"EdidActive" },
  {  &gEfiEdidOverrideProtocolGuid,                   L"EdidOverride" },

  {  &SimplePointerProtocol,                          L"SimpPtr" },
  {  &AbsolutePointerProtocol,                        L"AbsPtr" },

  {  &gEfiDriverSupportedEfiVersionProtocolGuid,      L"DrvSupEfiVer" },
  {  &gEfiDriverDiagnosticsProtocolGuid,              L"DrvDiag" },
  {  &gEfiDriverDiagnostics2ProtocolGuid,             L"DrvDiag2" },
  {  &gEfiDriverConfigurationProtocolGuid,            L"DrvConfig" },

	{  &EfiPartTypeSystemPartitionGuid,                 L"ESP" },
	{  &EfiPartTypeLegacyMbrGuid,                       L"GPT MBR" },

  {  &gEfiUsbPolicyProtocolGuid,                      L"UsbPol" },
  {  &gEfiUsbTimingPolicyProtocolGuid,                L"UsbTimPol" },
  {  &gEfiUsbIoProtocolGuid,                          L"UsbIo" },
  {  &gEfiUsb2HcProtocolGuid,                         L"Usb2Hc" },
  {  &gEfiUsbHcProtocolGuid,                          L"UsbHc" },

  {  &gEfiDataHubProtocolGuid,                        L"DataHub" },
  {  &gEfiPlatformIDEProtocolGuid,                    L"PlatformIDE" },
  {  &gEfiDiskInfoProtocolGuid,                       L"DiskInfo" },
  {  &gEfiScsiIoProtocolGuid,                         L"ScsiIo" },
  {  &gEfiExtScsiPassThruProtocolGuid,                L"ExtScsiPassThru" },
  {  &gEfiSioProtocolGuid,                            L"Sio"  },
  {  &gEfiIdeControllerInitProtocolGuid,              L"IdeCntlrInit" },
  {  &gEfiStorageSecurityCommandProtocolGuid,         L"StorSecurityCmd" },
  {  &gEfiHiiConfigAccessProtocolGuid,                L"HiiCfgAccess" },

  {  &gIntelGopGuid,                                  L"IntlGop" },

  {  &gAmiEfikeycodeProtocolGuid,                     L"AmiEfikeycode" },
  {  &gHotPlugDeviceGuid,                             L"HotPlugDev" },

  {  &gHddUnlockedGuid,                               L"HddUnlck" },
  {  &gHddSecurityEndProtocolGuid,                    L"HddSecurityEnd" },
  {  &gAhciBusInitProtocolGuid,                       L"AhciBusInit" },
  {  &gPchSataControllerDriverGuid,                   L"PchSataCntlrDrv" },

  {  &gEfiPlatformDriverOverrideProtocolGuid,         L"PltfrmDrvOvrride" },
  {  &gEfiBusSpecificDriverOverrideProtocolGuid,      L"BusDrvOvrride" },
  {  &gEfiDriverFamilyOverrideProtocolGuid,           L"DrvFamOvrride" },

	{  NULL, L"" }
};
// I've seen a rogue EFI_GUID that I can't find what it's for: 39487C79-236D-4666-87E5-09547CAAE1BC. It seems exclusive to Intel HD graphics family GOP handles.
// In "GUID Format" it's this: {0x39487c79, 0x236d, 0x4666, {0x87, 0xe5, 0x09, 0x54, 0x7c, 0xaa, 0xe1, 0xbc}}

EFI_STATUS WhatProtocols(EFI_HANDLE * HandleArray, UINTN NumHandlesInHandleArray)
{
  UINTN NumInHandle = 0;
  EFI_GUID ** ProtocolGUIDList;

  EFI_STATUS GOPStatus = EFI_SUCCESS;

  CHAR8 LanguageToUse[6] = {'e','n','-','U','S','\0'};
  CHAR8 LanguageToUse2[3] = {'e','n','\0'};
  CHAR8 LanguageToUse3[4] = {'e','n','g','\0'};

  CHAR16 DefaultDriverDisplayName[15] = L"No Driver Name";

  CHAR16 * DriverDisplayName = DefaultDriverDisplayName;

  for(UINT64 j = 0; j < NumHandlesInHandleArray; j++)
  {
    Print(L"Handle %llu: 0x%llx\r\n", j, HandleArray[j]);
    if(HandleArray[j] == NULL)
    {
      Print(L"Null Handle\r\n");
      continue;
    }

    GOPStatus = BS->ProtocolsPerHandle(HandleArray[j], &ProtocolGUIDList, &NumInHandle);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"ProtocolsPerHandle error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }

    for(UINT64 q = 0; q < NumInHandle; q++)
    {
      Print(L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x: ",
          ProtocolGUIDList[q]->Data1,
          ProtocolGUIDList[q]->Data2,
          ProtocolGUIDList[q]->Data3,
          ProtocolGUIDList[q]->Data4[0],
          ProtocolGUIDList[q]->Data4[1],
          ProtocolGUIDList[q]->Data4[2],
          ProtocolGUIDList[q]->Data4[3],
          ProtocolGUIDList[q]->Data4[4],
          ProtocolGUIDList[q]->Data4[5],
          ProtocolGUIDList[q]->Data4[6],
          ProtocolGUIDList[q]->Data4[7]
          );

      for (UINTN Index=0; KnownGuids[Index].Guid; Index++)
      {
        if (CompareGuid(ProtocolGUIDList[q], KnownGuids[Index].Guid) == 0)
        {
          Print(L"%s", KnownGuids[Index].GuidName);
        }
      }
      Print(L"\r\n");

      if (CompareGuid(ProtocolGUIDList[q], &gEfiComponentName2ProtocolGuid) == 0) // Display the name. At least we'll have something legible.
      {
        EFI_COMPONENT_NAME2_PROTOCOL *Name2Device;

        GOPStatus = BS->OpenProtocol(HandleArray[j], &ComponentName2Protocol, (void**)&Name2Device, NULL, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
        if(EFI_ERROR(GOPStatus))
        {
          Print(L"Name2Device OpenProtocol error. 0x%llx\r\n", GOPStatus);
          return GOPStatus;
        }

        GOPStatus = Name2Device->GetDriverName(Name2Device, LanguageToUse, &DriverDisplayName);
        if(GOPStatus == EFI_UNSUPPORTED)
        {
          GOPStatus = Name2Device->GetDriverName(Name2Device, LanguageToUse2, &DriverDisplayName);
          if(GOPStatus == EFI_UNSUPPORTED)
          {
            GOPStatus = Name2Device->GetDriverName(Name2Device, LanguageToUse3, &DriverDisplayName);
          }
        }
        if(EFI_ERROR(GOPStatus))
        {
          Print(L"Name2Device GetDriverName error. 0x%llx\r\n", GOPStatus);

          if(GOPStatus == EFI_UNSUPPORTED)
          {
            // This is the format of the language, since it isn't "en-US" or "en"
            // It will need to be implemented like the above arrays
            Print(L"First 10 language characters look like this:\r\n");
            for(UINT32 p = 0; p < 10; p++)
            {
              Print(L"%c", Name2Device->SupportedLanguages[p]);
            }
            Print(L"\r\n");

            Keywait(L"\0");
          }
          // You know, we have specifications for a reason.
          // Those who refuse to follow them get this.
          DriverDisplayName = DefaultDriverDisplayName;
        }
        Print(L"%s\r\n", DriverDisplayName);
      }

    }

    if(((j+1) % 2) == 0) // Show 2 handles at a time before prompt
    {
      Keywait(L"\0");
    }
  }
  Print(L"Done\r\n");
  Keywait(L"\0");

  if(ProtocolGUIDList)
  {
    GOPStatus = BS->FreePool(ProtocolGUIDList);
    if(EFI_ERROR(GOPStatus))
    {
      Print(L"ProtocolsPerHandle FreePool error. 0x%llx\r\n", GOPStatus);
      return GOPStatus;
    }
  }

  return GOPStatus;
}
#endif
