#pragma once
#include <mvIMPACT_CPP/mvIMPACT_acquire.h>
#include <apps/Common/exampleHelper.h>
#include <common/minmax.h>
#ifdef _WIN32
#   include <mvDisplay/Include/mvIMPACT_acquire_display.h>
#   include <windows.h>
#	include <cstdalign>
#   include <process.h>
#	include <mutex>
#endif
#include "datatype.h"
#include "circularBuffer.h"



using namespace mvIMPACT::acquire::display;

typedef struct __cameraSettings{
	int frameRate_Hz;
	int autoExposeControl;
	int binningMode;
	int exposeMode;
	int expose_us;
	int pixelFormat;
	int shutterMode;
}cameraSettings;

//-----------------------------------------------------------------------------
// the buffer we pass to the device driver must be aligned according to its requirements
// As we can't allocate aligned heap memory we will align it 'by hand'
class UserSuppliedHeapBuffer
	//-----------------------------------------------------------------------------
{
	char* pBuf_;
	int bufSize_;
	int alignment_;
	int pitch_;
public:
	explicit UserSuppliedHeapBuffer(int bufSize, int alignment, int pitch) : pBuf_(0), bufSize_(bufSize), alignment_(alignment), pitch_(pitch)
	{
		if (bufSize_ > 0)
		{
			pBuf_ = new char[bufSize_ + alignment_];
		}
	}
	~UserSuppliedHeapBuffer()
	{
		delete[] pBuf_;
	}
	char* getPtr(void)
	{
		if (alignment_ <= 1)
		{
			return pBuf_;
		}
		return reinterpret_cast<char*>((reinterpret_cast<UINT_PTR>(pBuf_), static_cast<UINT_PTR>(alignment_)));
	}
	int getSize(void) const
	{
		return bufSize_;
	}
};

typedef std::vector<UserSuppliedHeapBuffer*> CaptureBufferContainer;

// for camera properties
typedef std::map<std::string, Property> StringPropMap;

//-----------------------------------------------------------------------------
struct CaptureParameter
	//-----------------------------------------------------------------------------
{
	Device*                 pDev;
#ifdef USE_MATRIXVISION_DISPLAY_API
	ImageDisplayWindow*     pDisplayWindow;
#endif // #ifdef USE_DISPLAY
#ifdef BUILD_WITH_OPENCV_SUPPORT
	std::string             openCVDisplayTitle;
	std::string             openCVResultDisplayTitle;
#endif // #ifdef BUILD_WITH_OPENCV_SUPPORT
	FunctionInterface       fi;
	ImageRequestControl     irc;
	Statistics              statistics;
	bool                    boUserSuppliedMemoryUsed;
	bool                    boAlwaysUseNewUserSuppliedBuffers;
	int                     bufferSize;
	int                     bufferAlignment;
	int                     bufferPitch;
	CaptureBufferContainer  buffers;

	/* shared circular buffer */
	struct FIFO				*pCbuf;
	imageHeader_t			*pheader;
	std::mutex				*pheaderMutex;
	bool					headerInit;

	CaptureParameter(Device* p, struct FIFO* sharedBuf, imageHeader_t *ph, std::mutex *pmutex) : pDev(p), fi(p), irc(p), statistics(p), boUserSuppliedMemoryUsed(false),
		boAlwaysUseNewUserSuppliedBuffers(false), bufferSize(0), bufferAlignment(0), bufferPitch(0), buffers(),
		pCbuf(sharedBuf), pheader(ph), pheaderMutex(pmutex), headerInit(false)
	{
#ifdef USE_MATRIXVISION_DISPLAY_API
		// IMPORTANT: It's NOT save to create multiple display windows in multiple threads!!!
		const std::string windowTitle("mvIMPACT_acquire sample, Device " + pDev->serial.read());
		pDisplayWindow = new ImageDisplayWindow(windowTitle);
#endif // #ifdef USE_DISPLAY
#ifdef BUILD_WITH_OPENCV_SUPPORT
		openCVDisplayTitle = std::string("mvIMPACT_acquire sample, Device " + pDev->serial.read() + ", OpenCV display");
		openCVResultDisplayTitle = openCVDisplayTitle + "(Result)";
#endif // #ifdef BUILD_WITH_OPENCV_SUPPORT
	}
	~CaptureParameter()
	{
#ifdef USE_DISPLAY
		delete pDisplayWindow;
#endif // #ifdef USE_DISPLAY
	}
};

void runLiveLoop(CaptureParameter& captureParams);
void endLiveLoop();
