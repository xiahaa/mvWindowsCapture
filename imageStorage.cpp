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
#include "imageStorage.h"
#include "datatype.h"
#include <thread>
#include <atomic>
#include <fstream>
#include "config.h"
/* opencv */
#   include <opencv2/core/core.hpp>
#   include <opencv2/highgui/highgui.hpp>
#   include <opencv2/imgproc/imgproc.hpp>

using namespace std;

#pragma warning(disable:4996)

static OFStreamThread logstream;
static std::thread threads[NUM_STOR_THREADS];
static std::atomic<bool> threadRunFlags[NUM_STOR_THREADS];
static ofstream logfile;

//-----------------------------------------------------------------------------
//
unsigned int imageStorageThread(unsigned int tid, string outDir, atomic<bool> &threadRun, void* pData)
{
	ThreadStorageParam* pThreadParameter = reinterpret_cast<ThreadStorageParam*>(pData);
	SafeQueue<frame_t>& queue = pThreadParameter->imageQueue;

	unsigned int fileErrCnt = 0;
	string devname[2] = { "m", "s" };
	char fileNameC[128] = { 0 };

	frame_t currFrame;

	//run if thread is instructed to run OR if queue contains data (to avoid hanging if queue is empty)
	while (threadRun || (!threadRun && !queue.empty()))
	{
		//get latest (front) element of Image Queue.
		currFrame = queue.dequeue();
		//dummy frame received, end thread!
		if (currFrame.cam > 2) {
#if ENABLE_LOG
			LOG_INFO("%s%d%s\n", "INFO: Storage thread #", tid, ": Received dummy frame.");
#endif
#if ENABLE_LOG_DUMP
			logstream << "INFO: Storage thread #" << tid << ": Received dummy frame.\n";
#endif
			break;
		}

		//generate filename with padded counter, use either default folder "./" or folder selected by outDir.
		//add decimationFactor prefix if necessary
		//At this point, it is assumed that outDir has been validated.
		int save_bmp = 0;

		if (pThreadParameter->mvCapParams.bmpEnabled)
		{
			//store current image as BMP
			if (currFrame.decim == 2 || currFrame.decim == 4 || currFrame.decim == 8)
				sprintf(fileNameC, "%s%s_%06d_%d.bmp", outDir.c_str(), devname[currFrame.cam].c_str(),
					currFrame.id, currFrame.decim);
			else
				sprintf(fileNameC, "%s%s_%06d.bmp", outDir.c_str(), devname[currFrame.cam].c_str(),
					currFrame.id);
			save_bmp = 1;
		}
		else
		{
			//store current image as PNG using the compression parameters defined above.
			//append filetype
			if (currFrame.decim == 2 || currFrame.decim == 4 || currFrame.decim == 8)
				sprintf(fileNameC, "%s%s_%06d_%d.png", outDir.c_str(), devname[currFrame.cam].c_str(),
					currFrame.id, currFrame.decim);
			else
				sprintf(fileNameC, "%s%s_%06d.png", outDir.c_str(), devname[currFrame.cam].c_str(),
					currFrame.id);
		}

		try {
			if (!threadRun) 
			{
#if ENABLE_LOG
				LOG_INFO("INFO: Storage thread #%d: Storing %s file left in queue \" %s", tid, save_bmp==0?"PNG":"BMP",
					fileNameC);
#endif
#if ENABLE_LOG_DUMP
				logstream << "INFO: Storage thread #" << tid << ": Storing "<< (save_bmp == 0 ? "PNG" : "BMP") <<" file \""
					<< fileNameC << "\"" << "\n";
#endif
			}

			std::ofstream file(fileNameC, std::ios::out | std::ios::binary);
			if (file)
			{
				file.write((char*)&currFrame.data[0], currFrame.data.size() * sizeof(uchar));
				file.close();
			}

#if USE_IMWRITE
			if (pThreadParameter->mvCapParams.bmpEnabled)
			//imwrite(fileNameC, currFrame.data);
			else
			//imwrite(fileNameC, currFrame.data, compression_params);
#endif
		}
		catch (runtime_error& ex) 
		{
#if ENABLE_LOG
			LOG_INFO("Storage ERROR: An error occurred when storing %s file \"%s %s", 
				save_bmp == 0 ? "PNG" : "BMP", fileNameC, ex.what());
#endif
#if ENABLE_LOG_DUMP
			{
				logstream << "Storage ERROR: An error occurred when storing " << (save_bmp == 0 ? "PNG" : "BMP") << " file \""
					<< fileNameC << "\"" << ex.what() << "\n";

			}
#endif
			fileErrCnt++;
		}
	}

#if ENABLE_LOG
	LOG_INFO("INFO: Image Storage complete, tid:%d\n", tid);
	LOG_INFO("INFO: Image Storage Thread statistics - fileErrCnt: %d tid: %d", fileErrCnt, tid);
	LOG_INFO("ERROR: Image Storage Thread statistics - fileErrCnt: %d tid: %d", fileErrCnt, tid);
#endif

#if ENABLE_LOG_DUMP
	logstream << "INFO: Image Storage complete, tid:" << tid << "\n";
	logstream << (fileErrCnt == 0 ? "INFO: Image Storage Thread statistics - fileErrCnt:" \
		: "Storage ERROR: Image Storage Thread statistics - fileErrCnt:") \
		<< fileErrCnt << " tid:" << tid << "\n";
#endif
	return 0;
}

