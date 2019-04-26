/*
* mvMonoCapture.cpp
*
*  Created on: 19-12-2017
*		Author: xiaohu
*/
#include "stdafx.h"
#include "mvMonocapture.h"

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <chrono>
//thread
#include <atomic>
#include <thread>
#include <mutex>
//#include <condition_variable>
//types
#include <cstring>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
//mvcam
#include <apps/Common/exampleHelper.h>
#include "mvCamHelper.h"

//misc
#include <cstdint>
#include <cstdio>

#include "datatype.h"
//frame queue
#include "safe_queue.h"
//header files related to opencv
#include <opencv2/opencv.hpp>
#include "deviceConfigure.h"

using namespace std;

//Init logstream used by individual threads
static OFStreamThread logstream;
//11. Thread Initialisation & launch
//a) allocate thread parameters according to number of imageThreads
static std::atomic<bool> threadRunFlags[2];
static std::thread threads[2];
static ofstream logfile;

//std::mutex CoutThread::mutexPrint{};

//-----------------------------------------------------------------------------
//
unsigned int imageAcquisitionThread(void* pData, unsigned int camID,
	string devname, unsigned int nImagePairs, unsigned int decim,
	bool logEna, unsigned int logInt, bool dispEna, unsigned int dispInt,
	atomic<bool> &threadRun, void* pThreadParams)
{
	DeviceData* pThreadParameter = reinterpret_cast<DeviceData*>(pData);
	mvMonoCaptureParameters* pTreadParams = reinterpret_cast<mvMonoCaptureParameters*>(pThreadParams);

	unsigned int cnt = 0;
	unsigned int saveCnt = 0;

	//ChunkDataControl *pChunk = new ChunkDataControl(pThreadParameter->pDev_);
	// establish access to the statistic properties
	Statistics* pSS = pThreadParameter->statistics();
	// create an interface to the device found
	FunctionInterface* pFI = pThreadParameter->functionInterface();
	stringstream logSS;
	SafeQueue<raw_frame_t> &queue = pTreadParams->rawImageQueue;

	// create AcquisitionControl Interface
	GenICam::AcquisitionControl ac(pThreadParameter->device());
	//All image requests are sent to the capture queue.
	//Some cameras have more than one queue, but for this example,
	//  the default queue is used.
	//In this loop, all pending requests are sent to the camera driver. Default operation is one image per request.
	//To modify the number of requests use the property mvIMPACT::acquire::SystemSettings::requestCount
	//  at runtime or the property mvIMPACT::acquire::Device::defaultRequestCount BEFORE opening the device.
	TDMR_ERROR result = DMR_NO_ERROR;
	while ((result = static_cast<TDMR_ERROR>(pFI->imageRequestSingle()))
		== DMR_NO_ERROR) {
	};
	if (result != DEV_NO_FREE_REQUEST_AVAILABLE) {
		//LockedScope lockedScope( g_critSect );
		if (logEna == true) {
			logSS.str("");
			logSS
				<< "'FunctionInterface.imageRequestSingle' returned with an unexpected result: "
				<< result << "("
				<< ImpactAcquireException::getErrorCodeAsString(result)
				<< ")" << std::this_thread::get_id() << "\n";
			logstream << logSS.str();
		}
	}

	//reset statistics
	pSS->reset();

	manuallyStartAcquisitionIfNeeded(pThreadParameter->device(), *pFI);
	// run thread loop
	const Request* pRequest = 0;
	const unsigned int timeout_ms = 5000;
	bool run = 1;
	int requestNr = INVALID_ID;
	//Keep an extra image in the image request queue,
	//in case the image must be accessed twice (double buffering)
	int lastRequestNr = INVALID_ID;

	cv::Mat image, imageSrc, imageDest;
	raw_frame_t currFrame;

	string winName = "src" + std::to_string(camID);
	cv::namedWindow(winName, cv::WINDOW_NORMAL);
	cv::resizeWindow(winName, 640, 480);
	cv::moveWindow(winName, 100, 100);
	cv::Mat srcResize;

	int deciCnt = 0;

	while (threadRun && run) {
		// wait for results from the default capture queue
		requestNr = pFI->imageRequestWaitFor(timeout_ms);
		if (pFI->isRequestNrValid(requestNr)) {
			pRequest = pFI->getRequest(requestNr);
			if (pRequest->isOK()) {
				//set frame parameters and data
				currFrame.cam = camID;
				currFrame.decim = decim;

				image = cv::Mat(
					cv::Size(pRequest->imageWidth.read(), pRequest->imageHeight.read()),
					CV_8UC1, pRequest->imageData.read(),
					pRequest->imageLinePitch.read());

				currFrame.decim = 1;
				currFrame.data = image.clone();

				//currFrame.data.clear();
				//cv::imencode(".png",image,currFrame.data);
				bool needEnque = false;
				if (pTreadParams->mvCapParams.captureMode == CIRCULAR_PATTERN)
				{
					if (!pTreadParams->mvCapParams.axisSet)
					{
						if (saveCnt != pTreadParams->syncSaveCnt)
						{
							saveCnt = pTreadParams->syncSaveCnt;
							currFrame.id = cnt;
							currFrame.isSave = 1;
							queue.enqueue(currFrame);
							++cnt;
						}
					}
					else
					{
						currFrame.id = pRequest->infoFrameID.read();
						currFrame.isSave = 1;
						queue.enqueue(currFrame);
						++cnt;
					}
				}
				else
				{
					if (pTreadParams->mvCapParams.isCaptureContinuous == 1)
					{
						currFrame.id = pRequest->infoFrameID.read();
						currFrame.isSave = 1;
						queue.enqueue(currFrame);
						++cnt;
					}
					else
					{
						if (saveCnt != pTreadParams->syncSaveCnt)
						{
							saveCnt = pTreadParams->syncSaveCnt;
							currFrame.id = cnt;
							currFrame.isSave = 1;
							queue.enqueue(currFrame);
							++cnt;
						}
					}
				}
#if DISPLAY
				cv::resize(currFrame.data, srcResize, cv::Size(640, 480), 0, 0, CV_INTER_LINEAR);
				//currFrame.data.copyTo(srcResize);
#endif

#if	ENABLE_MANUAL_FOCUS
				cv::Size size = currFrame.data.size();
				cv::Mat laplacian(size.height, size.width, CV_16SC1);
				cv::Laplacian(currFrame.data, laplacian, CV_16SC1, 1, 1, 0, cv::BORDER_DEFAULT);
				cv::Scalar mean, std;
				cv::meanStdDev(laplacian, mean, std);
				stringstream varLapLSS;
				varLapLSS << "VarOfLapL= " << std.val[0] * std.val[0];
				cv::putText(srcResize, varLapLSS.str(),
					cvPoint(50, 50), CV_FONT_HERSHEY_COMPLEX_SMALL,
					2, cv::Scalar(255, 0, 0), 2, CV_AA);
#endif

#if DISPLAY
				cv::imshow(winName, srcResize);
				cv::waitKey(5);
#endif

#if 0
				// test to get device logged frame id
				unsigned long long fcnt = pSS->frameCount.read();
				cout << "frame count from mvcapture SDK: " << fcnt << endl;
				cout << "chunk frame id: " << pChunk->chunkFrameID.read() << endl;
				cout << "frame id: " << pRequest->infoFrameID.read() << endl;
				cout << "frame Nr: " << pRequest->infoFrameNr.read() << endl;
#endif

				//display statistics (and write log) every Xth frame. Interval is controlled by the user from logInt and dispInt
#if ENABLE_LOG
				if (cnt % dispInt == 0 && dispEna == true) {
					LOG_INFO("Cam %s:%s:%s,%s,%s,%s,%f",
						pThreadParameter->device()->serial.read().c_str(),
						pSS->framesPerSecond.name().c_str(),
						pSS->framesPerSecond.readS().c_str(),
						pSS->errorCount.name().c_str(),
						pSS->errorCount.readS().c_str(),
						"Exposure time (uS)",
						ac.exposureTime.read());
				}
#endif
#if ENABLE_LOG_DUMP
				if (cnt % logInt == 0 && logEna == true) {
					logSS.str("");
					logSS << "Cam " << pThreadParameter->device()->serial.read() << ": "
						<< pSS->framesPerSecond.name() << ": "
						<< pSS->framesPerSecond.readS() << ", " << pSS->errorCount.name()
						<< ": " << pSS->errorCount.readS() << ", "
						<< pRequest->infoFrameNr << ", " << pRequest->infoFrameID << ", "
						<< pSS->frameCount << ", " << "Exposure time (uS)" << ": "
						<< ac.exposureTime.read() << "\n";
					logstream << logSS.str();
				}
#endif
				if (nImagePairs > 0 && cnt >= nImagePairs) {
					run = false;
				}
			}
			else {
#if ENABLE_LOG_DUMP
				//LOG_INFO("ERROR: %s %d",pRequest->requestResult.readS(),std::this_thread::get_id());
				if (logEna == true) {
					logSS.str("");
					logSS << "ERROR: " << pRequest->requestResult.readS()
						<< std::this_thread::get_id() << "\n";
					logstream << logSS.str();
				}
#endif
			}
			if (pFI->isRequestNrValid(lastRequestNr)) {
				// this image has been stored thus the buffer is no longer needed...
				pFI->imageRequestUnlock(lastRequestNr);
			}
			lastRequestNr = requestNr;
			// send a new image request into the capture queue
			pFI->imageRequestSingle();
		}
		else {
#if ENABLE_LOG
			//// If the error code is -2119(DEV_WAIT_FOR_REQUEST_FAILED), the documentation will provide
			//// additional information under TDMR_ERROR in the interface reference
			LOG_INFO("ERROR: imageRequestWaitFor failed (%d %s, device %s) timeout value too small?",
				requestNr, ImpactAcquireException::getErrorCodeAsString(requestNr).c_str(),
				pThreadParameter->device()->serial.read().c_str());
#endif
#if ENABLE_LOG_DUMP
			if (logEna == true) {
				logSS.str("");
				logSS << "ERROR: imageRequestWaitFor failed (" << requestNr << ", "
					<< ImpactAcquireException::getErrorCodeAsString(requestNr)
					<< ", device " << pThreadParameter->device()->serial.read() << ")"
					<< ", timeout value too small?" << "\n"; //<< std::this_thread::get_id() << "\n";
				logstream << logSS.str();
			}
			//TODO: If an error occurs, the image acquisition should always end (gracefully)!
#endif
		}
	}
#if	ENABLE_LOG_DUMP
	if (logEna == true) {
		logSS.str("");
		logSS << "INFO: Overall good frames captured from device "
			<< pThreadParameter->device()->serial.read() << ": " << cnt
			<< "\n";
		logstream << logSS.str();
	}
#endif
	LOG_INFO("INFO: Overall good frames captured from device %s:%d",
		(pThreadParameter->device()->serial.read()).c_str(), cnt);

	manuallyStopAcquisitionIfNeeded(pThreadParameter->device(), *pFI);

	// In this sample all the next lines are redundant as the device driver will be
	// closed now, but in a real world application a thread like this might be started
	// several times an then it becomes crucial to clean up correctly.
	// free the last potentially locked request
	if (pFI->isRequestNrValid(requestNr)) {
		pFI->imageRequestUnlock(requestNr);
	}
	// clear the request queue
	pFI->imageRequestReset(0, 0);
	// extract and unlock all requests that are now returned as 'aborted'
	while ((requestNr = pFI->imageRequestWaitFor(0)) >= 0) {
		pFI->imageRequestUnlock(requestNr);
	}

	LOG_INFO("INFO: Closing Image Acquisition Thread, %s", devname.c_str());
	if (logEna == true) {
		logSS.str("");
		logSS << "INFO: Closing Image Acquisition Thread, " << devname << "\n";
		logstream << logSS.str();
	}
	//Ensure user presses any key as the application expects a key to end the main loop
	if (nImagePairs > 0) {
		LOG_INFO("%s", "Press [any key] to continue...");
	}
	return 0;
}

