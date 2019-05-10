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
#include "imageCircularPatternDetection.h"
#include "refinedCircularPositioningSystem.h"
#include "circularDetector.h"
#include "datatype.h"
#include <thread>
#include <atomic>
#include <fstream>
#include "config.h"
/* opencv */
#   include <opencv2/core/core.hpp>
#   include <opencv2/highgui/highgui.hpp>
#   include <opencv2/imgproc/imgproc.hpp>
// for gui
#include <signal.h>
#include <fstream>
#include <iomanip>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "boost/program_options.hpp"
#include "boost/timer.hpp"

using namespace std;

#pragma warning(disable:4996)

static OFStreamThread logstream;
static std::thread threads[1];
static std::atomic<bool> threadRunFlags[1];
static ofstream logfile;


bool readCaliFile(cv::Mat &camMatrix, cv::Mat &camDistCoeff)
{
	if (1)
	{
		cv::FileStorage fs(CALI_FILE, cv::FileStorage::READ);
		if (!fs.isOpened())
		{
			std::printf("cali file missing......!!!!!\n");
			return false;
		}
		else
		{
			fs["Camera_Matrix"] >> camMatrix;
			fs["Distortion_Coefficients"] >> camDistCoeff;
			std::cout << "calibrated result: cameraMatrix : " << camMatrix << std::endl;
			std::cout << "distcoeff : " << camDistCoeff << std::endl;
			return true;
		}
	}
	else
	{
		return true;
	}
}

namespace po = boost::program_options;

bool stop = false;
void interrupt(int s) {
	stop = true;
}

int clicked = 0;
std::vector<cv::Point> pt(4);
void mouse_callback(int event, int x, int y, int flags, void* param) {
	if (event == CV_EVENT_LBUTTONDOWN)
	{
		cout << "clicked window: " << clicked << " x:" << x << " y:" << y << endl;
		pt[clicked] = cv::Point(x, y);
		clicked = (clicked + 1)>3 ? 0 : (clicked + 1);
	}
}

int number_of_targets;

