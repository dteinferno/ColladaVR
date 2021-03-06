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

// Specify the trial structure 
struct trial {
	float time; // time of the trial
	int fback; // open (0) or closed (1) loop
	int polar; // translation (1) or rotation (0) for open loop
	int direction; // (1 or -1, 0 to not render anything)
	float olGain; // open loop gain (period or 360/gain = vel)
	float clGain; // closed loop gain
};

// Variables to set the starting conditions
int randomreset = 1;
float startingPos = -5.0f;

// Specify the fly body and head angles for software corrections
float flyAng = 30.0f * M_PI / 180;
float lookDownAng = 0;

int jumpStripe = 1; // also set stripe to be continuous in glmain

//const char * ColladaFname = "D:\\Environments\\Grid.dae";

// Environments and protocol fo cr probing PB-FB activity
//const char * ColladaFname = "D:\\Environments\\StripeBG.dae";
//const char * ColladaFname = "D:\\Environments\\ClutterBack.dae";
//const char * ColladaFname = "D:\\Environments\\GroundPlane.dae";

//const char * ColladaFname = "D:\\Environments\\MSClutterBack.dae";
//const char * ColladaFname = "D:\\Environments\\MSClutterBackWCyl.dae";
//const char * ColladaFname = "D:\\Environments\\StripeBackWGroundPlane.dae";

//const char * ColladaFname = "D:\\Environments\\ClutterBackWGroundPlane.dae";
//const char * ColladaFname = "D:\\Environments\\CylClutterBack.dae";
//const char * ColladaFname = "D:\\Environments\\FullScene.dae";
//const char * ColladaFname = "D:\\Environments\\TwoObjScene2.dae";
//const char * ColladaFname = "D:\\Environments\\MultiScene.dae";
//const char * ColladaFname = "D:\\Environments\\WhiteNoise.dae";
//const char * ColladaFname = "D:\\Environments\\TwoObjScene_Mount.dae";
//const char * ColladaFname = "D:\\Environments\\TwoObjScene_Cloud.dae";
//const char * ColladaFname = "D:\\Environments\\AllOn.dae";
//const char * ColladaFname = "D:\\Environments\\FlowV4.dae";
//const char * ColladaFname = "D:\\Environments\\OneCylV1_LightCylFlatBack.dae";

//Hannah
//const char * ColladaFname = "D:\\Environments\\PIworlds_opticFlowIn2D\\black_flatGroundPlane.dae";
//const char * ColladaFname = "D:\\Environments\\PIworlds_opticFlowIn2D\\stripe15deg_flatGroundPlane.dae";

////const char * ColladaFname = "D:\\Environments\\PIworlds_opticFlowIn2D\\black_texGroundPlane.dae";
////const char * ColladaFname = "D:\\Environments\\PIworlds_opticFlowIn2D\\stripe15deg_texGroundPlane.dae";

//const char * ColladaFname = "D:\\Environments\\PIworlds_opticFlowIn2D\\black_texFineGroundPlane.dae";
//const char * ColladaFname = "D:\\Environments\\PIworlds_opticFlowIn2D\\stripe15deg_texfineGroundPlane.dae";

//trial experiment[1] = { 185, 1, 0, 1, 0, 1 }; //125

//const char * ColladaFname = "D:\\Environments\\MSClutterBack.dae";
//const char * ColladaFname = "D:\\Environments\\MSClutterBackWCyl.dae";


// Hannah PFN
//const char * ColladaFname = "D:\\Environments\\StripeBG.dae";
//const char * ColladaFname = "D:\\Environments\\PIworlds_opticFlowIn2D\\stripe15deg_texfineGroundPlane.dae";
//trial experiment[1] = { 130, 1, 0, 1, 0, 1 };  // please don't change

//const char * ColladaFname = "D:\\Environments\\PIworlds_opticFlowIn2D\\stripe15deg_texfineGroundPlane_Big.dae";

