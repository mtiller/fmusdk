#include "fmuzip.h"

#include <string.h>
#include <stdlib.h>
#include <malloc.h>

#ifdef _MSC_VER
#include <windows.h>
#else
#include <unistd.h>
#endif

#define UNZIP_CMD "7z x -aoa -o"
#define BUFSIZE 4096

// return codes of the 7z command line tool
#define SEVEN_ZIP_NO_ERROR 0 // success
#define SEVEN_ZIP_WARNING 1  // e.g., one or more files were locked during zip
#define SEVEN_ZIP_ERROR 2
#define SEVEN_ZIP_COMMAND_LINE_ERROR 7
#define SEVEN_ZIP_OUT_OF_MEMORY 8
#define SEVEN_ZIP_STOPPED_BY_USER 255

#ifdef _MSC_VER
int fmuUnzip(const char *zipPath, const char *outPath) {
    int code;
    char cwd[BUFSIZE];
    char binPath[BUFSIZE];
    int n = strlen(UNZIP_CMD) + strlen(outPath) + 1 +  strlen(zipPath) + 9;
    char* cmd = (char*)calloc(sizeof(char), n);

    // remember current directory
    if (!GetCurrentDirectory(BUFSIZE, cwd)) {
        printf ("error: Could not get current directory: %s\n", strerror(GetLastError()));
        return 0; // error
    }
        
    // change to %FMUSDK_HOME%\bin to find 7z.dll and 7z.exe
    if (!GetEnvironmentVariable("FMUSDK_HOME", binPath, BUFSIZE)) {
        if (GetLastError() == ERROR_ENVVAR_NOT_FOUND) {
            printf ("error: Environment variable FMUSDK_HOME not defined.\n");
        }
        else {
            printf ("error: Could not get value of FMUSDK_HOME: %s\n",strerror(GetLastError()));
        }
        return 0; // error       
    }
#if WINDOWS
    strcat(binPath, "\\bin");
#else
    strcat(binPath, "/bin");
#endif
    if (!SetCurrentDirectory(binPath)) {
        printf ("error: could not change to directory '%s': %s\n", binPath, strerror(GetLastError())); 
        return 0; // error        
    }
   
    // run the unzip command
    // remove "> NUL" to see the unzip protocol
    sprintf(cmd, "%s%s \"%s\" > NUL", UNZIP_CMD, outPath, zipPath); 
    // printf("cmd='%s'\n", cmd);
    code = system(cmd);
    free(cmd);
    if (code!=SEVEN_ZIP_NO_ERROR) {
        switch (code) {
            printf("7z: ");
            case SEVEN_ZIP_WARNING:            printf("warning\n"); break;
            case SEVEN_ZIP_ERROR:              printf("error\n"); break;
            case SEVEN_ZIP_COMMAND_LINE_ERROR: printf("command line error\n"); break;
            case SEVEN_ZIP_OUT_OF_MEMORY:      printf("out of memory\n"); break;
            case SEVEN_ZIP_STOPPED_BY_USER:    printf("stopped by user\n"); break;
            default: printf("unknown problem\n");
        }
    }
    
    // restore current directory
    SetCurrentDirectory(cwd);
    
    return (code==SEVEN_ZIP_NO_ERROR || code==SEVEN_ZIP_WARNING) ? 1 : 0;  
}
#else
int fmuUnzip(const char *zipPath, const char *outPath) {
    int code;
    char cwd[BUFSIZE];
    char binPath[BUFSIZE];
    int n;
    char* cmd;

    // remember current directory
    if (!getcwd(cwd, BUFSIZE)) {
      printf ("error: Could not get current directory\n");
      return 0; // error
    }
        
    const char *FMUSDK_HOME = getenv("FMUSDK_HOME");
    // change to %FMUSDK_HOME%\bin to find 7z.dll and 7z.exe
    if (FMUSDK_HOME==NULL) {
      printf ("warning: Could not get value of FMUSDK_HOME, assuming 7zip is in your path.\n");
      FMUSDK_HOME = strdup("");
    } else {
#if WINDOWS
        strcat(binPath, "\\bin");
#else
        strcat(binPath, "/bin");
#endif
	if (!chdir(binPath)) {
	    printf ("error: could not change to directory '%s'\n", binPath);
	    return 0; // error        
	}
    }
   
    // run the unzip command
#if WINDOWS
    n = strlen(UNZIP_CMD) + strlen(outPath) + 1 +  strlen(zipPath) + 9;
    cmd = (char*)calloc(sizeof(char), n);
    // remove "> NUL" to see the unzip protocol
    sprintf(cmd, "%s%s \"%s\" > NUL", UNZIP_CMD, outPath, zipPath); 
#else
    n = strlen(UNZIP_CMD) + strlen(outPath) + 1 +  strlen(zipPath) + 16;
    cmd = (char*)calloc(sizeof(char), n);
    sprintf(cmd, "%s%s \"%s\" > /dev/null", UNZIP_CMD, outPath, zipPath); 
#endif
    printf("cmd='%s'\n", cmd);
    code = system(cmd);
    free(cmd);
    if (code!=SEVEN_ZIP_NO_ERROR) {
        switch (code) {
            printf("7z: ");
            case SEVEN_ZIP_WARNING:            printf("warning\n"); break;
            case SEVEN_ZIP_ERROR:              printf("error\n"); break;
            case SEVEN_ZIP_COMMAND_LINE_ERROR: printf("command line error\n"); break;
            case SEVEN_ZIP_OUT_OF_MEMORY:      printf("out of memory\n"); break;
            case SEVEN_ZIP_STOPPED_BY_USER:    printf("stopped by user\n"); break;
            default: printf("unknown problem\n");
        }
    }
    
    // restore current directory
    chdir(cwd);
    
    return (code==SEVEN_ZIP_NO_ERROR || code==SEVEN_ZIP_WARNING) ? 1 : 0;  
}
#endif
