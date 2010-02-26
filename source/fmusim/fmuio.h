/* ------------------------------------------------------------------------- 
 * fmuio.h
 * Code for handling I/O
 * Copyright 2010 QTronic GmbH. All rights reserved. 
 * -------------------------------------------------------------------------
 */

#ifndef fmuio_h
#define fmuio_h

#include "main.h"
#include <stdio.h>

extern void fmuLogger(fmiComponent c, fmiString instanceName,
	       fmiStatus status, fmiString category,
	       fmiString message, ...);

extern void outputRow(FMU *fmu, fmiComponent c, double time, FILE* file,
	       char separator, int header);
		   
extern int fmuError(const char *msg);

#endif // fmuio_h
