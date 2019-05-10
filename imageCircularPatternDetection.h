#pragma once

#ifdef _WIN32
#   include <windows.h>
#	include <cstdalign>
#   include <process.h>
#endif
#include "circularBuffer.h"
#include "datatype.h"
#include "safe_queue.h"
#include <mutex>

struct ThreadCircularParam
	//-----------------------------------------------------------------------------
{
	SafeQueue<raw_frame_t>		&imageQueue;
	mvCaptureParams_t		&mvCapParams;
	std::string				&baseDir;

	ThreadCircularParam(SafeQueue<raw_frame_t> &Queue, mvCaptureParams_t &params, std::string &_baseDir) :
		imageQueue(Queue), mvCapParams(params), baseDir(_baseDir)
	{
	}
	~ThreadCircularParam()
	{
	}
};

bool runCircularPatternDetection(ThreadCircularParam& circularPRParams);
bool endCircularPatternDetection(ThreadCircularParam& circularPRParams);