//const char * ColladaFname = "D:\\Environments\\AllOn.dae";
const char * ColladaFname = "D:\\Environments\\StripeBG_larger.dae";
//const char * ColladaFname = "D:\\Environments\\PIworlds_opticFlowIn2D\\stripe15deg_texFineGroundPlane_Big.dae";
//const char * ColladaFname = "D:\\Environments\\PIworlds_opticFlowIn2D\\twostripe15deg_texfineGroundPlane_Big.dae";
//trial experiment[1] = { 305, 1, 0, 1, 0, 1 };
//trial experiment[3] = { { 95, 1, 0, 0, 0, 1 }, { 120, 1, 0, 1, 0, 1 }, { 5, 1, 0, 0, 0, 1 } };
trial experiment[3] = { { 125, 1, 0, 0, 0, 1 }, { 240, 1, 0, 1, 0, 1 }, { 5, 1, 0, 0, 0, 1 } };
//trial experiment[6] = { { 65, 1, 0, 0, 0, 1 }, { 60, 1, 0, 1, 0, 1 }, { 30, 0, 0, 1, 5, 1 }, { 30, 0, 0, -1, 5, 1 }, { 5, 1, 0, 0, 0, 1 } };
//trial experiment[4] = { { 5, 1, 0, 0, 0, 1 }, { 30, 0, 0, 1, 5, 1 }, { 30, 0, 0, -1, 5, 1 }, { 5, 1, 0, 0, 0, 1 } };
//trial experiment[4] = { { 5, 1, 0, 0, 0, 1 }, { 60, 1, 0, 0, 0, 1 }, { 120, 1, 0, 1, 0, 1 }, { 5, 1, 0, 0, 0, 1 } };
//trial experiment[3] = { { 60, 1, 0, 0, 0, 1 }, { 140, 1, 0, 1, 0, 1 }, { 5, 1, 0, 0, 0, 1 }, };
//trial experiment[4] = { { 60, 1, 0, 0, 0, 1 }, { 60, 1, 0, 1, 0, 0.75 }, { 60, 1, 0, 1, 0, 1.25 }, { 5, 1, 0, 0, 0, 1 }, };
//trial experiment[10] = { { 5, 1, 0, 0, 0, 1 },
//{ 15, 0, 1, 1, 360 / 10, 1 }, { 15, 0, 1, -1, 360 / 10, 1 }, { 15, 0, 0, 1, 5, 1 }, { 15, 0, 0, -1, 5, 1 },
//{ 15, 0, 1, 1, 360 / 10, 1 }, { 15, 0, 1, -1, 360 / 10, 1 }, { 15, 0, 0, 1, 5, 1 }, { 15, 0, 0, -1, 5, 1 },
//{ 5, 1, 0, 0, 0 } };
//trial experiment[2] = { { 60, 1, 0, 0, 0, 1 }, { 125, 1, 0, 1, 0, 1 }};
//trial experiment[14] = { { 5, 1, 0, 0, 0, 1 },
//{ 15, 0, 1, 1, 360 / 5, 1 }, { 15, 0, 1, -1, 360 / 5, 1 }, { 15, 0, 0, 1, 5, 1 }, { 15, 0, 0, -1, 5, 1 },
//{ 15, 0, 1, 1, 360 / 5, 1 }, { 15, 0, 1, -1, 360 / 5, 1 }, { 15, 0, 0, 1, 5, 1 }, { 15, 0, 0, -1, 5, 1 },
//{ 15, 0, 1, 1, 360 / 5, 1 }, { 15, 0, 1, -1, 360 / 5, 1 }, { 15, 0, 0, 1, 5, 1 }, { 15, 0, 0, -1, 5, 1 },
//{ 5, 1, 0, 0, 0 } };
//trial experiment[8] = { { 5, 1, 0, 0, 0, 1 },
//{ 60, 1, 0, 1, 0, 1 }, { 60, 1, 0, 0, 0, 1 },
//{ 30, 0, 1, 1, 360 / 5, 1 }, { 30, 0, 1, -1, 360 / 5, 1 }, { 30, 0, 0, 1, 5, 1 }, { 30, 0, 0, -1, 5, 1 },
//{ 5, 1, 0, 0, 0 } };
//trial experiment[9] = { { 35, 1, 0, 0, 0, 1 },
//{ 30, 1, 0, 1, 0, 1 },
//{ 30, 0, 1, 1, 360 / 5, 1 }, { 30, 0, 1, -1, 360 / 5, 1 },
//{ 30, 0, 1, 1, 360 / 10, 1 }, { 30, 0, 1, -1, 360 / 10, 1 },
//{ 30, 0, 1, 1, 360 / 20, 1 }, { 30, 0, 1, -1, 360 / 20, 1 },
//{ 5, 1, 0, 0, 0 } };
//trial experiment[3] = { { 65, 1, 0, 0, 0, 1 }, { 120, 1, 0, 1, 0, 1 }, { 5, 1, 0, 0, 0, 1 } };
//trial experiment[1] = { 3600, 1, 0, 1, 0, 1 };
//trial experiment[4] = { { 30, 0, 1, 1, 360 / 5, 1 }, { 30, 0, 1, -1, 360 / 5, 1 },
//{ 30, 0, 1, 1, 360 / 10, 1 }, { 30, 0, 1, -1, 360 / 10, 1 } };

