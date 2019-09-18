# Simple UEFI Bootloader
A UEFI bootloader for bare-metal x86-64 applications. Looking for the ARM64 version? Get it here: https://github.com/KNNSpeed/Simple-UEFI-Bootloader-ARM64  

**Version 2.3**

This bootloader is like a much simpler version of GRUB/Elilo/Windows Boot Manager, but mainly meant for writing your own operating system-less 64-bit programs, kernels, or full operating systems. It supports Windows, Linux, and Mac executable binaries (PE32+, 64-bit ELF, and 64-bit Mach-O formats). It also supports... Well, I'll let you figure that one out yourself. ;)

***See the "Releases" tab for usage information and downloads, and please post any bugs, feature requests, etc. in "Issues."***  

A minimal, cross-platform development environment for making your own programs, kernels, and/or operating systems that use this bootloader can be found here (demo binary programs are also there; note that they do require AVX support): https://github.com/KNNSpeed/Simple-Kernel/

## Bootloader Features  

- UEFI 2.x support
- Loads and executes kernels compiled as native Windows PE32+, Linux 64-bit ELF, and Mac OS 64-bit Mach-O files ***(1)***
- Passes load options from a user-generated text file directly to kernel files
- Multiple bootloader instances can coexist on one system to load separate kernel files, complete with their own load options, all using the system's native UEFI boot manager to select between them
- Multi-GPU framebuffer support ***(2)***
- ACPI support
- The ability to get the full system memory map to do whatever you want with it
- Fits on a floppy diskette, and some systems can actually boot it from a floppy
- Minimal UEFI development environment tuned for Windows, Mac, and Linux included in repository ***(3)***

***(1)*** *See the Simple-Kernel repository for proper compilation options for each operating system.*  
***(2)*** *This typically only works with one monitor per GPU due to how most GPU firmwares are implemented.*  
***(3)*** *See the below "How to Build from Source" section for complete compilation instructions for each platform, and then all you need to do is put your code in "src" and "inc" in place of mine. Once compiled, your program can be run in the same way as described in "Releases" using a UEFI-supporting VM like Hyper-V or on actual hardware.*  

## Target System Requirements  

- x86-64 architecture  
- Secure Boot must be disabled  
- More than 4GB RAM (though it seems to work OK with less, e.g. Hyper-V with only 1GB)  
- A graphics card (Intel, AMD, NVidia, etc.) **with UEFI GOP support**  
- A keyboard  

The earliest GPUs with UEFI GOP support were released around the Radeon HD 7xxx series (~2011). Anything that age or newer should have UEFI GOP support, though older models, like early 7970s, required owners to contact GPU vendors to get UEFI-compatible firmware. On Windows, you can check if your graphics card(s) have UEFI GOP support by downloading TechPowerUp's GPU-Z utility and seeing whether or not the UEFI checkbox is checked. If it is, you're all set! On Macs, boot the system with rEFIt and, in the "About rEFIt" tool (it's one of the small icons) a compatible system will state "Screen Output: Graphics Output (UEFI)" right above "Return to Main Menu."  

*NOTE: You need to check each graphics card if there is a mix, as you will only be able to use the ones with UEFI GOP support. Per the system requirements above, you need at least one compliant device.*  

## License and Crediting  

Please see the LICENSE file for information on all licenses covering code created for and used in this project.  

***TL;DR:***  

If you don't give credit to this project, per the license you aren't allowed to do anything with any of its source code that isn't already covered by an existing license (in other words, my license covers most of the code I wrote). That's pretty much it, and why it's "almost" PD, or "PD with Credit" if I have to give it a nickname: there's no restriction on what it gets used for as long as the license is satisfied. If you have any issues, feature requests, etc. please post in "Issues" so it can be attended to/fixed.  

Note that each of these files already has appropriate crediting at the top, so you could just leave what's already there to satisfy the terms. You really should see the license file for complete information, though (it's short!!).  

## Usage

### How to Use

1. **[External Drive, ignore step 2]** On an external FAT or FAT32 drive (USB, floppy, whatever), make an "EFI" folder, and inside that make a "BOOT" folder. Put the desired bootloader binary in the BOOT folder, and name the binary BOOTX64.EFI. This is just standard booting procedure: a FAT32 volume containing \\EFI\\BOOT\\BOOTX64.EFI is defined by specification to be the default UEFI boot file for that drive on x86-64.

