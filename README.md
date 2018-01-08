# Simple-UEFI-Bootloader
A UEFI bootloader for bare-metal x86-64 applications.  
  
**Version 1.1**
  
This bootloader is like a much simpler version of GRUB/Elilo/Windows Boot Manager, mainly meant for writing your own operating system-less 64-bit programs or full operating systems. Supports Windows, Linux, and Mac executable binaries (PE32+, 64-bit ELF, and 64-bit Mach-O formats). It also supports... Well, I'll let you figure that one out yourself. ;) 
  
A simple bare-metal kernel using this bootloader will be uploaded here:  
https://github.com/KNNSpeed/Simple-Kernel/
  
Please post any bugs, feature requests, etc. in "Issues."  
  
**Features:**
- UEFI 2.x support  
- Loads kernels compiled as native Windows PE32+, Linux 64-bit ELF, and Mac OS 64-bit Mach-O files (see my Simple-Kernel repository for proper compilation options)
- Reset protection for kernel file loading (if the system resets without cleanly clearing the memory, the bootloader will try to reuse the same memory location)
- Multi-GPU framebuffer support (typically only works with one monitor per GPU due to how most GPU firmwares are implemented)  
- ACPI support  
- The ability to get the full system memory map to do whatever you want with it  
- Fits on a floppy diskette, and some systems can actually boot it from a floppy  
  
**System Requirements:**  
- x86-64 architecture (won't work with anything else)  
- Secure Boot must be disabled  
- More than 4GB RAM (though it seems to work OK with less, e.g. Hyper-V with only 1GB)  
- A graphics card (Intel, AMD, NVidia, etc.) **with UEFI GOP support**  
- A keyboard  

The earliest GPUs with UEFI GOP support were released around the Radeon HD 7xxx series (~2011). Anything that age or newer should have UEFI GOP support, though older models, like early 7970s, required owners to contact GPU vendors to get UEFI-compatible firmware. On Windows, you can check by downloading TechPowerUp's GPU-Z utility and seeing whether or not the UEFI checkbox is checked. If it is, you're all set!
  
**How to Build from Source:**  
Requires GCC 7.1.0 or later and Binutils 2.29.1 or later. I cannot make any guarantees whatsoever for earlier versions, especially with the number of compilation and linking flags used.

***Windows:***  
1. Download and extract or clone this repository into a dedicated folder, preferably somewhere easy like C:\BareMetalx64

2. Download MinGW-w64 "x86_64-posix-seh" from https://sourceforge.net/projects/mingw-w64/ (click "Files" and scroll down - pay attention to the version numbers!).

3. Extract the archive into the "Backend" folder.

4. Open Windows PowerShell or the Command Prompt in the "Simple-UEFI-Bootloader" folder and type ".\Compile.bat"

    *That's it! It should compile and a binary called "BOOTX64.EFI" will be output into the "Backend" folder.*

***Linux:***  

TODO (requires building GCC and binutils, unless the system has the correct versions already built-in. In this case you'd just need to modify the Compile.sh script to call the right gcc/ld/objcopy, etc., and then run the script)

***Mac:***  
1. Download and extract or clone this repository into a dedicated folder, preferably somewhere easy like ~/BareMetalx64

2. Install MacPorts: https://www.macports.org/

3. In Terminal, get the MinGW-w64 package via "sudo port install mingw-w64" ("sudo port install x86_64-w64-mingw32-gcc" might also work)

4. Once MinGW-w64 is installed, open Terminal in the "Simple-UEFI-Bootloader" folder and run "./Compile-Mac.sh"

    *That's it! It should compile and a binary called "BOOTX64.EFI" will be output into the "Backend" folder.*
