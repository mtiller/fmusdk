#include "fmuio.h"

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

extern FMU fmu;

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
static void outputRow(FMU *fmu, fmiComponent c, double time, FILE* file, char separator, int header) {
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
void fmuLogger(fmiComponent c, fmiString instanceName,
	       fmiStatus status, fmiString category,
	       fmiString message, ...) {
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