2. **[Internal Drive, ignore step 1]** Put this program anywhere you want on the EFI System Partition and add it to your UEFI firmware as a boot option. The default bootable file that UEFI firmware looks for is BOOTX64.EFI in the directory \\EFI\\BOOT\\, so you could just rename the bootloader file accordingly and put it at that location to boot it automatically.  

3. Put the bare metal program or kernel somewhere on the same FAT32 drive or EFI System Partition.

4. Make a file called **Kernel64.txt**--this should be stored **in the same folder as the bootloader itself.** See the next section for how to properly format this file.

5. Boot your machine, and enter the boot device menu. This is commonly achieved by pressing a key like F10 or F12 at the boot logo. Select the option **UEFI ... [your drive or boot entry containing the bootloader and kernel]** -- the name varies depending on the type of drive used and the motherboard model.

    NOTE: You should be sure your system supports booting from external media if you are using a USB drive, and ensure that the system is not configured to boot in Legacy or BIOS mode (i.e. it has UEFI booting enabled). Also, spaces in file/folder names are not allowed.

That's it! If your kernel file's entry point function is something like **main_function(LOADER_PARAMS * LP)**, it should load after you select how you want your graphics output device(s) configured. See https://github.com/KNNSpeed/Simple-Kernel for an example, including proper compilation options.

### Kernel64.txt Format and Contents

Kernel64.txt should be stored in UTF-16 format in the same directory as the boot loader on the EFI system partition. Windows Notepad and Wordpad can save text files in this format (select "Unicode Text Document" or "UTF-16 LE" as the encoding format in the "Save As" dialog). Linux users can use gedit or xed, saving as a .txt file with UTF-16 encoding. Also, it does not matter if the file uses Windows (CRLF) or Unix (LF) line endings, but the file does need a 2-byte identification Byte Order Mark (BOM). Don't worry too much about the BOM; it gets added automatically by all of the aforementioned editors when saving with the correct encoding.  

The contents of the text file are simple: only three lines are needed. The first line should be the filename and location of the kernel to be booted relative to the root of the EFI system partition, e.g. \\EFI\\Kernel1\\MyKernel.64 or \\Socks\\Bubbles.ReallyLongFileExtension (do not use spaces or special characters in file/folder names), and the second line is the string of load options to be passed to the kernel, e.g. "root=/dev/nvme0n1p5 initrd=\\\EFI\\\ubuntu\\\initrd.img ro rootfstype=ext4 debug ignore_loglevel libata.force=dump_id crashkernel=384M-:128M quiet splash acpi_rev_override=1 acpi_osi=Linux" (without quotes! -- note this example is just a random string of Linux arguments). The third line should be blank--and make sure there is a third line, as this program expects a line break to denote the end of the kernel arguments.**  

That's it!  

 ** Technically you could use the remainder of the text file to contain an actual text document. You could put this info in there if you wanted, or your favorite song lyrics, though the smaller the text file the faster it is to load.  

### Booting Multiple Kernels

A copy of the bootloader and a kernel64.txt file is required for every kernel in a multi-use situation. Recommended practice for booting multiple kernels is to make a folder for each kernel, and each folder should contain its own bootloader, kernel64.txt, and kernel file. The method to boot multiple kernel files varies by machine: generally there is a firmware boot menu accessed by F10, F11, F12, etc. at power-on, and entries can be added to this menu in the UEFI firmware setup (accessed by F2, DEL, etc. at power-on). Some machines may need boot entries added by the Linux program efibootmgr, and some might only work with one UEFI application stored in the folder \\EFI\\BOOT\\ with the filename BOOTX64.EFI. In more inconvenient cases like these, it is probably easier to just boot from FAT32-formatted USB drives using the same \\EFI\\BOOT\\BOOTX64.EFI convention. If the UEFI firmware allows booting from them, CDs/DVDs and FAT/FAT16-formatted drives (like floppies) can be used with the same file/folder naming scheme, too.

## How to Build from Source  

Requires GCC 8.0.0 or later and Binutils 2.29.1 or later. I cannot make any guarantees whatsoever for earlier versions, especially with the number of compilation and linking flags used. I'd personally recommend using the same version as used for Simple-Kernel, as these two projects are designed to share the same compiler folder.  

