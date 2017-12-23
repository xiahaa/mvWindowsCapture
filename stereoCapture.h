#ifndef _STEREO_CAPTURE_H_
#define _STEREO_CAPTURE_H_

#ifdef _MSC_VER // is Microsoft compiler?
#   if _MSC_VER < 1300  // is 'old' VC 6 compiler?
#       pragma warning( disable : 4786 ) // 'identifier was truncated to '255' characters in the debug information'
#   endif // #if _MSC_VER < 1300
#endif // #ifdef _MSC_VER
#include <conio.h>
#include <windows.h>
#include <process.h>
#include <iostream>
#include "threadDatatype.h"

#include <apps/Common/exampleHelper.h>
#include <mvIMPACT_CPP/mvIMPACT_acquire.h>
#include <mvDisplay/Include/mvIMPACT_acquire_display.h>
using namespace std;
using namespace mvIMPACT::acquire;
using namespace mvIMPACT::acquire::display;

//-----------------------------------------------------------------------------
class DeviceData : public ThreadData
	//-----------------------------------------------------------------------------
{
	Device* pDev_;
	FunctionInterface* pFI_;
	IOSubSystemBlueCOUGAR* pIOSS_;
	Statistics* pSS_;
	ImageDisplayWindow* pDisplayWindow_;
	int lastRequestNr_;
	class CriticalSection g_critSect;
public:
	explicit DeviceData(Device* p) : ThreadData(), pDev_(p), pFI_(0), pSS_(0), pDisplayWindow_(0), pIOSS_(0), lastRequestNr_(INVALID_ID) {}
	~DeviceData()
	{
		delete pFI_;
		delete pSS_;
		delete pIOSS_;
		delete pDisplayWindow_;
		if (pFI_->isRequestNrValid(lastRequestNr_))
		{
			pFI_->imageRequestUnlock(lastRequestNr_);
			lastRequestNr_ = INVALID_ID;
		}
	}
	void init(const std::string& windowName)
	{
		pDev_->open();
		CameraSettingsBase cs(pDev_);
		// A buffers timeout shall never elapse
		cs.imageRequestTimeout_ms.write(0);
		pFI_ = new FunctionInterface(pDev_);
		pSS_ = new Statistics(pDev_);
		pIOSS_ = new IOSubSystemBlueCOUGAR(pDev_);
		{
			LockedScope lockedScope(g_critSect);
			cout << "Please note that there will be just one refresh for the display window, so if it is" << endl
				<< "hidden under another window the result will not be visible." << endl;
		}
		pDisplayWindow_ = new ImageDisplayWindow(windowName);
	}
	Device* device(void) const
	{
		return pDev_;
	}
	FunctionInterface* functionInterface(void) const
	{
		return pFI_;
	}
	IOSubSystemBlueCOUGAR* IOSS(void) const
	{
		return pIOSS_;
	}
	Statistics* statistics(void) const
	{
		return pSS_;
	}
	ImageDisplayWindow* pDisp(void)
	{
		return pDisplayWindow_;
	}
};

#endif