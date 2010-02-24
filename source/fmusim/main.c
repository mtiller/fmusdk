/* ------------------------------------------------------------------------- 
 * main.c
 * Implements simulation of a single FMU instance using the forward Euler
 * method for numerical integration.
 * Command syntax: see printHelp()
 * Simulates the given FMU from t = 0 .. tEnd with fixed step size h and 
 * writes the computed solution to file 'result.csv'.
 * The CSV file (comma-separated values) may e.g. be plotted using 
 * OpenOffice Calc or Microsoft Excel. 
 * This progamm demonstrates basic use of an FMU.
 * Real applications may use advanced numerical solvers instead, means to 
 * exactly locate state events in time, graphical plotting utilities, support 
 * for coexecution of many FMUs, stepping and debug support, user control
 * of parameter and start values etc. 
 * All this is missing here.
 * Free libraries and tools used to implement this simulator:
 *  - eXpat 2.0.1 XML parser, see http://expat.sourceforge.net
 *  - 7z.exe 4.57 zip and unzip tool, see http://www.7-zip.org
 * Copyright 2010 QTronic GmbH. All rights reserved. 
 * -------------------------------------------------------------------------
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "main.h"

#define XML_FILE  "modelDescription.xml"
#define DLL_DIR   "binaries\\win32\\"
#define RESULT_FILE "result.csv"

static FMU fmu; // the fmu to simulate

static void printHelp(const char* fmusim) {
    printf("command syntax: %s <model.fmu> <tEnd> <h> <loggingOn> <csv separator>\n", fmusim);
    printf("   <model.fmu> .... path to FMU, relative to current dir or absolute, required\n");
    printf("   <tEnd> ......... end  time of simulation, optional, defaults to 1.0 sec\n");
    printf("   <h> ............ step size of simulation, optional, defaults to 0.1 sec\n");
    printf("   <loggingOn> .... 1 to activate logging,   optional, defaults to 0\n");
    printf("   <csv separator>. column separator char in csv file, optional, defaults to ';'\n");
}

int main(int argc, char *argv[]) {
    const char* fmuFileName;
    char* fmuPath;
    char* tmpPath;
    char* xmlPath;
    char* dllPath;
    
    // define default argument values
    double tEnd = 1.0;
    double h=0.1;
    int loggingOn = 0;
    char csv_separator = ';';

    // parse command line arguments
    if (argc>1) {
        fmuFileName = argv[1];
    }
    else {
        printf("error: no fmu file\n");
        printHelp(argv[0]);
        exit(EXIT_FAILURE);
    }
    if (argc>2) {
        if (sscanf(argv[2],"%lf", &tEnd) != 1) {
            printf("error: The given end time (%s) is not a number\n", argv[2]);
            exit(EXIT_FAILURE);
        }
    }
    if (argc>3) {
        if (sscanf(argv[3],"%lf", &h) != 1) {
            printf("error: The given stepsize (%s) is not a number\n", argv[3]);
            exit(EXIT_FAILURE);
        }
    }
    if (argc>4) {
        if (sscanf(argv[4],"%d", &loggingOn) != 1 || loggingOn<0 || loggingOn>1) {
            printf("error: The given logging flag (%s) is not boolean\n", argv[4]);
            exit(EXIT_FAILURE);
        }
    }
    if (argc>5) {
        if (strlen(argv[5]) != 1) {
            printf("error: The given CSV separator char (%s) is not valid\n", argv[5]);
            exit(EXIT_FAILURE);
        }
        csv_separator = argv[5][0];
    }
    if (argc>6) {
        printf("warning: Ignoring %d additional arguments: %s ...\n", argc-6, argv[6]);
        printHelp(argv[0]);
    }

    // get absolute path to FMU, NULL if not found
    fmuPath = getFmuPath(fmuFileName);
    if (!fmuPath) exit(EXIT_FAILURE);

    // unzip the FMU to the tmpPath directory
    tmpPath = getTmpPath();
    if (!unzip(fmuPath, tmpPath)) exit(EXIT_FAILURE);

    // parse tmpPath\modelDescription.xml
    xmlPath = calloc(sizeof(char), strlen(tmpPath) + strlen(XML_FILE) + 1);
    sprintf(xmlPath, "%s%s", tmpPath, XML_FILE);
    fmu.modelDescription = parse(xmlPath);
    free(xmlPath);
    if (!fmu.modelDescription) exit(EXIT_FAILURE);

    // load the FMU dll
    dllPath = calloc(sizeof(char), strlen(tmpPath) + strlen(DLL_DIR) 
            + strlen( getModelIdentifier(fmu.modelDescription)) +  strlen(".dll") + 1);
    sprintf(dllPath,"%s%s%s.dll", tmpPath, DLL_DIR, getModelIdentifier(fmu.modelDescription));
    if (!loadDll(dllPath, &fmu)) exit(EXIT_FAILURE); 
    free(dllPath);
    free(fmuPath);
    free(tmpPath);

    // run the simulation
    printf("FMU Simulator: run '%s' from t=0..%g with step size h=%g, loggingOn=%d, csv separator='%c'\n", 
            fmuFileName, tEnd, h, loggingOn, csv_separator);
    simulate(&fmu, tEnd, h, loggingOn, csv_separator);
    printf("CSV file '%s' written", RESULT_FILE);

    // release FMU 
    FreeLibrary(fmu.dllHandle);
    freeElement(fmu.modelDescription);
    return EXIT_SUCCESS;
}
