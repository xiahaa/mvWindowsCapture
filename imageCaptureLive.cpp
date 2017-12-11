/* Matrix Vision C++ SDK for acquiring image data */
#include "stdafx.h"
#include <iostream>
#include "imageCaptureLive.h"
#include "propertyList.h"
#include <mvIMPACT_CPP/mvIMPACT_acquire_GenICam.h>

#ifdef BUILD_WITH_OPENCV_SUPPORT
// In order to build this application with OpenCV support there are 2 options:
// a) Use Visual Studio 2013 or later with OpenCV 2.4.13. With this version this example project already contains
//    project configurations 'Debug OpenCV' and 'Release OpenCV'. The only thing left to do in that case is to define
//    an environment variable 'OPENCV_DIR' that points to the top-level folder of the OpenCV 2.4.13 package. Afterwards
//    when restarting Visual Studio the application can be build with a simple OpenCV example.
//    - Different OpenCV versions can be used as well, but you need to state the OpenCV lib names matching your
//      version in the linker section then as the libraries contain the version in their name
// b) Older Visual Studio versions or Linux/gcc approaches must make sure that the include and lib paths to
//    OpenCV are set up correctly. However information on how to efficiently create OpenCV buffers can be extracted
//    from the code without compiling this application as well. Simply look for every occurrence of 'BUILD_WITH_OPENCV_SUPPORT'
//    in this source file.
#   include <opencv2/core/core.hpp>
#   include <opencv2/highgui/highgui.hpp>
#   include <opencv2/imgproc/imgproc.hpp>
#endif // #ifdef BUILD_WITH_OPENCV_SUPPORT

using namespace std;
using namespace mvIMPACT::acquire;

static bool s_boTerminated = false;

void checkCaptureBufferAddress(const Request* const pRequest, bool boShouldContainUserSuppliedMemory, const CaptureBufferContainer& buffers);
int  createCaptureBuffer(FunctionInterface& fi, CaptureBufferContainer& buffers, const int bufferSize, const int bufferAlignment, const int bufferPitch, unsigned int requestNr);
int  createCaptureBuffers(FunctionInterface& fi, CaptureBufferContainer& buffers, const int bufferSize, const int bufferAlignment, const int bufferPitch);
void freeCaptureBuffer(FunctionInterface& fi, CaptureBufferContainer& buffers, unsigned int requestNr);
void freeCaptureBuffers(FunctionInterface& fi, CaptureBufferContainer& buffers);
CaptureBufferContainer::iterator lookUpBuffer(CaptureBufferContainer& buffers, void* pAddr);

//-----------------------------------------------------------------------------
/// \brief This function checks whether a buffer returned from an acquisition into a
/// request that has been assigned a user supplied buffer really contains a buffer
/// pointer that has been assigned by the user.
void checkCaptureBufferAddress(const Request* const pRequest, bool boShouldContainUserSuppliedMemory, const CaptureBufferContainer& buffers)
//-----------------------------------------------------------------------------
{
	if (boShouldContainUserSuppliedMemory && (pRequest->imageMemoryMode.read() != rimmUser))
	{
		cout << "ERROR: Request number " << pRequest->getNumber() << " is supposed to contain user supplied memory, but claims that it doesn't." << endl;
		return;
	}
	else if (!boShouldContainUserSuppliedMemory)
	{
		if (pRequest->imageMemoryMode.read() == rimmUser)
		{
			cout << "ERROR: Request number " << pRequest->getNumber() << " is supposed NOT to contain user supplied memory, but claims that it does." << endl;
		}
		return;
	}

	void* pAddr = pRequest->imageData.read();
	const CaptureBufferContainer::size_type vSize = buffers.size();
	for (CaptureBufferContainer::size_type i = 0; i < vSize; i++)
	{
		if (pAddr == buffers[i]->getPtr())
		{
			// found the buffer that has been assigned by the user
			return;
		}
	}

	cout << "ERROR: A buffer has been returned, that doesn't match any of the buffers assigned as user memory in request number " << pRequest->getNumber() << "." << endl;
	cout << "Buffer got: 0x" << pAddr << endl;
	cout << "Buffers allocated:" << endl;
	for (CaptureBufferContainer::size_type j = 0; j < vSize; j++)
	{
		cout << "[" << j << "]: 0x" << reinterpret_cast<void*>(buffers[j]->getPtr()) << endl;
	}
}