bool startMonoImageAcquisitions(mvMonoCaptureParameters &mvCapHandler, DeviceData* pdev)
{
	string baseDir = mvCapHandler.baseDir;

#if ENABLE_LOG_DUMP
	string storageLog = baseDir + "encoding.log";
	logfile.open(storageLog, std::ios::out);
	if (logfile.is_open())
	{
		std::cout << "INFO: acquisition Logfile ready." << std::endl;
		logfile << "acquisition LOG START" << std::endl;
		//Init logstream used by individual threads
		logstream.init(&logfile);
	}
	else
	{
		std::cout << "ERROR: Unable to open logfile. Logging disabled." << endl;
		return false;
	}
#endif

	//10. Trigger Initialisation
	//setup and activate trigger (for master device) if no external sync is used
	try
	{
		if (mvCapHandler.mvCapParams.extSyncEnabled == false)
		{
			//Type 0 Acquisition - Trigger controlled by master camera
			if ((mvCapHandler.mvCapParams.triggerFrequency_Hz >= 1)
				&& (mvCapHandler.mvCapParams.triggerFrequency_Hz <= 30))
			{
#if 1
				if (mvCapHandler.mvCapParams.writeLogEnabled == true)
				{
					logfile << "INFO: The camera trigger frequency has been set to "
						<< mvCapHandler.mvCapParams.triggerFrequency_Hz << "Hz" << endl;
				}
#endif
				std::cout << endl
					<< "INFO: The camera trigger frequency has been set to "
					<< mvCapHandler.mvCapParams.triggerFrequency_Hz << "Hz" << endl;
			}
			else
			{
				std::cout << "ERROR: Invalid camera trigger frequency of freq="
					<< mvCapHandler.mvCapParams.triggerFrequency_Hz << "Hz has been set." << endl
					<< "Accepted values range from 1 - 30 Hz." << endl;
				return false;
			}
			setupFrameTimer(pdev, mvCapHandler.mvCapParams.triggerFrequency_Hz);
		}

		//make sure frame trigger signals are disabled by default.
		disableFrameTriggerOutput(pdev, 0);
		disableFrameTriggerOutput(pdev, 1);
	}
	catch (const ImpactAcquireException& e) {
#if 0
		if (capParams.writeLogEnabled == true) {
			logfile << "ERROR: An error occurred(error code: "
				<< e.getErrorCodeAsString() << ")." << endl;
		}
#endif
		cout << "ERROR: An error occurred(error code: "
			<< e.getErrorCodeAsString() << ")." << endl;
	}

	//b) launch image acquisition and storage threads
	for (std::atomic<bool>& flag : threadRunFlags) {
		flag = true;
	}
	cout << "INFO: Launching image acquisition threads..." << endl;

	threads[0] = std::thread(imageAcquisitionThread, pdev, 0,
		"master", mvCapHandler.mvCapParams.nImagePairs,
		mvCapHandler.mvCapParams.decimFactor, 
		mvCapHandler.mvCapParams.writeLogEnabled,
		mvCapHandler.mvCapParams.logInterval, 
		mvCapHandler.mvCapParams.displayStatsEnabled, 
		mvCapHandler.mvCapParams.displayInterval,
		std::ref(threadRunFlags[0]), &mvCapHandler);

	{
		if (mvCapHandler.mvCapParams.samplingMode == 1)
		{
			//Enable/Set trigger input according to selected trigger operation (internal/external)
			//Frame trigger input is always set. Sync trigger input is only used when external pulse (on pin 4)
			setupFrameTriggerInput(pdev, mvCapHandler.mvCapParams.masterTrigIn);
			if (mvCapHandler.mvCapParams.extSyncEnabled == true) {
				setupSyncTriggerInput(pdev, mvCapHandler.mvCapParams.extTrigIn);
			}
			//enable trigger pulses when both cameras are ready.
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			if (mvCapHandler.mvCapParams.extSyncEnabled == false) {
				//enable master trigger, timer2 controlled
				enableFrameTriggerOutput(pdev, mvCapHandler.mvCapParams.masterTrigOut, "Timer2Active");
			}
			else {
				//enable master trigger, controlled by PPS (through mvLogicGates)
				//enableFrameTriggerOutput( devices[0],masterTrigOut);
				enableFrameTriggerOutput(pdev, mvCapHandler.mvCapParams.extTrigOut, "mvLogicGateOR1Output");
			}
		}
		else
		{
			setupStandaloneFrameTrigger(pdev);
		}
	}
#if ENABLE_LOG_DUMP
	// now all capture threads should be running...
	if (mvCapHandler.mvCapParams.writeLogEnabled == true) {
		logfile << "INFO: Image acquisition and storage threads launched successfully!" << endl;
	}
#endif
	cout << "INFO: Image acquisition and storage threads launched successfully!" << endl;

	//wait for user interrupt
	if (mvCapHandler.mvCapParams.nImagePairs == 0) {
#if ENABLE_LOG_DUMP
		if (mvCapHandler.mvCapParams.writeLogEnabled == true) {
			logfile << "INFO: Continuous image acquisition started..." << endl;
		}
#endif
		cout << "INFO: Continuous image acquisition started..." << endl;
	}
	else {
#if ENABLE_LOG_DUMP
		if (mvCapHandler.mvCapParams.writeLogEnabled == true) {
			logfile << "INFO: Acquiring " << mvCapHandler.mvCapParams.nImagePairs << " image pairs..." << endl;
		}
#endif
		cout << "INFO: Acquiring " << mvCapHandler.mvCapParams.nImagePairs << " image pairs..." << endl;
	}
	return 0;
}

bool endMonoImageAcquisition(mvMonoCaptureParameters &mvCapHandler, DeviceData* pdev)
{
	//stop all threads by deactivating the threadRunFlags
	for (std::atomic<bool>& flag : threadRunFlags)
	{
		flag = false;
	}
	cout << "INFO: Terminating all acquisition threads..." << endl;
	
	for (std::thread& thread : threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	//13. Thread termination
	//disable frame trigger output (only relevant if masterTrig is used)
	if (mvCapHandler.mvCapParams.extSyncEnabled == false) {
		disableFrameTriggerOutput(pdev, mvCapHandler.mvCapParams.masterTrigOut);
	}
	else {
		disableFrameTriggerOutput(pdev, mvCapHandler.mvCapParams.extTrigOut);
	}

#if ENABLE_LOG_DUMP
	logfile << "INFO: acquisition Threads terminated successfully!" << endl;
#endif
	logfile.close();
	cout << "INFO: acquisition Threads terminated successfully!" << endl;
	return true;
}