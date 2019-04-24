//==================================================================================================================================
//  Simple UEFI Bootloader: Main Header
//==================================================================================================================================
//
// Version 2.1
//
// Author:
//  KNNSpeed
//
// Source Code:
//  https://github.com/KNNSpeed/Simple-UEFI-Bootloader
//
// This file provides inclusions, #define switches, structure definitions, and function prototypes for the bootloader.
// See Bootloader.c for further details about this program.
//

#ifndef _Bootloader_H
#define _Bootloader_H

#include <efi.h>
#include <efilib.h>

// See the LICENSE file for information about the licenses covering elf.h, loader.h, and fat.h.
#include <pe.h>
#include "elf.h"
#include "loader.h"
#include "fat.h"
#include "dos.h"

#define MAJOR_VER 2
#define MINOR_VER 1

//==================================================================================================================================
// Useful Debugging Code
//==================================================================================================================================
//
// Enable useful debugging prints and convenient key-awaiting pauses
//
// NOTE: Due to little endianness, all printed data at dereferenced pointers is in LITTLE ENDIAN, so each byte (0xXX) is read
// left to right while the byte order is reversed (right to left)!!
//
// Debug binary has this uncommented, release has it commented
//#define ENABLE_DEBUG // Master debug enable switch

// Debug Lite only has the below flag enabled (for main debug binary, leave this commented out and only use the above definition)
//#define FINAL_LOADER_DEBUG_ENABLED

#ifdef ENABLE_DEBUG
    #define SHOW_KERNEL_METADATA
    #define DISABLE_UEFI_WATCHDOG_TIMER
    #define MAIN_DEBUG_ENABLED
    #define GOP_DEBUG_ENABLED
    #define GOP_NAMING_DEBUG_ENABLED
    #define LOADER_DEBUG_ENABLED
    #define PE_LOADER_DEBUG_ENABLED
    #define DOS_LOADER_DEBUG_ENABLED
    #define ELF_LOADER_DEBUG_ENABLED
    #define MACH_LOADER_DEBUG_ENABLED
    #define FINAL_LOADER_DEBUG_ENABLED

    #define MEMMAP_PRINT_ENABLED
    #define MEMORY_CHECK_INFO
//    #define MEMORY_DEBUG_ENABLED // Potential massive performance hit when enabling this and searching for free RAM page-by-page (it prints them all out)
#endif

//==================================================================================================================================
// Memory Allocation Debugging
//==================================================================================================================================
//
// The only reason to touch these #defines is if you are trying to debug this program.
//

// Some systems need the page-by-page search, so it's best to leave this commented out
//#define BY_PAGE_SEARCH_DISABLED

// Leave this alone: It exists in the event it's needed for some really screwy systems or for aid with debugging AllocatePages
// It's the "buggy firmware workaround."

#define MEMORY_CHECK_DISABLED

// Enabling debug mode will enable the memory check automatically
#ifdef ENABLE_DEBUG
#undef MEMORY_CHECK_DISABLED
#endif

//==================================================================================================================================
// Text File UCS-2 Definitions
//==================================================================================================================================
//
// LE - Little endian
// BE - Big endian
//

#define UTF8_BOM_LE 0xBFBBEF
#define UTF8_BOM_BE 0xEFBBBF

#define UTF16_BOM_LE 0xFEFF
#define UTF16_BOM_BE 0xFFFE

//==================================================================================================================================
// Loader Structures
//==================================================================================================================================
//
// These get passed to the kernel file. The entry point function in the kernel should implement something like this: kernel_main(LOADER_PARAMS * LP).
//
// The GPU_CONFIG structure is written this way so that kernel files can do something like this:
//
// LP->GPU_Configs->GPU_MODE.FrameBufferBase
// LP->GPU_Configs->GPU_MODE.Info->HorizontalResolution
// where GPU_MODE = GPUArray[0,1,2...NumberOfFrameBuffers]
//
// #define GTX1080 LP->GPU_CONFIG->GPUArray[0]
// GTX1080.FrameBufferBase
// GTX1080.Info->HorizontalResolution
//
// (It's just an idea.)
//

typedef struct {
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE  *GPUArray;             // This array contains the EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE structures for each available framebuffer
  UINT64                              NumberOfFrameBuffers; // The number of pointers in the array (== the number of available framebuffers)
} GPU_CONFIG;

typedef struct {
  UINT16                  Bootloader_MajorVersion;        // The major version of the bootloader
  UINT16                  Bootloader_MinorVersion;        // The minor version of the bootloader

  UINT32                  Memory_Map_Descriptor_Version;  // The memory descriptor version
  UINTN                   Memory_Map_Descriptor_Size;     // The size of an individual memory descriptor
  EFI_MEMORY_DESCRIPTOR  *Memory_Map;                     // The system memory map as an array of EFI_MEMORY_DESCRIPTOR structs
  UINTN                   Memory_Map_Size;                // The total size of the system memory map

  EFI_PHYSICAL_ADDRESS    Kernel_BaseAddress;             // The base memory address of the loaded kernel file
  UINTN                   Kernel_Pages;                   // The number of pages (1 page == 4096 bytes) allocated for the kernel file

  CHAR16                 *ESP_Root_Device_Path;           // A UTF-16 string containing the drive root of the EFI System Partition as converted from UEFI device path format
  UINT64                  ESP_Root_Size;                  // The size (in bytes) of the above ESP root string
  CHAR16                 *Kernel_Path;                    // A UTF-16 string containing the kernel's file path relative to the EFI System Partition root (it's the first line of Kernel64.txt)
  UINT64                  Kernel_Path_Size;               // The size (in bytes) of the above kernel file path
  CHAR16                 *Kernel_Options;                 // A UTF-16 string containing various load options (it's the second line of Kernel64.txt)
  UINT64                  Kernel_Options_Size;            // The size (in bytes) of the above load options string

  EFI_RUNTIME_SERVICES   *RTServices;                     // UEFI Runtime Services
  GPU_CONFIG             *GPU_Configs;                    // Information about available graphics output devices; see above GPU_CONFIG struct for details
  EFI_FILE_INFO          *FileMeta;                       // Kernel file metadata
  void                   *RSDP;                           // A pointer to the RSDP ACPI table
} LOADER_PARAMS;

//==================================================================================================================================
// Function Prototypes
//==================================================================================================================================
//
// The function prototypes for the functions used in the bootloader.
//

EFI_STATUS Keywait(CHAR16 *String);
UINT8 compare(const void* firstitem, const void* seconditem, UINT64 comparelength);

EFI_STATUS InitUEFI_GOP(EFI_HANDLE ImageHandle, GPU_CONFIG * Graphics);
EFI_STATUS GoTime(EFI_HANDLE ImageHandle, GPU_CONFIG * Graphics, void * RSDPTable);

UINT8 VerifyZeroMem(UINT64 NumBytes, UINT64 BaseAddr);
EFI_PHYSICAL_ADDRESS ActuallyFreeAddress(UINT64 pages, EFI_PHYSICAL_ADDRESS OldAddress);
EFI_PHYSICAL_ADDRESS ActuallyFreeAddressByPage(UINT64 pages, EFI_PHYSICAL_ADDRESS OldAddress);

VOID print_memmap(void);

#ifdef GOP_NAMING_DEBUG_ENABLED
EFI_STATUS WhatProtocols(EFI_HANDLE * HandleArray, UINTN NumHandlesInHandleArray);
#endif

#endif