//-----------------------------------------------------------------------------
//
unsigned int circularPatternDetectionThread(unsigned int tid, atomic<bool> &threadRun, void* pData)
{
	ThreadCircularParam* pThreadParameter = reinterpret_cast<ThreadCircularParam*>(pData);
	SafeQueue<raw_frame_t>& rawQueue = pThreadParameter->imageQueue;

	raw_frame_t rawFrame;

	/* load calibration */
	cv::Mat K, dist_coeff;
	readCaliFile(K, dist_coeff);

	bool init = false;
	/* init system */
	cv::Size frame_size;
	//cv::LocalizationSystem system;

	/* setup gui */
	if (1) {
		cvStartWindowThread();
		cv::namedWindow("frame", CV_WINDOW_NORMAL);
		cv::resizeWindow("frame", 640, 480);
		cv::setMouseCallback("frame", mouse_callback);
	}

	cv::Mat original_frame, frame;
	long frame_idx = 0;

	bool axisSet = false;
	int max_attempts = 1;
	int refine_steps = 1;

	string axisfile(pThreadParameter->mvCapParams.axisFile);
	circularPatternBasedLocSystems cplocsys(K, dist_coeff, pThreadParameter->mvCapParams.innerdiameter,
		pThreadParameter->mvCapParams.outterdiameter, pThreadParameter->mvCapParams.dimx, pThreadParameter->mvCapParams.dimy);

	cv::Mat frame_gray;

#if	IS_TRACKING_ENABLE
	int num_of_markers = 4;
	bool is_tracking = false;
#endif
	//cv::namedWindow("frame", CV_WINDOW_NORMAL);

	while (threadRun || !rawQueue.empty())
	{
		//get latest (front) element of Image Queue.
		rawFrame = rawQueue.dequeue();
		//dummy frame received, end thread!
		if (rawFrame.cam > 2)
		{
			break;
		}

		original_frame = rawFrame.data;
		original_frame.copyTo(frame);

		if (frame.channels() == 3)
			cv::cvtColor(frame, frame_gray, CV_BGR2GRAY);
		else
		{
			frame_gray = frame;
			cv::cvtColor(frame_gray, frame, cv::COLOR_GRAY2BGR);
		}

		std::chrono::time_point<std::chrono::system_clock> t1 = std::chrono::system_clock::now();
		
		if (cplocsys.detectPatterns(frame_gray, is_tracking) == num_of_markers) is_tracking = true;
		else is_tracking = false;

		cplocsys.localization();
		std::chrono::time_point<std::chrono::system_clock> t2 = std::chrono::system_clock::now();
		std::chrono::duration<double> lapse = t2 - t1;
		cplocsys.drawPatterns(frame);
		std::cout << "png compression time: " << lapse.count() << endl;
		
		if (pThreadParameter->mvCapParams.doAxisSet == true)
		{
			if (pThreadParameter->mvCapParams.axisSet == false)
			{
				cv::imshow("frame", frame);
				char key = cv::waitKey(0);
				if (key == 'a')
				{
					cplocsys.setAxisFrame(pt, axisfile);
					cplocsys.draw_axis(frame);
					cv::imshow("frame", frame);
					char key = cv::waitKey(0);
					if (key == 'o')
					{
						pThreadParameter->mvCapParams.axisSet = true;
					}
				}
			}
			else
			{
				cplocsys.transformPosition();
				logfile << frame_idx << ":";
				cplocsys.tostream(logfile);
				frame_idx++;
				/*	std::string base = "C:/Users/xiahaa/Documents/test1/1";
				static int id = 0;
				char file[128] = { 0 };
				sprintf(file, "%s/%08d.bmp", base.c_str(), id++);
				cv::imwrite(file, frame);*/
				cv::imshow("frame", frame);
				char key = cv::waitKey(10);
			}
		}
		else
		{
			pThreadParameter->mvCapParams.axisSet = true;
			logfile << frame_idx << ":";
			cplocsys.tostream(logfile);
			frame_idx++;
			cv::imshow("frame", frame);
			char key = cv::waitKey(10);
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
bool runCircularPatternDetection(ThreadCircularParam& circularPRParams)
//-----------------------------------------------------------------------------
{
	std::string baseDir = circularPRParams.baseDir;

#if ENABLE_LOG_DUMP
	std::string storageLog = baseDir + "position.log";
	logfile.open(storageLog, std::ios::out);
	if (logfile.is_open())
	{
		std::cout << "INFO: positioning Logfile ready." << std::endl;
		logfile << "positioning LOG START" << std::endl;
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
	threadRunFlags[0] = true;
	std::cout << "INFO: Launching image storage threads..." << std::endl;
	threads[0] = std::thread(circularPatternDetectionThread, 0, std::ref(threadRunFlags[0]), &circularPRParams);
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	return true;
}

bool endCircularPatternDetection(ThreadCircularParam& circularPRParams)
{
	//stop all threads by deactivating the threadRunFlags
	for (std::atomic<bool>& flag : threadRunFlags)
	{
		flag = false;
	}
	std::cout << "INFO: Terminating all storage threads..." << std::endl;
	//enqueue imageThreads "dummy frames" to ensure no threads hang at exit.
	raw_frame_t dummy;
	dummy.id = 3;
	dummy.cam = 255;
	dummy.decim = 1;
	circularPRParams.imageQueue.enqueue(dummy);

	for (std::thread& thread : threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

#if ENABLE_LOG_DUMP
	logfile << "INFO: positioning Threads terminated successfully!" << endl;
	logfile.close();
	cout << "INFO: positioning Threads terminated successfully!" << endl;
#endif

	std::cout << "INFO: positioning Threads terminated successfully!" << std::endl;
	return true;
}