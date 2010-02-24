#include "fmusim.h"
#include "fmuio.h"

#include <stdio.h>
#include <stdlib.h>

#ifndef _MSC_VER
#define TRUE 1
#define FALSE 0
#define min(a,b) (a>b ? b : a)
#endif

#define RESULT_FILE "result.csv"

// simulate the given FMU using the forward euler method.
// time events are processed by reducing step size to exactly hit tNext.
// state events are checked and fired only at the end of an Euler step. 
// the simulator may therefore miss state events and fires state events typically too late.
int fmuSimulate(FMU* fmu, double tEnd, double h, fmiBoolean loggingOn, char separator) {
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
        printf("could not write %s\n", RESULT_FILE);
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
  printf("CSV file '%s' written", RESULT_FILE);

  return 1; // success
}