// Environments and protocol for the optic flow experiments
//const char * ColladaFname = "D:\\Environments\\OneCylV1_NoCylFlatBack.dae";
//const char * ColladaFname = "D:\\Environments\\StripeBG.dae";
//const char * ColladaFname = "D:\\Environments\\SineBG.dae";
//const char * ColladaFname = "D:\\Environments\\WhiteNoise.dae";
//const char * ColladaFname = "D:\\Environments\\TreeTestBack.dae";
//trial experiment[3] = { { 5, 1, 0, 0, 0, 1 }, { 120, 1, 0, 0, 0, 1 }, { 5, 1, 0, 0, 0, 1 } };
//trial experiment[12] = { { 5, 1, 0, 0, 0, 1 },
//{ 30, 0, 0, 1, 5, 1 }, { 30, 0, 0, -1, 5, 1 },
//{ 30, 0, 0, 1, 5, 1 }, { 30, 0, 0, -1, 5, 1 },
//{ 30, 0, 0, 1, 5, 1 }, { 30, 0, 0, -1, 5, 1 },
//{ 30, 0, 0, 1, 5, 1 }, { 30, 0, 0, -1, 5, 1 },
//{ 30, 0, 0, 1, 5, 1 }, { 30, 0, 0, -1, 5, 1 },
//{ 5, 1, 0, 0, 0, 1 } };
//trial experiment[43] = { { 5, 1, 0, 0, 0, 1 },
//{ 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, 1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, 1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, -1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, -1, 6, 1 },
//{ 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, 1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, 1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, -1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, -1, 6, 1 },
//{ 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, 1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, 1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, -1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, -1, 6, 1 },
//{ 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, 1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, 1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, -1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, -1, 6, 1 },
//{ 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, 1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, 1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, -1, 6, 1 }, { 10, 1, 0, 1, 0, 1 }, { 3, 1, 2, -1, 6, 1 },
//{ 10, 1, 0, 1, 0, 1 }, { 5, 1, 0, 0, 0, 1 } };
//trial experiment[5] = { { 5, 1, 0, 0, 0, 1 }, { 10, 1, 0, 1, 0, 1 }, { 10, 1, 0, 1, 0, 2 }, { 10, 1, 0, 1, 0, 1 }, { 5, 1, 0, 0, 0, 1 } };

// Environments and protocol for the jumping vs. continuous stripe experiments
//const char * ColladaFname = "D:\\Environments\\StripeBG.dae";
//const char * ColladaFname = "D:\\Environments\\StripeBGInv.dae";
//trial experiment[3] = { { 10, 1, 0, 0, 0 }, { 120, 1, 0, 1, 0 }, { 10, 1, 0, 0, 0 } };


// Environments and protocol for the translation experiments
//const char * ColladaFname = "D:\\Environments\\OneCylV1_LightCylAlone.dae";
//const char * ColladaFname = "D:\\Environments\\OneCylV1_LightCylFlatBack.dae";
//const char * ColladaFname = "D:\\Environments\\OneCylV1_LightCyl3ObjBack.dae";
//const char * ColladaFname = "D:\\Environments\\OneCylV1_NoCyl3ObjBack.dae";
//trial experiment[3] = { { 10, 1, 0, 0, 0, 1 }, { 180, 1, 0, 1, 0, 1 }, { 10, 1, 0, 0, 0, 1 } };

// Inverted environments
//const char * ColladaFname = "D:\\Environments\\OneCylV1_LightCylAloneInv.dae";
//const char * ColladaFname = "D:\\Environments\\OneCylV1_LightCylFlatBackInv.dae";
//const char * ColladaFname = "D:\\Environments\\OneCylV1_LightCyl3ObjBackInv.dae";

// Optic flow style environment and protocol
//const char * ColladaFname = "D:\\Environments\\FlowV1.dae";
//trial experiment[6] = { { 5, 1, 0, 0, 0 }, { 15, 0, 1, 1, 360 / 5 }, { 15, 0, 1, -1, 360 / 5 }, { 15, 0, 0, 1, 5 }, { 15, 0, 0, -1, 5 }, { 35, 1, 0, 0, 0 } };

// Cluttered background
//const char * ColladaFname = "D:\\Environments\\ClutterBack.dae";
//const char * ColladaFname = "D:\\Environments\\MSClutterBack.dae";
//const char * ColladaFname = "D:\\Environments\\RealClutterBack.dae";
//const char * ColladaFname = "D:\\Environments\\OneCylV1_LightCylFlatBack.dae";
//const char * ColladaFname = "D:\\Environments\\CylClutterBack.dae";
//trial experiment[3] = { { 5, 1, 0, 0, 0, 1 }, { 120, 1, 0, 1, 0, 1 }, { 5, 1, 0, 0, 0, 1 } };

