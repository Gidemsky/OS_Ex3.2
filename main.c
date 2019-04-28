
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <malloc.h>
#include <fcntl.h>
#include <dirent.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#define ERROR_SYS "Error in system call\n"

#define MAX_LINE 150
#define ARG_NUM 2
#define CONFIG_LINES 3
#define ERROR_STD 2
#define CSV_RESULT_FILE "results.csv"
#define FILE_COMPARETION_PROG "./comp.out"
#define MAX_NAME 30
#define MAX_GRADE_REASON 30
#define COMPILED_FILE "compiled.out"//TODO:
#define COMPILE_FILE_TO_RUN "./compiled.out"
#define OUTPUT_FILE "output.txt"
#define TIMEOUT_WAIT 5
#define FAIL_FLAG -1
#define NO_C_FILE_MSG "0,NO_C_FILE"
#define COMPILATION_ERROR_MSG "20,COMPILATION_ERROR"
#define TIMEOUT_MSG "40,TIMEOUT"
#define BAD_OUTPUT_MSG "60,BAD_OUTPUT"
#define SIMILAR_OUTPUT_MSG "80,SIMILAR_OUTPUT"
#define GREAT_JOB_MSG "100,GREAT_JOB"

typedef struct {
    char directory[MAX_LINE];
    char inputFile[MAX_LINE];
    char correctOutput[MAX_LINE];
    int result;
} Configuration;

typedef struct {
    char name[MAX_NAME];
    char grade[MAX_GRADE_REASON];
    char toString[MAX_NAME+MAX_GRADE_REASON];
}StudentResult;

typedef enum {TRUE, FALSE} Bool;//TODO:

/**
 * error print to stderr
 */
void errorExcecution(){
    write(ERROR_STD, ERROR_SYS, strlen(ERROR_SYS));
}

///**
// * check if failed
// * @param result - the sign
// */
//void checkIfFail(int result){
//    if (result== FAIL_SIGN){
//        write(ERR_STD,"Error in system call\n",ERR_LEN);
//        exit(FAIL_SIGN);
//    }
//}

/**
 * check the the path is correct
 * @param param - path
 */
void checkPath(char param[MAX_LINE]){
    if(strcmp(param, "") == 0){
        errorExcecution();
        exit(4);
    }
}

/**
 * check if the directory is correct
 * @param dir - the dir
 * @param students - for free
 */
void checkDir(DIR *dir, struct Configuration *configuration){
    if (dir ==  NULL){
        free(configuration);
        errorExcecution();
        exit(FAIL_FLAG);
    }
}

/**
 *
 * @param file
 * @return
 */
const char *CheckFileDot(const char *file) {
    const char *dot_sign = strrchr(file, '.');
    /**
     * the last line is the same like:
     * if(!dot_sign || dot_sign == file){
     * return "";
     * return dot_sign + 1;
     */
    return !dot_sign || dot_sign == file ? "" : dot_sign + 1;
}

/**
 * checkes if the file is c type
 * @param pDirent pointer to the
 * @return
 */
int IsCTypeFile(struct dirent *pDirent){
    if(pDirent->d_type == DT_UNKNOWN || pDirent->d_type == DT_REG){
        if (strcmp(CheckFileDot(pDirent->d_name), "c") == 0){
            return 1;
        }
    }
    return 0;
}

/**
 * combines two strings for one path using '/' sign
 * @param path
 * @param first_path
 * @param second_path
 */
void CreatePathString(char path[MAX_LINE], const char* first_path, const char* second_path){
    memset(path, 0, MAX_LINE);
    snprintf(path, MAX_LINE, "%s/%s", first_path, second_path);
}

/**
 * print all the corrent line to the CSV
 * @param student_name
 * @param grade
 * @param config
 */
void WriteResult(char* student_name, char* grade, const Configuration* const config){
    StudentResult res;
    strcpy(res.name, student_name);
    strcpy(res.grade, grade);
    strcpy(res.toString, res.name);
    strcat(res.toString, ",");
    strcat(res.toString, res.grade);
    strcat(res.toString, "\0");
    write(config->result, res.toString, strlen(res.toString));
    write(config->result, "\n", strlen("\n"));
}

/**
 * compares the result with the correct one
 * @param student_name - the path of the file is there
 * @param config - contains the correct output
 */
void CompareOutput(char* student_name, const Configuration* const config){
    int pid;
    char output[MAX_LINE];
    strcpy(output, config->correctOutput);
    pid = fork();
    if(pid < 0){
        exit(FAIL_FLAG);
    }
    if(pid == 0){
        char *args[] = {FILE_COMPARETION_PROG, OUTPUT_FILE, output, NULL};
        if (execvp(args[0], args) < 0) {
            errorExcecution();
            exit(FAIL_FLAG);
        }
    } else {
        int status;
        if (waitpid(pid, &status, 0) < 0) {
            errorExcecution();
            return;
        }
        if(WIFEXITED(status)){
            switch(WEXITSTATUS(status)){
                case 1:
                    WriteResult(student_name, GREAT_JOB_MSG, config);
                    break;
                case 2:
                    WriteResult(student_name, BAD_OUTPUT_MSG, config);
                    break;
                case 3:
                    WriteResult(student_name, SIMILAR_OUTPUT_MSG, config);
                    break;
                default:
                    return;
            }
        }
    }
}

/**
 * run the program after compiling. this function use the second pid
 * @param student_name
 * @param config
 */
