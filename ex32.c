// Ori Dabush 212945760

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// different return codes
#define URGENT_ERROR -2
#define ERROR -1
#define IDENTICAL 1
#define DIFFERENT 2
#define SIMILAR 3

// a macro for printing an error message
#define PRINT_ERROR(str) write(2, str, sizeof(str))

// maximum path length
#define MAX_PATH_LENGTH 150

// maximum configuration file length
#define MAX_CONF_FILE_LENGTH 450

// new line token
#define NEW_LINE "\n"

// name of the results file
#define RESULTS_FILE_NAME "./results.csv"
#define ERRORS_FILE_NAME "./errors.txt"
#define OUT_FILE_NAME "a.out"
#define COMP_OUT_FILE_NAME "./comp.out"

// timeout limit (in seconds)
#define MAX_RUN_TIME 5

// grades
#define NO_C_FILE_GRADE 0
#define COMPLIATION_ERROR_GRADE 10
#define TIMEOUT_GRADE 20
#define WRONG_GRADE 50
#define SIMILAR_GRADE 75
#define EXCELLENT_GRADE 100

// a function that turns a grade into a string of the grade and the reason
const char *gradeToString(int grade)
{
    switch (grade)
    {
    case NO_C_FILE_GRADE:
        return "0,NO_C_FILE\n";
    case COMPLIATION_ERROR_GRADE:
        return "10,COMPILATION_ERROR\n";
    case TIMEOUT_GRADE:
        return "20,TIMEOUT\n";
    case WRONG_GRADE:
        return "50,WRONG\n";
    case SIMILAR_GRADE:
        return "75,SIMILAR\n";
    case EXCELLENT_GRADE:
        return "100,EXCELLENT\n";
    default:
        return NULL;
    }
}

// returns 1 if the name is the name of a c file, 0 otherwise
int isCFile(char *name)
{
    int len = strlen(name);
    if (name != NULL && len > 2)
    {
        // assure the extension is .c
        return name[len - 2] == '.' && name[len - 1] == 'c';
    }
    return 0;
}

// a function that checks the files in the configuration file
DIR *checkInputFiles(char *students, char *input, char *output)
{
    DIR *dir = opendir(students);
    // check that students directory is valid (exist & is a directory)
    if (dir == NULL)
    {
        printf("Not a valid directory\n");
        exit(ERROR);
    }
    // check that input file is valid (exist & readable)
    if (access(input, F_OK | R_OK) != 0)
    {
        printf("Input file not exist\n");
        if (closedir(dir) < 0)
        {
            PRINT_ERROR("Error in: closedir\n");
        }
        exit(ERROR);
    }
    // check that output file is valid (exist & readable)
    if (access(output, F_OK | R_OK) != 0)
    {
        printf("Output file not exist\n");
        if (closedir(dir) < 0)
        {
            PRINT_ERROR("Error in: closedir\n");
        }
        exit(ERROR);
    }
    return dir;
}

// join a path to a directory and a file in the directory
void pathJoin(char *path, char *file)
{
    if (path != NULL && file != NULL)
    {
        if (path[strlen(path) - 1] != '/')
        {
            strcat(path, "/");
        }
        strcat(path, file);
    }
}

// executes ./comp.out
int compareFiles(char *file1, char *file2)
{
    int child_pid = fork();
    if (child_pid < 0)
    {
        // fork error
        PRINT_ERROR("Error in: fork\n");
        exit(ERROR);
    }
    else if (child_pid == 0)
    {
        // child
        char *args[] = {COMP_OUT_FILE_NAME, file1, file2, NULL};
        // execute comp.out with the given files as arguments
        execvp(COMP_OUT_FILE_NAME, args);
        exit(ERROR);
    }
    else
    {
        int status;
        // wait for the comp.out command to finish
        waitpid(child_pid, &status, 0);
        int compReturnCode = WEXITSTATUS(status);
        // get the grade based on the comp.out return value
        switch (compReturnCode)
        {
        case IDENTICAL:
            return EXCELLENT_GRADE;
        case DIFFERENT:
            return WRONG_GRADE;
        case SIMILAR:
            return SIMILAR_GRADE;
        default:
            return ERROR;
        }
    }
}

