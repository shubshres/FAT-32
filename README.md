# FAT32
 Implemented a user space shell application that is capable of interpreting a FAT32 file system image. The utility must not corrupt the file system image and should be robust. No existing kernel code or any other FAT 32 utility code may be used in your program.


See Assignment 4 - FAT32.pdf for more assignment details


IMPLEMENTED COMMANDS
-----------------------------------------------------------
#open <image name>
-----------------------------------------------------------
 Opens fat32.img file with error handling

#close <filename>
-----------------------------------------------------------
 Closes fat32.img with error handling

#info
-----------------------------------------------------------
 Prints out values for: BPB_BytesPerSec BPB_SecPerClus BPB_RsvdSecCnt BPB_NumFATS BPB_FATSz32

#stat <filename>
-----------------------------------------------------------
 Print attributes and starting cluster number of the file or directory name.

#get <filename>
-----------------------------------------------------------
 Retrieves file from the FAT32 image and places it in your current working directory. 

 If it does not exist, program will print "Error: File not found."

#cd <folder>
-----------------------------------------------------------
 Changes directories similar to a bash shell.  

 Supports '..' to go back to previous directory
 
#ls
-----------------------------------------------------------
L ist directory contents.

#read <filename>
-----------------------------------------------------------
Reads from the given file at the position, in bytes, specified by the position parameter and output the number bytes specified. 
 
Byte is in size 1, use fseek and have the position be used as the offset. 
 
Number of bytes is the count to read.
