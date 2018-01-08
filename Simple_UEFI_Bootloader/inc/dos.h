//==================================================================================================================================
//  Simple UEFI Bootloader: DOS PSP Header
//==================================================================================================================================
//
// Author:
//  KNNSpeed
//
// Source Code:
//  https://github.com/KNNSpeed/Simple-UEFI-Bootloader
//
// Adapted from:
//  http://flint.cs.yale.edu/feng/research/BIOS/Resources/assembly/programsegmentprefix.html
//
// Maybe someone will find this useful.
//

#define DOS_EXECUTABLE 0x622

// PSP stands for "program segment prefix"
typedef struct _IMAGE_DOS_PSP {
  UINT16  psp_int20;
  UINT16  psp_seg_img_end;
  UINT8   psp_reserved1;
  char    psp_fardispatch[5]; // This array's [1] and [2] are also for CP/M .COM compatiblity
  UINT32  psp_terminate_address;
  UINT32  psp_ctrlbrk_address;
  UINT32  psp_criterr_address;
  UINT16  psp_parent_seg_addr;
  char    psp_reserved2[20];
  UINT16  psp_env_seg_addr;
  UINT32  psp_SS_SP_int21;
  UINT16  psp_handle_array;
  UINT32  psp_handle_array_ptr;
  UINT32  psp_ptr_to_prev_psp;
  char    psp_unused[20];
  char    psp_func_dispatch[3];
  char    psp_unused2[9];
  char    psp_FCB[36]; // FCB1, note that FCB2 is the last 20 bytes of this
  UINT8   psp_num_command_chars;
  char    psp_commandline[127]; // This is why the max number of command line characters is 127 in MS-DOS
} IMAGE_DOS_PSP, *PIMAGE_DOS_PSP;

#define IMAGE_SIZEOF_DOS_PSP 256

/*
From http://flint.cs.yale.edu/feng/research/BIOS/Resources/assembly/programsegmentprefix.html

Offset Size	      Description
  00   word	    machine code INT 20 instruction (CDh 20h)
  02   word	    top of memory in segment (paragraph) form
  04   byte	    reserved for DOS, usually 0
  05  5bytes	  machine code instruction long call to the DOS
                  function dispatcher (obsolete CP/M)
  06   word	    .COM programs bytes available in segment (CP/M)
  0A   dword	  INT 22 terminate address;  DOS loader jumps to this
                  address upon exit;  the EXEC function forces a child
                  process to return to the parent by setting this
                  vector to code within the parent (IP,CS)
  0E   dword	  INT 23 Ctrl-Break exit address; the original INT 23
                  vector is NOT restored from this pointer (IP,CS)
  12   dword	  INT 24 critical error exit address; the original
                  INT 24 vector is NOT restored from this field (IP,CS)
  16   word	    parent process segment addr (Undoc. DOS 2.x+)
                  COMMAND.COM has a parent id of zero, or its own PSP
  18  20bytes	  file handle array (Undocumented DOS 2.x+); if handle
                  array element is FF then handle is available.  Network
                  redirectors often indicate remotes files by setting
                  these to values between 80-FE.
  2C   word	    segment address of the environment, or zero (DOS 2.x+)
  2E   dword	  SS:SP on entry to last INT 21 function (Undoc. 2.x+) Ø
  32   word	    handle array size (Undocumented DOS 3.x+)
  34   dword	  handle array pointer (Undocumented DOS 3.x+)
  38   dword	  pointer to previous PSP (deflt FFFF:FFFF, Undoc 3.x+) Ø
  3C  20bytes	  unused in DOS before 4.01  Ø
  50   3bytes	  DOS function dispatcher CDh 21h CBh (Undoc. 3.x+) Ø
  53   9bytes	  unused
  5C  36bytes	  default unopened FCB #1 (parts overlayed by FCB #2)
  6C  20bytes	  default unopened FCB #2 (overlays part of FCB #1)
  80   byte	    count of characters in command tail;  all bytes
                  following command name;  also default DTA (128 bytes)
  81 127bytes	  all characters entered after the program name followed
                  by a CR byte

  - offset 5 contains a jump address which is 2 bytes too low for
    PSP's created by the DOS EXEC function in DOS 2.x+  Ø
  - program name and complete path can be found after the environment
    in DOS versions after 3.0.  See offset 2Ch.
  Ø see Bibliography for reference to "Undocumented DOS"

*/
