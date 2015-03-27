////////////////////////////////////////////////////////////////////////////////////
//       Main routine for OpenGl Open Loop and Closed Loop Visual Stimuli         //
////////////////////////////////////////////////////////////////////////////////////
//                           Dan Turner-Evans                                     //
//                          V0.0 - 10/15/2014                                     //
////////////////////////////////////////////////////////////////////////////////////
// winmain - create the windows and run the main loop                             //
// balloffset - programs to calculate the offset of the stripe for                //
//    open and closed loop stripes                                                //
// DAQ - save the frame trigger and the VR PD signal                              //
// glmain - contains the OpenGl rendering code                                    //
// system - system shutdown                                                       //
////////////////////////////////////////////////////////////////////////////////////
/*
Adapted from:
www.thepixels.net
by:    Greg Damon
gregd@thepixels.net
and segments of Jim Strother's Win API code
*/

#include "glmain.h"
#include <Windows.h>
#include "winmain.h"
#include "system.h"
#include "balloffset.h"
#include "wglext.h"
#include "DAQ.h"

// Open loop or closed loop
int closed;

// Update visual stim or stop the update
int stopped = 0;

// Filename for the synchronization file
char* syncfname;
char syncfnamebuf[100];

// Create device contexts and window handles for three windows
HDC hdc1;
HWND hwnd1;

// Create OpenGl context
HGLRC hglrc;

// Let the while loop run
bool quit = false;

// Initialize the pixel format index
int indexPixelFormat = 0;

// CreateWnd: creates a full screen window to span the projectors
void CreateWnd(HINSTANCE &hinst, int width, int height, int depth)
{
	// Find the middle projector
	POINT pt;
	pt.x = -SCRWIDTH;
	pt.y = 100;

	HMONITOR hmon; // monitor handles
	hmon = MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST);

	MONITORINFO mi;
	mi.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(hmon, &mi);

	// Set the window position based on the projector locations
	int posx1 = mi.rcMonitor.left;
	int posy1 = mi.rcMonitor.top;

	// Constants for fullscreen mode
	long wndStyle = WS_POPUP | WS_VISIBLE;

	// create the window
	hwnd1 = CreateWindowEx(NULL,
		WNDCLASSNAME,
		WNDNAME,
		wndStyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		posx1, posy1,
		width, height,
		NULL,
		NULL,
		hinst,
		NULL);

	hdc1 = GetDC(hwnd1);

	PIXELFORMATDESCRIPTOR pfd =
	{
		sizeof(PIXELFORMATDESCRIPTOR),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
		PFD_TYPE_RGBA,
		SCRDEPTH,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		32,
		0, 0, PFD_MAIN_PLANE, 0, 0, 0, 0
	};
	indexPixelFormat = ChoosePixelFormat(hdc1, &pfd);
	SetPixelFormat(hdc1, indexPixelFormat, &pfd);

	// Setup OpenGL
	hglrc = wglCreateContext(hdc1);
	wglMakeCurrent(hdc1, hglrc);
	glewExperimental = GL_TRUE;
	glewInit();
	InitOpenGL();
	ShowWindow(hwnd1, SW_SHOW);		// everything went OK, show the window
	UpdateWindow(hwnd1);
}

// The event handler
LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	switch (msg)
	{
	case WM_CREATE:
	{
					  break;
	}
	case WM_DESTROY:
	{
					   SysShutdown();
					   break;
	}
	case WM_SIZE:
	{
					break;
	}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
}

