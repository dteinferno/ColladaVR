////////////////////////////////////////////////////////////////////////////////////
//                          Header for WinMain                                    //
////////////////////////////////////////////////////////////////////////////////////
//                           Dan Turner-Evans                                     //
//                          V0.0 - 10/15/2014                                     //
////////////////////////////////////////////////////////////////////////////////////

#include <string>

// Define the projector dimensions
#define SCRWIDTH 720*3
#define SCRHEIGHT 1280
#define SCRDEPTH 32

// Define constants for the windows
#define WNDCLASSNAME "GLClass"
#define WNDNAME "Stripe"

// Define a handle and device context for the window
extern HDC hdc1; //HDC = handle device context
extern HWND hwnd1; //HWND = window handle

// Define OpenGl rendering context
extern HGLRC hglrc;

// Define index pixel format for the window
extern int indexPixelFormat;

// Define a constant to control the running of the main while loop
extern bool quit;

// Variable to control the direction of the open loop stripe (+/-1) AND whether or not to display anything at all
extern int olsdir; 

