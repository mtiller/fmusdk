/* ------------------------------------------------------------------------- 
 * fmuio.h
 * Code for handling I/O
 * Copyright 2010 QTronic GmbH. All rights reserved. 
 * -------------------------------------------------------------------------
 */

#ifndef fmuio_h
#define fmuio_h

#include "main.h"

void fmuLogger(fmiComponent c, fmiString instanceName,
	       fmiStatus status, fmiString category,
	       fmiString message, ...);

#endif // fmuio_h