#if 0
unsigned int __stdcall storageEntry(void* pData)
{
	ThreadStorageParam* pThreadParameter = reinterpret_cast<ThreadStorageParam*>(pData);
	unsigned int cnt = 0;
	
	imageHeader_t header;
	unsigned char *buf = NULL;

	char cfilename[128] = { 0 };
	char cfilenamePNG[128] = { 0 };

#define IMG_BUFFER	(1024*1024*32)
	buf = (buf == NULL) ? (unsigned char*)_aligned_malloc(IMG_BUFFER, 4) : buf;

	while (!existLoop)
	{
		if (!pThreadParameter->headerInit)
		{
			pThreadParameter->pheaderMutex->try_lock();
			if (pThreadParameter->pheader->width != 0)
			{
				pThreadParameter->headerInit = true;
				header = *(pThreadParameter->pheader);
			}
			pThreadParameter->pheaderMutex->unlock();
			Sleep(100);
			continue;
		}

		if (!is_FIFO_empty(pThreadParameter->pCbuf))
		{
			// copy
			FIFO_get(pThreadParameter->pCbuf, (unsigned char*)buf, header.height*header.width*header.bytesPerPixel);
			sprintf_s(cfilename, "%s/%08d.bmp", storeDir.c_str(), cnt++);
			string filename(cfilename);
#if 1
			cv::Mat openCVImage(cv::Size(header.width, header.height), CV_8UC1, buf, header.lineWidth);
			std::chrono::time_point<std::chrono::system_clock> t1 = std::chrono::system_clock::now();
			cv::imwrite(filename, openCVImage);
			std::chrono::time_point<std::chrono::system_clock> t2 = std::chrono::system_clock::now();
			printMemUsed(pThreadParameter->pCbuf);
#if 0
			// used for comparing storage speed for BMP and PNG
			sprintf_s(cfilenamePNG, "%s/%08d.png", storeDir.c_str(), cnt++);
			string filenamePNG(cfilenamePNG);
			vector<int> compression_params;
			compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
			compression_params.push_back(3);
			std::chrono::time_point<std::chrono::system_clock> t3 = std::chrono::system_clock::now();
			cv::imwrite(filenamePNG, openCVImage, compression_params);
			std::chrono::time_point<std::chrono::system_clock> t4 = std::chrono::system_clock::now();
			std::chrono::duration<double> e1 = t2 - t1;
			std::chrono::duration<double> e2 = t4 - t3;
			std::cout << "for BMP: " << e1.count() << " for PNG: " << e2.count() << std::endl;
#endif	
#endif
		}
		Sleep(80);
	}

	if (buf != NULL)
		_aligned_free(buf);
	buf = NULL;

	return 0;
}

HANDLE hThreadStorage = NULL;
hThreadStorage = (HANDLE)_beginthreadex(0, 0, storageEntry, (LPVOID)(&storageParams), 0, &dwThreadID);
// start the execution of the 'live' thread.
existLoop = false;
unsigned int dwThreadID = 1;
existLoop = true;
WaitForSingleObject(hThreadStorage, INFINITE);
CloseHandle(hThreadStorage);
#endif

//-----------------------------------------------------------------------------
bool runImageStorage(ThreadStorageParam& storageParams)
//-----------------------------------------------------------------------------
{
	//
	string storeDir = storageParams.baseDir;

#if ENABLE_LOG_DUMP
	string storageLog = storeDir + "storage.log";
	logfile.open(storageLog, std::ios::out);
	if (logfile.is_open()) {
		cout << "INFO: storage Logfile ready." << endl;
		logfile << "storage LOG START" << endl;
		//Init logstream used by individual threads
		logstream.init(&logfile);
	}
	else 
	{
		cout << "ERROR: Unable to open storage logfile. Logging disabled." << endl;
		return false;
	}
#endif

	//b) launch image acquisition and storage threads
	for (unsigned int i = 0; i < storageParams.storageThreadNum; i++)
	{
		threadRunFlags[i] = true;
	}
	cout << "INFO: Launching image storage threads..." << endl;

	for (unsigned int i = 0; i < storageParams.storageThreadNum; i++)
	{
		threads[i] = std::thread(imageStorageThread, i, storeDir, std::ref(threadRunFlags[i]), &storageParams);
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	return true;
}

bool endImageStorage(ThreadStorageParam& storageParams)
{
	//stop all threads by deactivating the threadRunFlags
	for (std::atomic<bool>& flag : threadRunFlags) 
	{
		flag = false;
	}
	cout << "INFO: Terminating all storage threads..." << endl;
	//enqueue imageThreads "dummy frames" to ensure no threads hang at exit.
	frame_t dummy;
	for (unsigned int i = 0; i < storageParams.storageThreadNum; i++)
	{
		dummy.id = 3;
		dummy.cam = 255;
		dummy.decim = 1;
		storageParams.imageQueue.enqueue(dummy);
	}

	for (std::thread& thread : threads)
	{
		if (thread.joinable()) 
		{
			thread.join();
		}
	}
#if ENABLE_LOG_DUMP
	logfile << "INFO: storage Threads terminated successfully!" << endl;
#endif
	logfile.close();
	cout << "INFO: storage Threads terminated successfully!" << endl;
	return true;
}