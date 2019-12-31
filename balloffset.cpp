////////////////////////////////////////////////////////////////////////////////////
//                  Main Routine for Finding the Ball Offset                      //
////////////////////////////////////////////////////////////////////////////////////
//                           Dan Turner-Evans                                     //
//                          V0.0 - 10/15/2014                                     //
////////////////////////////////////////////////////////////////////////////////////

#include "glmain.h"
#include <windows.h>
#include "balloffset.h""
#include "DAQ.h"

// Set up a counter for the open loop object changes
LARGE_INTEGER li;
float PCFreq = 0;
__int64 CounterStart;


// FOR CLOSED LOOP - store the offsets
boost::mutex io_mutex;
float BallOffsetRot = 0.0f;
float BallOffsetFor = 0.0f;
float BallOffsetSide = 0.0f;
float BoundaryStopCorrection = 0.0f;
float dx0 = 0.0f;
float dx1 = 0.0f;
float dy0 = 0.0f;
float dy1 = 0.0f;

//Declare global FTDI variables for communicating with the treadmill
FT_HANDLE ftHandle;
unsigned char wBuffer[1024];
DWORD txBytes = 0;

// To tell the DAQ when to run
boost::mutex io_mutex2;
int DAQRun = 1;

// Initialize the offset variables and the treadmill
void InitOffset()
{
	// Start the DAQ
	boost::thread thrd2(&DAQDat);

	// Get the PC frequency and starting time
	QueryPerformanceFrequency(&li);
	PCFreq = li.QuadPart;
	QueryPerformanceCounter(&li);
	CounterStart = li.QuadPart;

	//Initialize the treadmill
	TreadMillStart();

	boost::thread thrd(&TreadMillDat);

}

// FOR OPEN LOOP
// Set the loop duration and calculate the offset due to time passing
void TimeOffset(float &tOffset, int dir, __int64 start, float period) {
	// get delta time for this iteration:
	QueryPerformanceCounter(&li);
	float fDeltaTime = ((li.QuadPart - CounterStart)/ PCFreq) - start;
	float gain = 360 / period;
	tOffset = dir*fDeltaTime*gain;
}


// A routine to initialize the treadmill 
void TreadMillStart()
{
	//FTDI Variables
	FT_STATUS ftStatus = FT_OK;
	DWORD numDevs, devID;


	//Detect the number of FTDI devices connected
	// You may have multiple devices using FTDI interfaces
	//  This is commonly the case with RS232-USB adaptors
	//  In this case, you'll have to determine who's who by trial and error
	FT_CreateDeviceInfoList(&numDevs);
	//Case of no devices plugged in
	if (numDevs == 0){
		printf("No FTDI Devices Found");
		Sleep(2000);
	}
	//if only one FTDI device found, assume it's a treadmill
	if (numDevs == 1){
		printf("1 Device Detected, Connecting to Dev:0");
		devID = 0;
	}
	else{ //Allow user to specify which device ID to use
		printf("%d Devices Detected, Enter device ID (0-%d): ", numDevs, numDevs - 1);
		scanf_s("%d", &devID);
		printf("\n\nConnecting to Dev:%d", devID);
	}
	printf("\n\n");
	Sleep(1000);

	//Configure and Connect to Treadmill serial interface
	ftStatus |= FT_Open(devID, &ftHandle);
	ftStatus |= FT_ResetDevice(ftHandle);
	ftStatus |= FT_SetTimeouts(ftHandle, 2000, 2000);
	ftStatus |= FT_SetDataCharacteristics(ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
	ftStatus |= FT_SetFlowControl(ftHandle, FT_FLOW_NONE, NULL, NULL);
	ftStatus |= FT_SetBaudRate(ftHandle, 1250000);  //1.25MBaud Communication rate
	if (ftStatus != FT_OK) { printf("Error connecting to FTDI interface\n"); Sleep(1000); }

	//Stop any existing data stream from the treadmill
	wBuffer[0] = 254;
	wBuffer[1] = 0;
	FT_Write(ftHandle, wBuffer, 2, &txBytes);
	Sleep(20);

	FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);

	//Set to independent mode
	wBuffer[0] = 246;
	wBuffer[1] = 1;
	FT_Write(ftHandle, wBuffer, 2, &txBytes);

	//Start Motion Data @ High Speed (4kHz)
	wBuffer[0] = 255;
	wBuffer[1] = 0;
	FT_Write(ftHandle, wBuffer, 2, &txBytes);
}

