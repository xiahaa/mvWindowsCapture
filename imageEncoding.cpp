#include "stdafx.h"
#include <iostream>
#include <string>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <chrono>
#include <io.h>
#include <direct.h>
#include "datatype.h"
#include <thread>
#include <atomic>
#include <fstream>
#include "config.h"
#include "imageEncoding.h"
/* opencv */
#   include <opencv2/core/core.hpp>
#   include <opencv2/highgui/highgui.hpp>
#   include <opencv2/imgproc/imgproc.hpp>

using namespace std;

#pragma warning(disable:4996)

static OFStreamThread logstream;
static std::thread threads[NUM_ENC_THREADS];
static std::atomic<bool> threadRunFlags[NUM_ENC_THREADS];
static ofstream logfile;

unsigned int imageEncodingThread(unsigned int tid, unsigned int compression, atomic<bool> &threadRun, void* pData)
{
	ThreadEncodingParam* pThreadParameter = reinterpret_cast<ThreadEncodingParam*>(pData);
	SafeQueue<frame_t>& queue = pThreadParameter->imageQueue;
	SafeQueue<raw_frame_t>& rawQueue = pThreadParameter->rawImageQueue;

	vector<int> compression_params;
	compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
	compression_params.push_back(compression);
	compression_params.push_back(CV_IMWRITE_PNG_STRATEGY);
	//compression_params.push_back(IMWRITE_PNG_STRATEGY_DEFAULT);
	compression_params.push_back(CV_IMWRITE_PNG_STRATEGY_HUFFMAN_ONLY);

	frame_t currFrame;
	raw_frame_t rawFrame;

	unsigned int encodingCnt = 0;

	while (threadRun || !rawQueue.empty())
	{
		//get latest (front) element of Image Queue.
		rawFrame = rawQueue.dequeue();
		//dummy frame received, end thread!
		if (rawFrame.cam > 2) 
		{
#if ENABLE_LOG
			LOG_INFO("%s%d%s\n", "INFO: encoding thread #", tid, ": Received dummy frame.");
#endif
#if ENABLE_LOG_DUMP
			logstream << "INFO: Storage thread #" << tid << ": Received dummy frame.\n";
#endif
			break;
		}

		try {
			if (!threadRun)
			{
				LOG_INFO("INFO: encoding thread #%d %d", tid, rawFrame.id);
			}

			currFrame.cam = rawFrame.cam;
			currFrame.id = rawFrame.id;
			currFrame.decim = rawFrame.decim;
			currFrame.data.clear();

			std::chrono::time_point<std::chrono::system_clock> t1 = std::chrono::system_clock::now();

			if (pThreadParameter->mvCapParams.bmpEnabled)
				cv::imencode(".bmp", rawFrame.data, currFrame.data);
			else
				cv::imencode(".png", rawFrame.data, currFrame.data);
		
			std::chrono::time_point<std::chrono::system_clock> t2 = std::chrono::system_clock::now();
			std::chrono::duration<double> lapse = t2 - t1;
			std::cout << "png compression time: " << lapse.count() << endl;

			queue.enqueue(currFrame);
			encodingCnt++;
		}
		catch (runtime_error& ex) {
#if ENABLE_LOG
			LOG_INFO("ERROR: An error occurred when encoding image file \"%s", ex.what());
#endif
		}
	}

#if ENABLE_LOG
	LOG_INFO("INFO: Image Encoding complete, tid:%d\n", tid);
	LOG_INFO("INFO: Image encoding Thread statistics - fileErrCnt: %d tid: %d", encodingCnt, tid);
#endif
	return 0;
}


//-----------------------------------------------------------------------------
bool runImageEncoding(ThreadEncodingParam& encodingParams)
//-----------------------------------------------------------------------------
{
	//
	string baseDir = encodingParams.baseDir;

#if ENABLE_LOG_DUMP
	string storageLog = baseDir + "encoding.log";
	logfile.open(storageLog, std::ios::out);
	if (logfile.is_open()) 
	{
		std::cout << "INFO: Logfile ready." << std::endl;
		logfile << "LOG START" << std::endl;
		//Init logstream used by individual threads
		logstream.init(&logfile);
	}
	else
	{
		std::cout << "ERROR: Unable to open logfile. Logging disabled." << endl;
		return false;
	}
#endif

	//b) launch image acquisition and storage threads
	for (unsigned int i = 0; i < encodingParams.encodingThreadNum; i++)
	{
		threadRunFlags[i] = true;
	}
	std::cout << "INFO: Launching image encoding threads..." << std::endl;

	for (unsigned int i = 0; i < encodingParams.encodingThreadNum; i++)
	{
		threads[i] = std::thread(imageEncodingThread, i, PNG_COMPRESSION_RATIO, std::ref(threadRunFlags[i]), &encodingParams);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	return true;
}

bool endImageEncoding(ThreadEncodingParam& encodingParams)
{
	//stop all threads by deactivating the threadRunFlags
	for (std::atomic<bool>& flag : threadRunFlags)
	{
		flag = false;
	}
	cout << "INFO: Terminating all encoding threads..." << endl;
	//enqueue imageThreads "dummy frames" to ensure no threads hang at exit.
	raw_frame_t dummy;
	for (unsigned int i = 0; i < encodingParams.encodingThreadNum; i++)
	{
		dummy.id = 3;
		dummy.cam = 255;
		dummy.decim = 1;
		encodingParams.rawImageQueue.enqueue(dummy);
	}

	for (std::thread& thread : threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}
#if ENABLE_LOG_DUMP
	logfile << "INFO: Encoding Threads terminated successfully!" << endl;
#endif
	logfile.close();
	cout << "INFO: Encoding Threads terminated successfully!" << endl;
	return true;
}