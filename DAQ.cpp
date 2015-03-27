////////////////////////////////////////////////////////////////////////////////////
//                         Main Routine for DAQ                                   //
////////////////////////////////////////////////////////////////////////////////////
//                           Dan Turner-Evans                                     //
//                          V0.0 - 10/23/2014                                     //
////////////////////////////////////////////////////////////////////////////////////

#include "DAQ.h"

// Define a callback to pull out data continuously every n samples
int32 CVICALLBACK EveryNCallback(TaskHandle taskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData);

// Callback to register when an event is done
int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData);

//Create a file to store the offset position at each point in time
FILE *str;

int DAQDat(void)
{
	int32       error = 0;
	TaskHandle  outputTaskHandle = 0;
	TaskHandle  inputTaskHandle = 0;
	float64     pulseData[3];
	char        errBuff[2048] = { '\0' };
	int         i = 0;
	int32   	written;

	fopen_s(&str, syncfname, "w");

	// Pulse to trigger acquisition
	pulseData[0] = 0;
	pulseData[1] = 5;
	pulseData[2] = 0;

	/*********************************************/
	// DAQmx Configure Code for the output
	/*********************************************/
	DAQmxErrChk(DAQmxCreateTask("", &outputTaskHandle));
	DAQmxErrChk(DAQmxCreateAOVoltageChan(outputTaskHandle, "Dev1/ao0", "", -10.0, 10.0, DAQmx_Val_Volts, NULL));
	DAQmxErrChk(DAQmxCfgSampClkTiming(outputTaskHandle, "", 3.0, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, 3));
	DAQmxErrChk(DAQmxRegisterDoneEvent(outputTaskHandle, 0, DoneCallback, NULL));

	/*********************************************/
	// DAQmx Write Code for the output
	/*********************************************/
	DAQmxErrChk(DAQmxWriteAnalogF64(outputTaskHandle, 3, 0, 10.0, DAQmx_Val_GroupByChannel, pulseData, &written, NULL));

	/*********************************************/
	// DAQmx Configure Code for the input
	/*********************************************/
	DAQmxErrChk(DAQmxCreateTask("", &inputTaskHandle));
	DAQmxErrChk(DAQmxCreateAIVoltageChan(inputTaskHandle, "Dev1/ai0:1", "", DAQmx_Val_Cfg_Default, -10.0, 10.0, DAQmx_Val_Volts, NULL));
	DAQmxErrChk(DAQmxCfgSampClkTiming(inputTaskHandle, "", 10000.0, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 2000));

	DAQmxErrChk(DAQmxRegisterEveryNSamplesEvent(inputTaskHandle, DAQmx_Val_Acquired_Into_Buffer, 1000, 0, EveryNCallback, NULL));
	DAQmxErrChk(DAQmxRegisterDoneEvent(inputTaskHandle, 0, DoneCallback, NULL));

	/*********************************************/
	// DAQmx Start Code for Output
	/*********************************************/
	DAQmxErrChk(DAQmxStartTask(outputTaskHandle));

	/*********************************************/
	// DAQmx Start Code for Input
	/*********************************************/
	DAQmxErrChk(DAQmxStartTask(inputTaskHandle));

	/*********************************************/
	// Run the DAQ Acquisition
	/*********************************************/
	while (DAQRun)
		;

Error:
	if (DAQmxFailed(error))
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
	if (inputTaskHandle != 0) {
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(outputTaskHandle);
		DAQmxClearTask(outputTaskHandle);
		DAQmxStopTask(inputTaskHandle);
		DAQmxClearTask(inputTaskHandle);
	}
	if (DAQmxFailed(error))
		printf("DAQmx Error: %s\n", errBuff);
	printf("End of program, press Enter key to quit\n");
	getchar();
	return 0;
}

int32 CVICALLBACK EveryNCallback(TaskHandle inputTaskHandle, int32 everyNsamplesEventType, uInt32 nSamples, void *callbackData)
{
	int32       error = 0;
	char        errBuff[2048] = { '\0' };
	static int  totalRead = 0;
	int32       read = 0;
	float64     data[2000];

	/*********************************************/
	// DAQmx Read Code
	/*********************************************/
	DAQmxErrChk(DAQmxReadAnalogF64(inputTaskHandle, 1000, 10.0, DAQmx_Val_GroupByScanNumber, data, 2000, &read, NULL));

	//Print the elapsed time
	for (int i = 0; i<1000; i++)
	{
		fprintf(str, "%f,", data[2 * i + 1]);
		fprintf(str, "%f\n", data[2 * i]);
	}

	if (read>0) {
		printf("Acquired %d samples. Total %d\r", read, totalRead += read);
		fflush(stdout);
	}

Error:
	if (DAQmxFailed(error)) {
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		/*********************************************/
		// DAQmx Stop Code
		/*********************************************/
		DAQmxStopTask(inputTaskHandle);
		DAQmxClearTask(inputTaskHandle);
		printf("DAQmx Error: %s\n", errBuff);
	}
	return 0;
}

int32 CVICALLBACK DoneCallback(TaskHandle taskHandle, int32 status, void *callbackData)
{
	int32   error = 0;
	char    errBuff[2048] = { '\0' };

	// Check to see if an error stopped the task.
	DAQmxErrChk(status);

Error:
	if (DAQmxFailed(error)) {
		DAQmxGetExtendedErrorInfo(errBuff, 2048);
		DAQmxClearTask(taskHandle);
		printf("DAQmx Error: %s\n", errBuff);
	}
	return 0;
}
