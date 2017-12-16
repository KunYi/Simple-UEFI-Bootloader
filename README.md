# Simple-UEFI-Bootloader
A UEFI bootloader for bare-metal x86-64 applications.  
  
It's like a much simpler version of GRUB, mainly meant for writing your own operating system-less 64-bit programs. Supports Windows, Linux, and Mac executable binaries (PE32+, 64-bit ELF, and 64-bit Mach-O formats). It also supports... Well, I'll let you figure that one out yourself. ;)  

**Coming Soon**  

Current status: It works, but it's a 1000+ line "main" function and needs to be cleaned up before I upload it.
  
A simple bare-metal kernel using this bootloader will be uploaded here:  
https://github.com/KNNSpeed/Simple-Kernel/

Why not upload it as-is?  
Consider the fact that, once something is released online, it can very rapidly disseminate all over the place. If that ever were to happen here, it would be a shame if what spread were an ugly mess. The bootloader.c file cannot be compiled without its headers, and it's just here to show that it does in fact exist.
  
A full description of how to actually compile, etc. will be added once I finish my to-do list and clean up the mess of code.  
  
"A delayed game is eventually good, but a rushed game is forever bad."  
-Shigeru Miyamoto  
