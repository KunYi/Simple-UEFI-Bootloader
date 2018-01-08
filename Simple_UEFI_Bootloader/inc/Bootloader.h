//==================================================================================================================================
//  Simple UEFI Bootloader: Main Header
//==================================================================================================================================
//
// Version 1.1
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

//==================================================================================================================================
// Useful Debugging Code
//==================================================================================================================================
//
// Enable useful debugging prints and convenient key-awaiting pauses
//
//NOTE: Due to little endianness of x86-64, all printed data at dereferenced pointers is in LITTLE ENDIAN, so each byte (0xXX) is read
// left to right while the byte order is reversed (right to left)!!
//
// Debug binary has this uncommented, release has it commented
//#define ENABLE_DEBUG // Master debug enable switch

#ifdef ENABLE_DEBUG
    #define SHOW_KERNEL_METADATA
    #define DISABLE_UEFI_WATCHDOG_TIMER
    #define MAIN_DEBUG_ENABLED
    #define GOP_DEBUG_ENABLED
//    #define GOP_NAMING_DEBUG_ENABLED // Unimplemented due to unimplemented naming section
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

// Leave this commented out: AllocateAnyPages seems to enjoy giving non-zero "free" memory addresses, and some
// systems really need this workaround because they put important stuff at the returned address for some reason.
// It's the "buggy firmware workaround."

//#define MEMORY_CHECK_DISABLED

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
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE  *GPUArray; // This array contains the EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE structures for each available framebuffer
  UINT64                              NumberOfFrameBuffers; // The number of pointers in the array (== the number of available framebuffers)
} GPU_CONFIG;

typedef struct {
  EFI_MEMORY_DESCRIPTOR  *Memory_Map;
  EFI_RUNTIME_SERVICES   *RTServices;
  GPU_CONFIG             *GPU_Configs;
  EFI_FILE_INFO          *FileMeta;
  void                   *RSDP;
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

#endif
