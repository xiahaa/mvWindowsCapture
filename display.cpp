// display.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include "imageCaptureLive.h"

#ifdef _WIN32
#   include <windows.h>
#	include <cstdalign>
#   include <process.h>
#elif defined(linux) || defined(__linux) || defined(__linux__)
#   if defined(__x86_64__) || defined(__aarch64__) || defined(__powerpc64__) // -m64 makes GCC define __powerpc64__
typedef uint64_t UINT_PTR;
#   elif defined(__i386__) || defined(__arm__) || defined(__powerpc__) // and -m32 __powerpc__
typedef uint32_t UINT_PTR;
#   endif
#endif // #ifdef _WIN32

#if _DEBUG
#pragma comment(lib, "opencv_core310d.lib")
#pragma comment(lib, "opencv_highgui310d.lib")
#pragma comment(lib, "opencv_imgproc310d.lib")
#else
#pragma comment(lib, "opencv_core310.lib")
#pragma comment(lib, "opencv_highgui310.lib")
#pragma comment(lib, "opencv_imgproc310.lib")
#endif

using namespace std;

//-----------------------------------------------------------------------------
int main(int /*argc*/, char* /*argv*/[])
//-----------------------------------------------------------------------------
{
	DeviceManager devMgr;
	Device* pDev = getDeviceFromUserInput(devMgr);
	if (!pDev)
	{
		cout << "Unable to continue!";
		cout << "Press [ENTER] to end the application" << endl;
		cin.get();
		return 0;
	}

	cout << "Initialising the device. This might take some time..." << endl;
	try
	{
		pDev->open();
	}
	catch (const ImpactAcquireException& e)
	{
		// this e.g. might happen if the same device is already opened in another process...
		cout << "An error occurred while opening device " << pDev->serial.read()
			<< "(error code: " << e.getErrorCodeAsString() << ")." << endl
			<< "Press [ENTER] to end the application..." << endl;
		cin.get();
		return 0;
	}

	CaptureParameter captureParams(pDev);

	//=============================================================================
	//========= Capture loop into memory managed by the driver (default) ==========
	//=============================================================================
	cout << "The device will try to capture continuously into memory automatically allocated be the device driver." << endl
		<< "This is the default behaviour." << endl;
	cout << "Press [ENTER] to end the continuous acquisition." << endl;
	runLiveLoop(captureParams);

	return 0;
}


