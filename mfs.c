/*
   Shubhayu Shrestha
   UTA ID: 1001724804

   Mohammed Ahmed
   UTA ID: 1001655176
*/

// The MIT License (MIT)
//
// Copyright (c) 2020 Trevor Bakker
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h> //
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h> //
#include <errno.h>
#include <string.h> //
#include <signal.h>
#include <stdint.h> //
#include <ctype.h>

#define MAX_NUM_ARGUMENTS 5

#define WHITESPACE " \t\n" // We want to split our command line up into tokens \
                           // so we need to define what delimits our tokens.   \
                           // In this case  white space                        \
                           // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255 // The maximum command-line size

FILE *fp; //file pointer - Had to move it because of NextLB

//info variables
int16_t BPB_BytsPerSec; //Bytes Per Sector
int8_t BPD_SecPerClus;  //Sectors Per Cluster
int16_t BPB_RsvdSecCnt; //Reserved Sector Count

int8_t BPB_NumFATS;  //Number of FATs
int32_t BPB_FATSz32; //count of sectors occupied by one FAT

//global variable for CD
int currDir; // initializing a current directory that we will utilize for the other functions
int GlobalOffsetValue = 0;
int previousOffset;

//Directory Entry
struct __attribute__((__packed__)) DirectoryEntry
{
  char DIR_Name[11];
  uint8_t DIR_Attr;
  uint8_t Unused1[8];
  uint16_t DIR_FirstClusterHigh;
  uint8_t Unused2[4];
  uint16_t DIR_FirstClusterLow;
  uint32_t DIR_FileSize;
};

struct DirectoryEntry dir[16];

//FUNCTIONS

// Name        :   LBA To Offset Function
// Parameters  :   The current sector number that points to a block of data
// Returns     :   The value of the address for that block of data
// Description :   Finds the starting address of a block of data given the sector number

int LBAToOffset(int32_t sector)
{

  return ((sector - 2) * BPB_BytsPerSec) + (BPB_BytsPerSec * BPB_RsvdSecCnt) + (BPB_NumFATS * BPB_FATSz32 * BPB_BytsPerSec);
}

// Name        :   NextLB
// Purpose     :   Given a logical block address, look up into the first FAT
//             :   and return the logical block address of the block in the file.
//             :   If there is no further blocks, then return -1

int16_t NextLB(uint32_t sector)
{
  uint32_t FATAddress = (BPB_BytsPerSec * BPB_RsvdSecCnt) + (sector * 4);
  int16_t val;
  fseek(fp, FATAddress, SEEK_SET);
  fread(&val, 2, 1, fp);
  return val;
}

