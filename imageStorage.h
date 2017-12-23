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

struct ThreadStorageParam
	//-----------------------------------------------------------------------------
{
#if 0
	/* shared circular buffer */
	struct FIFO				*pCbuf;	
	imageHeader_t			*pheader;
	std::mutex				*pheaderMutex;
	bool					headerInit;
#endif

	SafeQueue<frame_t>		&imageQueue;
	const unsigned int		 storageThreadNum;
	const std::string		&baseDir;
	const mvCaptureParams_t		&mvCapParams;
#if 0
	ThreadStorageParam(struct FIFO* sharedBuf, imageHeader_t *ph, std::mutex *pmutex) : 
		pCbuf(sharedBuf), pheader(ph), pheaderMutex(pmutex), headerInit(false)
	{
	}
#endif
	ThreadStorageParam(SafeQueue<frame_t> &Queue, int ThreadNum, std::string &dir, mvCaptureParams_t &params) :
		imageQueue(Queue), storageThreadNum(ThreadNum), baseDir(dir), mvCapParams(params)
	{
	}
	~ThreadStorageParam()
	{
	}
};

bool runImageStorage(ThreadStorageParam& storageParams);
bool endImageStorage(ThreadStorageParam& storageParams);