void execute(char *student_name, const Configuration *const config){
    int pid = fork();
    if(pid < 0){
        exit(1);
    }
    if (pid != 0) {
        int status;
        sleep(TIMEOUT_WAIT);
        pid_t return_pid = waitpid(pid, &status, WNOHANG);
        if (return_pid < 0) {
            errorExcecution();
            return;
        } else if (return_pid == 0) {
            WriteResult(student_name, TIMEOUT_MSG, config);
            kill(pid, SIGKILL);
            return;
        } else if (return_pid == pid) {
            if (WIFEXITED(status)) {
                CompareOutput(student_name, config);
            }
        }
    } else {
        int out, in;
        in = open(config->inputFile, O_RDONLY);
        out = open(OUTPUT_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (in < 0 || out < 0){
            errorExcecution();
            exit(FAIL_FLAG);
        }
        if (dup2(in, STDIN_FILENO) < 0 || dup2(out, STDOUT_FILENO) < 0) {
            errorExcecution();
            exit(FAIL_FLAG);
        }
        if (execlp(COMPILE_FILE_TO_RUN, COMPILE_FILE_TO_RUN, NULL) < 0) {
            errorExcecution();
            exit(FAIL_FLAG);
        }
    }
}

/**
 * compile the file.c - the C file
 * @param filePath
 * @param student_name
 * @param config
 */
void makeCompilation(char *filePath, char *student_name, const Configuration *const config){
    int pid;
    char *args[] = {"gcc", "-o", COMPILED_FILE, filePath, NULL};
    pid = fork();
    if(pid < 0){
        errorExcecution();
        exit(FAIL_FLAG);
    }
    if (pid != 0) {
        int status;
        if (waitpid(pid, &status, WCONTINUED) < 0) {
            return;
        }
        if (status != 0) {
            WriteResult(student_name, COMPILATION_ERROR_MSG, config);
            return;
        } else {
            execute(student_name, config);
        }
    } else {
        execvp(args[0], args);
        errorExcecution();
        exit(FAIL_FLAG);
    }
}

/**
 *
 * @param dir_name
 * @param studentName
 * @param config
 * @param isExist
 */
void HandleStudentDir(char *dir_name, char *studentName, const Configuration* config, Bool* isExist){
    DIR *dir = opendir(dir_name);
    checkDir(dir, (struct Configuration *) config);
    struct dirent *item;
    while ((item = readdir(dir)) != NULL) {
        if (IsCTypeFile(item)) {
            *isExist = TRUE;
            char filePath[MAX_LINE];
            CreatePathString(filePath, dir_name, item->d_name);
            makeCompilation(filePath, studentName, config);
            closedir(dir);
            return;
        } else if(item->d_type == DT_DIR){
            if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0)
                continue;
            char path[MAX_LINE];
            CreatePathString(path, dir_name, item->d_name);
            HandleStudentDir(path, studentName, config, isExist);
        }
    }
    closedir(dir);
}

/**
 * Checks and handles the directorys
 * @param dir_name
 * @param config
 */
void handleDiractory(char *dir_name, const Configuration* config){
    DIR *dir = opendir(dir_name);
    checkDir(dir, (struct Configuration *) config);
    struct dirent *item;
    Bool exist;
    while ((item = readdir(dir)) != NULL) {
        if (item->d_type == DT_DIR) {
            if (strcmp(item->d_name, ".") == 0 || strcmp(item->d_name, "..") == 0) {
                continue;
            }
            exist = FALSE;
            char newDirPath[MAX_LINE];
            //init the newDirPath
            memset(newDirPath, 0, sizeof(newDirPath));
            CreatePathString(newDirPath, dir_name, item->d_name);
            HandleStudentDir(newDirPath, item->d_name, config, &exist);
            if (exist == FALSE) {
                WriteResult(item->d_name, NO_C_FILE_MSG, config);
            }
        }
    }
    closedir(dir);
}

/**
 * Initialize the Configuration struct from the Configuration file
 * @param buffer configuration's file contant.
 * @param config The destination to place in.
 */
void SetConfigData(char buffer[MAX_LINE * CONFIG_LINES], Configuration *config){
    char *line;
    char lines[CONFIG_LINES][MAX_LINE];
    int i = 0;
    line = strtok(buffer, "\n");
    while(line != NULL){
        strcpy(lines[i++], line);
        line = strtok(NULL, "\n");
        if(line!=NULL){
            checkPath(line);
        }
    }
    strcpy(config->directory, lines[0]);
    strcpy(config->inputFile, lines[1]);
    strcpy(config->correctOutput, lines[2]);
    config->result = open(CSV_RESULT_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (config->result < 0) {
        exit(1);
    }
}

/**
 * the main program run
 * @param file
 * @return program's return value
 */
int programHandler(char **file){
    char *configuration_file = file[1];
    int configFile = open(configuration_file, O_RDONLY);
    char contant[MAX_LINE * CONFIG_LINES];
    ssize_t bytesRead = read(configFile, contant, sizeof(contant));
    if(bytesRead < 0){
        errorExcecution();
        return 1;
    }
    Configuration* configuration = calloc(sizeof(Configuration), sizeof(char));
    SetConfigData(contant, configuration);
    handleDiractory(configuration->directory, configuration);
    //remove the used files
    if (unlink(OUTPUT_FILE) < 0) {
        errorExcecution();
    }
    if (unlink(COMPILED_FILE) < 0) {
        errorExcecution();
    }
    close(configuration->result);
    free(configuration);
    return 0;
}

/**
 * main function
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char* argv[]) {
    if(argc != ARG_NUM){
        errorExcecution();
        return 1;
    }
    return(programHandler(argv));
}