// Ori Dabush 212945760

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <unistd.h>

// different return codes
#define ERROR -1
#define IDENTICAL 1
#define DIFFERENT 2
#define SIMILAR 3

// num of bytes we read every iteration
#define BUFFER_SIZE 1024

// a macro for printing an error message
#define PRINT_ERROR(str) write(2, str, sizeof(str))

// macro for getting the difference between lowercase and uppercase characters
#define CASE_DIFF abs('a' - 'A')

// a macro to close a file and print an error message in error
#define CLOSE_FILE(fd) \
    if (close(fd) < 0) \
    PRINT_ERROR("Error in: close\n")

// a function to check if a char is a letter
int isLetter(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

// a function to check if two letters are equal (ignoring case)
int isSameLetter(char c1, char c2)
{
    return isLetter(c1) && isLetter(c2) && abs(c1 - c2) == CASE_DIFF;
}

// a function to copy numOfBytes from src to dst
void copyBuffer(char *dst, char *src, int numOfBytes)
{
    int i;
    for (i = 0; i < numOfBytes; ++i)
    {
        dst[i] = src[i];
    }
}

// the main - reciving two paths in argv and checks if the files in the paths are identical, similar or different.
int main(int argc, char *argv[])
{
    // amount of arguments check
    if (argc != 3)
    {
        return ERROR;
    }

    // trying to open the files (read only mode)
    int fd1 = open(argv[1], O_RDONLY);
    if (fd1 < 0)
    {
        PRINT_ERROR("Error in: open\n");
        return ERROR;
    }
    int fd2 = open(argv[2], O_RDONLY);
    if (fd2 < 0)
    {
        // closing the first file descriptor in case of an error
        CLOSE_FILE(fd1);
        PRINT_ERROR("Error in: open\n");
        return ERROR;
    }

    // if the paths are equal the files are identical
    if (strcmp(argv[1], argv[2]) == 0)
    {
        CLOSE_FILE(fd1);
        CLOSE_FILE(fd2);
        return IDENTICAL;
    }

    char buff1[BUFFER_SIZE], buff2[BUFFER_SIZE];
    // reading BUFFER_SIZE (1024) bytes from the files
    int bytesRead1 = read(fd1, buff1, BUFFER_SIZE);
    if (bytesRead1 < 0)
    {
        PRINT_ERROR("Error in: read\n");
        CLOSE_FILE(fd1);
        CLOSE_FILE(fd2);
        return ERROR;
    }
    int bytesRead2 = read(fd2, buff2, BUFFER_SIZE);
    if (bytesRead2 < 0)
    {
        PRINT_ERROR("Error in: read\n");
        CLOSE_FILE(fd1);
        CLOSE_FILE(fd2);
        return ERROR;
    }

    int isSimilar = 0, index1 = 0, index2 = 0;
    while (bytesRead1 > 0 && bytesRead2 > 0)
    {
        // comparing the current buffers
        while (index1 < bytesRead1 && index2 < bytesRead2)
        {
            char c1 = buff1[index1], c2 = buff2[index2];
            // idential
            if (c1 == c2)
            {
                ++index1;
                ++index2;
            }
            // similar
            else if (c1 == ' ' || c1 == '\n')
            {
                ++index1;
                isSimilar = 1;
            }
            else if (c2 == ' ' || c2 == '\n')
            {
                ++index2;
                isSimilar = 1;
            }
            else if (isSameLetter(c1, c2))
            {
                ++index1;
                ++index2;
                isSimilar = 1;
            }
            // different
            else
            {
                CLOSE_FILE(fd1);
                CLOSE_FILE(fd2);
                return DIFFERENT;
            }
        }

        // if the buffer didn't reached their end, copy the rest of them to the beggining and read to the remaining space
        if (index1 != bytesRead1)
        {
            copyBuffer(buff1, buff1 + index1, bytesRead1 - index1);
            index1 = bytesRead1 - index1;
        }
        else
        {
            index1 = 0;
        }
        if (index2 != bytesRead2)
        {
            copyBuffer(buff2, buff2 + index2, bytesRead2 - index2);
            index2 = bytesRead2 - index2;
        }
        else
        {
            index2 = 0;
        }
        // index1 and index2 are now the index where we should start read in
        bytesRead1 = read(fd1, buff1 + index1, BUFFER_SIZE - index1);
        if (bytesRead1 < 0)
        {
            PRINT_ERROR("Error in: read\n");
            CLOSE_FILE(fd1);
            CLOSE_FILE(fd2);
            return ERROR;
        }
        bytesRead1 += index1;
        bytesRead2 = read(fd2, buff2 + index2, BUFFER_SIZE - index2);
        if (bytesRead2 < 0)
        {
            PRINT_ERROR("Error in: read\n");
            CLOSE_FILE(fd1);
            CLOSE_FILE(fd2);
            return ERROR;
        }
        bytesRead2 += index2;

        index1 = 0;
        index2 = 0;
    }

    // check that the files didn't reached their endif

    // if both of the files reached their and, return a result based on the isSimilar flag
    if (bytesRead1 == 0 && bytesRead2 == 0)
    {
        CLOSE_FILE(fd1);
        CLOSE_FILE(fd2);
        if (isSimilar)
        {
            return SIMILAR;
        }
        return IDENTICAL;
    }
    // if only one file reached its end, return a result based on if the rest of the other file contains only spaces and line breaks or not

    if (bytesRead1 == 0)
    {
        CLOSE_FILE(fd1);
        do
        {
            while (index2 != bytesRead2)
            {
                if (buff2[index2] != ' ' && buff2[index2] != '\n')
                {
                    CLOSE_FILE(fd2);
                    return DIFFERENT;
                }
                ++index2;
            }
            bytesRead2 = read(fd2, buff2, BUFFER_SIZE);
            if (bytesRead2 < 0)
            {
                PRINT_ERROR("Error in: read\n");
                CLOSE_FILE(fd2);
                return ERROR;
            }
        } while (bytesRead2 > 0);
        if (bytesRead2 == 0)
        {
            CLOSE_FILE(fd2);
            return SIMILAR;
        }
        // keep reading fd2
    }

    if (bytesRead2 == 0)
    {
        CLOSE_FILE(fd2);
        do
        {
            while (index1 != bytesRead1)
            {
                if (buff1[index1] != ' ' && buff1[index1] != '\n')
                {
                    CLOSE_FILE(fd1);
                    return DIFFERENT;
                }
                ++index1;
            }
            bytesRead1 = read(fd1, buff1, BUFFER_SIZE);
            if (bytesRead1 < 0)
            {
                PRINT_ERROR("Error in: read\n");
                CLOSE_FILE(fd1);
                return ERROR;
            }
        } while (bytesRead1 > 0);
        if (bytesRead1 == 0)
        {
            CLOSE_FILE(fd1);
            return SIMILAR;
        }
        // keep reading fd1
    }
}