//==================================================================================================================================
//  Simple UEFI Bootloader: Bootloader Entrypoint
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
// About this program:
//
// This program is an x86-64 bootloader for UEFI-based systems. It's like GRUB, but simpler! (Though it won't boot Linux, Windows, etc.)
// It loads programs named "Kernel64" and passes the following structure to them:
/*
  typedef struct {
    UINTN                   Memory_Map_Size;            // The total size of the system memory map
    UINTN                   Memory_Map_Descriptor_Size; // The size of an individual memory descriptor
    EFI_MEMORY_DESCRIPTOR  *Memory_Map;                 // The system memory map as an array of EFI_MEMORY_DESCRIPTOR structs
    EFI_RUNTIME_SERVICES   *RTServices;                 // UEFI Runtime Services
    GPU_CONFIG             *GPU_Configs;                // Information about available graphics output devices; see below for details
    EFI_FILE_INFO          *FileMeta;                   // Kernel64 file metadata
    void                   *RSDP;                       // A pointer to the RSDP ACPI table
  } LOADER_PARAMS;
*/
//
// GPU_CONFIG is a custom structure that is defined as follows:
//
/*
  typedef struct {
    EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE  *GPUArray;             // An array of EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE structs defining each available framebuffer
    UINT64                              NumberOfFrameBuffers; // The number of structs in the array (== the number of available framebuffers)
  } GPU_CONFIG;
*/
//
// This bootloader is primarily intended to enable programs to run "bare-metal," i.e. without an operating system, on x86-64 machines.
// Technically this means that any program loaded by this one is an operating system kernel, but the main idea is to enable programming
// an x86-64 computer like a microcontroller such as an Arduino, STM32F7, C8051, etc.
//
// In addition to having complete freedom over one's own machine, this also means that accessing any hardware external to the CPU
// requires initalizing and programming that hardware directly. This is what operating system drivers do, as well as what embedded
// system programmers have to do on a daily basis. In other words, if you want keyboard input, you need to program the PS/2 or USB
// subsystem to get it.
//
// The above LOADER_PARAMS block simply takes whatever the UEFI boot environment has already set up and passes it to the user program,
// which should be named Kernel64. This gives users a place to start when developing applications using this bootloader, as well as a
// standard way to run bare metal programs on UEFI-based x86-64 machines. Also, starting from scratch on x86-64 is a very painful
// process... Especially when you can't use the screen to debug anything.
//
// Note: GPU_Configs provides access to linear framebuffer addresses for directly drawing to connected screens--specifically one for each
// active display per GPU. Typically there is one active display per GPU, but it is up to the GPU firmware maker to deterrmine that.
// See "12.10 Rules for PCI/AGP Devices" in the UEFI Specification 2.7 Errata A for more details: http://www.uefi.org/specifications
//

#include "Bootloader.h"

#define MAJOR_VER 1
#define MINOR_VER 4

//==================================================================================================================================
//  efi_main: Main Function
//==================================================================================================================================
//
// Loader's "main" function. This bootloader's program entry point is defined as efi_main per UEFI application convention.
//

