# Simple-UEFI-Bootloader
A UEFI bootloader for bare-metal x86-64 applications.  
  
It's like a much simpler version of GRUB, mainly meant for writing your own operating system-less 64-bit programs. Supports Windows, Linux, and Mac executable binaries (PE32+, 64-bit ELF, and 64-bit Mach-O formats). It also supports... Well, I'll let you figure that one out yourself. ;)  

**Coming Soon**  

Current status: It works, and a binary release should happen later today (1/5/2018). Loader block's in the source code for anyone to look at.
  
A simple bare-metal kernel using this bootloader will be uploaded here:  
https://github.com/KNNSpeed/Simple-Kernel/

A full description of how to actually compile, etc. will be posted here. I will also be including a full, multi-platform portable build system I designed that makes compiling as simple as typing ".\Compile" in the command line and uses GCC/binutils, MinGW-w64, etc.

Additionally, there are some parts of GNU-EFI that needed to be updated and augmented, so that will be included out of necessity.
  
"A delayed game is eventually good, but a rushed game is forever bad."  
-Shigeru Miyamoto  

Some features to look forward to:  
-UEFI 2.x support  
-Loads kernels compiled as Windows PE32+, Linux 64-bit ELF, and Mac OS 64-bit Mach-O (see my Simple-Kernel repository for proper compilation options)  
-Multi-GPU framebuffer support (typically only works with one monitor per GPU due to how most GPU firmwares are implemented)  
-ACPI support  
-The ability to get the full system memory map to do whatever you want with it  
-Fits on a floppy diskette, and some systems can actually boot this from one  
  
System Recommendations:  
-x86-64 architecture (won't work with anything else)  
-Secure Boot MUST be disabled  
-More than 4GB RAM (seems to work OK with less, e.g. Hyper-V with 1GB)  
-A graphics card (Intel, AMD, NVidia, etc.) with UEFI GOP support  
-A keyboard  
