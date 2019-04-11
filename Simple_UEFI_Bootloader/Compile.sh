#!/bin/bash
#
# =================================
#
# RELEASE VERSION 1.1
#
# GCC UEFI Bootloader Linux Compile Script
#
# by KNNSpeed
#
# =================================
#

#
# set +v disables displaying all of the code you see here in the command line
#

set +v

#
# Convert Windows-style line endings (CRLF) to Unix-style line endings (LF)
#

perl -pi -e 's/\r\n/\n/g' c_files_linux.txt
perl -pi -e 's/\r\n/\n/g' h_files.txt

#
# Set various paths needed for portable compilation
#

CurDir=$PWD
GCC_FOLDER_NAME=gcc-linux
EFI_FOLDER_NAME=gnu-efi-3.0.9
BINUTILS_FOLDER_NAME=binutils-2.29.1-linux
LinkerScript="$EFI_FOLDER_NAME/gnuefi/elf_x86_64_efi.lds"

# So that GCC knows where to find as and ld
export PATH=$BINUTILS_FOLDER_NAME/bin:$PATH

#
# These help with debugging the PATH to make sure it is set correctly
#

# echo $PATH
# read -n1 -r -p "Press any key to continue..."

#
# Move into the Backend folder, where all the magic happens
#

cd ../Backend

#
# First things first, delete the objects list to rebuild it later
#

rm objects.list

#
# Create the HFILES variable, which contains the massive set of includes (-I)
# needed by GCC.
#
# Two of the include folders are always included, and they
# are $CurDir/inc/ (the user-header directory) and $CurDir/startup/
#

HFILES=-I$CurDir/inc/\ -I$CurDir/startup/

#
# Loop through the h_files.txt file and turn each include directory into -I strings
#

while read h; do
  HFILES=$HFILES\ -I$h
done < $CurDir/h_files.txt

#
# These are useful for debugging this script, namely to make sure you aren't
# missing any include directories.
#

# echo $HFILES
# read -n1 -r -p "Press any key to continue..."

#
# Loop through and compile the backend .c files, which are listed in c_files_linux.txt
#

set -v
while read f; do
  echo "$GCC_FOLDER_NAME/bin/gcc" -ffreestanding -fpic -fshort-wchar -fno-stack-protector -fno-stack-check -fno-strict-aliasing -fno-merge-all-constants -m64 -mno-red-zone -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args --std=c11 $HFILES -Og -g3 -Wall -Wextra -Wdouble-promotion -fmessage-length=0 -c -MMD -MP -Wa,-adhln="${f%.*}.out" -MF"${f%.*}.d" -MT"${f%.*}.o" -o "${f%.*}.o" "$f"
  "$GCC_FOLDER_NAME/bin/gcc" -ffreestanding -fpic -fshort-wchar -fno-stack-protector -fno-stack-check -fno-strict-aliasing -fno-merge-all-constants -m64 -mno-red-zone -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args --std=c11 $HFILES -Og -g3 -Wall -Wextra -Wdouble-promotion -fmessage-length=0 -c -MMD -MP -Wa,-adhln="${f%.*}.out" -MF"${f%.*}.d" -MT"${f%.*}.o" -o "${f%.*}.o" "$f"
done < $CurDir/c_files_linux.txt
set +v

#
# Compile the .c files in the startup folder
#

#set -v
#for f in $CurDir/startup/*.c; do
#  echo "$GCC_FOLDER_NAME/bin/gcc" -ffreestanding -fpic -fshort-wchar -fno-stack-protector -fno-stack-check -fno-strict-aliasing -fno-merge-all-constants -m64 -mno-red-zone -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args --std=c11 $HFILES -Og -g3 -Wall -Wextra -Wdouble-promotion -fmessage-length=0 -c -MMD -MP -Wa,-adhln="${f%.*}.out" -MF"${f%.*}.d" -MT"${f%.*}.o" -o "${f%.*}.o" "${f%.*}.c"
#  "$GCC_FOLDER_NAME/bin/gcc" -ffreestanding -fpic -fshort-wchar -fno-stack-protector -fno-stack-check -fno-strict-aliasing -fno-merge-all-constants -m64 -mno-red-zone -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args --std=c11 $HFILES -Og -g3 -Wall -Wextra -Wdouble-promotion -fmessage-length=0 -c -MMD -MP -Wa,-adhln="${f%.*}.out" -MF"${f%.*}.d" -MT"${f%.*}.o" -o "${f%.*}.o" "${f%.*}.c"
#done
#set +v

