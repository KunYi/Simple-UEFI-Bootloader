//==================================================================================================================================
//  Simple UEFI Bootloader: Kernel Loader and Entry Point Jump
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
// This file contains the multiplatform kernel file loader and bootstrapper.
//

#include "Bootloader.h"

//==================================================================================================================================
//  GoTime: Kernel Loader
//==================================================================================================================================
//
// Load Kernel64 (64-bit PE32+, ELF, or Mach-O), exit boot services, and jump to the entry point of Kernel64.
//

EFI_STATUS GoTime(EFI_HANDLE ImageHandle, GPU_CONFIG * Graphics, void *RSDPTable)
{
#ifdef GOP_DEBUG_ENABLED
  // Integrity check
  for(UINT64 k = 0; k < Graphics->NumberOfFrameBuffers; k++)
  {
    Print(L"GPU Mode: %u of %u\r\n", Graphics->GPUArray[k].Mode, Graphics->GPUArray[k].MaxMode - 1);
    Print(L"GPU FB: 0x%016llx\r\n", Graphics->GPUArray[k].FrameBufferBase);
    Print(L"GPU FB Size: 0x%016llx\r\n", Graphics->GPUArray[k].FrameBufferSize);
    Print(L"GPU SizeOfInfo: %u Bytes\r\n", Graphics->GPUArray[k].SizeOfInfo);
    Print(L"GPU Info Ver: 0x%x\r\n", Graphics->GPUArray[k].Info->Version);
    Print(L"GPU Info Res: %ux%u\r\n", Graphics->GPUArray[k].Info->HorizontalResolution, Graphics->GPUArray[k].Info->VerticalResolution);
    Print(L"GPU Info PxFormat: 0x%x\r\n", Graphics->GPUArray[k].Info->PixelFormat);
    Print(L"GPU Info PxInfo (R,G,B,Rsvd Masks): 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n", Graphics->GPUArray[k].Info->PixelInformation.RedMask, Graphics->GPUArray[k].Info->PixelInformation.GreenMask, Graphics->GPUArray[k].Info->PixelInformation.BlueMask, Graphics->GPUArray[k].Info->PixelInformation.ReservedMask);
    Print(L"GPU Info PxPerScanLine: %u\r\n", Graphics->GPUArray[k].Info->PixelsPerScanLine);
    Keywait(L"\0");
  }
#endif

#ifdef LOADER_DEBUG_ENABLED
  Print(L"GO GO GO!!!\r\n");
#endif

  EFI_STATUS GoTimeStatus;

  // Load file called Kernel64 from this drive's root

	EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  // Reserve memory for LoadedImage
/*
  GoTimeStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_LOADED_IMAGE_PROTOCOL), (void**)&LoadedImage);
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
  // Reserve memory for filesystem structure
/*
  GoTimeStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL), (void**)&FileSystem);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"FileSystem AllocatePool error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }
*/
  // Parent device of BOOTX64.EFI (the ImageHandle originally passed in is this very file)
  // Loadedimage is an EFI_LOADED_IMAGE_PROTOCOL pointer that points to BOOTX64.EFI
  GoTimeStatus = ST->BootServices->OpenProtocol(LoadedImage->DeviceHandle, &FileSystemProtocol, (void**)&FileSystem, ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"FileSystem OpenProtocol error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }

  EFI_FILE *CurrentDriveRoot;
/*
  GoTimeStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_FILE_PROTOCOL), (void**)&CurrentDriveRoot);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"CurrentDriveRoot AllocatePool error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }
*/
  GoTimeStatus = FileSystem->OpenVolume(FileSystem, &CurrentDriveRoot);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"OpenVolume error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }

  EFI_FILE *KernelFile;
/*
  GoTimeStatus = ST->BootServices->AllocatePool(EfiBootServicesData, sizeof(EFI_FILE_PROTOCOL), (void**)&KernelFile);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"KernelFile AllocatePool error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }
*/
  // Open the kernel file from current drive root and point to it with KernelFile
	GoTimeStatus = CurrentDriveRoot->Open(CurrentDriveRoot, &KernelFile, L"Kernel64", EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY);
	if (EFI_ERROR(GoTimeStatus))
  {
		ST->ConOut->OutputString(ST->ConOut, L"Kernel64 file is missing\r\n");
		return GoTimeStatus;
	}

#ifdef LOADER_DEBUG_ENABLED
  Keywait(L"Kernel file opened.\r\n");
#endif

  // Get address of start of file
//  UINT64 FileStartPosition;
//  KernelFile->GetPosition(KernelFile, &FileStartPosition);
//  Keywait(L"Kernel file start position acquired.\r\n");

  // Default ImageBase for 64-bit PE DLLs
  EFI_PHYSICAL_ADDRESS Header_memory = 0x400000;

  UINTN FileInfoSize;

  GoTimeStatus = KernelFile->GetInfo(KernelFile, &gEfiFileInfoGuid, &FileInfoSize, NULL);
  // GetInfo will intentionally error out and provide the correct fileinfosize value

#ifdef LOADER_DEBUG_ENABLED
  Print(L"FileInfoSize: %llu Bytes\r\n", FileInfoSize);
#endif

  EFI_FILE_INFO *FileInfo;
  GoTimeStatus = ST->BootServices->AllocatePool(EfiLoaderData, FileInfoSize, (void**)&FileInfo); // Reserve memory for file info/attributes and such, to prevent it from getting run over
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"FileInfo AllocatePool error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }

  // Actually get the metadata
  GoTimeStatus = KernelFile->GetInfo(KernelFile, &gEfiFileInfoGuid, &FileInfoSize, FileInfo);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"GetInfo error. 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }

#ifdef SHOW_KERNEL_METADATA
  // Show metadata
  Print(L"FileName: %s\r\n", FileInfo->FileName);
  Print(L"Size: %llu\r\n", FileInfo->Size);
  Print(L"FileSize: %llu\r\n", FileInfo->FileSize);
  Print(L"PhysicalSize: %llu\r\n", FileInfo->PhysicalSize);
  Print(L"Attribute: %llx\r\n", FileInfo->Attribute);
/*
  NOTE: Attributes:

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
#endif

#ifdef LOADER_DEBUG_ENABLED
  Keywait(L"GetInfo memory allocated and populated.\r\n");
#endif

  // Read file header
//  UINTN size = 0x40; // Size of DOS header
  UINTN size = sizeof(IMAGE_DOS_HEADER);
  IMAGE_DOS_HEADER DOSheader;

// Don't need to allocate memory for non-pointer DOS headers
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

#ifdef LOADER_DEBUG_ENABLED
  Keywait(L"DOS Header read from file.\r\n");
#endif

  // For the entry point jump, we need to know if the file uses ms_abi (is a PE image) or sysv_abi (*NIX image) calling convention
  UINT8 KernelisPE = 0;

  // Check header
  if(DOSheader.e_magic == IMAGE_DOS_SIGNATURE) // MZ
  {
    //----------------------------------------------------------------------------------------------------------------------------------
    //  64-Bit PE32+ Loader
    //----------------------------------------------------------------------------------------------------------------------------------

#ifdef LOADER_DEBUG_ENABLED
    Keywait(L"DOS header passed.\r\n");
    Print(L"e_lfanew: 0x%x\r\n", DOSheader.e_lfanew);
//    Print(L"FileStart: 0x%llx\r\n", &FileStartPosition);
//    Print(L"Corrected filestart: 0x%llx\r\n", &FileStartPosition + (UINT64)DOSheader.e_lfanew);
#endif

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

// Don't need to allocate memory for non-pointer PE headers
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

#ifdef LOADER_DEBUG_ENABLED
    Print(L"PE Header Signature: 0x%x\r\n", PEHeader.Signature);
#endif

    //NOTE: SetPosition is RELATIVE TO THE FILE. Phoenix wiki is unclear:
    // the position is not "absolute" (implying absolute memory location with
    // respect to system memory address 0x0), it's "absolute relative to the
    // file start"
    if(PEHeader.Signature == IMAGE_NT_SIGNATURE)
    {
      // PE

#ifdef LOADER_DEBUG_ENABLED
      Keywait(L"PE header passed.\r\n");
#endif

      if(PEHeader.FileHeader.Machine == IMAGE_FILE_MACHINE_X64 && PEHeader.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) // PE Headers have Signature, FileHeader, and OptionalHeader
      {
        // PE32+

#ifdef LOADER_DEBUG_ENABLED
        Keywait(L"PE32+ header passed.\r\n");
#endif

        if (PEHeader.OptionalHeader.Subsystem != IMAGE_SUBSYSTEM_EFI_APPLICATION) // Was it compiled with -Wl,--subsystem,10 (MINGW-W64 GCC)?
        {
          GoTimeStatus = EFI_INVALID_PARAMETER;
          Print(L"Not a UEFI PE32+ application...\r\n");
          Print(L"Subsystem: %hu\r\n", PEHeader.OptionalHeader.Subsystem); // If it's 3, it was compiled as a Windows CUI (command line) program, and instead needs to be linked with the above GCC flag.
          return GoTimeStatus;
        }

        // It's a PE image
        KernelisPE = 1;

#ifdef LOADER_DEBUG_ENABLED
        Keywait(L"UEFI PE32+ header passed.\r\n");
#endif

        UINT64 i; // Iterator
        UINT64 virt_size = 0; // Size of all the data sections combined, which we need to know in order to allocate the right number of pages
        UINT64 Numofsections = (UINT64)PEHeader.FileHeader.NumberOfSections; // Number of sections described at end of PE headers
        size = IMAGE_SIZEOF_SECTION_HEADER*Numofsections; // Size of section header table in file

//        IMAGE_SECTION_HEADER *section_headers_table_pointer = section_headers_table; // Pointer to the first section header is the same as a pointer to the table
#ifdef PE_LOADER_DEBUG_ENABLED
        Print(L"Numofsections: %llu, size: %llu\r\n", Numofsections, size);
//        Print(L"section_headers_table_pointer: 0x%llx\r\n&section_headers_table[0]: 0x%llx\r\n", section_headers_table_pointer, &section_headers_table[0]);
        Keywait(L"\0");
#endif

        IMAGE_SECTION_HEADER * section_headers_table; // This table is an array of section headers

        GoTimeStatus = ST->BootServices->AllocatePool(EfiBootServicesData, size, (void**)&section_headers_table);
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Section headers table AllocatePool error. 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }

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

#ifdef PE_LOADER_DEBUG_ENABLED
          Print(L"current section address: 0x%x, size: 0x%x\r\n", specific_section_header->VirtualAddress, specific_section_header->Misc.VirtualSize);
          Print(L"current section address + size 0x%x\r\n", specific_section_header->VirtualAddress + specific_section_header->Misc.VirtualSize);
#endif

          virt_size = (virt_size > (UINT64)(specific_section_header->VirtualAddress + specific_section_header->Misc.VirtualSize) ? virt_size: (UINT64)(specific_section_header->VirtualAddress + specific_section_header->Misc.VirtualSize));
        }

#ifdef PE_LOADER_DEBUG_ENABLED
        Print(L"virt_size: 0x%llx\r\n", virt_size);
        Keywait(L"Section Headers table passed.\r\n");
#endif

        UINTN Header_size = (UINTN)PEHeader.OptionalHeader.SizeOfHeaders;

#ifdef PE_LOADER_DEBUG_ENABLED
        Print(L"Total image size: %llu Bytes\r\nHeaders total size: %llu Bytes\r\n", (UINT64)PEHeader.OptionalHeader.SizeOfImage, Header_size);
#endif
        // NOTE: This implies the max file size for a PE executable is 4GB (SizeOfImage is a UINT32).
        // In any event, this has to be loaded from FAT32. You can't have a file larger than 4GB (32-bit max) on FAT32 anyways.
        // A 4GB bootloader, or even kernel, would be insane. You'd need to use a 64-bit linux ELF for those.

        UINT64 pages = EFI_SIZE_TO_PAGES(virt_size); // To get number of pages (typically 4KB per), rounded up

#ifdef PE_LOADER_DEBUG_ENABLED
        Print(L"pages: %llu\r\n", pages);
        Print(L"Expected ImageBase: 0x%llx\r\n", PEHeader.OptionalHeader.ImageBase);

  #ifdef MEMMAP_PRINT_ENABLED
        print_memmap();
        Keywait(L"Done printing MemMap.\r\n");
  #endif
#endif

        EFI_PHYSICAL_ADDRESS AllocatedMemory = PEHeader.OptionalHeader.ImageBase;
//        EFI_PHYSICAL_ADDRESS AllocatedMemory = 0x400000;

#ifdef PE_LOADER_DEBUG_ENABLED
        Print(L"Address of AllocatedMemory: 0x%llx\r\n", &AllocatedMemory);
#endif

        GoTimeStatus = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &AllocatedMemory);
//        GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &AllocatedMemory);
//        AllocatedMemory = 0xFFFFFFFF; // This appears to be what AllocateAnyPages does.
//        GoTimeStatus = BS->AllocatePages(AllocateMaxAddress, EfiLoaderData, pages, &AllocatedMemory);
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Could not allocate pages for PE32+ sections. Error code: 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }

#ifdef PE_LOADER_DEBUG_ENABLED
        Print(L"AllocatedMemory location: 0x%llx\r\n", AllocatedMemory);
#endif

        // Zero the allocated pages
//        ZeroMem(&AllocatedMemory, (pages << EFI_PAGE_SHIFT));

#ifndef MEMORY_CHECK_DISABLED
        // If that memory isn't actually free due to weird firmware behavior...
        // Iterate through the entirety of what was just allocated and check to make sure it's all zeros
        // Start buggy firmware workaround
        if(VerifyZeroMem(pages << EFI_PAGE_SHIFT, AllocatedMemory))
        {

          // From UEFI Specification 2.7, Errata A (http://www.uefi.org/specifications):
          // MemoryType values in the range 0x80000000..0xFFFFFFFF are reserved for use by
          // UEFI OS loaders that are provided by operating system vendors.

  #ifdef MEMORY_CHECK_INFO
          Print(L"Non-zero memory location allocated. Verifying cause...\r\n");
  #endif

          // Compare what's there with the kernel file's first bytes; the system might have been reset and the non-zero
          // memory is what remains of last time. This can be safely overwritten to avoid cluttering up system RAM.

          // Sure hope there aren't any other page-aligned kernel images floating around in memory marked as free
          UINT64 MemCheck = IMAGE_DOS_SIGNATURE; // Good thing we know what to expect!

          if(compare((EFI_PHYSICAL_ADDRESS*)AllocatedMemory, &MemCheck, 2))
          {
            // Do nothing, we're fine
  #ifdef MEMORY_CHECK_INFO
            Print(L"System was reset. No issues.\r\n");
  #endif
          }
          else // Not our remains, proceed with discovery of viable memory address
          {

  #ifdef MEMORY_CHECK_INFO
            Print(L"Searching for actually free memory...\r\nPerhaps the firmware is buggy?\r\n");
  #endif

            // Free the pages (well, return them to the system as they were...)
            GoTimeStatus = BS->FreePages(AllocatedMemory, pages);
            if(EFI_ERROR(GoTimeStatus))
            {
              Print(L"Could not free pages for PE32+ sections. Error code: 0x%llx\r\n", GoTimeStatus);
              return GoTimeStatus;
            }

            // NOTE: CANNOT create an array of all compatible free addresses because the array takes up memory. So does the memory map.
            // This results in a paradox, so we need to scan the memory map every time we need to find a new address...

            // It appears that AllocateAnyPages uses a "MaxAddress" approach. This will go bottom-up instead.
            EFI_PHYSICAL_ADDRESS NewAddress = 0; // Start at zero
            EFI_PHYSICAL_ADDRESS OldAllocatedMemory = AllocatedMemory;

            GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress); // Need to check 0x0
            while(GoTimeStatus != EFI_SUCCESS)
            { // Keep checking free memory addresses until one works

              if(GoTimeStatus == EFI_NOT_FOUND)
              {
                // 0's not a good address (not enough contiguous pages could be found), get another one
                NewAddress = ActuallyFreeAddress(pages, NewAddress);
                // Make sure the new address isn't the known bad one
                if(NewAddress == OldAllocatedMemory)
                {
                  // Get a new address if it is
                  NewAddress = ActuallyFreeAddress(pages, NewAddress);
                }
                else if(NewAddress >= 0x100000000) // Need to stay under 4GB for PE32+
                {
                  NewAddress = -1;
                }
              }
              else if(EFI_ERROR(GoTimeStatus))
              {
                Print(L"Could not get an address for PE32+ pages. Error code: 0x%llx\r\n", GoTimeStatus);
                return GoTimeStatus;
              }

              if(NewAddress == -1)
              {
                // If you get this, you had no memory free anywhere.
                Print(L"No memory marked as EfiConventionalMemory...\r\n");
                return GoTimeStatus;
              }

              // Allocate the new address
              GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress);
              // This loop shouldn't run more than once, but in the event something is at 0x0 we need to
              // leave the loop with an allocated address

            }

            // Got a new address that's been allocated--save it
            AllocatedMemory = NewAddress;

            // Verify it's empty
            while((NewAddress != -1) && VerifyZeroMem(pages << EFI_PAGE_SHIFT, AllocatedMemory)) // Loop this in case the firmware is really screwed
            { // It's not empty :(

              // Sure hope there aren't any other page-aligned kernel images floating around in memory marked as free
              if(compare((EFI_PHYSICAL_ADDRESS*)AllocatedMemory, &MemCheck, 2))
              {
                // Do nothing, we're fine
  #ifdef MEMORY_CHECK_INFO
                Print(L"System appears to have been reset. No issues.\r\n");
  #endif

                break;
              }
              else
              { // Gotta keep looking for a good memory address

  #ifdef MEMORY_DEBUG_ENABLED
                Print(L"Still searching... 0x%llx\r\n", AllocatedMemory);
  #endif

                // It's not actually free...
                GoTimeStatus = BS->FreePages(AllocatedMemory, pages);
                if(EFI_ERROR(GoTimeStatus))
                {
                  Print(L"Could not free pages for PE32+ sections (loop). Error code: 0x%llx\r\n", GoTimeStatus);
                  return GoTimeStatus;
                }

                // Allocate a new address
                GoTimeStatus = EFI_NOT_FOUND;
                while((GoTimeStatus != EFI_SUCCESS) && (NewAddress != -1))
                {
                  if(GoTimeStatus == EFI_NOT_FOUND)
                  {
                    // Get an address (ideally, this should be very rare)
                    NewAddress = ActuallyFreeAddress(pages, NewAddress);
                    // Make sure the new address isn't the known bad one
                    if(NewAddress == OldAllocatedMemory)
                    {
                      // Get a new address if it is
                      NewAddress = ActuallyFreeAddress(pages, NewAddress);
                    }
                    else if(NewAddress >= 0x100000000) // Need to stay under 4GB
                    {
                      NewAddress = -1; // Get out of this loop, do a more thorough check
                      break;
                    }
                    // This loop will run until we get a good address (shouldn't be more than once, if ever)
                  }
                  else if(EFI_ERROR(GoTimeStatus))
                  {
                    // EFI_OUT_OF_RESOURCES means the firmware's just not gonna load anything.
                    Print(L"Could not get an address for PE32+ pages (loop). Error code: 0x%llx\r\n", GoTimeStatus);
                    return GoTimeStatus;
                  }
                  // NOTE: The number of times the message "No more free addresses" pops up
                  // helps indicate which NewAddress assignment hit the end.

                  GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress);
                } // loop

                // It's a new address
                AllocatedMemory = NewAddress;

                // Verify new address is empty (in loop), if not then free it and try again.
              } // else
            } // End VerifyZeroMem while loop

            // Ran out of easy addresses, time for a more thorough check
            // Hopefully no one ever gets here
            if(AllocatedMemory == -1)
            { // NewAddress is also -1

  #ifdef BY_PAGE_SEARCH_DISABLED // Set this to disable ByPage searching
              Print(L"No easy addresses found with enough space and containing only zeros.\r\nConsider enabling page-by-page search.\r\n");
              return GoTimeStatus;
  #endif

  #ifndef BY_PAGE_SEARCH_DISABLED
    #ifdef MEMORY_CHECK_INFO
              Print(L"Performing page-by-page search.\r\nThis might take a while...\r\n");
    #endif

    #ifdef MEMORY_DEBUG_ENABLED
              Keywait(L"About to search page by page\r\n");
    #endif

              NewAddress = 0x80000000 - EFI_PAGE_SIZE; // Start over
              // Allocate the page's address
              GoTimeStatus = EFI_NOT_FOUND;
              while(GoTimeStatus != EFI_SUCCESS)
              {
                if(GoTimeStatus == EFI_NOT_FOUND)
                {
                  // Nope, get another one
                  NewAddress = ActuallyFreeAddressByPage(pages, NewAddress);
                  // Make sure the new address isn't the known bad one
                  if(NewAddress == OldAllocatedMemory)
                  {
                    // Get a new address if it is
                    NewAddress = ActuallyFreeAddressByPage(pages, NewAddress);
                  }
                  else if(NewAddress >= 0x100000000) // Need to stay under 4GB
                  {
                    NewAddress = ActuallyFreeAddress(pages, 0); // Start from the first suitable EfiConventionalMemory address.
                    // This is for BIOS vendors who blanketly set 0x80000000 in an EfiReservedMemoryType section.
                  }
                }
                else if(EFI_ERROR(GoTimeStatus))
                {
                  Print(L"Could not get an address for PE32+ pages by page. Error code: 0x%llx\r\n", GoTimeStatus);
                  return GoTimeStatus;
                }

                if(NewAddress == -1)
                {
                  // If you somehow get this, you really had no memory free anywhere.
                  Print(L"Hmm... How did you get here?\r\n");
                  return GoTimeStatus;
                }

                GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress);
              }

              AllocatedMemory = NewAddress;

              while(VerifyZeroMem(pages << EFI_PAGE_SHIFT, AllocatedMemory))
              {
                // Sure hope there aren't any other page-aligned kernel images floating around in memory marked as free
                if(compare((EFI_PHYSICAL_ADDRESS*)AllocatedMemory, &MemCheck, 2))
                {
                  // Do nothing, we're fine
    #ifdef MEMORY_CHECK_INFO
                  Print(L"System might have been reset. Hopefully no issues.\r\n");
    #endif

                  break;
                }
                else
                {

    #ifdef MEMORY_DEBUG_ENABLED
                Print(L"Still searching by page... 0x%llx\r\n", AllocatedMemory);
    #endif

                  // It's not actually free...
                  GoTimeStatus = BS->FreePages(AllocatedMemory, pages);
                  if(EFI_ERROR(GoTimeStatus))
                  {
                    Print(L"Could not free pages for PE32+ sections by page (loop). Error code: 0x%llx\r\n", GoTimeStatus);
                    return GoTimeStatus;
                  }

                  GoTimeStatus = EFI_NOT_FOUND;
                  while(GoTimeStatus != EFI_SUCCESS)
                  {
                    if(GoTimeStatus == EFI_NOT_FOUND)
                    {
                      // Nope, get another one
                      NewAddress = ActuallyFreeAddressByPage(pages, NewAddress);
                      // Make sure the new address isn't the known bad one
                      if(NewAddress == OldAllocatedMemory)
                      {
                        // Get a new address if it is
                        NewAddress = ActuallyFreeAddressByPage(pages, NewAddress);
                      }
                      else if(NewAddress >= 0x100000000) // Need to stay under 4GB
                      {
                        Print(L"Too much junk below 4GB. Complain to your motherboard vendor.\r\nTry using a 64-bit ELF or MACH-O kernel binary instead of PE32+.\r\n");
                        NewAddress = ActuallyFreeAddress(pages, 0); // Either the BIOS vendor didn't read the spec or you need to use RAM space above 4GB.
                      }
                    }
                    else if(EFI_ERROR(GoTimeStatus))
                    {
                      Print(L"Could not get an address for PE32+ pages by page (loop). Error code: 0x%llx\r\n", GoTimeStatus);
                      return GoTimeStatus;
                    }

                    if(NewAddress == -1)
                    {
                      // Well, darn. Something's up with the system memory. Maybe you have 4GB or less?
                      Print(L"Do you have 4GB or less of RAM? Looks like you need > 4GB for this.\r\nThat also means you'll need to use 64-bit ELF or MACH-O kernels.\r\n");
                      return GoTimeStatus;
                    }

                    GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress);
                  } // loop

                  AllocatedMemory = NewAddress;

                } // else
              } // end ByPage VerifyZeroMem loop
  #endif
            } // End "big guns"

            // Got a good address!
  #ifdef MEMORY_CHECK_INFO
            Print(L"Found!\r\n");
  #endif
          } // End discovery of viable memory address (else)
          // Can move on now
        } // End VerifyZeroMem buggy firmware workaround (outermost if)
#endif

#ifdef PE_LOADER_DEBUG_ENABLED
        Print(L"New AllocatedMemory location: 0x%llx\r\n", AllocatedMemory);
        Keywait(L"Allocate Pages passed.\r\n");

  #ifdef MEMMAP_PRINT_ENABLED
        // Check the address given to AllocatedMemory as listed in the MemMap
        print_memmap();
        Keywait(L"Done printing MemMap.\r\n");
  #endif

        // Map headers
        Print(L"\nLoading Headers:\r\n");
        Print(L"Check:\r\nSectionAddress: 0x%llx\r\nData there: 0x%016llx%016llx (Should be 0)\r\n", AllocatedMemory, *(EFI_PHYSICAL_ADDRESS*)(AllocatedMemory + 8), *(EFI_PHYSICAL_ADDRESS*)AllocatedMemory); // Print the first 128 bits of data at that address to compare
        Keywait(L"\0");
#endif

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

#ifdef PE_LOADER_DEBUG_ENABLED
        // Little endian; print various 16 bytes to make sure data landed in memory properly
        Print(L"\r\nVerify:\r\nSectionAddress: 0x%llx\r\nData there (first 16 bytes): 0x%016llx%016llx\r\n", AllocatedMemory, *(EFI_PHYSICAL_ADDRESS*)(AllocatedMemory + 8), *(EFI_PHYSICAL_ADDRESS*)AllocatedMemory);
        Print(L"Last 16 bytes: 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS *)(AllocatedMemory + Header_size - 8), *(EFI_PHYSICAL_ADDRESS *)(AllocatedMemory + Header_size - 16));
        Print(L"Next 16 bytes: 0x%016llx%016llx (should be 0)\r\n", *(EFI_PHYSICAL_ADDRESS *)(AllocatedMemory + Header_size + 8), *(EFI_PHYSICAL_ADDRESS *)(AllocatedMemory + Header_size));
        Keywait(L"\0");
#endif

        for(i = 0; i < Numofsections; i++) // Load sections into memory
        {
          IMAGE_SECTION_HEADER *specific_section_header = &section_headers_table[i];
          UINTN RawDataSize = (UINTN)specific_section_header->SizeOfRawData;
          EFI_PHYSICAL_ADDRESS SectionAddress = AllocatedMemory + (UINT64)specific_section_header->VirtualAddress;

#ifdef PE_LOADER_DEBUG_ENABLED
          Print(L"\n%llu. current section address: 0x%x, RawDataSize: 0x%llx\r\n", i+1, specific_section_header->VirtualAddress, RawDataSize);
          Print(L"current destination address: 0x%llx, AllocatedMemory base: 0x%llx\r\n", SectionAddress, AllocatedMemory);
          Print(L"PointerToRawData: 0x%llx\r\n", (UINT64)specific_section_header->PointerToRawData);
          Print(L"Check:\r\nSectionAddress: 0x%llx\r\nData there: 0x%016llx%016llx (should be 0)\r\n", SectionAddress, *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + 8), *(EFI_PHYSICAL_ADDRESS *)SectionAddress); // Print the first 128 bits of data at that address to compare
          Print(L"About to load section %llu of %llu...\r\n", i + 1, Numofsections);
          Keywait(L"\0");
#endif

          GoTimeStatus = KernelFile->SetPosition(KernelFile, (UINT64)specific_section_header->PointerToRawData);
          if(EFI_ERROR(GoTimeStatus))
          {
            Print(L"Section SetPosition error. 0x%llx\r\n", GoTimeStatus);
            return GoTimeStatus;
          }

          if(RawDataSize != 0) // Apparently some UEFI implementations can't deal with reading 0 byte sections
          {
            GoTimeStatus = KernelFile->Read(KernelFile, &RawDataSize, (EFI_PHYSICAL_ADDRESS*)SectionAddress); // (void*)SectionAddress
            if(EFI_ERROR(GoTimeStatus))
            {
              Print(L"Section read error. 0x%llx\r\n", GoTimeStatus);
              return GoTimeStatus;
            }
          }

#ifdef PE_LOADER_DEBUG_ENABLED
          Print(L"\r\nVerify:\r\nSectionAddress: 0x%llx\r\nData there (first 16 bytes): 0x%016llx%016llx\r\n", SectionAddress, *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + 8), *(EFI_PHYSICAL_ADDRESS*)SectionAddress); // Print the first 128 bits of data at that address to compare
          Print(L"Last 16 bytes: 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize - 8), *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize - 16));
          Print(L"Next 16 bytes: 0x%016llx%016llx (0 unless last section)\r\n", *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize + 8), *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize));
          // "Next 16 bytes" should be 0 unless last section
#endif
        }

        // Done with section_headers_table
        if(section_headers_table)
        {
          GoTimeStatus = BS->FreePool(section_headers_table);
          if(EFI_ERROR(GoTimeStatus))
          {
            Print(L"Error freeing section_headers_table pool. 0x%llx\r\n", GoTimeStatus);
            Keywait(L"\0");
          }
        }

#ifdef PE_LOADER_DEBUG_ENABLED
        Keywait(L"\nLoad file sections into allocated pages passed.\r\n");
#endif

        // Apply relocation fixes, if necessary
        if((AllocatedMemory != PEHeader.OptionalHeader.ImageBase) && (PEHeader.OptionalHeader.NumberOfRvaAndSizes > IMAGE_DIRECTORY_ENTRY_BASERELOC)) // Need to perform relocations
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

#ifdef PE_LOADER_DEBUG_ENABLED
            Print(L"AllocatedMemory: 0x%llx, ImageBase: 0x%llx, Delta: 0x%llx\r\n", AllocatedMemory, PEHeader.OptionalHeader.ImageBase, delta);
#endif
          }
          else
          {
            delta = PEHeader.OptionalHeader.ImageBase - AllocatedMemory;

#ifdef PE_LOADER_DEBUG_ENABLED
            Print(L"AllocatedMemory: 0x%llx, ImageBase: 0x%llx, Delta: -0x%llx\r\n", AllocatedMemory, PEHeader.OptionalHeader.ImageBase, delta);
#endif
          }

          UINT64 NumRelocationsPerChunk;

          for(;(Relocation_Directory_Base->SizeOfBlock) && (Relocation_Directory_Base < RelocTableEnd);)
          {

#ifdef PE_LOADER_DEBUG_ENABLED
            Print(L"\nSizeOfBlock: %u Bytes\r\n", Relocation_Directory_Base->SizeOfBlock);
            Print(L"Rel_dir_base: 0x%llx, RelTableEnd: 0x%llx\r\n", Relocation_Directory_Base, RelocTableEnd);
#endif

            EFI_PHYSICAL_ADDRESS page = AllocatedMemory + (UINT64)Relocation_Directory_Base->VirtualAddress; //This virtual address is page-specific, and needs to be offset by Header_memory
            UINT16* DataToFix = (UINT16*)((UINT8*)Relocation_Directory_Base + IMAGE_SIZEOF_BASE_RELOCATION); // The base relocation size is 8 bytes (64 bits)
            NumRelocationsPerChunk = (Relocation_Directory_Base->SizeOfBlock - IMAGE_SIZEOF_BASE_RELOCATION)/sizeof(UINT16);

#ifdef PE_LOADER_DEBUG_ENABLED
            Print(L"DataToFix: 0x%hx, Base page: 0x%llx\r\n", DataToFix, page);
            Print(L"NumRelocations in this chunk: %llu\r\n", NumRelocationsPerChunk);
            Keywait(L"About to relocate this chunk...\r\n");
#endif

            for(i = 0; i < NumRelocationsPerChunk; i++)
            {
              if(DataToFix[i] >> EFI_PAGE_SHIFT == IMAGE_REL_BASED_ABSOLUTE) // IMAGE_REL_BASED_ABSOLUTE == 0
              {
                // Nothing, this is a padding area

#ifdef PE_LOADER_DEBUG_ENABLED
                Print(L"%llu of %llu -- Padding Area\r\n", i+1, NumRelocationsPerChunk);
#endif
              }
              else if (DataToFix[i] >> EFI_PAGE_SHIFT == IMAGE_REL_BASED_DIR64) // IMAGE_REL_BASED_DIR64 == 10: Only doing 64-bit offset relocation (that's all that's needed for these PE32+ files), so check uppper 4 bits of each DataToFix entry
              {

#ifdef PE_LOADER_DEBUG_ENABLED
                Print(L"%llu of %llu, DataToFix[%llu]: 0x%hx\r\n", i+1, NumRelocationsPerChunk, i, DataToFix[i]);
#endif

                if(AllocatedMemory > PEHeader.OptionalHeader.ImageBase)
                {

#ifdef PE_LOADER_DEBUG_ENABLED
                  Print(L"Page: 0x%llx, Current Address: 0x%llx, Data there: 0x%llx\r\n", page, (UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK)), *((UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK))));
#endif

                  *((UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK))) += delta;
                  // Lower 12 bits of each DataToFix entry (each entry is of size word) needs to be added to virtual address of the block.
                  // The real location of the virtual address in memory is stored in "page".
                  // This is an "on-the-fly" RAM patch.

#ifdef PE_LOADER_DEBUG_ENABLED
                  Print(L"Delta: 0x%llx, Corrected Data there: 0x%llx\r\n", delta, *((UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK))));
#endif
                }
                else
                {

#ifdef PE_LOADER_DEBUG_ENABLED
                  Print(L"Page: 0x%llx, Current Address: 0x%llx, Data there: 0x%llx\r\n", page, (UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK)), *((UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK))));
#endif

                  *((UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK))) -= delta;

#ifdef PE_LOADER_DEBUG_ENABLED
                  Print(L"Delta: -0x%llx, Corrected Data there: 0x%llx\r\n", delta, *((UINT64*)((UINT8*)page + (DataToFix[i] & EFI_PAGE_MASK))));
#endif
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

#ifdef PE_LOADER_DEBUG_ENABLED
          Print(L"Well that's convenient. No relocation necessary.\r\n");
#endif
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

#ifdef PE_LOADER_DEBUG_ENABLED
        Print(L"Header_memory: 0x%llx, AllocatedMemory: 0x%llx, EntryPoint: 0x%x\r\n", Header_memory, AllocatedMemory, PEHeader.OptionalHeader.AddressOfEntryPoint);
        Print(L"Data at Header_memory (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(Header_memory + 8), *(EFI_PHYSICAL_ADDRESS*)Header_memory);
#endif

      // Loaded! On to memorymap and exitbootservices...
      // NOTE: Executable entry point is now defined in Header_memory's contained address, which is AllocatedMemory + OptionalHeader.AddressOfEntryPoint

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
      //----------------------------------------------------------------------------------------------------------------------------------
      //  MZ Loader
      //----------------------------------------------------------------------------------------------------------------------------------

      Keywait(L"Seems like a 16-bit MS-DOS executable to me...\r\n");

      // :)
      Keywait(L"Well, if you insist...\r\n");

      size = (UINT64)(512*DOSheader.e_cp + DOSheader.e_cblp - 16*DOSheader.e_cparhdr); // Bytes of MZ load module
      UINT64 pages = EFI_SIZE_TO_PAGES(size);

#ifdef DOS_LOADER_DEBUG_ENABLED
      Print(L"e_cp: %hu, e_cblp: %hu, e_cparhdr: %hu\r\n", DOSheader.e_cp, DOSheader.e_cblp, DOSheader.e_cparhdr);
      Print(L"file size: %llu, load module size: %llu, pages: %llu\r\n", size + 16*(UINT64)DOSheader.e_cparhdr, size, pages);
#endif

      EFI_PHYSICAL_ADDRESS DOSMem = 0x100;
      GoTimeStatus = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &DOSMem);
      if(EFI_ERROR(GoTimeStatus))
      {
        Print(L"Could not allocate pages for MZ load module. Error code: 0x%llx\r\n", GoTimeStatus);
        return GoTimeStatus;
      }

      // Zero the allocated pages
//      ZeroMem(&DOSMem, (pages << EFI_PAGE_SHIFT));

#ifdef DOS_LOADER_DEBUG_ENABLED
      Print(L"DOSMem location: 0x%llx\r\n", DOSMem);
      Keywait(L"Allocate Pages passed.\r\n");
#endif

      GoTimeStatus = KernelFile->SetPosition(KernelFile, (UINT64)DOSheader.e_cparhdr*16); // Load module is right after the header
      if(EFI_ERROR(GoTimeStatus))
      {
        Print(L"Reset SetPosition error (MZ). 0x%llx\r\n", GoTimeStatus);
        return GoTimeStatus;
      }

#ifdef DOS_LOADER_DEBUG_ENABLED
      Print(L"current destination address: 0x%llx, DOSMem base: 0x%llx, size: 0x%llx\r\n", DOSMem, DOSMem, size);
      Print(L"Check:\r\nDOSMem: 0x%llx\r\nData there: 0x%016llx%016llx (should be 0)\r\n", DOSMem, *(EFI_PHYSICAL_ADDRESS*)(DOSMem + 8), *(EFI_PHYSICAL_ADDRESS *)DOSMem); // Print the first 128 bits of data at that address to compare
      Keywait(L"\0");
#endif

      GoTimeStatus = KernelFile->Read(KernelFile, &size, (EFI_PHYSICAL_ADDRESS*)DOSMem); // Read load module into DOSMem
      if(EFI_ERROR(GoTimeStatus))
      {
        Print(L"Load module read error (MZ). 0x%llx\r\n", GoTimeStatus);
        return GoTimeStatus;
      }

#ifdef DOS_LOADER_DEBUG_ENABLED
      Print(L"\r\nVerify:\r\nDOSMem: 0x%llx\r\nData there (first 16 bytes): 0x%016llx%016llx\r\n", DOSMem, *(EFI_PHYSICAL_ADDRESS*)(DOSMem + 8), *(EFI_PHYSICAL_ADDRESS*)DOSMem); // Print the first 128 bits of data at that address to compare
      Print(L"Last 16 bytes: 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(DOSMem + size - 8), *(EFI_PHYSICAL_ADDRESS*)(DOSMem + size - 16));
      Print(L"Next 16 bytes: 0x%016llx%016llx (0 unless last section)\r\n", *(EFI_PHYSICAL_ADDRESS*)(DOSMem + size + 8), *(EFI_PHYSICAL_ADDRESS*)(DOSMem + size));
      Keywait(L"\0");
#endif

      // mov relocated e_sp to %rsp

      // Normally, entry point is in e_ip...
      Header_memory = DOSMem + (UINT64)DOSheader.e_ip*16;

#ifdef DOS_LOADER_DEBUG_ENABLED
      Print(L"\r\nHeader_memory: 0x%llx, DOSMem: 0x%llx, EntryPoint: 0x%llx\r\n", Header_memory, DOSMem, (UINT64)DOSheader.e_ip*16);
      Print(L"Data at Header_memory (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(Header_memory + 8), *(EFI_PHYSICAL_ADDRESS*)Header_memory);
      Keywait(L"\0");
#endif
      // Loaded! On to memorymap and exitbootservices...

      // No idea what will happen if these three lines get commented out and the program tries to run. It might actually work if a 64-bit program got packed in there with a correctly-formatted header.
      GoTimeStatus = DOS_EXECUTABLE;
      Print(L"\r\nError Code: 0x%llx\r\nThis program cannot be run in UEFI mode.\r\n:P\r\n", GoTimeStatus);
      return GoTimeStatus;
    }
  }
  else // Check if it's an ELF, if it's not a PE32+
  {
    //----------------------------------------------------------------------------------------------------------------------------------
    //  64-Bit ELF Loader
    //----------------------------------------------------------------------------------------------------------------------------------

    // Slightly less terrible way of doing this; just a placeholder.
    GoTimeStatus = KernelFile->SetPosition(KernelFile, 0);
    if(EFI_ERROR(GoTimeStatus))
    {
      Print(L"Reset SetPosition error (ELF). 0x%llx\r\n", GoTimeStatus);
      return GoTimeStatus;
    }

    Elf64_Ehdr ELF64header;
    size = sizeof(ELF64header); // This works because it's not a pointer

    GoTimeStatus = KernelFile->Read(KernelFile, &size, &ELF64header);
    if(EFI_ERROR(GoTimeStatus))
    {
      Print(L"Header read error (ELF). 0x%llx\r\n", GoTimeStatus);
      return GoTimeStatus;
    }

#ifdef LOADER_DEBUG_ENABLED
    Keywait(L"ELF header read from file.\r\n");
#endif

    if(compare(&ELF64header.e_ident[EI_MAG0], ELFMAG, SELFMAG)) // Check for \177ELF (hex: \xfELF)
    {
      // ELF!

#ifdef LOADER_DEBUG_ENABLED
      Keywait(L"ELF header passed.\r\n");
#endif

      // Check if 64-bit
      if(ELF64header.e_ident[EI_CLASS] == ELFCLASS64 && ELF64header.e_machine == EM_X86_64)
      {

#ifdef LOADER_DEBUG_ENABLED
        Keywait(L"ELF64 header passed.\r\n");
#endif

        if (ELF64header.e_type != ET_DYN)
        {
          GoTimeStatus = EFI_INVALID_PARAMETER;
          Print(L"Not a position-independent, executable ELF64 application...\r\n");
          Print(L"e_type: 0x%hx\r\n", ELF64header.e_type); // If it's 3, we're good and won't see this. Hopefully the image was compiled with -fpie and linked with -static-pie
          return GoTimeStatus;
        }

#ifdef LOADER_DEBUG_ENABLED
        Keywait(L"Executable ELF64 header passed.\r\n");
#endif

        UINT64 i; // Iterator
        UINT64 virt_size = 0; // Virtual address max
        UINT64 virt_min = -1; // Minimum virtual address for page number calculation, -1 wraps around to max 64-bit number
        UINT64 Numofprogheaders = (UINT64)ELF64header.e_phnum;
        size = Numofprogheaders*(UINT64)ELF64header.e_phentsize; // Size of all program headers combined

        Elf64_Phdr * program_headers_table;

        GoTimeStatus = ST->BootServices->AllocatePool(EfiBootServicesData, size, (void**)&program_headers_table);
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Program headers table AllocatePool error. 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }

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

#ifdef ELF_LOADER_DEBUG_ENABLED
            Print(L"current program address: 0x%x, size: 0x%x\r\n", specific_program_header->p_vaddr, specific_program_header->p_memsz);
            Print(L"current program address + size 0x%x\r\n", specific_program_header->p_vaddr + specific_program_header->p_memsz);
#endif

            virt_size = (virt_size > (specific_program_header->p_vaddr + specific_program_header->p_memsz) ? virt_size: (specific_program_header->p_vaddr + specific_program_header->p_memsz));
            virt_min = (virt_min < (specific_program_header->p_vaddr) ? virt_min: (specific_program_header->p_vaddr));
          }
        }

#ifdef ELF_LOADER_DEBUG_ENABLED
        Print(L"virt_size: 0x%llx, virt_min: 0x%llx, difference: 0x%llx\r\n", virt_size, virt_min, virt_size - virt_min);
        Keywait(L"Program Headers table passed.\r\n");
#endif

        // Virt_min is technically also the base address of the loadable segments
        UINT64 pages = EFI_SIZE_TO_PAGES(virt_size - virt_min); //To get number of pages (typically 4KB per), rounded up

#ifdef ELF_LOADER_DEBUG_ENABLED
        Print(L"pages: %llu\r\n", pages);
#endif
        EFI_PHYSICAL_ADDRESS AllocatedMemory = 0x400000; // Default for ELF

#ifdef ELF_LOADER_DEBUG_ENABLED
        Print(L"Address of AllocatedMemory: 0x%llx\r\n", &AllocatedMemory);
#endif

        GoTimeStatus = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &AllocatedMemory);
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Could not allocate pages for ELF program segments. Error code: 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }

        // Zero the allocated pages
//        ZeroMem(&AllocatedMemory, (pages << EFI_PAGE_SHIFT));

#ifndef MEMORY_CHECK_DISABLED
        // If that memory isn't actually free due to weird firmware behavior...
        // Iterate through the entirety of what was just allocated and check to make sure it's all zeros
        // Start buggy firmware workaround
        if(VerifyZeroMem(pages << EFI_PAGE_SHIFT, AllocatedMemory))
        {

          // From UEFI Specification 2.7, Errata A (http://www.uefi.org/specifications):
          // MemoryType values in the range 0x80000000..0xFFFFFFFF are reserved for use by
          // UEFI OS loaders that are provided by operating system vendors.
  #ifdef MEMORY_CHECK_INFO
          Print(L"Non-zero memory location allocated. Verifying cause...\r\n");
  #endif
          // Compare what's there with the kernel file's first bytes; the system might have been reset and the non-zero
          // memory is what remains of last time. This can be safely overwritten to avoid cluttering up system RAM.

          // Sure hope there aren't any other page-aligned kernel images floating around in memory marked as free
          // Good thing we know what to expect!

          if(compare((EFI_PHYSICAL_ADDRESS*)AllocatedMemory, ELFMAG, SELFMAG))
          {
            // Do nothing, we're fine
  #ifdef MEMORY_CHECK_INFO
            Print(L"System was reset. No issues.\r\n");
  #endif
          }
          else // Not our remains, proceed with discovery of viable memory address
          {

  #ifdef MEMORY_CHECK_INFO
            Print(L"Searching for actually free memory...\r\nPerhaps the firmware is buggy?\r\n");
  #endif
            // Free the pages (well, return them to the system as they were...)
            GoTimeStatus = BS->FreePages(AllocatedMemory, pages);
            if(EFI_ERROR(GoTimeStatus))
            {
              Print(L"Could not free pages for ELF sections. Error code: 0x%llx\r\n", GoTimeStatus);
              return GoTimeStatus;
            }

            // NOTE: CANNOT create an array of all compatible free addresses because the array takes up memory. So does the memory map.
            // This results in a paradox, so we need to scan the memory map every time we need to find a new address...

            // It appears that AllocateAnyPages uses a "MaxAddress" approach. This will go bottom-up instead.
            EFI_PHYSICAL_ADDRESS NewAddress = 0; // Start at zero
            EFI_PHYSICAL_ADDRESS OldAllocatedMemory = AllocatedMemory;

            GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress); // Need to check 0x0
            while(GoTimeStatus != EFI_SUCCESS)
            { // Keep checking free memory addresses until one works

              if(GoTimeStatus == EFI_NOT_FOUND)
              {
                // 0's not a good address (not enough contiguous pages could be found), get another one
                NewAddress = ActuallyFreeAddress(pages, NewAddress);
                // Make sure the new address isn't the known bad one
                if(NewAddress == OldAllocatedMemory)
                {
                  // Get a new address if it is
                  NewAddress = ActuallyFreeAddress(pages, NewAddress);
                }
                // Address can be > 4GB
              }
              else if(EFI_ERROR(GoTimeStatus))
              {
                Print(L"Could not get an address for ELF pages. Error code: 0x%llx\r\n", GoTimeStatus);
                return GoTimeStatus;
              }

              if(NewAddress == -1)
              {
                // If you get this, you had no memory free anywhere.
                Print(L"No memory marked as EfiConventionalMemory...\r\n");
                return GoTimeStatus;
              }

              // Allocate the new address
              GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress);
              // This loop shouldn't run more than once, but in the event something is at 0x0 we need to
              // leave the loop with an allocated address

            }

            // Got a new address that's been allocated--save it
            AllocatedMemory = NewAddress;

            // Verify it's empty
            while((NewAddress != -1) && VerifyZeroMem(pages << EFI_PAGE_SHIFT, AllocatedMemory)) // Loop this in case the firmware is really screwed
            { // It's not empty :(

              // Sure hope there aren't any other page-aligned kernel images floating around in memory marked as free
              if(compare((EFI_PHYSICAL_ADDRESS*)AllocatedMemory, ELFMAG, SELFMAG))
              {
                // Do nothing, we're fine
  #ifdef MEMORY_CHECK_INFO
                Print(L"System appears to have been reset. No issues.\r\n");
  #endif

                break;
              }
              else
              { // Gotta keep looking for a good memory address

  #ifdef MEMORY_DEBUG_ENABLED
                Print(L"Still searching... 0x%llx\r\n", AllocatedMemory);
  #endif

                // It's not actually free...
                GoTimeStatus = BS->FreePages(AllocatedMemory, pages);
                if(EFI_ERROR(GoTimeStatus))
                {
                  Print(L"Could not free pages for ELF sections (loop). Error code: 0x%llx\r\n", GoTimeStatus);
                  return GoTimeStatus;
                }

                // Allocate a new address
                GoTimeStatus = EFI_NOT_FOUND;
                while((GoTimeStatus != EFI_SUCCESS) && (NewAddress != -1))
                {
                  if(GoTimeStatus == EFI_NOT_FOUND)
                  {
                    // Get an address (ideally, this should be very rare)
                    NewAddress = ActuallyFreeAddress(pages, NewAddress);
                    // Make sure the new address isn't the known bad one
                    if(NewAddress == OldAllocatedMemory)
                    {
                      // Get a new address if it is
                      NewAddress = ActuallyFreeAddress(pages, NewAddress);
                    }
                    // Address can be > 4GB
                    // This loop will run until we get a good address (shouldn't be more than once, if ever)
                  }
                  else if(EFI_ERROR(GoTimeStatus))
                  {
                    // EFI_OUT_OF_RESOURCES means the firmware's just not gonna load anything.
                    Print(L"Could not get an address for ELF pages (loop). Error code: 0x%llx\r\n", GoTimeStatus);
                    return GoTimeStatus;
                  }
                  // NOTE: The number of times the message "No more free addresses" pops up
                  // helps indicate which NewAddress assignment hit the end.

                  GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress);
                } // loop

                // It's a new address
                AllocatedMemory = NewAddress;

                // Verify new address is empty (in loop), if not then free it and try again.
              } // else
            } // End VerifyZeroMem while loop

            // Ran out of easy addresses, time for a more thorough check
            // Hopefully no one ever gets here
            if(AllocatedMemory == -1)
            { // NewAddress is also -1

  #ifdef BY_PAGE_SEARCH_DISABLED // Set this to disable ByPage searching
              Print(L"No easy addresses found with enough space and containing only zeros.\r\nConsider enabling page-by-page search.\r\n");
              return GoTimeStatus;
  #endif

  #ifndef BY_PAGE_SEARCH_DISABLED
    #ifdef MEMORY_CHECK_INFO
              Print(L"Performing page-by-page search.\r\nThis might take a while...\r\n");
    #endif

    #ifdef MEMORY_DEBUG_ENABLED
              Keywait(L"About to search page by page\r\n");
    #endif

              NewAddress = ActuallyFreeAddress(pages, 0); // Start from the first suitable EfiConventionalMemory address.
              // Allocate the page's address
              GoTimeStatus = EFI_NOT_FOUND;
              while(GoTimeStatus != EFI_SUCCESS)
              {
                if(GoTimeStatus == EFI_NOT_FOUND)
                {
                  // Nope, get another one
                  NewAddress = ActuallyFreeAddressByPage(pages, NewAddress);
                  // Make sure the new address isn't the known bad one
                  if(NewAddress == OldAllocatedMemory)
                  {
                    // Get a new address if it is
                    NewAddress = ActuallyFreeAddressByPage(pages, NewAddress);
                  }
                  // Adresses very well might be > 4GB with the filesizes these are allowed to be
                }
                else if(EFI_ERROR(GoTimeStatus))
                {
                  Print(L"Could not get an address for ELF pages by page. Error code: 0x%llx\r\n", GoTimeStatus);
                  return GoTimeStatus;
                }

                if(NewAddress == -1)
                {
                  // If you somehow get this, you really had no memory free anywhere.
                  Print(L"Hmm... How did you get here?\r\n");
                  return GoTimeStatus;
                }

                GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress);
              }

              AllocatedMemory = NewAddress;

              while(VerifyZeroMem(pages << EFI_PAGE_SHIFT, AllocatedMemory))
              {
                // Sure hope there aren't any other page-aligned kernel images floating around in memory marked as free
                if(compare((EFI_PHYSICAL_ADDRESS*)AllocatedMemory, ELFMAG, SELFMAG))
                {
                  // Do nothing, we're fine
    #ifdef MEMORY_CHECK_INFO
                  Print(L"System might have been reset. Hopefully no issues.\r\n");
    #endif

                  break;
                }
                else
                {

    #ifdef MEMORY_DEBUG_ENABLED
                Print(L"Still searching by page... 0x%llx\r\n", AllocatedMemory);
    #endif

                  // It's not actually free...
                  GoTimeStatus = BS->FreePages(AllocatedMemory, pages);
                  if(EFI_ERROR(GoTimeStatus))
                  {
                    Print(L"Could not free pages for ELF sections by page (loop). Error code: 0x%llx\r\n", GoTimeStatus);
                    return GoTimeStatus;
                  }

                  GoTimeStatus = EFI_NOT_FOUND;
                  while(GoTimeStatus != EFI_SUCCESS)
                  {
                    if(GoTimeStatus == EFI_NOT_FOUND)
                    {
                      // Nope, get another one
                      NewAddress = ActuallyFreeAddressByPage(pages, NewAddress);
                      // Make sure the new address isn't the known bad one
                      if(NewAddress == OldAllocatedMemory)
                      {
                        // Get a new address if it is
                        NewAddress = ActuallyFreeAddressByPage(pages, NewAddress);
                      }
                      // Address can be > 4GB
                    }
                    else if(EFI_ERROR(GoTimeStatus))
                    {
                      Print(L"Could not get an address for ELF pages by page (loop). Error code: 0x%llx\r\n", GoTimeStatus);
                      return GoTimeStatus;
                    }

                    if(AllocatedMemory == -1)
                    {
                      // Well, darn. Something's up with the system memory.
                      return GoTimeStatus;
                    }

                    GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress);
                  } // loop

                  AllocatedMemory = NewAddress;

                } // else
              } // end ByPage VerifyZeroMem loop
  #endif
            } // End "big guns"

            // Got a good address!
  #ifdef MEMORY_CHECK_INFO
            Print(L"Found!\r\n");
  #endif
          } // End discovery of viable memory address (else)
          // Can move on now
        } // End VerifyZeroMem buggy firmware workaround (outermost if)
#endif

#ifdef ELF_LOADER_DEBUG_ENABLED
        Print(L"New AllocatedMemory location: 0x%llx\r\n", AllocatedMemory);
        Keywait(L"Allocate Pages passed.\r\n");
#endif

        // No need to copy headers to memory for ELFs, just the program itself
        // Only want to include PT_LOAD segments
        for(i = 0; i < Numofprogheaders; i++) // Load sections into memory
        {
          Elf64_Phdr *specific_program_header = &program_headers_table[i];
          UINTN RawDataSize = specific_program_header->p_filesz; // 64-bit ELFs can have 64-bit file sizes!
          EFI_PHYSICAL_ADDRESS SectionAddress = AllocatedMemory + specific_program_header->p_vaddr; // 64-bit ELFs use 64-bit addressing!

#ifdef ELF_LOADER_DEBUG_ENABLED
          Print(L"\n%llu. current section address: 0x%x, RawDataSize: 0x%llx\r\n", i+1, specific_program_header->p_vaddr, RawDataSize);
#endif

          if(specific_program_header->p_type == PT_LOAD)
          {

#ifdef ELF_LOADER_DEBUG_ENABLED
            Print(L"current destination address: 0x%llx, AllocatedMemory base: 0x%llx\r\n", SectionAddress, AllocatedMemory);
            Print(L"PointerToRawData: 0x%llx\r\n", specific_program_header->p_offset);
            Print(L"Check:\r\nSectionAddress: 0x%llx\r\nData there: 0x%016llx%016llx (should be 0)\r\n", SectionAddress, *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + 8), *(EFI_PHYSICAL_ADDRESS *)SectionAddress); // print the first 128 bits of that address to compare
            Print(L"About to load section %llu of %llu...\r\n", i + 1, Numofprogheaders);
            Keywait(L"\0");
#endif

            GoTimeStatus = KernelFile->SetPosition(KernelFile, specific_program_header->p_offset); // p_offset is a UINT64 relative to the beginning of the file, just like Read() expects!
            if(EFI_ERROR(GoTimeStatus))
            {
              Print(L"Program segment SetPosition error (ELF). 0x%llx\r\n", GoTimeStatus);
              return GoTimeStatus;
            }

            if(RawDataSize != 0) // Apparently some UEFI implementations can't deal with reading 0 byte sections
            {
              GoTimeStatus = KernelFile->Read(KernelFile, &RawDataSize, (EFI_PHYSICAL_ADDRESS*)SectionAddress); // (void*)SectionAddress
              if(EFI_ERROR(GoTimeStatus))
              {
                Print(L"Program segment read error (ELF). 0x%llx\r\n", GoTimeStatus);
                return GoTimeStatus;
              }
            }
#ifdef ELF_LOADER_DEBUG_ENABLED
            Print(L"\r\nVerify:\r\nSectionAddress: 0x%llx\r\nData there (first 16 bytes): 0x%016llx%016llx\r\n", SectionAddress, *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + 8), *(EFI_PHYSICAL_ADDRESS*)SectionAddress); // print the first 128 bits of that address to compare
            Print(L"Last 16 bytes: 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize - 8), *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize - 16));
            Print(L"Next 16 bytes: 0x%016llx%016llx (0 unless last section)\r\n", *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize + 8), *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize));
            // "Next 16 bytes" should be 0 unless last section
#endif
          }
          else
          {

#ifdef ELF_LOADER_DEBUG_ENABLED
            Print(L"Not a PT_LOAD section. Type: 0x%x\r\n", specific_program_header->p_type);
#endif
          }
        }

        // Done with program_headers_table
        if(program_headers_table)
        {
          GoTimeStatus = BS->FreePool(program_headers_table);
          if(EFI_ERROR(GoTimeStatus))
          {
            Print(L"Error freeing program headers table pool. 0x%llx\r\n", GoTimeStatus);
            Keywait(L"\0");
          }
        }

#ifdef ELF_LOADER_DEBUG_ENABLED
        Keywait(L"\nLoad file sections into allocated pages passed.\r\n");
#endif

        // Link kernel with -static-pie and there's no need for relocations beyond the base-relative ones just done. YUS!

        // e_entry should be a 64-bit relative memory address, and gives the kernel's entry point
        Header_memory = AllocatedMemory + ELF64header.e_entry;

#ifdef ELF_LOADER_DEBUG_ENABLED
        Print(L"Header_memory: 0x%llx, AllocatedMemory: 0x%llx, EntryPoint: 0x%x\r\n", Header_memory, AllocatedMemory, ELF64header.e_entry);
        Print(L"Data at Header_memory (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(Header_memory + 8), *(EFI_PHYSICAL_ADDRESS*)Header_memory);
#endif

        // Loaded! On to memorymap and exitbootservices...
        // NOTE: Executable entry point is now defined in Header_memory's contained address, which is AllocatedMemory + ELF64header.e_entry

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
      //----------------------------------------------------------------------------------------------------------------------------------
      //  64-Bit Mach-O Loader
      //----------------------------------------------------------------------------------------------------------------------------------

      // Slightly less terrible way of doing this; just a placeholder.
      GoTimeStatus = KernelFile->SetPosition(KernelFile, 0);
      if(EFI_ERROR(GoTimeStatus))
      {
        Print(L"Reset SetPosition error (Mach). 0x%llx\r\n", GoTimeStatus);
        return GoTimeStatus;
      }

      struct mach_header_64 MACheader;
      size = sizeof(MACheader); // This works because it's not a pointer

      GoTimeStatus = KernelFile->Read(KernelFile, &size, &MACheader);
      if(EFI_ERROR(GoTimeStatus))
      {
        Print(L"Header read error (Mach). 0x%llx\r\n", GoTimeStatus);
        return GoTimeStatus;
      }

#ifdef LOADER_DEBUG_ENABLED
      Keywait(L"Mach header read from file.\r\n");
#endif

      if(MACheader.magic == MH_MAGIC_64 && MACheader.cputype == CPU_TYPE_X86_64)
      {
        // Big endian: 0xfeedfacf && little endian: 0x07000001

#ifdef LOADER_DEBUG_ENABLED
        Keywait(L"Mach64 header passed.\r\n");
#endif

        if (MACheader.filetype != MH_EXECUTE)
        {
          GoTimeStatus = EFI_INVALID_PARAMETER;
          Print(L"Not an executable Mach64 application...\r\n");
          Print(L"filetype: 0x%x\r\n", MACheader.filetype); // If it's 0x5, we're good and won't see this.
          return GoTimeStatus;
        }

#ifdef LOADER_DEBUG_ENABLED
        Keywait(L"Executable Mach64 header passed.\r\n");
#endif

        UINT64 i; // Iterator
        UINT64 virt_size = 0;
        UINT64 virt_min = -1; // Wraps around to max 64-bit number
        UINT64 Numofcommands = (UINT64)MACheader.ncmds;
        //load_command load_commands_table[Numofcommands]; // commands are variably sized.
        size = (UINT64)MACheader.sizeofcmds; // Size of all commands combined. Well, that's convenient!
        UINT64 current_spot = 0;

        CHAR8 * commands_buffer; // *grumble grumble*

        GoTimeStatus = ST->BootServices->AllocatePool(EfiBootServicesData, size, (void**)&commands_buffer);
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Commands buffer AllocatePool error. 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }

// Go to load commands, which is right after the mach_header (NOTE: cursor's already there)
/*
        GoTimeStatus = KernelFile->SetPosition(KernelFile, sizeof(MACheader));
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Error setting file position for mapping (Mach64). 0x%llx\r\n", GoTimeStatus);
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

#ifdef MACH_LOADER_DEBUG_ENABLED
            Print(L"current segment address: 0x%x, size: 0x%x\r\n", specific_segment_command->vmaddr, specific_segment_command->vmsize);
            Print(L"current segment address + size 0x%x\r\n", specific_segment_command->vmaddr + specific_segment_command->vmsize);
#endif

            virt_size = (virt_size > (specific_segment_command->vmaddr + specific_segment_command->vmsize) ? virt_size: (specific_segment_command->vmaddr + specific_segment_command->vmsize));
            virt_min = (virt_min < (specific_segment_command->vmaddr) ? virt_min: (specific_segment_command->vmaddr));
          }
          current_spot += (UINT64)specific_load_command->cmdsize;
        }

#ifdef MACH_LOADER_DEBUG_ENABLED
        Print(L"virt_size: 0x%llx, virt_min: 0x%llx, difference: 0x%llx\r\n", virt_size, virt_min, virt_size - virt_min);
#endif

        if(current_spot != size)
        {
          GoTimeStatus = EFI_INVALID_PARAMETER;
          Print(L"Hmmm... current_spot: %llu != total cmd size: %llu\r\n", current_spot, size);
          return GoTimeStatus;
        }

#ifdef MACH_LOADER_DEBUG_ENABLED
        Print(L"current_spot: %llu == total cmd size: %llu\r\n", current_spot, size);
        Keywait(L"Load commands buffer passed.\r\n");
#endif

        UINT64 pages = EFI_SIZE_TO_PAGES(virt_size - virt_min); // To get number of pages (typically 4KB per), rounded up

#ifdef MACH_LOADER_DEBUG_ENABLED
        Print(L"pages: %llu\r\n", pages);
#endif

        EFI_PHYSICAL_ADDRESS AllocatedMemory = 0x100000000; // Default for non-pagezero Mach-O with a 4GB pagezero

#ifdef MACH_LOADER_DEBUG_ENABLED
        Print(L"Address of AllocatedMemory: 0x%llx\r\n", &AllocatedMemory);
#endif

        GoTimeStatus = BS->AllocatePages(AllocateAnyPages, EfiLoaderData, pages, &AllocatedMemory);
        if(EFI_ERROR(GoTimeStatus))
        {
          Print(L"Could not allocate pages for Mach64 segment sections. Error code: 0x%llx\r\n", GoTimeStatus);
          return GoTimeStatus;
        }

        // Zero the allocated pages
//        ZeroMem(&AllocatedMemory, (pages << EFI_PAGE_SHIFT));

#ifndef MEMORY_CHECK_DISABLED
        // If that memory isn't actually free due to weird firmware behavior...
        // Iterate through the entirety of what was just allocated and check to make sure it's all zeros
        // Start buggy firmware workaround
        if(VerifyZeroMem(pages << EFI_PAGE_SHIFT, AllocatedMemory))
        {

          // From UEFI Specification 2.7, Errata A (http://www.uefi.org/specifications):
          // MemoryType values in the range 0x80000000..0xFFFFFFFF are reserved for use by
          // UEFI OS loaders that are provided by operating system vendors.
  #ifdef MEMORY_CHECK_INFO
          Print(L"Non-zero memory location allocated. Verifying cause...\r\n");
  #endif
          // Compare what's there with the kernel file's first bytes; the system might have been reset and the non-zero
          // memory is what remains of last time. This can be safely overwritten to avoid cluttering up system RAM.

          // Sure hope there aren't any other page-aligned kernel images floating around in memory marked as free
          UINT64 MemCheck = MH_MAGIC_64; // Good thing we know what to expect!

          if(compare((EFI_PHYSICAL_ADDRESS*)AllocatedMemory, &MemCheck, 4))
          {
            // Do nothing, we're fine
  #ifdef MEMORY_CHECK_INFO
            Print(L"System was reset. No issues.\r\n");
  #endif
          }
          else // Not our remains, proceed with discovery of viable memory address
          {

  #ifdef MEMORY_CHECK_INFO
            Print(L"Searching for actually free memory...\r\nPerhaps the firmware is buggy?\r\n");
  #endif
            // Free the pages (well, return them to the system as they were...)
            GoTimeStatus = BS->FreePages(AllocatedMemory, pages);
            if(EFI_ERROR(GoTimeStatus))
            {
              Print(L"Could not free pages for Mach64 sections. Error code: 0x%llx\r\n", GoTimeStatus);
              return GoTimeStatus;
            }

            // NOTE: CANNOT create an array of all compatible free addresses because the array takes up memory. So does the memory map.
            // This results in a paradox, so we need to scan the memory map every time we need to find a new address...

            // It appears that AllocateAnyPages uses a "MaxAddress" approach. This will go bottom-up instead.
            EFI_PHYSICAL_ADDRESS NewAddress = 0; // Start at zero
            EFI_PHYSICAL_ADDRESS OldAllocatedMemory = AllocatedMemory;

            GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress); // Need to check 0x0
            while(GoTimeStatus != EFI_SUCCESS)
            { // Keep checking free memory addresses until one works

              if(GoTimeStatus == EFI_NOT_FOUND)
              {
                // 0's not a good address (not enough contiguous pages could be found), get another one
                NewAddress = ActuallyFreeAddress(pages, NewAddress);
                // Make sure the new address isn't the known bad one
                if(NewAddress == OldAllocatedMemory)
                {
                  // Get a new address if it is
                  NewAddress = ActuallyFreeAddress(pages, NewAddress);
                }
                // Address can be > 4GB
              }
              else if(EFI_ERROR(GoTimeStatus))
              {
                Print(L"Could not get an address for Mach64 pages. Error code: 0x%llx\r\n", GoTimeStatus);
                return GoTimeStatus;
              }

              if(NewAddress == -1)
              {
                // If you get this, you had no memory free anywhere.
                Print(L"No memory marked as EfiConventionalMemory...\r\n");
                return GoTimeStatus;
              }

              // Allocate the new address
              GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress);
              // This loop shouldn't run more than once, but in the event something is at 0x0 we need to
              // leave the loop with an allocated address

            }

            // Got a new address that's been allocated--save it
            AllocatedMemory = NewAddress;

            // Verify it's empty
            while((NewAddress != -1) && VerifyZeroMem(pages << EFI_PAGE_SHIFT, AllocatedMemory)) // Loop this in case the firmware is really screwed
            { // It's not empty :(

              // Sure hope there aren't any other page-aligned kernel images floating around in memory marked as free
              if(compare((EFI_PHYSICAL_ADDRESS*)AllocatedMemory, &MemCheck, 4))
              {
                // Do nothing, we're fine
  #ifdef MEMORY_CHECK_INFO
                Print(L"System appears to have been reset. No issues.\r\n");
  #endif

                break;
              }
              else
              { // Gotta keep looking for a good memory address

  #ifdef MEMORY_DEBUG_ENABLED
                Print(L"Still searching... 0x%llx\r\n", AllocatedMemory);
  #endif

                // It's not actually free...
                GoTimeStatus = BS->FreePages(AllocatedMemory, pages);
                if(EFI_ERROR(GoTimeStatus))
                {
                  Print(L"Could not free pages for Mach64 sections (loop). Error code: 0x%llx\r\n", GoTimeStatus);
                  return GoTimeStatus;
                }

                // Allocate a new address
                GoTimeStatus = EFI_NOT_FOUND;
                while((GoTimeStatus != EFI_SUCCESS) && (NewAddress != -1))
                {
                  if(GoTimeStatus == EFI_NOT_FOUND)
                  {
                    // Get an address (ideally, this should be very rare)
                    NewAddress = ActuallyFreeAddress(pages, NewAddress);
                    // Make sure the new address isn't the known bad one
                    if(NewAddress == OldAllocatedMemory)
                    {
                      // Get a new address if it is
                      NewAddress = ActuallyFreeAddress(pages, NewAddress);
                    }
                    // Address can be > 4GB
                    // This loop will run until we get a good address (shouldn't be more than once, if ever)
                  }
                  else if(EFI_ERROR(GoTimeStatus))
                  {
                    // EFI_OUT_OF_RESOURCES means the firmware's just not gonna load anything.
                    Print(L"Could not get an address for Mach64 pages (loop). Error code: 0x%llx\r\n", GoTimeStatus);
                    return GoTimeStatus;
                  }
                  // NOTE: The number of times the message "No more free addresses" pops up
                  // helps indicate which NewAddress assignment hit the end.

                  GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress);
                } // loop

                // It's a new address
                AllocatedMemory = NewAddress;

                // Verify new address is empty (in loop), if not then free it and try again.
              } // else
            } // End VerifyZeroMem while loop

            // Ran out of easy addresses, time for a more thorough check
            // Hopefully no one ever gets here
            if(AllocatedMemory == -1)
            { // NewAddress is also -1

  #ifdef BY_PAGE_SEARCH_DISABLED // Set this to disable ByPage searching
              Print(L"No easy addresses found with enough space and containing only zeros.\r\nConsider enabling page-by-page search.\r\n");
              return GoTimeStatus;
  #endif

  #ifndef BY_PAGE_SEARCH_DISABLED
    #ifdef MEMORY_CHECK_INFO
              Print(L"Performing page-by-page search.\r\nThis might take a while...\r\n");
    #endif

    #ifdef MEMORY_DEBUG_ENABLED
              Keywait(L"About to search page by page\r\n");
    #endif

              NewAddress = ActuallyFreeAddress(pages, 0); // Start from the first suitable EfiConventionalMemory address.
              // Allocate the page's address
              GoTimeStatus = EFI_NOT_FOUND;
              while(GoTimeStatus != EFI_SUCCESS)
              {
                if(GoTimeStatus == EFI_NOT_FOUND)
                {
                  // Nope, get another one
                  NewAddress = ActuallyFreeAddressByPage(pages, NewAddress);
                  // Make sure the new address isn't the known bad one
                  if(NewAddress == OldAllocatedMemory)
                  {
                    // Get a new address if it is
                    NewAddress = ActuallyFreeAddressByPage(pages, NewAddress);
                  }
                  // Addresses very well might be > 4GB for the filesizes these are allowed to be
                }
                else if(EFI_ERROR(GoTimeStatus))
                {
                  Print(L"Could not get an address for Mach64 pages by page. Error code: 0x%llx\r\n", GoTimeStatus);
                  return GoTimeStatus;
                }

                if(NewAddress == -1)
                {
                  // If you somehow get this, you really had no memory free anywhere.
                  Print(L"Hmm... How did you get here?\r\n");
                  return GoTimeStatus;
                }

                GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress);
              }

              AllocatedMemory = NewAddress;

              while(VerifyZeroMem(pages << EFI_PAGE_SHIFT, AllocatedMemory))
              {

                // Sure hope there aren't any other page-aligned kernel images floating around in memory marked as free
                if(compare((EFI_PHYSICAL_ADDRESS*)AllocatedMemory, &MemCheck, 4))
                {
                  // Do nothing, we're fine
    #ifdef MEMORY_CHECK_INFO
                  Print(L"System might have been reset. Hopefully no issues.\r\n");
    #endif

                  break;
                }
                else
                {

    #ifdef MEMORY_DEBUG_ENABLED
                Print(L"Still searching by page... 0x%llx\r\n", AllocatedMemory);
    #endif

                  // It's not actually free...
                  GoTimeStatus = BS->FreePages(AllocatedMemory, pages);
                  if(EFI_ERROR(GoTimeStatus))
                  {
                    Print(L"Could not free pages for Mach64 sections by page (loop). Error code: 0x%llx\r\n", GoTimeStatus);
                    return GoTimeStatus;
                  }

                  GoTimeStatus = EFI_NOT_FOUND;
                  while(GoTimeStatus != EFI_SUCCESS)
                  {
                    if(GoTimeStatus == EFI_NOT_FOUND)
                    {
                      // Nope, get another one
                      NewAddress = ActuallyFreeAddressByPage(pages, NewAddress);
                      // Make sure the new address isn't the known bad one
                      if(NewAddress == OldAllocatedMemory)
                      {
                        // Get a new address if it is
                        NewAddress = ActuallyFreeAddressByPage(pages, NewAddress);
                      }
                      // Address can be > 4GB
                    }
                    else if(EFI_ERROR(GoTimeStatus))
                    {
                      Print(L"Could not get an address for Mach64 pages by page (loop). Error code: 0x%llx\r\n", GoTimeStatus);
                      return GoTimeStatus;
                    }

                    if(AllocatedMemory == -1)
                    {
                      // Well, darn. Something's up with the system memory.
                      return GoTimeStatus;
                    }

                    GoTimeStatus = BS->AllocatePages(AllocateAddress, EfiLoaderData, pages, &NewAddress);
                  } // loop

                  AllocatedMemory = NewAddress;

                } // else
              } // end ByPage VerifyZeroMem loop
  #endif
            } // End "big guns"

            // Got a good address!
  #ifdef MEMORY_CHECK_INFO
            Print(L"Found!\r\n");
  #endif
          } // End discovery of viable memory address (else)
          // Can move on now
        } // End VerifyZeroMem buggy firmware workaround (outermost if)
#endif

#ifdef MACH_LOADER_DEBUG_ENABLED
        Print(L"New AllocatedMemory location: 0x%llx\r\n", AllocatedMemory);
        Keywait(L"Allocate Pages passed.\r\n");
#endif

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

#ifdef MACH_LOADER_DEBUG_ENABLED
            Print(L"\n%llu. current section address: 0x%x, RawDataSize: 0x%llx\r\n", i+1, specific_segment_command->vmaddr, RawDataSize);
            Print(L"current destination address: 0x%llx, AllocatedMemory base: 0x%llx\r\n", SectionAddress, AllocatedMemory);
            Print(L"PointerToRawData: 0x%llx\r\n", specific_segment_command->fileoff);
            Print(L"Check:\r\nSectionAddress: 0x%llx\r\nData there: 0x%016llx%016llx (should be 0)\r\n", SectionAddress, *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + 8), *(EFI_PHYSICAL_ADDRESS *)SectionAddress); // Print the first 128 bits of data at that address to compare
            Print(L"About to load section %llu of %llu...\r\n", i + 1, Numofcommands);
            Keywait(L"\0");
#endif

            GoTimeStatus = KernelFile->SetPosition(KernelFile, specific_segment_command->fileoff); // p_offset is a UINT64 relative to the beginning of the file, just like Read() expects!
            if(EFI_ERROR(GoTimeStatus))
            {
              Print(L"Program segment SetPosition error (Mach64). 0x%llx\r\n", GoTimeStatus);
              return GoTimeStatus;
            }

            if(RawDataSize != 0) // Apparently some UEFI implementations can't deal with reading 0 byte sections
            {
              GoTimeStatus = KernelFile->Read(KernelFile, &RawDataSize, (EFI_PHYSICAL_ADDRESS*)SectionAddress); // (void*)SectionAddress
              if(EFI_ERROR(GoTimeStatus))
              {
                Print(L"Program segment read error (Mach64). 0x%llx\r\n", GoTimeStatus);
                return GoTimeStatus;
              }
            }

#ifdef MACH_LOADER_DEBUG_ENABLED
            Print(L"\r\nVerify:\r\nSectionAddress: 0x%llx\r\nData there (first 16 bytes): 0x%016llx%016llx\r\n", SectionAddress, *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + 8), *(EFI_PHYSICAL_ADDRESS*)SectionAddress); // Print the first 128 bits of data at that address to compare
            Print(L"Last 16 bytes: 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize - 8), *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize - 16));
            Print(L"Next 16 bytes: 0x%016llx%016llx (0 unless last section)\r\n", *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize + 8), *(EFI_PHYSICAL_ADDRESS*)(SectionAddress + RawDataSize));
            // "Next 16 bytes" should be 0 unless last section
#endif

          }
          else if(specific_load_command->cmd == LC_UNIXTHREAD)
          {
            //Get the entry point
            UINT64 *unixthread_data = (UINT64*)specific_load_command; // Or use an array of 64-bit UINTs and grab RIP. The other registers should be zero.
            entrypointoffset = unixthread_data[18]; // Get entrypoint from RIP register initialization

#ifdef MACH_LOADER_DEBUG_ENABLED
            Print(L"Entry Point: 0x%llx\r\n", entrypointoffset);
#endif
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

#ifdef MACH_LOADER_DEBUG_ENABLED
            Print(L"Not a LC_SEGMENT_64 section. Type: 0x%x\r\n", specific_load_command->cmd);
#endif
          }
          current_spot += (UINT64)specific_load_command->cmdsize;
        }

        // Done with commands_buffer
        if(commands_buffer)
        {
          GoTimeStatus = BS->FreePool(commands_buffer);
          if(EFI_ERROR(GoTimeStatus))
          {
            Print(L"Error freeing commands buffer pool. 0x%llx\r\n", GoTimeStatus);
            Keywait(L"\0");
          }
        }

#ifdef MACH_LOADER_DEBUG_ENABLED
        Keywait(L"\nLoad file sections into allocated pages passed.\r\n");
#endif

        // entrypointoffset should be a 64-bit relative mem address of the entry point of the kernel
        Header_memory = AllocatedMemory + entrypointoffset;

#ifdef MACH_LOADER_DEBUG_ENABLED
        Print(L"Header_memory: 0x%llx, AllocatedMemory: 0x%llx, EntryPoint: 0x%x\r\n", Header_memory, AllocatedMemory, entrypointoffset);
        Print(L"Data at Header_memory (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(Header_memory + 8), *(EFI_PHYSICAL_ADDRESS*)Header_memory);
#endif

        // Loaded! On to memorymap and exitbootservices...
        // NOTE: Executable entry point is now defined in Header_memory's contained address, which is AllocatedMemory + entrypointoffset
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

#ifdef FINAL_LOADER_DEBUG_ENABLED
  Print(L"Header_memory: 0x%llx\r\n", Header_memory);
  Print(L"Data at Header_memory (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(Header_memory + 8), *(EFI_PHYSICAL_ADDRESS*)Header_memory);

  if(KernelisPE)
  {
    Print(L"Kernel uses MS ABI\r\n");
  }
  else
  {
    Print(L"Kernel uses SYSV ABI\r\n");
  }

  Keywait(L"\0");

  // Integrity check
  for(UINT64 k = 0; k < Graphics->NumberOfFrameBuffers; k++)
  {
    Print(L"GPU Mode: %u of %u\r\n", Graphics->GPUArray[k].Mode, Graphics->GPUArray[k].MaxMode - 1);
    Print(L"GPU FB: 0x%016llx\r\n", Graphics->GPUArray[k].FrameBufferBase);
    Print(L"GPU FB Size: 0x%016llx\r\n", Graphics->GPUArray[k].FrameBufferSize);
    Print(L"GPU SizeOfInfo: %u Bytes\r\n", Graphics->GPUArray[k].SizeOfInfo);
    Print(L"GPU Info Ver: 0x%x\r\n", Graphics->GPUArray[k].Info->Version);
    Print(L"GPU Info Res: %ux%u\r\n", Graphics->GPUArray[k].Info->HorizontalResolution, Graphics->GPUArray[k].Info->VerticalResolution);
    Print(L"GPU Info PxFormat: 0x%x\r\n", Graphics->GPUArray[k].Info->PixelFormat);
    Print(L"GPU Info PxInfo (R,G,B,Rsvd Masks): 0x%08x, 0x%08x, 0x%08x, 0x%08x\r\n", Graphics->GPUArray[k].Info->PixelInformation.RedMask, Graphics->GPUArray[k].Info->PixelInformation.GreenMask, Graphics->GPUArray[k].Info->PixelInformation.BlueMask, Graphics->GPUArray[k].Info->PixelInformation.ReservedMask);
    Print(L"GPU Info PxPerScanLine: %u\r\n", Graphics->GPUArray[k].Info->PixelsPerScanLine);
    Keywait(L"\0");
  }

  Print(L"RSDP address: 0x%llx\r\n", RSDPTable);
  Print(L"Data at RSDP (first 16 bytes): 0x%016llx%016llx\r\n", *(EFI_PHYSICAL_ADDRESS*)(RSDPTable + 8), *(EFI_PHYSICAL_ADDRESS*)RSDPTable);
#endif

  // All EfiBootServicesData are freed by a call to ExitBootServices().
  // The only reason these were allocated in the first place was to stop this program from overwriting itself.
  // There should not be any memory leaks since AllocatePools were defined to be exactly the size they needed to be.
  // These FreePools also error out.
/*
  // Free previously allocated file-related (Kernel64) pools
  if(KernelFile)
  {
    GoTimeStatus = BS->FreePool(KernelFile);
    if(EFI_ERROR(GoTimeStatus))
    {
      Print(L"Error freeing KernelFile pool. 0x%llx\r\n", GoTimeStatus);
      Keywait(L"\0");
    }
  }

  if(CurrentDriveRoot)
  {
    GoTimeStatus = BS->FreePool(CurrentDriveRoot);
    if(EFI_ERROR(GoTimeStatus))
    {
      Print(L"Error freeing CurrentDriveRoot pool. 0x%llx\r\n", GoTimeStatus);
      Keywait(L"\0");
    }
  }

  if(FileSystem)
  {
    GoTimeStatus = BS->FreePool(FileSystem);
    if(EFI_ERROR(GoTimeStatus))
    {
      Print(L"Error freeing FileSystem pool. 0x%llx\r\n", GoTimeStatus);
      Keywait(L"\0");
    }
  }

  if(LoadedImage)
  {
    GoTimeStatus = BS->FreePool(LoadedImage);
    if(EFI_ERROR(GoTimeStatus))
    {
      Print(L"Error freeing LoadedImage pool. 0x%llx\r\n", GoTimeStatus);
      Keywait(L"\0");
    }
  }

#ifdef LOADER_DEBUG_ENABLED
  Keywait(L"File pools freed.\r\n");
#endif
*/
  // Reserve memory for the loader block
  LOADER_PARAMS * Loader_block;
  GoTimeStatus = BS->AllocatePool(EfiLoaderData, sizeof(LOADER_PARAMS), (void**)&Loader_block);
  if(EFI_ERROR(GoTimeStatus))
  {
    Print(L"Error allocating loader block pool. Error: 0x%llx\r\n", GoTimeStatus);
    return GoTimeStatus;
  }

#ifdef FINAL_LOADER_DEBUG_ENABLED
  Print(L"Loader block allocated, size of structure: %llu\r\n", sizeof(LOADER_PARAMS));
  Keywait(L"About to get MemMap and exit boot services...\r\n");
#endif

  // Hm... This appears to also reset the video mode to mode 0...
  // Clear screen while we still have EFI services
  // ST->ConOut->ClearScreen(ST->ConOut);
  // Eh, this won't matter once debug statements are turned off, anyways, since the only time user input is required is before GOP SetMode

 //----------------------------------------------------------------------------------------------------------------------------------
 //  Get Memory Map and Exit Boot Services
 //----------------------------------------------------------------------------------------------------------------------------------

  // UINTN is the largest uint type supported. For x86_64, this is uint64_t
  UINTN MemMapSize = 0, MemMapKey, MemMapDescriptorSize;
  UINT32 MemMapDescriptorVersion;
  EFI_MEMORY_DESCRIPTOR * MemMap = NULL;

/*
  // Simple version:
  GoTimeStatus = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
  // Will error intentionally
  GoTimeStatus = BS->AllocatePool(EfiLoaderData, MemMapSize, (void **)&MemMap);
  if(EFI_ERROR(GoTimeStatus)) // Error! Wouldn't be safe to continue.
    {
      Print(L"MemMap AllocatePool error. 0x%llx\r\n", GoTimeStatus);
      return GoTimeStatus;
    }
  GoTimeStatus = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
  GoTimeStatus = BS->ExitBootServices(ImageHandle, MemMapKey);
*/

// Below is a better, but more complex version. EFI Spec recommends this method; apparently some systems need a second call to ExitBootServices.

  // Get memory map and exit boot services
  GoTimeStatus = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
  if(GoTimeStatus == EFI_BUFFER_TOO_SMALL)
  {
    GoTimeStatus = BS->AllocatePool(EfiLoaderData, MemMapSize, (void **)&MemMap); // Allocate pool for MemMap (it should always be resident in memory)
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

    GoTimeStatus = BS->FreePool(MemMap);
    if(EFI_ERROR(GoTimeStatus)) // Error! Wouldn't be safe to continue.
    {
      Print(L"Error freeing MemMap pool from failed ExitBootServices. 0x%llx\r\n", GoTimeStatus);
      Keywait(L"\0");
    }

#ifdef LOADER_DEBUG_ENABLED
    Print(L"ExitBootServices #1 failed. 0x%llx, Trying again...\r\n", GoTimeStatus);
    Keywait(L"\0");
#endif

    MemMapSize = 0;
    GoTimeStatus = BS->GetMemoryMap(&MemMapSize, MemMap, &MemMapKey, &MemMapDescriptorSize, &MemMapDescriptorVersion);
    if(GoTimeStatus == EFI_BUFFER_TOO_SMALL)
    {
      GoTimeStatus = BS->AllocatePool(EfiLoaderData, MemMapSize, (void **)&MemMap);
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
    GoTimeStatus = BS->FreePool(MemMap);
    if(EFI_ERROR(GoTimeStatus)) // Error! Wouldn't be safe to continue.
    {
      Print(L"Error freeing MemMap pool. 0x%llx\r\n", GoTimeStatus);
    }
    Print(L"MemMapSize: %llx, MemMapKey: %llx\r\n", MemMapSize, MemMapKey);
    Print(L"DescriptorSize: %llx, DescriptorVersion: %x\r\n", MemMapDescriptorSize, MemMapDescriptorVersion);
    return GoTimeStatus;
  }

  //----------------------------------------------------------------------------------------------------------------------------------
  //  Entry Point Jump
  //----------------------------------------------------------------------------------------------------------------------------------

/*
  // Example entry point jump
  typedef void (EFIAPI *EntryPointFunction)();
  EntryPointFunction EntryPointPlaceholder = (EntryPointFunction)(Header_memory);
//  Print(L"EntryPointPlaceholder: %llx\r\n", (UINT64)EntryPointPlaceholder);
  EntryPointPlaceholder();
*/

  // Example one-liners for an entry point jump
  // ((EFIAPI void(*)(EFI_RUNTIME_SERVICES*)) Header_memory)(RT);
  // ((EFIAPI void(*)(void)) Header_memory)(); // A void-returning function that takes no arguments. Neat!

/*
  // No loader block version
  typedef void (EFIAPI *EntryPointFunction)(EFI_MEMORY_DESCRIPTOR * Memory_Map, EFI_RUNTIME_SERVICES* RTServices, EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE* GPU_Mode, EFI_FILE_INFO* FileMeta, void * RSDP); // Placeholder names for jump
  EntryPointFunction EntryPointPlaceholder = (EntryPointFunction)(Header_memory);
  EntryPointPlaceholder(MemMap, RT, &Graphics, FileInfo, RSDPTable);
*/

/*
  // Loader block defined in header
  typedef struct {
    UINTN                   Memory_Map_Size;
    UINTN                   Memory_Map_Descriptor_Size;
    EFI_MEMORY_DESCRIPTOR  *Memory_Map;
    EFI_RUNTIME_SERVICES   *RTServices;
    GPU_CONFIG             *GPU_Configs;
    EFI_FILE_INFO          *FileMeta;
    void                   *RSDP;
  } LOADER_PARAMS;
*/

  // This shouldn't modify the memory map.
  // TODO: Add base address of kernel (Header_memory EFI_PHYSICAL_ADDRESS) and size (in pages), same for bootloader (ImageHandle.ImageBase (void*) and .ImageSize (uint64) so it can be reclaimed)
  Loader_block->Memory_Map_Size = MemMapSize;
  Loader_block->Memory_Map_Descriptor_Size = MemMapDescriptorSize;
  Loader_block->Memory_Map = MemMap;
  Loader_block->RTServices = RT;
  Loader_block->GPU_Configs = Graphics;
  Loader_block->FileMeta = FileInfo;
  Loader_block->RSDP = RSDPTable;

  // Jump to entry point, and WE ARE LIVE!!
  if(KernelisPE)
  {
    typedef void (__attribute__((ms_abi)) *EntryPointFunction)(LOADER_PARAMS * LP); // Placeholder names for jump
    EntryPointFunction EntryPointPlaceholder = (EntryPointFunction)(Header_memory);
    EntryPointPlaceholder(Loader_block);
  }
  else
  {
    typedef void (__attribute__((sysv_abi)) *EntryPointFunction)(LOADER_PARAMS * LP); // Placeholder names for jump
    EntryPointFunction EntryPointPlaceholder = (EntryPointFunction)(Header_memory);
    EntryPointPlaceholder(Loader_block);
  }

  // Should never get here
  return GoTimeStatus;
}