// The main loop
int WINAPI WinMain(HINSTANCE hinstance, HINSTANCE hprevinstance, LPSTR lpcmdline, int nshowcmd)
{
	MSG msg;

	// Create a windows class for subsequently creating windows
	WNDCLASSEX ex;
	ex.cbSize = sizeof(WNDCLASSEX);
	ex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	ex.lpfnWndProc = WinProc;
	ex.cbClsExtra = 0;
	ex.cbWndExtra = 0;
	ex.hInstance = hinstance;
	ex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	ex.hCursor = LoadCursor(NULL, IDC_ARROW);
	ex.hbrBackground = NULL;
	ex.lpszMenuName = NULL;
	ex.lpszClassName = WNDCLASSNAME;
	ex.hIconSm = NULL;

	if (!RegisterClassEx(&ex))
	{
		MessageBox(NULL, "Failed to register the windows class", "Window Reg Error", MB_OK);
		return 1;
	}

	// Create the windows
	CreateWnd(hinstance, SCRWIDTH, SCRHEIGHT, SCRDEPTH);

	// Prompt the user for a filename and directory
	OPENFILENAME ofn;       // common dialog box structure
	ZeroMemory(&ofn, sizeof(ofn));
	char szFile[260];       // buffer for file name

	HWND hwndsave;

	// create the save as window
	hwndsave = CreateWindowEx(NULL,
		WNDCLASSNAME,
		WNDNAME,
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
		100, 100,
		600, 600,
		NULL,
		NULL,
		hinstance,
		NULL);
	HANDLE hf;              // file handle

	szFile[0] = '\0';
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwndsave;
	ofn.lpstrFilter = "Text\0*.TXT\0";
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile) / sizeof(*szFile);
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = NULL;
	ofn.lpstrInitialDir = (LPSTR)NULL;
	ofn.Flags = OFN_SHOWHELP | OFN_OVERWRITEPROMPT;
	ofn.lpstrTitle = "Specify the filename";
	ofn.lpstrDefExt = "txt";

	GetSaveFileName(&ofn);

	//Create a file to store the offset position at each point in time
	FILE *str;
	fopen_s(&str, ofn.lpstrFile, "w");

	//Set the filename for the DAQ data
	strncpy_s(syncfnamebuf, 100, ofn.lpstrFile, strlen(ofn.lpstrFile) - 4);
	strcat_s(syncfnamebuf, 100, "_SYNC.txt");
	syncfname = syncfnamebuf;

	// Print local time as a string.
	char s[30];
	size_t i;
	struct tm tim;
	time_t now;
	now = time(NULL);
	localtime_s(&tim, &now);
	i = strftime(s, 30, "%b %d, %Y; %H:%M:%S\n", &tim);
	fprintf(str, "Current date and time: %s\n", s);

	// Variables to store the treadmill update signal
	float dx0Now = 0.0f;
	float dx1Now = 0.0f;
	float dy0Now = 0.0f;
	float dy1Now = 0.0f;
	InitOffset();
	Sleep(1000);

	int cw = 1; // Move the open loop stripe in a clockwise direction
	int ccw = -1; // Move the open loop stripe in a counterclockwise direction
	int randomreset = 1;

	PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT = NULL;
	PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT = NULL;
	wglSwapIntervalEXT =
		(PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	wglGetSwapIntervalEXT =
		(PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

	// The main loop
	while (!quit)
	{
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				quit = true;

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}


		// Timestamp closed loop output in order to recreate later
		QueryPerformanceCounter(&li);
		float netTime = (li.QuadPart - CounterStart) / PCFreq;

		if (randomreset)
		{
			//Generate a random starting offset
			srand(time(0));
			io_mutex.lock();
			BallOffsetRot = fmod(rand(), 180) - 90;
			BallOffsetFor = 0.0f;//-dist2stripe*0.5;
			BallOffsetSide = 0.0f;
			io_mutex.unlock();
			randomreset = 0;
		}

		/////////////////////// EXPERIMENT SPECIFICS LIVE HERE /////////////////////////////
		if (netTime < 1)
		{
			closed = 1;
			olsdir = 0;
		}
		if (netTime > 1 && netTime < 0.5 * 60)
		{
			closed = 1;		
			olsdir = 0;
		}
		if (netTime > 0.5 * 60 && netTime < 4.5 * 60)
		{
			closed = 1;
			olsdir = 1;
		}
		if (netTime > 4.5 * 60 && netTime < 5 * 60)
		{
			closed = 1;
			olsdir = 0;
		}
		if (netTime > 5 * 60)
			break;
		////////////////////////////////////////////////////////////////////////////////////

		//Switch contexts and draw
		wglMakeCurrent(hdc1, hglrc);
		RenderFrame(olsdir);

		//Swapbuffers
		SwapBuffers(hdc1);

		// Note the time that's passed
		QueryPerformanceCounter(&li);
		netTime = (li.QuadPart - CounterStart) / PCFreq;

		// Pull out the relevant values
		io_mutex.lock();
		if (closed)
			BallOffsetRotNow = BallOffsetRot;
		BallOffsetForNow = BallOffsetFor;
		BallOffsetSideNow = BallOffsetSide;
		dx0Now = dx0;
		dx1Now = dx1;
		dy0Now = dy0;
		dy1Now = dy1;
		io_mutex.unlock();

		//Print the elapsed time
		fprintf(str, "Elapsed time:\t%f\t", netTime);
		//Print the offset to the log file
		fprintf(str, "Rotational Offset:\t%f\t", BallOffsetRotNow);
		fprintf(str, "Forward Offset:\t%f\t", BallOffsetForNow);
		fprintf(str, "Lateral Offset:\t%f\t", BallOffsetSideNow);
		fprintf(str, "dx0:\t%f\t", dx0Now);
		fprintf(str, "dx1:\t%f\t", dx1Now);
		fprintf(str, "dy0:\t%f\t", dy0Now);
		fprintf(str, "dy1:\t%f\t", dy1Now);
		fprintf(str, "closed:\t%d\t", closed);
		fprintf(str, "olsdir:\t%d\n", olsdir);

		if (GetAsyncKeyState(VK_ESCAPE) || (netTime > 30 * 60))
			SysShutdown();
	}
	GLShutdown();
	fclose(str);
	return msg.lParam;
}