//-----------------------------------------------------------------------------
int createCaptureBuffer(FunctionInterface& fi, CaptureBufferContainer& buffers, const int bufferSize, const int bufferAlignment, const int bufferPitch, unsigned int requestNr)
//-----------------------------------------------------------------------------
{
	int functionResult = DMR_NO_ERROR;
	Request* pRequest = fi.getRequest(requestNr);
	UserSuppliedHeapBuffer* pBuffer = new UserSuppliedHeapBuffer(bufferSize, bufferAlignment, bufferPitch);
	if ((functionResult = pRequest->attachUserBuffer(pBuffer->getPtr(), pBuffer->getSize())) != DMR_NO_ERROR)
	{
		cout << "An error occurred while attaching a buffer to request number " << requestNr << ": " << ImpactAcquireException::getErrorCodeAsString(functionResult) << "." << endl;
		delete pBuffer;
		return -1;
	}
	buffers.push_back(pBuffer);
	return 0;
}

//-----------------------------------------------------------------------------
int createCaptureBuffers(FunctionInterface& fi, CaptureBufferContainer& buffers, const int bufferSize, const int bufferAlignment, const int bufferPitch)
//-----------------------------------------------------------------------------
{
	freeCaptureBuffers(fi, buffers);

	unsigned int requestCnt = fi.requestCount();
	for (unsigned int i = 0; i < requestCnt; i++)
	{
		try
		{
			const int result = createCaptureBuffer(fi, buffers, bufferSize, bufferAlignment, bufferPitch, i);
			if (result != 0)
			{
				freeCaptureBuffers(fi, buffers);
				return result;
			}
		}
		catch (const ImpactAcquireException& e)
		{
			freeCaptureBuffers(fi, buffers);
			return e.getErrorCode();
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
void freeCaptureBuffer(FunctionInterface& fi, CaptureBufferContainer& buffers, unsigned int requestNr)
//-----------------------------------------------------------------------------
{
	try
	{
		int functionResult = DMR_NO_ERROR;
		Request* pRequest = fi.getRequest(requestNr);
		if (pRequest->imageMemoryMode.read() == rimmUser)
		{
			if ((functionResult = pRequest->detachUserBuffer()) != DMR_NO_ERROR)
			{
				cout << "An error occurred while detaching a buffer from request number " << requestNr << " : " << ImpactAcquireException::getErrorCodeAsString(functionResult) << "." << endl;
			}
		}
		CaptureBufferContainer::iterator it = lookUpBuffer(buffers, pRequest->imageData.read());
		if (it != buffers.end())
		{
			delete (*it);
			buffers.erase(it);
		}
	}
	catch (const ImpactAcquireException& e)
	{
		cout << "An error occurred while changing the mode of request number " << requestNr << ": " << e.getErrorCodeAsString() << "." << endl;
	}
}

//-----------------------------------------------------------------------------
void freeCaptureBuffers(FunctionInterface& fi, CaptureBufferContainer& buffers)
//-----------------------------------------------------------------------------
{
	const unsigned int requestCnt = fi.requestCount();
	for (unsigned int i = 0; i < requestCnt; i++)
	{
		freeCaptureBuffer(fi, buffers, i);
	}

	if (buffers.empty() == false)
	{
		cout << "Error! The buffer container should be empty now but still contains " << buffers.size() << " elements!" << endl;
	}
}


//-----------------------------------------------------------------------------
CaptureBufferContainer::iterator lookUpBuffer(CaptureBufferContainer& buffers, void* pAddr)
//-----------------------------------------------------------------------------
{
	const CaptureBufferContainer::iterator itEND = buffers.end();
	CaptureBufferContainer::iterator it = buffers.begin();
	while (it != itEND)
	{
		if (pAddr == (*it)->getPtr())
		{
			// found the buffer that has been assigned by the user
			break;
		}
		++it;
	}
	return it;
}

//-----------------------------------------------------------------------------
void displayImage(CaptureParameter* pCaptureParameter, Request* pRequest)
//-----------------------------------------------------------------------------
{
#if !defined(USE_DISPLAY) && !defined(BUILD_WITH_OPENCV_SUPPORT)
	// suppress compiler warnings
	pRequest = pRequest;
	pCaptureParameter = pCaptureParameter;
#endif // #if !defined(USE_DISPLAY) && !defined(BUILD_WITH_OPENCV_SUPPORT)
#ifdef USE_DISPLAY
	pCaptureParameter->pDisplayWindow->GetImageDisplay().SetImage(pRequest);
	pCaptureParameter->pDisplayWindow->GetImageDisplay().Update();
#endif // #ifdef USE_DISPLAY
#ifdef BUILD_WITH_OPENCV_SUPPORT
	int openCVDataType = CV_8UC1;
	switch (pRequest->imagePixelFormat.read())
	{
	case ibpfMono8:
		openCVDataType = CV_8UC1;
		break;
	case ibpfMono10:
	case ibpfMono12:
	case ibpfMono14:
	case ibpfMono16:
		openCVDataType = CV_16UC1;
		break;
	case ibpfMono32:
		openCVDataType = CV_32SC1;
		break;
	case ibpfBGR888Packed:
	case ibpfRGB888Packed:
		openCVDataType = CV_8UC3;
		break;
	case ibpfRGBx888Packed:
		openCVDataType = CV_8UC4;
		break;
	case ibpfRGB101010Packed:
	case ibpfRGB121212Packed:
	case ibpfRGB141414Packed:
	case ibpfRGB161616Packed:
		openCVDataType = CV_16UC3;
		break;
	case ibpfMono12Packed_V1:
	case ibpfMono12Packed_V2:
	case ibpfBGR101010Packed_V2:
	case ibpfRGB888Planar:
	case ibpfRGBx888Planar:
	case ibpfYUV422Packed:
	case ibpfYUV422_10Packed:
	case ibpfYUV422_UYVYPacked:
	case ibpfYUV422_UYVY_10Packed:
	case ibpfYUV422Planar:
	case ibpfYUV444Packed:
	case ibpfYUV444_10Packed:
	case ibpfYUV444_UYVPacked:
	case ibpfYUV444_UYV_10Packed:
	case ibpfYUV444Planar:
	case ibpfYUV411_UYYVYY_Packed:
		cout << "ERROR! Don't know how to render this pixel format (" << pRequest->imagePixelFormat.readS() << ") in OpenCV! Select another one e.g. by writing to mvIMPACT::acquire::ImageDestination::pixelFormat!" << endl;
		exit(42);
		break;
	}

	//std::cout<<"capture frequency"<<pRequest->

	//cv::Mat  openCVImage(cv::Size(pRequest->imageWidth.read(), pRequest->imageHeight.read()), CV_8UC1);
	//memcpy(openCVImage.data, pRequest->imageData.read(), openCVImage.rows*openCVImage.cols * 1);

	cv::Mat openCVImage(cv::Size(pRequest->imageWidth.read(), pRequest->imageHeight.read()), openCVDataType, pRequest->imageData.read(), pRequest->imageLinePitch.read());
	cv::namedWindow(pCaptureParameter->openCVDisplayTitle, cv::WINDOW_NORMAL);
	cv::resizeWindow(pCaptureParameter->openCVDisplayTitle, 640, 480);
	cv::imshow(pCaptureParameter->openCVDisplayTitle, openCVImage);
	// OpenCV event handling: you need this!
	cv::waitKey(5);
	// apply Canny Edge detection and display the result too
	//cv::Mat edgesMat;
	/*	switch (openCVDataType)
	{
	case CV_16UC3:
	cout << "This format seems to crash the Canny Edge detector. Will display the original image instead!" << endl;
	edgesMat = openCVImage;
	break;
	default:
	cv::Canny(openCVImage, edgesMat, 35.0, 55.0);
	break;
	}
	cv::imshow(pCaptureParameter->openCVResultDisplayTitle, edgesMat);
	// OpenCV event handling: you need this!
	cv::waitKey(5);
	*/
#endif // #ifdef BUILD_WITH_OPENCV_SUPPORT
}


// 
//-----------------------------------------------------------------------------
void populatePropertyMap(StringPropMap& m, ComponentIterator it, const string& currentPath = "")
//-----------------------------------------------------------------------------
{
	while (it.isValid())
	{
		string fullName(currentPath);
		if (fullName != "")
		{
			fullName += "/";
		}
		fullName += it.name();
		if (it.isList())
		{
			populatePropertyMap(m, it.firstChild(), fullName);
		}
		else if (it.isProp())
		{
			m.insert(make_pair(fullName, Property(it)));
		}
		++it;
		// method object will be ignored...
	}
}

#ifdef INIT_PROPERTY_MAP
bool initPropertyMap(Device *pDev)
{
	StringPropMap			propertyMap;

	// obtain all the settings related properties available for this device
	// Only work with the 'Base' setting. For more information please refer to the manual (working with settings)
	DeviceComponentLocator locator(pThreadParameter->pDev, dltSetting, "Base");
	populatePropertyMap(propertyMap, ComponentIterator(locator.searchbase_id()).firstChild());
	try
	{
		// this category is not supported by every device, thus we can expect an exception if this feature is missing
		locator = DeviceComponentLocator(pThreadParameter->pDev, dltIOSubSystem);
		populatePropertyMap(propertyMap, ComponentIterator(locator.searchbase_id()).firstChild());
	}
	catch (const ImpactAcquireException&) {}
	locator = DeviceComponentLocator(pThreadParameter->pDev, dltRequest);
	populatePropertyMap(propertyMap, ComponentIterator(locator.searchbase_id()).firstChild());
	locator = DeviceComponentLocator(pThreadParameter->pDev, dltSystemSettings);
	populatePropertyMap(propertyMap, ComponentIterator(locator.searchbase_id()).firstChild(), string("SystemSettings"));
	locator = DeviceComponentLocator(pThreadParameter->pDev, dltInfo);
	populatePropertyMap(propertyMap, ComponentIterator(locator.searchbase_id()).firstChild(), string("Info"));
	populatePropertyMap(propertyMap, ComponentIterator(pThreadParameter->pDev->hDev()).firstChild(), string("Device"));


}
#endif

bool configureMonoCamera(Device *pDev)
{
	GenICam::AcquisitionControl ac(pDev);
	GenICam::CounterAndTimerControl ctc(pDev);

	const int fps = 10;
	// if support FrameRateExactness feature
	if (ac.acquisitionFrameRateEnable.read())
	{
		ac.acquisitionFrameRate.write(fps);// 10 Hz sampling
	}
	else
	{
		// else use Timer to get specific frame per rate
		ctc.timerSelector.writeS("Timer1");			// use timer 1
		ctc.timerDelay.write(0);					// no delay
		ctc.timerDuration.write(1000000.0 / fps);	// overflow time
		ctc.timerTriggerSource.writeS("Timer1End");	// trigger generated
		ac.triggerSelector.writeS("FrameStart");	// triggered behavior
		ac.triggerMode.writeS("On");				// enable trigger mode
		ac.triggerSource.writeS("Timer1End");		// trigger souce
	}
	conditionalSetEnumPropertyByString(ac.exposureMode, "Timed");		//
	conditionalSetEnumPropertyByString(ac.exposureAuto, "Continuous");	// AE mode 
	conditionalSetEnumPropertyByString(ac.mvExposureAutoSpeed, "Fast");	// AE speed
	ac.acquisitionMode.writeS("Continuous");							// acquision mode
	//ac.exposureTime.write(20000.000);
	ac.mvExposureAutoLowerLimit.write(100);								// minimum AE time
	ac.mvExposureAutoUpperLimit.write(100000);							// maximum AE time
	ac.mvExposureAutoAverageGrey.write(50);								// 

	return true;
}

//-----------------------------------------------------------------------------
unsigned int DMR_CALL liveLoop(void* pData)
//-----------------------------------------------------------------------------
{
	CaptureParameter* pThreadParameter = reinterpret_cast<CaptureParameter*>(pData);

	/* GenlCam wrapper class for property control */
	GenICam::AcquisitionControl ac(pThreadParameter->pDev);
	GenICam::DeviceControl dc(pThreadParameter->pDev);
	GenICam::ImageFormatControl ifc(pThreadParameter->pDev);
	GenICam::CounterAndTimerControl ctc(pThreadParameter->pDev);
	// Chunk mode is needed in order to get back all the information needed to properly check
	// if an image has been taken using the desired parameters.
	GenICam::ChunkDataControl cdc(pThreadParameter->pDev);
	cdc.chunkModeActive.write(bTrue);

	configureMonoCamera(pThreadParameter->pDev);

	bool showPropertiesOnce = false;

	// Send all requests to the capture queue. There can be more than 1 queue for some devices, but for this sample
	// we will work with the default capture queue. If a device supports more than one capture or result
	// queue, this will be stated in the manual. If nothing is mentioned about it, the device supports one
	// queue only. This loop will send all requests currently available to the driver. To modify the number of requests
	// use the property mvIMPACT::acquire::SystemSettings::requestCount at runtime or the property
	// mvIMPACT::acquire::Device::defaultRequestCount BEFORE opening the device.
	TDMR_ERROR result = DMR_NO_ERROR;
	while ((result = static_cast<TDMR_ERROR>(pThreadParameter->fi.imageRequestSingle())) == DMR_NO_ERROR) {};
	if (result != DEV_NO_FREE_REQUEST_AVAILABLE)
	{
		cout << "'FunctionInterface.imageRequestSingle' returned with an unexpected result: " << result
			<< "(" << ImpactAcquireException::getErrorCodeAsString(result) << ")" << endl;
	}

	manuallyStartAcquisitionIfNeeded(pThreadParameter->pDev, pThreadParameter->fi);
	// run thread loop
	unsigned int cnt = 0;
	const unsigned int timeout_ms = 500;
	Request* pRequest = 0;
	// we always have to keep at least 2 images as the display module might want to repaint the image, thus we
	// cannot free it unless we have a assigned the display to a new buffer.
	Request* pPreviousRequest = 0;
	while (!s_boTerminated)
	{
		// wait for results from the default capture queue
		int requestNr = pThreadParameter->fi.imageRequestWaitFor(timeout_ms);
		pRequest = pThreadParameter->fi.isRequestNrValid(requestNr) ? pThreadParameter->fi.getRequest(requestNr) : 0;
		if (pRequest)
		{
			if (pRequest->isOK())
			{
				if (!showPropertiesOnce)
				{
					showPropertiesOnce = true;
#if SHOW_INTERESTED_PROPERTIES
	#if SHOW_DEVICE_PROP			1
		DUMPDEVICEPROP(dc)
    #endif
	#if SHOW_IMG_FORMAT_PROP		1
		DUMPIMGFORMATPROP(ifc)
	#endif
	#if SHOW_ACQUISION_PROP			1
		DUMPACQUISIONPROP(ac)
	#endif
#else
					for_each(propertyMap.begin(), propertyMap.end(), DisplayProperty());
#endif
				}
				
				++cnt;
				// here we can display some statistical information every 100th image
				if (cnt % 100 == 0)
				{
					cout << "exposure time" << pRequest->chunkExposureTime.read() << endl;
					cout << "Info from " << pThreadParameter->pDev->serial.read()
						<< ": " << pThreadParameter->statistics.framesPerSecond.name() << ": " << pThreadParameter->statistics.framesPerSecond.readS()
						<< ", " << pThreadParameter->statistics.errorCount.name() << ": " << pThreadParameter->statistics.errorCount.readS()
						<< ", " << pThreadParameter->statistics.captureTime_s.name() << ": " << pThreadParameter->statistics.captureTime_s.readS()
						<< ", CaptureDimension: " << pRequest->imageWidth.read() << "x" << pRequest->imageHeight.read() << "(" << pRequest->imagePixelFormat.readS() << ")" << endl;
				}
				displayImage(pThreadParameter, pRequest);
				checkCaptureBufferAddress(pRequest, pThreadParameter->boUserSuppliedMemoryUsed, pThreadParameter->buffers);
			}
			else
			{
				cout << "Error: " << pRequest->requestResult.readS() << endl;
			}
			if (pPreviousRequest)
			{
				// this image has been displayed thus the buffer is no longer needed...
				pPreviousRequest->unlock();
				if (pThreadParameter->boAlwaysUseNewUserSuppliedBuffers)
				{
					// attach a fresh piece of memory
					freeCaptureBuffer(pThreadParameter->fi, pThreadParameter->buffers, pPreviousRequest->getNumber());
					createCaptureBuffer(pThreadParameter->fi, pThreadParameter->buffers, pThreadParameter->bufferSize, pThreadParameter->bufferAlignment, pThreadParameter->bufferPitch, pPreviousRequest->getNumber());
				}
			}
			pPreviousRequest = pRequest;
			// send a new image request into the capture queue
			pThreadParameter->fi.imageRequestSingle();
		}
		else
		{
			// If the error code is -2119(DEV_WAIT_FOR_REQUEST_FAILED), the documentation will provide
			// additional information under TDMR_ERROR in the interface reference
			cout << "imageRequestWaitFor failed (" << requestNr << ", " << ImpactAcquireException::getErrorCodeAsString(requestNr) << ")"
				<< ", timeout value too small?" << endl;
		}
#if defined(linux) || defined(__linux) || defined(__linux__)
		s_boTerminated = checkKeyboardInput() == 0 ? false : true;
#endif // #if defined(linux) || defined(__linux) || defined(__linux__)
	}
	manuallyStopAcquisitionIfNeeded(pThreadParameter->pDev, pThreadParameter->fi);
#ifdef USE_MATRIXVISION_DISPLAY_API
	// stop the display from showing freed memory
	pThreadParameter->pDisplayWindow->GetImageDisplay().SetImage(reinterpret_cast<Request*>(0));
#endif // #ifdef USE_DISPLAY
#ifdef BUILD_WITH_OPENCV_SUPPORT
	//cv::destroyAllWindows();
#endif // #ifdef BUILD_WITH_OPENCV_SUPPORT
	// In this sample all the next lines are redundant as the device driver will be
	// closed now, but in a real world application a thread like this might be started
	// several times an then it becomes crucial to clean up correctly.

	// free the last potentially locked request
	if (pRequest)
	{
		pRequest->unlock();
	}
	// clear all queues
	pThreadParameter->fi.imageRequestReset(0, 0);
	return 0;
}

//-----------------------------------------------------------------------------
void runLiveLoop(CaptureParameter& captureParams)
//-----------------------------------------------------------------------------
{
	s_boTerminated = false;
#if defined(linux) || defined(__linux) || defined(__linux__)
	liveLoop(&captureParams);
#else
	// start the execution of the 'live' thread.
	unsigned int dwThreadID = 0;
	HANDLE hThread = (HANDLE)_beginthreadex(0, 0, liveLoop, (LPVOID)(&captureParams), 0, &dwThreadID);
	cin.get();
	s_boTerminated = true;
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
#endif // #if defined(linux) || defined(__linux) || defined(__linux__)
}