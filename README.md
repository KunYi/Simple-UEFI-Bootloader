# Simple-UEFI-Bootloader
A UEFI bootloader for bare-metal x86-64 applications.  
  
**Version 1.2**
  
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
  
**License and Crediting:**  
  
Please see the LICENSE file for information on the licenses covering the code created for and used in this project. The file also contains important information on how to credit this project when using parts of it in other projects.  
  
I tried to keep licensing for original code reasonable, concise, and permissive, and all I'm really asking for is appropriate citation (see the "Custom Attribution License" section of the file). Even if the only section you looked at was the Custom Attribution License section, that would be great (it's pretty short!). Thanks!  
  
**How to Build from Source:**  
  
Requires GCC 7.1.0 or later and Binutils 2.29.1 or later. I cannot make any guarantees whatsoever for earlier versions, especially with the number of compilation and linking flags used.

***Windows:***  
1. Download and extract or clone this repository into a dedicated folder, preferably somewhere easy like C:\BareMetalx64

2. Download MinGW-w64 "x86_64-posix-seh" from https://sourceforge.net/projects/mingw-w64/ (click "Files" and scroll down - pay attention to the version numbers!).

3. Extract the archive into the "Backend" folder.

4. Open Windows PowerShell or the Command Prompt in the "Simple-UEFI-Bootloader" folder and type ".\Compile.bat"

    *That's it! It should compile and a binary called "BOOTX64.EFI" will be output into the "Backend" folder.*
  
***Mac:***  
1. Download and extract or clone this repository into a dedicated folder, preferably somewhere easy like ~/BareMetalx64

2. Install the latest MacPorts: https://www.macports.org/

3. In Terminal, get the MinGW-w64 package via "sudo port install mingw-w64" ("sudo port install x86_64-w64-mingw32-gcc" might also work)

    NOTE: Make sure that MacPorts downloads a version using the correct GCC and Binutils! You may need to run "sudo port selfupdate" if you aren't freshly installing MacPorts before running the above install command.

4. Once MinGW-w64 is installed, open Terminal in the "Simple-UEFI-Bootloader" folder and run "./Compile-Mac.sh"

    *That's it! It should compile and a binary called "BOOTX64.EFI" will be output into the "Backend" folder.*
  
***Linux:***  

1. Download and extract or clone this repository into a dedicated folder, preferably somewhere easy like ~/BareMetalx64

2. If, in the terminal, "gcc --version" reports GCC 7.1.0 or later and "ld --version" reports 2.29.1 or later, do steps 2a, 2b, and 2c. Otherwise go to step 3.
    
    2a. Type "which gcc" in the terminal, and make a note of what it says (something like /usr/bin/gcc or /usr/local/bin/gcc)
    
    2b. Open Compile.sh in an editor of your choice (nano, gedit, vim, etc.) and set the GCC_FOLDER_NAME variable at the top to be the part before "bin" (e.g. /usr or /usr/local, without the last slash). Do the same thing for BINUTILS_FOLDER_NAME, except use the output of "which ld" to get the directory path preceding "bin" instead.
    
    2c. Now set the terminal to the Simple-UEFI-Bootloader folder and run Compile.sh, which should work and output BOOTX64.EFI in the Backend folder. That's it!

3. Looks like we need to build GCC & Binutils. Navigate to the "Backend" folder in terminal and do "git clone git://gcc.gnu.org/git/gcc.git" there. This will download a copy of GCC 8.0.0, which is what I have been using (need this version for the Simple-Kernel). If that git link ever changes, you'll need to find wherever the official GCC git repository ran off to.

4. Once GCC has been cloned, in the cloned folder do "./configure -v --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu --prefix=gcc-8 --enable-checking=release --enable-languages=c --disable-multilib"

    NOTE: If you want, you can enable other languages like c++, fortran, objective-c (objc), go, etc. with enable-languages. You can also change the name of the folder it will built into by changing --prefix=[desired folder]. The above command line will configure GCC to be made in a folder called gcc-8 inside the "Backend" folder.

5. After configuration completes, do "make -j [twice the number of cores of your CPU]" and go eat lunch. Unfortunately, sometimes building the latest GCC produces build-time errors; I ran into an "aclocal-1.15" issue when building via Linux on Windows (fixed by installing the latest version of Ubuntu on Windows and using the latest autoconf).

6. Now just do "make install" and GCC will be put into the gcc-8 folder from step 4.

7. Next, grab binutils 2.29.1 or later from https://ftp.gnu.org/gnu/binutils/ and extract the archive to Backend.

8. In the extracted Binutils folder, do "mkdir build" and "cd build" before configuring with "../configure --prefix=binutils-binaries --enable-gold --enable-ld=default --enable-plugins --enable-shared --disable-werror"

    NOTE: The "prefix" flag means the same thing as GCC's.

9. Once configuration is finished, do "make -j [twice the number of CPU cores]" and go have dinner.

10. Once make is done making, do "make -k check" and do a crossword or something. There should be a very small number of errors, if any.

11. Finally, do "make install" to install the package into binutils-binaries. Congrats, you just built some of the biggest Linux sources ever.

12. Open Compile.sh in an editor of your choice (nano, gedit, vim, etc.) and set the GCC_FOLDER_NAME variable at the top (e.g. gcc-8 without any slashes). Do the same thing for the BINUTILS_FOLDER_NAME, except use the binutils-binaries folder.

13. At long last, you should be able to run "./Compile.sh" from within the "Simple-UEFI-Bootloader" folder.

    *That's it! It should compile and a binary called "BOOTX64.EFI" will be output into the "Backend" folder.*

    For more information about building GCC and Binutils, see these: http://www.linuxfromscratch.org/blfs/view/cvs/general/gcc.html & http://www.linuxfromscratch.org/lfs/view/development/chapter06/binutils.html  
  
**Change Log:**

V1.2 - Resolved a major issue that prevented larger SYSV ABI kernels (ELF) from running. (1/8/2018)

V1.1 - Code organization and build system uploaded. Also fixed bugs. (1/7/2018)

V1.0 - Initial release. Fully featured for PE32+ images. (1/5/2018)