//Run the treadmill at 200 Hz and continuosly update the offset. 
void TreadMillDat()
{
	//Set the calibration factor
	float Cam1RotCalibfact = 1.92;// 1.83;// 1.18;//1.14;// 1.15;// 0.87;//  1.71;
	float Cam2RotCalibfact = 1.22;// 1.31;// 0.903;// 1.14;// 0.93;// 0.88;//  1.46;
	float Cam1PosCalibfact = 74.7;// 78;// 121;// 126;// 125;// 165;// 114;
	float Cam2PosCalibfact = 117;// 110;// 159;// 126;// 153;// 163;// 134;

	//Camera Data Bins
	int dx[2], dy[2];
	float dxmod[2], dymod[2];

	// FTDI variables
	unsigned char rBuffer[240];
	DWORD rxBytes;

	while (1)
	{
		//Read 40 packets of data (12 bytes per packet)
		FT_Read(ftHandle, rBuffer, 20 * 12, &rxBytes);

		if (rxBytes != 240){
			printf("Bad Read\n");
			Sleep(1);
		}

		//Accumulate Motion Data for this 200Hz chunk
		dx[0] = 0; dx[1] = 0; dy[0] = 0; dy[1] = 0;
		for (int i = 0; i < 240; i += 12){
			dx[0] += ((int)rBuffer[i + 2]) - 128;
			dy[0] += ((int)rBuffer[i + 3]) - 128;
			dx[1] += ((int)rBuffer[i + 4]) - 128;
			dy[1] += ((int)rBuffer[i + 5]) - 128;
		}

		//Correct for fly's angle on the ball;
		dxmod[0] = dx[0] * cos(flyAng) + dy[0] * sin(flyAng);
		dymod[0] = dy[0] * cos(flyAng) + dx[0] * sin(flyAng);
		dxmod[1] = dx[1] * cos(flyAng) + dy[1] * sin(-flyAng);
		dymod[1] = dy[1] * cos(flyAng) + dx[1] * sin(-flyAng);

		float deltaFor = (float)(dymod[0] / Cam1PosCalibfact + dymod[1] / Cam2PosCalibfact)*sqrt(2) / 2;
		float deltaSide = (float)(dymod[0] / Cam1PosCalibfact - dymod[1] / Cam2PosCalibfact)*sqrt(2) / 2;
		//Update the offset given the ball movement
		io_mutex.lock();
		BallOffsetRot += (float)(dxmod[0] * Cam1RotCalibfact + dxmod[1] * Cam2RotCalibfact) / 2;
		BallOffsetFor += deltaFor*cosf(BallOffsetRot * M_PI / 180) + deltaSide*sinf(BallOffsetRot * M_PI / 180);
		BallOffsetSide += deltaFor*sinf(BallOffsetRot * M_PI / 180) - deltaSide*cosf(BallOffsetRot * M_PI / 180);

		float innerscale = 0.75f;
		float outerscale = 1.25f;

		for (int obj = 0; obj<trans_vec.size(); obj++)
		{
			// Apply barriers to keep the fly from going out of the arena or reaching an object
			if (pow(BallOffsetFor - trans_vec[obj].trans_data[1], 2) + pow(BallOffsetSide - trans_vec[obj].trans_data[0], 2) > pow(trans_vec[obj].scale_data[0], 2)
				& pow(BallOffsetFor - trans_vec[obj].trans_data[1], 2) + pow(BallOffsetSide - trans_vec[obj].trans_data[0], 2) < pow(outerscale*trans_vec[obj].scale_data[0], 2))
			{
				BoundaryStopCorrection = outerscale*trans_vec[obj].scale_data[0] / sqrt(pow(BallOffsetFor - trans_vec[obj].trans_data[1], 2) + pow(BallOffsetSide - trans_vec[obj].trans_data[0], 2));
				BallOffsetFor = BoundaryStopCorrection * (BallOffsetFor - trans_vec[obj].trans_data[1]) + trans_vec[obj].trans_data[1];
				BallOffsetSide = BoundaryStopCorrection * (BallOffsetSide - trans_vec[obj].trans_data[0]) + trans_vec[obj].trans_data[0];
			}
			if (pow(BallOffsetFor - trans_vec[obj].trans_data[1], 2) + pow(BallOffsetSide - trans_vec[obj].trans_data[0], 2) < pow(trans_vec[obj].scale_data[0], 2)
				& pow(BallOffsetFor - trans_vec[obj].trans_data[1], 2) + pow(BallOffsetSide - trans_vec[obj].trans_data[0], 2) > pow(innerscale*trans_vec[obj].scale_data[0], 2))
			{
				BoundaryStopCorrection = innerscale*trans_vec[obj].scale_data[0] / sqrt(pow(BallOffsetFor - trans_vec[obj].trans_data[1], 2) + pow(BallOffsetSide - trans_vec[obj].trans_data[06], 2));
				BallOffsetFor = BoundaryStopCorrection * (BallOffsetFor - trans_vec[obj].trans_data[1]) + trans_vec[obj].trans_data[1];
				BallOffsetSide = BoundaryStopCorrection * (BallOffsetSide - trans_vec[obj].trans_data[0]) + trans_vec[obj].trans_data[0];
			}
		}

		dx0 += (float)dx[0];
		dx1 += (float)dx[1];
		dy0 += (float)dy[0];
		dy1 += (float)dy[1];

		io_mutex.unlock();
	}

}

void CloseOffset()
{
	//thrd.detach();

	//Stop Acquisition
	wBuffer[0] = 254;
	wBuffer[1] = 0;
	FT_Write(ftHandle, wBuffer, 2, &txBytes);

	//Stop the DAQ
	io_mutex2.lock();
	DAQRun = 0;
	io_mutex2.unlock();

	//Close serial interface
	FT_Close(ftHandle);
}