#include "fmuinit.h"

#include "xml_parser.h"
#include <stdio.h>
#include <dlfcn.h>

#define BUFSIZE 4096

static void* getAdr(FMU *fmu, const char* functionName){
    char name[BUFSIZE];
    void* fp;
    sprintf(name, "%s_%s", getModelIdentifier(fmu->modelDescription), functionName);
#ifdef _MSC_VER
    fp = GetProcAddress(fmu->dllHandle, name);
#else
    fp = dlsym(fmu->dllHandle, name);
#endif
    if (!fp) {
        printf ("error: Function %s not found in dll\n", name);        
    }
    return fp;
}

// Load the given dll and set function pointers in fmu
int fmuLoadDll(const char* dllPath, FMU *fmu) {
#ifdef _MSC_VER
    HANDLE h = LoadLibrary(dllPath);
#else
    printf("dllPath = %s\n", dllPath);
    HANDLE h = dlopen(dllPath, RTLD_LAZY);
#endif
    if (!h) {
        printf("error: Could not load %s\n", dllPath);
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

void fmuFree(FMU *fmu) {
#ifdef _MSC_VER
  FreeLibrary(fmu->dllHandle);
  freeElement(fmumodelDescription);
#else
  dlclose(fmu->dllHandle);
  freeElement(fmu->modelDescription);
#endif
}