EFI_STATUS efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable)
{
  // ImageHandle is this program's own EFI_HANDLE
  // SystemTable is the EFI system table of the machine

  // Initialize the GNU-EFI library
  InitializeLib(ImageHandle, SystemTable);
/*
  From InitializeLib:

  ST = SystemTable;
  BS = SystemTable->BootServices;
  RT = SystemTable->RuntimeServices;

*/
  EFI_STATUS Status;

#ifdef DISABLE_UEFI_WATCHDOG_TIMER
  // Disable watchdog timer for debugging
  Status = BS->SetWatchdogTimer (0, 0, 0, NULL);
  if(EFI_ERROR(Status))
  {
    Print(L"Error stopping watchdog, timeout still counting down...\r\n");
  }
#endif

  // Print out general system info
  EFI_TIME Now;
  Status = RT->GetTime(&Now, NULL);
  if(EFI_ERROR(Status))
  {
    Print(L"Error getting time...\r\n");
    return Status;
  }

  Print(L"%02hhu/%02hhu/%04hu - %02hhu:%02hhu:%02hhu.%u\r\n", Now.Month, Now.Day, Now.Year, Now.Hour, Now.Minute, Now.Second, Now.Nanosecond); // GNU-EFI apparently has a print function for time... Oh well.
#ifdef MAIN_DEBUG_ENABLED
  #ifdef MEMORY_DEBUG_ENABLED // Very slow memory debug version
    Print(L"Simple UEFI Bootloader - V%d.%d DEBUG (Memory)\r\n", MAJOR_VER, MINOR_VER);
  #else // Standard debug version
    Print(L"Simple UEFI Bootloader - V%d.%d DEBUG\r\n", MAJOR_VER, MINOR_VER);
  #endif
#else
  #ifdef FINAL_LOADER_DEBUG_ENABLED // Lite debug version
    Print(L"Simple UEFI Bootloader - V%d.%d DEBUG (Lite)\r\n", MAJOR_VER, MINOR_VER);
  #else // Release version
    Print(L"Simple UEFI Bootloader - V%d.%d\r\n", MAJOR_VER, MINOR_VER);
  #endif
#endif
  Print(L"Copyright (c) 2017-2019 KNNSpeed\r\n\n");

#ifdef MAIN_DEBUG_ENABLED
  Print(L"System Table Header Info\r\nSignature: 0x%lx\r\nRevision: 0x%08x\r\nHeader Size: %u Bytes\r\nCRC32: 0x%08x\r\nReserved: 0x%x\r\n\n", ST->Hdr.Signature, ST->Hdr.Revision, ST->Hdr.HeaderSize, ST->Hdr.CRC32, ST->Hdr.Reserved);
#else
  Print(L"System Table Header Info\r\nSignature: 0x%lx\r\nRevision: 0x%08x\r\n\n", ST->Hdr.Signature, ST->Hdr.Revision, ST->Hdr.HeaderSize, ST->Hdr.CRC32, ST->Hdr.Reserved);
#endif

  Print(L"Firmware Vendor: %s\r\nFirmware Revision: 0x%08x\r\n\n", ST->FirmwareVendor, ST->FirmwareRevision);

#ifdef MAIN_DEBUG_ENABLED
  Keywait(L"\0");
#endif

  // Configuration table info
  Print(L"Number of configuration tables: %lu\r\n", ST->NumberOfTableEntries);

  // Search for ACPI tables
  UINT8 RSDPfound = 0;
  UINT8 RSDP_index = 0;

  for(UINT8 i=0; i < ST->NumberOfTableEntries; i++)
  {
// This print is for debugging
#ifdef MAIN_DEBUG_ENABLED
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
#endif

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
    for(UINT8 i=0; i < ST->NumberOfTableEntries; i++)
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

#ifdef MAIN_DEBUG_ENABLED
  Keywait(L"\0");

  // View memmap before too much happens to it
  print_memmap();
  Keywait(L"Done printing MemMap.\r\n");
#endif

  // Create graphics structure
  GPU_CONFIG *Graphics;
  Status = ST->BootServices->AllocatePool(EfiLoaderData, sizeof(GPU_CONFIG), (void**)&Graphics);
  if(EFI_ERROR(Status))
  {
    Print(L"Graphics AllocatePool error. 0x%llx\r\n", Status);
    return Status;
  }

#ifdef MAIN_DEBUG_ENABLED
  Print(L"Graphics struct allocated\r\n");
#endif

  // Set up graphics
  Status = InitUEFI_GOP(ImageHandle, Graphics);
  if(EFI_ERROR(Status))
  {
    Print(L"InitUEFI_GOP error. 0x%llx\r\n", Status);
    Keywait(L"\0");
    return Status;
  }

#ifdef MAIN_DEBUG_ENABLED
  Keywait(L"InitUEFI_GOP finished.\r\n");

  // Data verification
  Print(L"RSDP address: 0x%llx\r\n", ST->ConfigurationTable[RSDP_index].VendorTable);
  Print(L"Data at RSDP (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(ST->ConfigurationTable[RSDP_index].VendorTable + 8), *(EFI_PHYSICAL_ADDRESS*)ST->ConfigurationTable[RSDP_index].VendorTable);
#endif

  // Load a program and exit boot services, then pass a loader block to that program's entry point to execute the program
  Status = GoTime(ImageHandle, Graphics, ST->ConfigurationTable[RSDP_index].VendorTable);

  // Pause to evaluate any errors
  Keywait(L"GoTime returned...\r\n");
  return Status;
}

//==================================================================================================================================
//  Keywait: Pause
//==================================================================================================================================
//
// A simple pause function that waits for user input before continuing.
// Adapted from http://wiki.osdev.org/UEFI_Bare_Bones
//

EFI_STATUS Keywait(CHAR16 *String)
{
  EFI_STATUS Status;
  EFI_INPUT_KEY Key;
  Print(String);

  Status = ST->ConOut->OutputString(ST->ConOut, L"Press any key to continue...");
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  // Clear keystroke buffer
  Status = ST->ConIn->Reset(ST->ConIn, FALSE);
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  // Poll for key
  while ((Status = ST->ConIn->ReadKeyStroke(ST->ConIn, &Key)) == EFI_NOT_READY);

  // Clear keystroke buffer (this is just a pause)
  Status = ST->ConIn->Reset(ST->ConIn, FALSE);
  if (EFI_ERROR(Status))
  {
    return Status;
  }

  Print(L"\r\n");

  return Status;
}
