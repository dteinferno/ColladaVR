#include <windows.h>
#include "system.h"
#include "winmain.h"

void SysShutdown(void)
{
	wglMakeCurrent(hdc1, NULL);		// release device context
	wglDeleteContext(hglrc);		// delete rendering context
	PostQuitMessage(0);				// make sure the window will be destroyed
}