***Windows:***  
1. Download and extract or clone this repository into a dedicated folder, preferably somewhere easy like C:\BareMetalx64

2. Download MinGW-w64 "x86_64-posix-seh" from https://sourceforge.net/projects/mingw-w64/ (click "Files" and scroll down - pay attention to the version numbers!).

3. Extract the archive into the "Backend" folder. As a check, "Backend/mingw-w64/bin" will exist if done correctly.

4. Open Windows PowerShell or the Command Prompt in the "Simple-UEFI-Bootloader" folder and type ".\Compile.bat"

    *That's it! It should compile and a binary called "BOOTX64.EFI" will be output into the "Backend" folder.*

***Mac:***  
1. Download and extract or clone this repository into a dedicated folder, preferably somewhere easy like ~/BareMetalx64

2. Install the latest MacPorts: https://www.macports.org/

3. In Terminal, get the MinGW-w64 package via "sudo port install mingw-w64" ("sudo port install x86_64-w64-mingw32-gcc" should also work if you don't want the 32-bit portion of MinGW-w64)

    NOTE: Make sure that MacPorts downloads a version using the correct GCC and Binutils! You may need to run "sudo port selfupdate" if you aren't freshly installing MacPorts before running the above install command.

4. Once MinGW-w64 is installed, open Terminal in the "Simple-UEFI-Bootloader" folder and run "./Compile-Mac.sh"

    *That's it! It should compile and a binary called "BOOTX64.EFI" will be output into the "Backend" folder.*

***Linux:***  
1. Download and extract or clone this repository into a dedicated folder, preferably somewhere easy like ~/BareMetalx64

2. If, in the terminal, "gcc --version" reports GCC 8.0.0 or later and "ld --version" reports 2.29.1 or later, do steps 2a, 2b, and 2c. Otherwise go to step 3.

    2a. Type "which gcc" in the terminal, and make a note of what it says (something like /usr/bin/gcc or /usr/local/bin/gcc)

    2b. Open Compile.sh in an editor of your choice (nano, gedit, vim, etc.) and set the GCC_FOLDER_NAME variable at the top to be the part before "bin" (e.g. /usr or /usr/local, without the last slash). Do the same thing for BINUTILS_FOLDER_NAME, except use the output of "which ld" to get the directory path preceding "bin" instead.

    2c. Now set the terminal to the Simple-UEFI-Bootloader folder and run "./Compile.sh", which should work and output BOOTX64.EFI in the Backend folder. *That's it!*

3. Looks like we need to build GCC & Binutils. Navigate to the "Backend" folder in terminal and do "git clone git://gcc.gnu.org/git/gcc.git" there. This will download a copy of the latest GCC, which is necessary for "static-pie" support (when combined with Binutils 2.29.1 or later, it allows  statically-linked, position-independent executables to be created; earlier versions do not). If that git link ever changes, you'll need to find wherever the official GCC git repository ran off to.

4. Once GCC has been cloned, in the cloned folder do "contrib/download_prerequisites" and then "./configure -v --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu --prefix=$PWD/../gcc-8 --enable-checking=release --enable-languages=c --disable-multilib"

    NOTE: If you want, you can enable other languages like c++, fortran, objective-c (objc), go, etc. with enable-languages. You can also change the name of the folder it will built into by changing --prefix=[desired folder]. The above command line will configure GCC to be made in a folder called gcc-8 inside the "Backend" folder. Be aware that --prefix requires an absolute path.

5. After configuration completes, do "make -j [twice the number of cores of your CPU]" and go eat lunch. Unfortunately, sometimes building the latest GCC produces build-time errors; I ran into an "aclocal-1.15" issue when building via Linux on Windows (fixed by installing the latest version of Ubuntu on Windows and using the latest autoconf).

6. Now just do "make install" and GCC will be put into the gcc-8 folder from step 4.

7. Next, grab binutils 2.29.1 or later from https://ftp.gnu.org/gnu/binutils/ and extract the archive to Backend.

8. In the extracted Binutils folder, do "mkdir build" and "cd build" before configuring with "../configure --prefix=$PWD/../binutils-binaries --enable-gold --enable-ld=default --enable-plugins --enable-shared --disable-werror"

    NOTE: The "prefix" flag means the same thing as GCC's.

9. Once configuration is finished, do "make -j [twice the number of CPU cores]" and go have dinner.

10. Once make is done making, do "make -k check" and do a crossword or something. There should be a very small number of errors, if any.

11. Finally, do "make install" to install the package into binutils-binaries. Congratulations, you've just built some of the biggest Linux sources ever! You can also safely delete the folders that were created by the cloning process now--they'll be several GBs at this point.

12. Open Compile.sh in an editor of your choice (nano, gedit, vim, etc.) and set the GCC_FOLDER_NAME variable at the top (e.g. gcc-8 without any slashes). Do the same thing for the BINUTILS_FOLDER_NAME, except use the binutils-binaries folder.

13. At long last, you should be able to run "./Compile.sh" from within the "Simple-UEFI-Bootloader" folder.

    *That's it! It should compile and a binary called "BOOTX64.EFI" will be output into the "Backend" folder.*

    For more information about building GCC and Binutils, see these: http://www.linuxfromscratch.org/blfs/view/cvs/general/gcc.html & http://www.linuxfromscratch.org/lfs/view/development/chapter06/binutils.html  

## Change Log

V2.3 (9/18/2019) - Added support for Macs that have both EFI 1.10 and UEFI GOP, as Apple ported UEFI GOP to some of its older systems. Tested as far back as a Late 2011 MacBook Pro, which was updated all the way to High Sierra 10.13.6 (High Sierra came with a couple firmware updates, putting it at Boot ROM version 87.0.0.0.0 and SMC version 1.69f4). If the Mac can boot with rEFIt, and rEFIt's "About rEFIt" tool states, "Screen Output: Graphics Output (UEFI)," then it should be compatible. Also added the ability to do relative relocations for ELF64 files. This allows more complex kernels compiled in ELF64 format to work more easily/with significantly less extensive modifications than before. Additionally, the compile script options have been updated and reordered for efficiency and optimization improvements, but the minimum required GCC version is now 8.0.0 for -static-pie support. Finally, fixed a memory map size calculation bug that has managed to exist since the very beginning--all it took were Apple's crazy memory maps to find it.

V2.2 (5/21/2019) - In Loader Params, the RSDP pointer has been changed to the Configuration Table pointer. This allows programs to use all available configuration tables, not just the ACPI ones. Also changed some of the initial print statements, added Number_of_ConfigTables and UEFI_Version to the loader parameters, and added a 90 second menu timer for the multi-GPU graphics device selection and the single GPU resolution selection menus. Also made graphics mode selection more consistent when using characters to denote modes >10. There's also what looks like a "startup screen" now, some of which was always there despite only being viewable on standard resolutions higher than 1024x768. This not-really-new screen has a (stoppable) 10-second timeout so that it doesn't really get in the way of anything. Oh, and those pesky Wsign-compare warnings when compiling debug binaries are gone now (switched to using ~0ULL, which really should've been used from the get-go instead of -1).  

V2.1 (4/24/2019) - Fixed a regression introduced in V1.4 (it isn't a bug with this code, it's a widespread issue with UEFI firmware): The ClearScreen function, which is just supposed to clear the screen to whatever the background color is and reset the cursor to (0,0), behaves wildly differently depending on firmware. Some video drivers don't reset cursor position, some do a whole video mode reset and set the video mode to 0, effectively destroying any other mode previously set, and some actually work correctly. I forgot about this and left calls to ClearScreen in, but they're gone now. An extra newline now takes the place of where ClearScreen use to be used. Also updated backend GNU-EFI to use the UEFI 2.x OpenProtocol() function instead of the now-archaic EFI 1.0 HandleProtocol() function (this loader was never intended for EFI 1.x devices anyway).

V2.0 (4/23/2019) - Added the need for Kernel64.txt in the same style as Kernelcmd.txt from V2.x of https://github.com/KNNSpeed/UEFI-Stub-Loader. With this major change, kernels don't need to be named Kernel64 anymore, a string of load options can be passed to kernels, and multi-booting on one machine is supported by way of using the machine's built-in UEFI boot manager. How to format Kernel64.txt has been added to both this document and the usage information in "Releases." Also added new loader params: ESP_Root_Device_Path, ESP_Root_Size, Kernel_Path, Kernel_Path_Size, Kernel_Options, Kernel_Options_Size.

V1.5 (4/10/2019) - Updated graphics output names to blacklist known erroneous drivers from claiming to be graphics devices, added loader params: Bootloader_MajorVersion, Bootloader_MinorVersion, Kernel_BaseAddress, Kernel_Pages, Memory_Map_Descriptor_Version, created ARM64 version (https://github.com/KNNSpeed/Simple-UEFI-Bootloader-ARM64).

V1.4 (4/1/2019) - Major updates to menu display (should work better with lower resolutions), graphics out device names now work, fixed underlying cause of the need for the "weird memory hack" (it's still useful for debugging, though it's now disabled when compiling non-debug versions). Updates from here on out will likely be to the Loader Params as needed by Simple-Kernel; this release should hopefully iron out the last of any kinks. It runs a little faster, now, too!

V1.3 (2/21/2019) - Added Memory_Map_Size and Memory_Map_Descriptor_Size to loader parameters. This is to prevent having to rely on potentially inconsistent techniques (particularly in really unlucky corner cases) to get the memory map size in the kernel. Also updated GNU-EFI backend to 3.0.9, updated the README format to be easier to follow, updated the license TL;DR to be shorter and clearer (i.e. made it actually a TL;DR)--note that the license terms have not changed, only the wording in the TL;DR.

V1.2 (1/8/2018) - Resolved a major issue that prevented larger SYSV ABI kernels (ELF, Mach-O) from running. As far as I can tell everything works properly and completely now.

V1.1 (1/7/2018) - Code organization and build system uploaded. Also fixed bugs.

V1.0 (1/5/2018) - Initial release. Fully featured for PE32+ images.

## Acknowledgements  

- [Nigel Croxon](https://sourceforge.net/u/noxorc/profile/) for [GNU-EFI](https://sourceforge.net/projects/gnu-efi/)
- [UEFI Forum](http://www.uefi.org/) for the [UEFI Specification Version 2.7 (Errata A)](http://www.uefi.org/sites/default/files/resources/UEFI%20Spec%202_7_A%20Sept%206.pdf), as well as for [previous UEFI 2.x specifications](http://www.uefi.org/specifications)
- [OSDev Wiki](http://wiki.osdev.org/Main_Page) for its wealth of available information
- [PhoenixWiki](http://wiki.phoenix.com/wiki/index.php/Category:UEFI) for very handy documentation on UEFI functions
- [Matthew Garrett](https://mjg59.dreamwidth.org/) for detailed information about UEFI firmware behavior (e.g. ["Getting started with UEFI development"](https://mjg59.dreamwidth.org/18773.html?thread=767573))
- [The GNU project](https://www.gnu.org/home.en.html) for [GCC](https://gcc.gnu.org/), a fantastic and versatile compiler, and [Binutils](https://www.gnu.org/software/binutils/), equally fantastic binary utilities
- [MinGW-w64](https://mingw-w64.org/doku.php) for porting GCC & Binutils to Windows
- [Mojca Miklavec](https://github.com/mojca) for [porting MinGW-w64 to MacPorts](https://github.com/macports/macports-ports/tree/master/cross/mingw-w64)
- [Matt Pietrek](https://blogs.msdn.microsoft.com/matt_pietrek/) for [extensive documentation on the Windows PE file format](https://msdn.microsoft.com/en-us/library/ms809762.aspx)
- Tool Interface Standard for [extensive documentation on ELF files](https://www.uclibc.org/docs/elf.pdf)
- [Apple Inc.](https://www.apple.com/) for [documentation on Mach-O files](https://developer.apple.com/library/content/documentation/DeveloperTools/Conceptual/MachOTopics/0-Introduction/introduction.html)
- [Low Level Bits](https://lowlevelbits.org/) for [further documentation on Mach-O files](https://lowlevelbits.org/parsing-mach-o-files/)
- [The rEFIt Project](http://refit.sourceforge.net/) for having figured out how to switch from graphics mode to text mode on Macs with EFI 1.10.
- [Bruno Bierbaumer](https://github.com/0xbb) for figuring out the [apple_set_os() function](https://github.com/0xbb/apple_set_os.efi) needed to prevent certain Macs' firmware from disabling graphics devices and causing boot issues on those Macs (like the Late 2013 MacBook Pro with discrete graphics, model MacBookPro11,3)
- [cyrozap](https://github.com/cyrozap) for noting ambiguity in licensing terms
