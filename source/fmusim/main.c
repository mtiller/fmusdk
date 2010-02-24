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

#define BUFSIZE 4096
#define UNZIP_CMD "7z x -aoa -o"
#define XML_FILE  "modelDescription.xml"
#define DLL_DIR   "binaries\\win32\\"
#define RESULT_FILE "result.csv"

// return codes of the 7z command line tool
#define SEVEN_ZIP_NO_ERROR 0 // success
#define SEVEN_ZIP_WARNING 1  // e.g., one or more files were locked during zip
#define SEVEN_ZIP_ERROR 2
#define SEVEN_ZIP_COMMAND_LINE_ERROR 7
#define SEVEN_ZIP_OUT_OF_MEMORY 8
#define SEVEN_ZIP_STOPPED_BY_USER 255

static FMU fmu; // the fmu to simulate

static int unzip(const char *zipPath, const char *outPath) {
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
    strcat(binPath, "\\bin");
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

// fmuFileName is an absolute path, e.g. "C:\test\a.fmu"
// or relative to the current dir, e.g. "..\test\a.fmu"
static char* getFmuPath(const char* fmuFileName){
    OFSTRUCT fileInfo;
    if (HFILE_ERROR==OpenFile(fmuFileName, &fileInfo, OF_EXIST)) {
        printf ("error: Could not open FMU '%s': %s\n", fmuFileName, strerror(GetLastError()));
        return NULL;
    }
    //printf ("full path to FMU: '%s'\n", fileInfo.szPathName); 
    return strdup(fileInfo.szPathName);
}

static char* getTmpPath() {
    char tmpPath[BUFSIZE];
    if(! GetTempPath(BUFSIZE, tmpPath)) {
        printf ("error: Could not find temporary disk space: %d\n", strerror(GetLastError()));
        return NULL;
    }
    strcat(tmpPath, "fmu\\");
    return strdup(tmpPath);
}

static void* getAdr(FMU *fmu, const char* functionName){
    char name[BUFSIZE];
    void* fp;
    sprintf(name, "%s_%s", getModelIdentifier(fmu->modelDescription), functionName);
    fp = GetProcAddress(fmu->dllHandle, name);
    if (!fp) {
        printf ("error: Function %s not found in dll\n", name);        
    }
    return fp;
}

// Load the given dll and set function pointers in fmu
static int loadDll(const char* dllPath, FMU *fmu) {
    HANDLE h = LoadLibrary(dllPath);
    if (!h) {
        printf("error: Could not load %s: %s\n", dllPath, strerror(GetLastError()));
        return 0; // failure
    }
    fmu->dllHandle = h;
    fmu->getModelTypesPlatform   = (fGetModelTypesPlatform) getAdr(fmu, "fmiGetModelTypesPlatform");
    fmu->getVersion              = (fGetVersion)         getAdr(fmu, "fmiGetVersion");
    fmu->instantiateModel        = (fInstantiateModel)   getAdr(fmu, "fmiInstantiateModel");
    fmu->freeModelInstance       = (fFreeModelInstance)  getAdr(fmu, "fmiFreeModelInstance");
    fmu->setDebugLogging         = (fSetDebugLogging)    getAdr(fmu, "fmiSetDebugLogging");
    fmu->setTime                 = (fSetTime)            getAdr(fmu, "fmiSetTime");
    fmu->setContinuousStates     = (fSetContinuousStates)getAdr(fmu, "fmiSetContinuousStates");
    fmu->completedIntegratorStep = (fCompletedIntegratorStep)getAdr(fmu, "fmiCompletedIntegratorStep");
    fmu->setReal                 = (fSetReal)            getAdr(fmu, "fmiSetReal");
    fmu->setInteger              = (fSetInteger)         getAdr(fmu, "fmiSetInteger");
    fmu->setBoolean              = (fSetBoolean)         getAdr(fmu, "fmiSetBoolean");
    fmu->setString               = (fSetString)          getAdr(fmu, "fmiSetString");
    fmu->initialize              = (fInitialize)         getAdr(fmu, "fmiInitialize");
    fmu->getDerivatives          = (fGetDerivatives)     getAdr(fmu, "fmiGetDerivatives");
    fmu->getEventIndicators      = (fGetEventIndicators) getAdr(fmu, "fmiGetEventIndicators");
    fmu->getReal                 = (fGetReal)            getAdr(fmu, "fmiGetReal");
    fmu->getInteger              = (fGetInteger)         getAdr(fmu, "fmiGetInteger");
    fmu->getBoolean              = (fGetBoolean)         getAdr(fmu, "fmiGetBoolean");
    fmu->getString               = (fGetString)          getAdr(fmu, "fmiGetString");
    fmu->eventUpdate             = (fEventUpdate)        getAdr(fmu, "fmiEventUpdate");
    fmu->getContinuousStates     = (fGetContinuousStates)getAdr(fmu, "fmiGetContinuousStates");
    fmu->getNominalContinuousStates = (fGetNominalContinuousStates)getAdr(fmu, "fmiGetNominalContinuousStates");
    fmu->getStateValueReferences = (fGetStateValueReferences)getAdr(fmu, "fmiGetStateValueReferences");
    fmu->terminate               = (fTerminate)          getAdr(fmu, "fmiTerminate");
    return 1; // success  
}

static void doubleToCommaString(char* buffer, double r){
    char* comma;
    sprintf(buffer, "%.16g", r);
    comma = strchr(buffer, '.');
    if (comma) *comma = ',';
}

// output time and all non-alias variables in CSV format
// if separator is ',', columns are separated by ',' and '.' is used for floating-point numbers.
// otherwise, the given separator (e.g. ';' or '\t') is to separate columns, and ',' is used for 
// floating-point numbers.
static void outputRow(FMU *fmu, fmiComponent c, double time, FILE* file, char separator, boolean header) {
    int k;
    fmiReal r;
    fmiInteger i;
    fmiBoolean b;
    fmiString s;
    fmiValueReference vr;
    ScalarVariable** vars = fmu->modelDescription->modelVariables;
    char buffer[32];
    
    // print first column
    if (header) 
        fprintf(file, "time"); 
    else {
        if (separator==',') 
            fprintf(file, "%.16g", time);
        else {
            // separator is e.g. ';' or '\t'
            doubleToCommaString(buffer, time);
            fprintf(file, "%s", buffer);       
        }
    }
    
    // print all other columns
    for (k=0; vars[k]; k++) {
        ScalarVariable* sv = vars[k];
        if (getAlias(sv)!=enu_noAlias) continue;
        if (header) {
            // output names only
            fprintf(file, "%c%s", separator, getName(sv));
        }
        else {
            // output values
            vr = getValueReference(sv);
            switch (sv->typeSpec->type){
                case elm_Real:
                    fmu->getReal(c, &vr, 1, &r);
                    if (separator==',') 
                        fprintf(file, ",%.16g", r);
                    else {
                        // separator is e.g. ';' or '\t'
                        doubleToCommaString(buffer, r);
                        fprintf(file, "%c%s", separator, buffer);       
                    }
                    break;
                case elm_Integer:
                    fmu->getInteger(c, &vr, 1, &i);
                    fprintf(file, "%c%d", separator, i);
                    break;
                case elm_Boolean:
                    fmu->getBoolean(c, &vr, 1, &b);
                    fprintf(file, "%c%d", separator, b);
                    break;
                case elm_String:
                    fmu->getString(c, &vr, 1, &s);
                    fprintf(file, "%c%s", separator, s);
                    break;
            }
        }
    } // for
    
    // terminate this row
    fprintf(file, "\n"); 
}

static const char* fmiStatusToString(fmiStatus status){
    switch (status){
        case fmiOK:      return "ok";
        case fmiWarning: return "warning";
        case fmiDiscard: return "discard";
        case fmiError:   return "error";
        case fmiFatal:   return "fatal";
        default:         return "?";
    }
}

// search a fmu for the given variable
// return NULL if not found or vr = fmiUndefinedValueReference
static ScalarVariable* getSV(FMU* fmu, char type, fmiValueReference vr) {
    int i;
    Elm tp;
    ScalarVariable** vars = fmu->modelDescription->modelVariables;
    if (vr==fmiUndefinedValueReference) return NULL;
    switch (type) {
        case 'r': tp = elm_Real;    break;
        case 'i': tp = elm_Integer; break;
        case 'b': tp = elm_Boolean; break;
        case 's': tp = elm_String;  break;                
    }
    for (i=0; vars[i]; i++) {
        ScalarVariable* sv = vars[i];
        if (vr==getValueReference(sv) && tp==sv->typeSpec->type) 
            return sv;
    }
    return NULL;
}

// replace e.g. #r1365# by variable name and ## by # in message
// copies the result to buffer
static void replaceRefsInMessage(const char* msg, char* buffer, int nBuffer, FMU* fmu){
    int i=0; // position in msg
    int k=0; // position in buffer
    int n;
    char c = msg[i];
    while (c!='\0' && k < nBuffer) {
        if (c!='#') {
            buffer[k++]=c;
            i++;
            c = msg[i];
        }
        else {
            char* end = strchr(msg+i+1, '#');
            if (!end) {
                printf("unmatched '#' in '%s'\n", msg);
                buffer[k++]='#';
                break;
            }
            n = end - (msg+i);
            if (n==1) {
                // ## detected, output #
                buffer[k++]='#';
                i += 2;
                c = msg[i];
            }
            else {
                char type = msg[i+1]; // one of ribs
                fmiValueReference vr;
                int nvr = sscanf(msg+i+2, "%u", &vr);
                if (nvr==1) {
                    // vr of type detected, e.g. #r12#
                    ScalarVariable* sv = getSV(fmu, type, vr);
                    const char* name = sv ? getName(sv) : "?";
                    sprintf(buffer+k, "%s", name);
                    k += strlen(name);
                    i += (n+1);
                    c = msg[i]; 
                }
                else {
                    // could not parse the number
                    printf("illegal value reference at position %d in '%s'\n", i+2, msg);
                    buffer[k++]='#';
                    break;
                }
            }
        }
    } // while
    buffer[k] = '\0';
}

#define MAX_MSG_SIZE 1000
static void fmuLogger(fmiComponent c, fmiString instanceName, fmiStatus status,
               fmiString category, fmiString message, ...) {
    char msg[MAX_MSG_SIZE];
    char* copy;
    va_list argp;

    // replace C format strings
	va_start(argp, message);
    vsprintf(msg, message, argp);

    // replace e.g. ## and #r12#  
    copy = strdup(msg);
    replaceRefsInMessage(copy, msg, MAX_MSG_SIZE, &fmu);
    free(copy);
    
    // print the final message
    if (!instanceName) instanceName = "?";
    if (!category) category = "?";
    printf("%s %s (%s): %s\n", fmiStatusToString(status), instanceName, category, msg);
}

static int error(const char* message){
    printf("%s\n", message);
    return 0;
}


// simulate the given FMU using the forward euler method.
// time events are processed by reducing step size to exactly hit tNext.
// state events are checked and fired only at the end of an Euler step. 
// the simulator may therefore miss state events and fires state events typically too late.
static int simulate(FMU* fmu, double tEnd, double h, fmiBoolean loggingOn, char separator) {
    int i, n;
    double dt, tPre;
    fmiBoolean timeEvent, stateEvent, stepEvent;
    double time;  
    int nx;                          // number of state variables
    int nz;                          // number of state event indicators
    double *x;                       // continuous states
    double *xdot;                    // the crresponding derivatives in same order
    double *z = NULL;                // state event indicators
    double *prez = NULL;             // previous values of state event indicators
    fmiEventInfo eventInfo;          // updated by calls to initialize and eventUpdate
    ModelDescription* md;            // handle to the parsed XML file        
    const char* guid;                // global unique id of the fmu
    fmiCallbackFunctions callbacks;  // called by the model during simulation
    fmiComponent c;                  // instance of the fmu 
    fmiStatus fmiFlag;               // return code of the fmu functions
    fmiReal t0 = 0;                  // start time
    fmiBoolean toleranceControlled = fmiFalse;
    int nSteps = 0;
    int nTimeEvents = 0;
    int nStepEvents = 0;
    int nStateEvents = 0;
    FILE* file;

    // instantiate the fmu
    md = fmu->modelDescription;
    guid = getString(md, att_guid);
    callbacks.logger = fmuLogger;
    callbacks.allocateMemory = calloc;
    callbacks.freeMemory = free;
    c = fmu->instantiateModel(getModelIdentifier(md), guid, callbacks, loggingOn);
    if (!c) return error("could not instantiate model");
    
    // allocate memory 
    nx = getNumberOfStates(md);
    nz = getNumberOfEventIndicators(md);
    x    = (double *) calloc(nx, sizeof(double));
    xdot = (double *) calloc(nx, sizeof(double));
    if (nz>0) {
        z    =  (double *) calloc(nz, sizeof(double));
        prez =  (double *) calloc(nz, sizeof(double));
    }
    if (!x || !xdot || nz>0 && (!z || !prez)) return error("out of memory");

    // open result file
    if (!(file=fopen(RESULT_FILE, "w"))) {
        printf("could not write %s because:\n", RESULT_FILE);
        printf("    %s\n", strerror(errno));
        return 0; // failure
    }
        
    // set the start time and initialize
    time = t0;
    fmiFlag =  fmu->setTime(c, t0);
    if (fmiFlag > fmiWarning) return error("could not set time");
    fmiFlag =  fmu->initialize(c, toleranceControlled, t0, &eventInfo);
    if (fmiFlag > fmiWarning)  error("could not initialize model");
    if (eventInfo.terminateSimulation) {
        printf("model requested termination at init");
        tEnd = time;
    }
  
    // output solution for time t0
    outputRow(fmu, c, t0, file, separator, TRUE);  // output column names
    outputRow(fmu, c, t0, file, separator, FALSE); // output values

    // enter the simulation loop
    while (time < tEnd) {
     // get current state and derivatives
     fmiFlag = fmu->getContinuousStates(c, x, nx);
     if (fmiFlag > fmiWarning) return error("could not retrieve states");
     fmiFlag = fmu->getDerivatives(c, xdot, nx);
     if (fmiFlag > fmiWarning) return error("could not retrieve derivatives");

     // advance time
     tPre = time;
     time = min(time+h, tEnd);
     timeEvent = eventInfo.upcomingTimeEvent && eventInfo.nextEventTime < time;     
     if (timeEvent) time = eventInfo.nextEventTime;
     dt = time - tPre; 
     fmiFlag = fmu->setTime(c, time);
     if (fmiFlag > fmiWarning) error("could not set time");

     // perform one step
     for (i=0; i<nx; i++) x[i] += dt*xdot[i]; // forward Euler method
     fmiFlag = fmu->setContinuousStates(c, x, nx);
     if (fmiFlag > fmiWarning) return error("could not set states");
     if (loggingOn) printf("Step %d to t=%.16g\n", nSteps, time);
    
     // Check for step event, e.g. dynamic state selection
     fmiFlag = fmu->completedIntegratorStep(c, &stepEvent);
     if (fmiFlag > fmiWarning) return error("could not complete intgrator step");

     // Check for state event
     for (i=0; i<nz; i++) prez[i] = z[i]; 
     fmiFlag = fmu->getEventIndicators(c, z, nz);
     if (fmiFlag > fmiWarning) return error("could not retrieve event indicators");
     stateEvent = FALSE;
     for (i=0; i<nz; i++) 
         stateEvent = stateEvent || (prez[i] * z[i] < 0);  
     
     // handle events
     if (timeEvent || stateEvent || stepEvent) {
        
        if (timeEvent) {
            nTimeEvents++;
            if (loggingOn) printf("time event at t=%.16g\n", time);
        }
        if (stateEvent) {
            nStateEvents++;
            if (loggingOn) for (i=0; i<nz; i++)
                printf("state event %s z[%d] at t=%.16g\n", 
                        (prez[i]>0 && z[i]<0) ? "-\\-" : "-/-", i, time);
        }
        if (stepEvent) {
            nStepEvents++;
            if (loggingOn) printf("step event at t=%.16g\n", time);
        }

        // event iteration in one step, ignoring intermediate results
        fmiFlag = fmu->eventUpdate(c, fmiFalse, &eventInfo);
        if (fmiFlag > fmiWarning) return error("could not perform event update");
        
        // terminate simulation, if requested by the model
        if (eventInfo.terminateSimulation) {
            printf("model requested termination at t=%.16g\n", time);
            break; // success
        }

        // check for change of value of states
        if (eventInfo.stateValuesChanged && loggingOn) {
            printf("state values changed at t=%.16g\n", time);
        }
        
        // check for selection of new state variables
        if (eventInfo.stateValueReferencesChanged && loggingOn) {
            printf("new state variables selected at t=%.16g\n", time);
        }
       
     } // if event
     outputRow(fmu, c, time, file, separator, FALSE); // output values for this step
     nSteps++;
  } // while  

  // cleanup
  fclose(file);
  if (x!=NULL) free(x);
  if (xdot!= NULL) free(xdot);
  if (z!= NULL) free(z);
  if (prez!= NULL) free(prez);

  // print simulation summary 
  printf("Simulation from %g to %g terminated successful\n", t0, tEnd);
  printf("  steps ............ %d\n", nSteps);
  printf("  fixed step size .. %g\n", h);
  printf("  time events ...... %d\n", nTimeEvents);
  printf("  state events ..... %d\n", nStateEvents);
  printf("  step events ...... %d\n", nStepEvents);

  return 1; // success
}

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