// run pathToOutFile with input redirection to the input file and error redirection to errorsFd, and compare the output to the output file
int runProgram(char *pathToOutFile, char *pathToStudentsDir, char *input, char *output, int errorsFd)
{
    // the temporary output file will be the a.out file with .txt extension
    char outputFile[MAX_PATH_LENGTH];
    strcpy(outputFile, pathToOutFile);
    strcat(outputFile, ".txt");
    int child_pid = fork();
    time_t start = time(NULL);
    if (child_pid < 0)
    {
        PRINT_ERROR("Error in: fork\n");
        exit(URGENT_ERROR);
    }
    else if (child_pid == 0)
    {
        // child
        // input redirection
        int inputFd = open(input, O_RDONLY);
        if (inputFd < 0)
        {
            PRINT_ERROR("Error in: open\n");
            exit(ERROR);
        }
        if (dup2(inputFd, 0) < 0)
        {
            close(inputFd);
            exit(ERROR);
        }
        if (close(inputFd) < 0)
        {
            exit(ERROR);
        }
        // output redirection
        int outputFd = open(outputFile, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        if (outputFd < 0)
        {
            exit(ERROR);
        }
        if (dup2(outputFd, 1) < 0)
        {
            close(outputFd);
            exit(ERROR);
        }
        if (close(outputFd) < 0)
        {
            exit(ERROR);
        }
        // error redirection
        if (dup2(errorsFd, 2) < 0)
        {
            exit(URGENT_ERROR);
        }
        // execute program (a.out)
        char *args[] = {pathToOutFile, NULL};
        execvp(pathToOutFile, args);
        exit(ERROR);
    }
    else
    {
        // father
        waitpid(child_pid, NULL, 0);
        // check that the program executed no more than 5 seconds (MAX_RUN_TIME)
        time_t end = time(NULL);
        if (end - start > MAX_RUN_TIME)
        {
            // remove the temporary output file
            if (remove(outputFile) < 0)
            {
                return ERROR;
            }
            return TIMEOUT_GRADE;
        }
        // compare the programs output with the wanted output using comp.out
        int res = compareFiles(outputFile, output);
        // remove the temporary output file
        if (remove(outputFile) < 0)
        {
            return ERROR;
        }
        return res;
    }
}

// get the c file, input, wanted output and errorsFd, compile and run the program and compare the output using comp.out
int gradeStudent(char *pathToCFile, char *pathToStudentsDir, char *input, char *output, int errorsFd)
{
    char pathToOutFile[MAX_PATH_LENGTH];
    strcpy(pathToOutFile, pathToStudentsDir);
    pathJoin(pathToOutFile, OUT_FILE_NAME);
    int child_pid = fork();
    if (child_pid < 0)
    {
        PRINT_ERROR("Error in: fork\n");
        return URGENT_ERROR;
    }
    else if (child_pid == 0)
    {
        // child
        char *args[] = {"gcc", pathToCFile, "-o", pathToOutFile, NULL};
        // errors redirection
        if (dup2(errorsFd, 2) < 0)
        {
            exit(URGENT_ERROR);
        }
        // compile the c file
        execvp("gcc", args); // we assume gcc is in PATH
        exit(ERROR);
    }
    else
    {
        // father
        int status;
        // wait for the compilation to end
        waitpid(child_pid, &status, 0);
        // get the gcc return code
        int gccReturnCode = WEXITSTATUS(status);
        if (gccReturnCode == ERROR)
        {
            // error in execvp
            return ERROR;
        }
        else if (gccReturnCode == URGENT_ERROR)
        {
            // error in dup2 with errorsFd
            return URGENT_ERROR;
        }
        else if (gccReturnCode != 0)
        {
            // compilation failed
            return COMPLIATION_ERROR_GRADE;
        }
        else
        {
            // compilation succeeded
            int grade = runProgram(pathToOutFile, pathToStudentsDir, input, output, errorsFd);
            if (remove(pathToOutFile) < 0)
            {
                return ERROR;
            }
            return grade;
        }
    }
}

// write to the file descriptor. returns 0 on success.
int tryWrite(int fd, const char *buffer, int size)
{
    if (write(fd, buffer, size) < 0)
    {
        return ERROR;
    }
    return 0;
}

// get the path to the students directory and input, output files and results and error file descriptors and
// compile every student's program, run it, compare the output and write the grade in results file and the errors
// in errors file.
void gradeStudents(char *students, char *input, char *output, int resultsFd, int errorsFd)
{
    // check that the paths are valid
    DIR *dir = checkInputFiles(students, input, output);

    struct dirent *student;
    char path[MAX_PATH_LENGTH];
    strcpy(path, students);

    int studentsLength = strlen(path);
    // iterate over the directory entries (students)
    while ((student = readdir(dir)) != NULL)
    {
        char *studentDirName = student->d_name;

        // check that the entry is not . or ..
        if (strcmp(studentDirName, ".") == 0 || strcmp(studentDirName, "..") == 0)
        {
            continue;
        }

        // get the full path to the entry
        pathJoin(path, studentDirName);

        // check that the entry is a directory and open it
        DIR *studentDir = opendir(path);
        if (studentDir != NULL)
        {
            struct dirent *entry;
            int grade = 0;
            // find the c file in the directory and get the grade
            while ((entry = readdir(studentDir)) != NULL)
            {
                if (isCFile(entry->d_name))
                {
                    char cFile[MAX_PATH_LENGTH];
                    strcpy(cFile, path);
                    pathJoin(cFile, entry->d_name);
                    grade = gradeStudent(cFile, path, input, output, errorsFd);
                    break;
                }
            }

            // check that there isn't an urgent error
            if (grade == URGENT_ERROR)
            {
                closedir(studentDir);
                if (closedir(dir) < 0)
                {
                    PRINT_ERROR("Error in: closedir\n");
                    return;
                }
            }

            if (grade != ERROR)
            {
                // write path,grade,reason to resultsFd
                if (tryWrite(resultsFd, studentDirName, strlen(studentDirName)) == ERROR)
                {
                    closedir(studentDir);
                    if (closedir(dir) < 0)
                    {
                        PRINT_ERROR("Error in: closedir\n");
                    }
                    return;
                }
                if (tryWrite(resultsFd, ",", 1) == ERROR)
                {
                    closedir(studentDir);
                    if (closedir(dir) < 0)
                    {
                        PRINT_ERROR("Error in: closedir\n");
                    }
                    return;
                }
                const char *gradeString = gradeToString(grade);
                if (gradeString != NULL)
                {
                    if (tryWrite(resultsFd, gradeString, strlen(gradeString)) == ERROR)
                    {
                        closedir(studentDir);
                        if (closedir(dir) < 0)
                        {
                            PRINT_ERROR("Error in: closedir\n");
                        }
                        return;
                    }
                }
            }

            // close the student's directory
            closedir(studentDir);
        }

        // cut the path to the students directory to only contain the main directory
        path[studentsLength] = '\0';
    }

    // close the main directory
    if (closedir(dir) < 0)
    {
        PRINT_ERROR("Error in: closedir\n");
    }
}

// the main function - recieves the configuration file as an argument, reads it and execute the gradeStudents function
int main(int argc, char *argv[])
{
    // check the number of arguments
    if (argc != 2)
    {
        return ERROR;
    }

    // open and read the configuration file
    int confFile = open(argv[1], O_RDONLY);
    if (confFile < 0)
    {
        PRINT_ERROR("Error in: open\n");
        return ERROR;
    }
    char buff[MAX_CONF_FILE_LENGTH];
    int bytesRead = read(confFile, buff, MAX_CONF_FILE_LENGTH);
    if (bytesRead < 0)
    {
        PRINT_ERROR("Error in: read\n");
        if (close(confFile) < 0)
        {
            PRINT_ERROR("Error in: close\n");
        }
        return ERROR;
    }
    char *students, *input, *output, *token;
    // split the configuraton file by lines
    token = strtok(buff, NEW_LINE);
    if (token == NULL)
    {
        if (close(confFile) < 0)
        {
            PRINT_ERROR("Error in: close\n");
        }
        return ERROR;
    }
    // first line is the path to students directory
    students = token;
    token = strtok(NULL, NEW_LINE);
    if (token == NULL)
    {
        if (close(confFile) < 0)
        {
            PRINT_ERROR("Error in: close\n");
        }
        return ERROR;
    }
    // second line is the path to the input file
    input = token;
    token = strtok(NULL, NEW_LINE);
    if (token == NULL)
    {
        if (close(confFile) < 0)
        {
            PRINT_ERROR("Error in: close\n");
        }
        return ERROR;
    }
    // third line is the path to the output file
    output = token;
    if (close(confFile) < 0)
    {
        PRINT_ERROR("Error in: close\n");
        return ERROR;
    }

    // open the results and errors files
    int resultsFd = open(RESULTS_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (resultsFd < 0)
    {
        PRINT_ERROR("Error in: open\n");
        return ERROR;
    }
    int errorsFd = open(ERRORS_FILE_NAME, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    if (errorsFd < 0)
    {
        PRINT_ERROR("Error in: open\n");
        if (close(resultsFd) < 0)
        {
            PRINT_ERROR("Error in: close\n");
        }
        return ERROR;
    }

    gradeStudents(students, input, output, resultsFd, errorsFd);

    // close the results and errors files
    if (close(resultsFd) < 0)
    {
        PRINT_ERROR("Error in: close\n");
    }
    if (close(errorsFd) < 0)
    {
        PRINT_ERROR("Error in: close\n");
    }
    return 0;
}