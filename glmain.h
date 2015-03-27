/*
Add GLEW.  From the website:
The OpenGL Extension Wrangler Library (GLEW) is a cross-platform open-source C/C++ extension loading library.
GLEW provides efficient run-time mechanisms for determining which OpenGL extensions are supported on the target platform.
OpenGL core and extension functionality is exposed in a single header file.
GLEW has been tested on a variety of operating systems, including Windows, Linux, Mac OS X, FreeBSD, Irix, and Solaris.
*/
#include "GL/glew.h"

// Add libraries
#include <string>
#include <vector>
#include <iostream>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <tchar.h>

////////////////////////////////////////////////////////////////////////////////////
//        Header File for the Main Routine for Drawing Objects via Open           //
////////////////////////////////////////////////////////////////////////////////////
//                           Dan Turner-Evans                                     //
//                          V0.0 - 10/15/2014                                     //
////////////////////////////////////////////////////////////////////////////////////

#include <time.h>
#include <cstdio>
#include <map>
#include <list>
#define GLM_SWIZZLE	// Enable GLM Swizzling, must be before glm is included!
#include "glm\glm.hpp"  // Core GLM stuff, same as GLSL math.
#include "glm\ext.hpp"	// GLM extensions.

// Boost headers for setting up the data server
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>

// Load Simple OpenGL Image Library
#include "SOIL.h"

// Define pi for use in converting degrees to radians
#ifndef M_PI
#define M_PI 3.1415926535
#endif

// To switch between open and closed loop
extern int closed;

// To stop all motion
extern int stopped;

// Set the distance between the fly and the initial object of interest
extern float dist2stripe;

// Variables that hold the current ball offset
extern float BallOffsetRotNow;
extern float BallOffsetForNow;
extern float BallOffsetSideNow;

// Initialize the OpenGL buffers, etc...
void InitOpenGL(void);

// Draw the scene
void RenderFrame(int direction);

// Clear the buffers, delete the shaders, etc...
void GLShutdown(void);

// Load an object
bool loadOBJ(const char * path, std::vector<glm::vec3> & out_vertices, std::vector<glm::vec2> & out_uvs, std::vector<glm::vec3> & out_normals);
