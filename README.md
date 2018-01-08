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
- Reset protection for PE32+ loading (if the system resets without cleanly clearing the memory, the bootloader will try to reuse the same memory location--but only for PE32+ files) - *V1.1 adds this for ELF and Mach-O*
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

The earliest GPUs with UEFI GOP support were released around the Radeon HD 7xxx series (~2011). Anything that age or newer should have UEFI GOP support, though older models like early 7970s required owners to contact GPU vendors to get UEFI-compatible firmware. On Windows, you can check by downloading TechPowerUp's GPU-Z utility and seeing whether or not the UEFI checkbox is checked. If it is, you're all set!
  
**How to Build from Source:**  
A full description of how to actually compile, etc. will be posted here once it's written up. I will also be including a full, multi-platform portable build system I designed that makes compiling as simple as typing ".\Compile" in the command line and uses GCC/binutils, MinGW-w64, etc.
