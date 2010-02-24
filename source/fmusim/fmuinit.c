#include "fmuinit.h"

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
int fmuLoadDll(const char* dllPath, FMU *fmu) {
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