#
# Compile the .s files in the startup folder (Any assembly files needed to
# initialize the system)
#

#set -v
#for f in $CurDir/startup/*.s; do
#  echo "$BINUTILS_FOLDER_NAME/bin/as" -64 -I"$CurDir/inc/" -g -o "${f%.*}.o" "${f%.*}.s"
#  "$BINUTILS_FOLDER_NAME/bin/as" -64 -I"$CurDir/inc/" -g -o "${f%.*}.o" "${f%.*}.s"
#done
#set +v

#
# Compile user .c files
#

set -v
for f in $CurDir/src/*.c; do
  echo "$GCC_FOLDER_NAME/bin/gcc" -ffreestanding -fpic -fshort-wchar -fno-stack-protector -fno-stack-check -fno-strict-aliasing -fno-merge-all-constants -m64 -mno-red-zone -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args --std=c11 $HFILES -Og -g3 -Wall -Wextra -Wdouble-promotion -fmessage-length=0 -c -MMD -MP -Wa,-adhln="${f%.*}.out" -MF"${f%.*}.d" -MT"${f%.*}.o" -o "${f%.*}.o" "${f%.*}.c"
  "$GCC_FOLDER_NAME/bin/gcc" -ffreestanding -fpic -fshort-wchar -fno-stack-protector -fno-stack-check -fno-strict-aliasing -fno-merge-all-constants -m64 -mno-red-zone -DGNU_EFI_USE_MS_ABI -maccumulate-outgoing-args --std=c11 $HFILES -Og -g3 -Wall -Wextra -Wdouble-promotion -fmessage-length=0 -c -MMD -MP -Wa,-adhln="${f%.*}.out" -MF"${f%.*}.d" -MT"${f%.*}.o" -o "${f%.*}.o" "${f%.*}.c"
done
set +v

#
# Create the objects.list file, which contains properly-formatted (i.e. has
# forward slashes) locations of compiled Backend .o files
#

while read f; do
  echo "${f%.*}.o" | tee -a objects.list
done < $CurDir/c_files_linux.txt

#
# Add compiled .o files from the startup directory to objects.list
#

#for f in $CurDir/startup/*.o; do
#  echo "$f" | tee -a objects.list
#done

#
# Add compiled user .o files to objects.list
#

for f in $CurDir/src/*.o; do
  echo "$f" | tee -a objects.list
done

#
# Link the object files using all the objects in objects.list and a linker
# script (in the Backend/Linker directory) to generate the output binary, which
# is called "program.so"
#

set -v
"$BINUTILS_FOLDER_NAME/bin/ld" -T$LinkerScript -nostdlib --warn-common --no-undefined -znocombreloc -s -shared -Bsymbolic -Map=output.map -o "program.so" @"objects.list"
set +v
# Remove -s in the above command to keep debug symbols in the output binary.

#
# Create an EFI executable from program.so and output the program size
#

echo
echo Generating binary and Printing size information:
echo
"$BINUTILS_FOLDER_NAME/bin/objcopy" -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .rel.* -j .rela.* -j .rel* -j .rela* -j .reloc --target=efi-app-x86_64 "program.so" "BOOTX64.EFI"
"$BINUTILS_FOLDER_NAME/bin/size" "BOOTX64.EFI"
echo

#
# Return to the folder started from
#

cd $CurDir

#
# Prompt user for next action
#

read -p "Cleanup, recompile, or done? [c for cleanup, r for recompile, any other key for done] " UPL

echo
echo "**********************************************************"
echo

case $UPL in
  [cC])
    exec ./Cleanup.sh
  ;;
  [rR])
    exec ./Compile.sh
  ;;
  *)
  ;;
esac
