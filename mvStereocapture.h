#pragma once

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
struct mvStereoCaptureParameters
	//-----------------------------------------------------------------------------
{
	const mvCaptureParams_t		&mvCapParams;
	std::string					&baseDir;
	//Init the cv::mat queue used for image acquisition
	//This queue is a multiple-producer, multiple user queue
	//Each frame has an id (0= master/, 1=slave) according to the above struct
	SafeQueue<raw_frame_t>	&rawImageQueue;
	unsigned int			threadNum;

	mvStereoCaptureParameters(mvCaptureParams_t &mvparams, std::string &dir, SafeQueue<raw_frame_t> &Queue, unsigned int trdNum) :
		mvCapParams(mvparams), baseDir(dir), rawImageQueue(Queue), threadNum(trdNum)
	{
	}
	~mvStereoCaptureParameters()
	{
	}
};

bool startStereoImageAcquisitions(mvStereoCaptureParameters &mvCapHandler, DeviceData* pMasterdev, DeviceData* pSlavedev);
bool endStereoImageAcquisition(mvStereoCaptureParameters &mvCapHandler, DeviceData* pMasterdev, DeviceData* pSlavedev);




