////////////////////////////////////////////////////////////////////////////////////
//                           Header for DAQ                                       //
////////////////////////////////////////////////////////////////////////////////////
//                           Dan Turner-Evans                                     //
//                          V0.0 - 10/23/2014                                     //
////////////////////////////////////////////////////////////////////////////////////

// Load NIDAQ header
#include <NIDAQmx.h>

// Load other headers
#include <Windows.h>
#include <stdio.h>
#include <math.h>

// Define the error check function
#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

// Determine when the DAQ is running
extern int DAQRun;

// The filename for the DAQ data
extern char* syncfname;

// The DAQ acquisition function
int DAQDat(void);