int main()
{

  int isOpen = 0; //bool to check if file is open

  int rootDir; //address for root directory

  char *cmd_str = (char *)malloc(MAX_COMMAND_SIZE);

  while (1)
  {
    // Print out the mfs prompt
    printf("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while (!fgets(cmd_str, MAX_COMMAND_SIZE, stdin))
      ;

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str = strdup(cmd_str);

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while (((arg_ptr = strsep(&working_str, WHITESPACE)) != NULL) &&
           (token_count < MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup(arg_ptr, MAX_COMMAND_SIZE);
      if (strlen(token[token_count]) == 0)
      {
        token[token_count] = NULL;
      }
      token_count++;
    }

    //checks for null input
    // ex. if a user enters blank line into msh>
    if (token[0] != NULL)
    {
      //OPEN A FILE
      //checks if user entered open command
      if (strcmp(token[0], "open") == 0)
      {
        if (!(isOpen))
        {
          //open file for reading
          fp = fopen(token[1], "r");

          //checks if file was opened
          if (fp == NULL)
          {
            printf("\nError: File system image not found.\n");
          }
          else
          {
            isOpen = 1; //marks file as open

            //Bytes Per Sector - (starts at 11, 2 bytes)
            fseek(fp, 11, SEEK_SET);
            fread(&BPB_BytsPerSec, 2, 1, fp);

            //Sectors Per Cluster - (starts at 13, 1 byte)
            fseek(fp, 13, SEEK_SET);
            fread(&BPD_SecPerClus, 1, 1, fp);

            //Reserved Sector Count - (starts at 14, 2 bytes)
            fseek(fp, 14, SEEK_SET);
            fread(&BPB_RsvdSecCnt, 2, 1, fp);

            //Number of FATs - (starts at 16, 1 bytes)
            fseek(fp, 16, SEEK_SET);
            fread(&BPB_NumFATS, 1, 1, fp);

            //BPB_FATSz32 - (Starts at 36, 4 bytes)
            fseek(fp, 36, SEEK_SET);
            fread(&BPB_FATSz32, 4, 1, fp);

            // store the root address to root directory after we initialize each variable
            rootDir = (BPB_NumFATS * BPB_FATSz32 * BPB_BytsPerSec) + (BPB_RsvdSecCnt * BPB_BytsPerSec);

            // store the calculated rootDir as our currDir since that is our current directory.
            currDir = rootDir;

            fseek(fp, currDir, SEEK_SET);
            fread(&dir[0], sizeof(struct DirectoryEntry), 16, fp);
          }
        }
        else
        {
          printf("Error: File system image already open.\n"); // print out error
                                                              // message if image
                                                              // already open
        }
      }

      //CLOSE A FILE
      //checks if user entered  CLOSE command
      if (strcmp(token[0], "close") == 0)
      {

        if (isOpen)
        {
          fclose(fp); //close file

          isOpen = 0; //marks file as closed
        }
        else
        {
          printf("Error: File system not open.\n");
        }
      }

      //CHECK INFO
      //checks if user entered  INFO command
      if (strcmp(token[0], "info") == 0)
      {

        if (isOpen)
        {
          //Bytes Per Sector - (starts at 11, 2 bytes)
          printf("BPB_BytsPerSec:\t%d\t%x\n", BPB_BytsPerSec, BPB_BytsPerSec);

          //Sectors Per Cluster - (starts at 13, 1 byte)
          printf("BPD_SecPerClus:\t%d\t%x\n", BPD_SecPerClus, BPD_SecPerClus);

          //Reserved Sector Count - (starts at 14, 2 bytes)
          printf("BPB_RsvdSecCnt:\t%d\t%x\n", BPB_RsvdSecCnt, BPB_RsvdSecCnt);

          //Number of FATs - (starts at 16, 1 bytes)
          printf("BPB_NumFATS:\t%d\t%x\n", BPB_NumFATS, BPB_NumFATS);

          //BPB_FATSz32 - (Starts at 36, 4 bytes)
          printf("BPB_FATSz32:\t%d\t%x\n", BPB_FATSz32, BPB_FATSz32);
        }
        else
        {
          printf("\nError: File system image must be opened first.\n\n");
        }
      }

      //LS COMMAND
      if (strcmp(token[0], "ls") == 0)
      {
        if (isOpen)
        {
          //printing contents of directory
          int i;
          for (i = 0; i < 16; i++)
          {
            //we create a temp variable in order to add null terminator
            //to the end of the filename

            //check if the file is deleted
            if ((dir[i].DIR_Name[0] & 0xe5) != 0xe5)
            {
              char filename[12];
              strncpy(&filename[0], &dir[i].DIR_Name[0], 11);
              filename[11] = '\0';

              //only print if attribute is 0x01, 0x10, or 0x20
              if ((dir[i].DIR_Attr == 0x01) || (dir[i].DIR_Attr == 0x10) || (dir[i].DIR_Attr == 0x20))
              {
                printf("%s\n", filename);
              }
            }
          }
        }
        else
        {
          printf("\nError: File system image must be opened first.\n\n");
        }
      }

      //STAT
      //checks if user entered  STAT command
      if (strcmp(token[0], "stat") == 0)
      {

        if (isOpen)
        {
          //store second parameter entered (The file we want stats for

          char expanded_name[12];
          memset(expanded_name, ' ', 12);

          char *token2 = strtok(token[1], ".");

          strncpy(expanded_name, token2, strlen(token2));

          token2 = strtok(NULL, ".");

          if (token2)
          {
            strncpy((char *)(expanded_name + 8), token2, strlen(token2));
          }

          expanded_name[11] = '\0';

          int i;
          for (i = 0; i < 11; i++)
          {
            expanded_name[i] = toupper(expanded_name[i]);
          }

          //Look for expanded name inside of directory

          //first find address of the root directory
          fseek(fp, currDir, SEEK_SET);
          fread(&dir[0], sizeof(struct DirectoryEntry), 16, fp);

          printf("\n");

          //printing contents of directory

          int fileMatches = 0;
          for (i = 0; i < 16; i++)
          {
            //we create a temp variable in order to add null terminator
            //to the end of the filename
            char filename[12];
            strncpy(&filename[0], &dir[i].DIR_Name[0], 11);
            filename[11] = '\0';

            //checks if the filename matches
            if (strncmp(expanded_name, filename, 11) == 0)
            {
              fileMatches = 1;
              printf("File Size:\t\t%d\n", dir[i].DIR_FileSize);
              printf("First Cluster Low:\t%d\n", dir[i].DIR_FirstClusterLow);
              printf("DIR_ATTR:\t\t%d\n", dir[i].DIR_Attr);
              printf("First Cluster High:\t%d\n\n", dir[i].DIR_FirstClusterHigh);
            }
          }

          if (fileMatches == 0)
          {
            printf("Error: File not found.\n");
          }
        }
        else
        {
          printf("\nError: File system image must be opened first.\n\n");
        }
      }

      //GET
      //checks if user entered  GET command

      if (strcmp(token[0], "get") == 0)
      {
        if (isOpen)
        {
          int position;
          int numBytes;

          int offsetValue;
          int lowCluster;

          int fileData;

          FILE *ofp; 
          unsigned char buffer[512];

          char expanded_name[12];
          memset(expanded_name, ' ', 12);

          char *token2 = strtok(token[1], ".");

          strncpy(expanded_name, token2, strlen(token2));

          token2 = strtok(NULL, ".");

          if (token2)
          {
            strncpy((char *)(expanded_name + 8), token2, strlen(token2));
          }

          expanded_name[11] = '\0';

          int i;
          for (i = 0; i < 11; i++)
          {
            expanded_name[i] = toupper(expanded_name[i]);
          }

          // loop through the entire directory
          fseek(fp, currDir, SEEK_SET);
          fread(&dir[0], sizeof(struct DirectoryEntry), 16, fp);

          printf("\n");

          //printing contents of directory

          int fileMatches = 0;
          for (i = 0; i < 16; i++)
          {
            //we create a temp variable in order to add null terminator
            //to the end of the filename
            char filename[12];
            strncpy(&filename[0], &dir[i].DIR_Name[0], 11);
            filename[11] = '\0';

            //checks if the filename matches
            if (strncmp(expanded_name, filename, 11) == 0)
            {
              // marks that we found the value
              fileMatches = 1;

              ofp = fopen(token[1], "w");


              //Set the initial position and number of bytes
              position = 0;                   //starts at beginning
              numBytes = dir[i].DIR_FileSize; //entire file

              // store the low cluster value
              lowCluster = dir[i].DIR_FirstClusterLow;

              //calculate the offset value based on the low cluster value of the file
              offsetValue = LBAToOffset(lowCluster);

              // add the offset to the the position

              position = position + offsetValue;


              // initializing array that will store the file data
              int arrayInteger[numBytes];

              int i;
              for (i = 0; i <= numBytes; i++)
              {
                //fseek to the offset
                fseek(fp, position + i, SEEK_SET);


                //read the data
                fread(&fileData, 1, 1, fp);

                buffer[i] = fileData;

                fseek(ofp, position + i, SEEK_SET);
                //write
                fwrite(&buffer[i], 1, 1, ofp);

                //store value
                arrayInteger[i] = fileData;
              }

              printf("\n");
            }
          }
          if (fileMatches == 0)
          {
            printf("\nError: File not found\n"); 
          }
        }
        else
        {
          printf("\nError: File system image must be opened first.\n\n");
        }
      }
      //CD
      //checks if user entered  CD command
      if (strcmp(token[0], "cd") == 0)
      {

        if (isOpen)
        {

          int i;
          char expanded_name[12];

          if (!(strcmp(token[1], "..")) == 0)
          {
            memset(expanded_name, ' ', 12);

            char *token2 = strtok(token[1], ".");

            strncpy(expanded_name, token2, strlen(token2));

            token2 = strtok(NULL, ".");

            if (token2)
            {
              strncpy((char *)(expanded_name + 8), token2, strlen(token2));
            }

            expanded_name[11] = '\0';

            for (i = 0; i < 11; i++)
            {
              expanded_name[i] = toupper(expanded_name[i]);
            }
          }
          else
          {
            memset(expanded_name, ' ', 12);
            
            strcpy(expanded_name, "..");
            
            expanded_name[11] = '\0';

            if (dir[1].DIR_FirstClusterLow == 0)
            {
              //store address of current directory
              currDir = rootDir;
            }
            else
            {
              int dotCluster = dir[1].DIR_FirstClusterLow;

              // setting the offset
              GlobalOffsetValue = LBAToOffset(dotCluster);

              //store address of current directory
              currDir = GlobalOffsetValue;
            }

            // moving to that directory using fseek
            fseek(fp, currDir, SEEK_SET);

            // copy from fp into our directory array utilizing fread
            fread(&dir[0], sizeof(struct DirectoryEntry), 16, fp);
          }

          //Look for expanded name inside of directory

          //first find address of the root directory
          fseek(fp, currDir, SEEK_SET);
          
          fread(&dir[0], sizeof(struct DirectoryEntry), 16, fp);

          printf("\n");

          //printing contents of directory

          int fileMatches = 0;
          for (i = 0; i < 16; i++)
          {
            //we create a temp variable in order to add null terminator
            //to the end of the filename
            char filename[12];

            strncpy(&filename[0], &dir[i].DIR_Name[0], 11);
            
            filename[11] = '\0';


            //checks if the filename matches
            if (strncmp(expanded_name, filename, 11) == 0)
            {
              fileMatches = 1;

              // setting the cluster value
              int lowCluster = dir[i].DIR_FirstClusterLow;

              //set previous offset to global offset
              previousOffset = GlobalOffsetValue;

              // setting the offset
              GlobalOffsetValue = LBAToOffset(lowCluster);

              //store address of current directory
              currDir = GlobalOffsetValue;

              // moving to that directory using fseek
              fseek(fp, GlobalOffsetValue, SEEK_SET);
              
              // copy from fp into our directory array utilizing fread
              fread(&dir[0], sizeof(struct DirectoryEntry), 16, fp);
            }
          }

          if (fileMatches == 0 && !((strcmp(token[1], "..")) == 0))
          {
            printf("Error: File not found.\n");
          }
        }
        else
        {
          printf("\nError: File system image must be opened first.\n\n");
        }
      }

      //READ
      //checks if user entered  READ command
      if (strcmp(token[0], "read") == 0)
      {
        if (isOpen)
        {
          int position;
          int numBytes;

          int offsetValue;
          int lowCluster;

          int fileData;

          char expanded_name[12];
          memset(expanded_name, ' ', 12);

          char *token2 = strtok(token[1], ".");

          strncpy(expanded_name, token2, strlen(token2));

          token2 = strtok(NULL, ".");

          if (token2)
          {
            strncpy((char *)(expanded_name + 8), token2, strlen(token2));
          }

          expanded_name[11] = '\0';

          int i;
          for (i = 0; i < 11; i++)
          {
            expanded_name[i] = toupper(expanded_name[i]);
          }

          // storing position integer from command line into variable
          position = atoi(token[2]);

          // storing number of bytes from command line in variable
          numBytes = atoi(token[3]);

          // loop through the entire directory
          fseek(fp, currDir, SEEK_SET);
          fread(&dir[0], sizeof(struct DirectoryEntry), 16, fp);

          printf("\n");

          //printing contents of directory

          int fileMatches = 0;
          for (i = 0; i < 16; i++)
          {
            //we create a temp variable in order to add null terminator
            //to the end of the filename
            char filename[12];
            strncpy(&filename[0], &dir[i].DIR_Name[0], 11);
            filename[11] = '\0';

            //checks if the filename matches
            if (strncmp(expanded_name, filename, 11) == 0)
            {
              // marks that we found the value
              fileMatches = 1;

              // store the low cluster value
              lowCluster = dir[i].DIR_FirstClusterLow;

              //calculate the offset value based on the low cluster value of the file
              offsetValue = LBAToOffset(lowCluster);

              // add the offset to the the position
              position = position + offsetValue;

              // initializing array that will store the file data
              int fileDataArray[numBytes];

              int i;
              for (i = 0; i <= numBytes; i++)
              {
                //fseek to the offset
                fseek(fp, position + i, SEEK_SET);

                //fread the data
                fread(&fileData, 1, 1, fp);

                // store the file data into the array
                fileDataArray[i] = fileData;
              }

              // for loop that prints out the contents of the array in base 10
              printf("\nBase 10:\t");
              for (i = 0; i < numBytes; i++)
              {
                printf("%d ", fileDataArray[i]);
              }

              // for loop that prints out the contents of the array in char
              printf("\nCharacter:\t");
              for (i = 0; i < numBytes; i++)
              {
                printf("%c ", fileDataArray[i]);
              }

              // for loop that prints out the contents of the array in hex
              printf("\nHex:\t\t");
              for (i = 0; i < numBytes; i++)
              {
                printf("%x ", fileDataArray[i]);
              }
              printf("\n\n");
            }
          }
        }
        else
        {
          printf("\nError: File system image must be opened first.\n\n");
        }
      }

      free(working_root);
    }
  }
  return 0;
}
