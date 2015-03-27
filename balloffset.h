////////////////////////////////////////////////////////////////////////////////////
//                   Header for Finding the Ball Offset                           //
////////////////////////////////////////////////////////////////////////////////////
//                           Dan Turner-Evans                                     //
//                          V0.0 - 10/15/2014                                     //
////////////////////////////////////////////////////////////////////////////////////

// Header for interfacing with the treadmill
#include "ftd2xx.h"

// Boost headers for setting up the data server
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

// Counter to serve as a reference for timestamping
extern __int64 CounterStart;
extern LARGE_INTEGER li;
extern float PCFreq;

//Declare global variables to lock (mutex) and store the net offset (for closed loop)
extern boost::mutex io_mutex;
extern float BallOffsetRot;
extern float BallOffsetFor;
extern float BallOffsetSide;
extern float dx0;
extern float dx1;
extern float dy0;
extern float dy1;

// Start the counter and set up the treadmill
void InitOffset(void);

// Calculate offsets for open loop
void TimeOffset(float &tOffset, int dir, time_t start);

// Start the treadmill
void TreadMillStart(void);

// Treadmill data server
void TreadMillDat(void);

// Stop collecting the offset
void CloseOffset(void);