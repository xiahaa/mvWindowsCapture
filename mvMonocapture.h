#pragma once
#include <mvIMPACT_CPP/mvIMPACT_acquire.h>
#include <mvIMPACT_CPP/mvIMPACT_acquire_GenICam.h>
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
#include "config.h"
#include "safe_queue.h"

//-----------------------------------------------------------------------------
struct mvMonoCaptureParameters
	//-----------------------------------------------------------------------------
{
	const mvCaptureParams_t		&mvCapParams;
	std::string					&baseDir;
	//Init the cv::mat queue used for image acquisition
	//This queue is a multiple-producer, multiple user queue
	//Each frame has an id (0= master/, 1=slave) according to the above struct
	SafeQueue<raw_frame_t>	&rawImageQueue;
	unsigned int			threadNum;
	const unsigned int		&syncSaveCnt;

	mvMonoCaptureParameters(mvCaptureParams_t &mvparams, std::string &dir, SafeQueue<raw_frame_t> &Queue, unsigned int trdNum, unsigned int &cnt) :
		mvCapParams(mvparams), baseDir(dir), rawImageQueue(Queue), threadNum(trdNum), syncSaveCnt(cnt)
	{
	}
	~mvMonoCaptureParameters()
	{
	}
};

bool startMonoImageAcquisitions(mvMonoCaptureParameters &mvCapHandler, DeviceData* pdev);
bool endMonoImageAcquisition(mvMonoCaptureParameters &mvCapHandler, DeviceData* pdev);