// Bump tracking behavior
//const char * ColladaFname = "D:\\Environments\\StripeBG.dae";
//const char * ColladaFname = "D:\\Environments\\ClutterBack.dae";
//const char * ColladaFname = "D:\\Environments\\OneCyl_ClutterBack.dae";
//const char * ColladaFname = "D:\\Environments\\OneCylV1_NoCyl3ObjBack.dae";
//trial experiment[3] = { { 5, 1, 0, 0, 0, 1 }, { 60, 1, 0, 1, 0, 1 }, { 5, 1, 0, 0, 0, 1 } };

// Tell the LED when to trigger
int LEDRun = 0;

// Filename for the synchronization file
char* syncfname;
char syncfnamebuf[1000];

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
	strncpy_s(syncfnamebuf, 1000, ofn.lpstrFile, strlen(ofn.lpstrFile) - 4);
	strcat_s(syncfnamebuf, 1000, "_SYNC.txt");
	syncfname = syncfnamebuf;

	//Print the Collada filename
	fprintf(str, "Collada file used: %s\n", ColladaFname);

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
	Sleep(3000);

	PFNWGLSWAPINTERVALEXTPROC       wglSwapIntervalEXT = NULL;
	PFNWGLGETSWAPINTERVALEXTPROC    wglGetSwapIntervalEXT = NULL;
	wglSwapIntervalEXT =
		(PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	wglGetSwapIntervalEXT =
		(PFNWGLGETSWAPINTERVALEXTPROC)wglGetProcAddress("wglGetSwapIntervalEXT");

	// Get information about the trials
	float exptTime = 0;
	for (int trialNum = 0; trialNum < sizeof(experiment)/sizeof(experiment[0]); trialNum++)
		exptTime = exptTime + experiment[trialNum].time;
	float timeOffset = 0;
	int trialNow = 0;

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
			BallOffsetRot = rand() % 360;// 0.0f;
			BallOffsetFor = startingPos;
			BallOffsetSide = 0.0f;
			io_mutex.unlock();
			randomreset = 0;
		}

		/////////////////////// EXPERIMENT SPECIFICS LIVE HERE /////////////////////////////
		if (netTime > experiment[trialNow].time + timeOffset)
		{
			timeOffset = timeOffset + experiment[trialNow].time;
			trialNow++;
		}

		if (netTime > exptTime)
		{
			SysShutdown();
		}
		
		////////////////////////////////////////////////////////////////////////////////////

		//Switch contexts and draw
		wglMakeCurrent(hdc1, hglrc);
		RenderFrame(experiment[trialNow].fback, experiment[trialNow].polar, experiment[trialNow].direction, lookDownAng, experiment[trialNow].olGain, timeOffset, netTime, experiment[trialNow].clGain);
		PDBox();

		//Swapbuffers
		SwapBuffers(hdc1);

		// Note the time that's passed
		QueryPerformanceCounter(&li);
		netTime = (li.QuadPart - CounterStart) / PCFreq;

		// Pull out the relevant values
		io_mutex.lock();
		//if (experiment[trialNow].fback)
		//{
		//	if (experiment[trialNow].polar != 2)
		//		BallOffsetRotNow = BallOffsetRot;
		//	BallOffsetForNow = BallOffsetFor;
		//	BallOffsetSideNow = BallOffsetSide;
		//}
		//else
		//{
		//	if (experiment[trialNow].polar)
		//	{
		//		BallOffsetRotNow = BallOffsetRot;
		//		BallOffsetSideNow = BallOffsetSide;
		//	}
		//	else
		//	{
		//		BallOffsetForNow = BallOffsetFor;
		//		BallOffsetSideNow = BallOffsetSide;
		//	}
		//}

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
		fprintf(str, "closed:\t%d\t", experiment[trialNow].fback);
		fprintf(str, "olsdir:\t%d\t", experiment[trialNow].direction);
		fprintf(str, "trans:\t%d\t", experiment[trialNow].polar);
		fprintf(str, "olgain:\t%f\t", experiment[trialNow].olGain);
		fprintf(str, "clgain:\t%f\n", experiment[trialNow].clGain);

		if (GetAsyncKeyState(VK_ESCAPE))
			SysShutdown();
		//if (GetAsyncKeyState(VK_SCROLL))
		//if ((netTime > 5) & (netTime < 5.1))
		LEDRun = 0;
		if (jumpStripe)
		{
			if ((netTime > 300) & (netTime < 300.1))
			{
				experiment[trialNow].polar = 3;
			}
			if ((netTime > 330) & (netTime < 330.1))
			{
				experiment[trialNow].polar = 4;
			}
		}
			
	}
	GLShutdown();
	fclose(str);
	return msg.lParam;
}