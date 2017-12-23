#pragma once

#ifdef _WIN32
#   include <windows.h>
#	include <cstdalign>
#   include <process.h>
#endif
#include "datatype.h"
#include "safe_queue.h"
#include <mutex>

struct ThreadEncodingParam
	//-----------------------------------------------------------------------------
{
	SafeQueue<raw_frame_t>	&rawImageQueue;
	SafeQueue<frame_t>		&imageQueue;
	const unsigned int		encodingThreadNum;
	const std::string			&baseDir;
	const mvCaptureParams_t		&mvCapParams;
	ThreadEncodingParam(SafeQueue<raw_frame_t> &RawQueue, SafeQueue<frame_t> &Queue, int ThreadNum, 
		std::string &dir, mvCaptureParams_t &params) :
		rawImageQueue(RawQueue), encodingThreadNum(ThreadNum), baseDir(dir), imageQueue(Queue),
		mvCapParams(params)
	{
	}
	~ThreadEncodingParam()
	{
	}
};

bool runImageEncoding(ThreadEncodingParam& encodingParams);
bool endImageEncoding(ThreadEncodingParam& encodingParams);
