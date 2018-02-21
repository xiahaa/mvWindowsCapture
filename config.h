#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdio.h>

#define BASE_DIR	"C:/Users/xiahaa/workspace/data/offline"

#define ENABLE_LOG				1

#define ENABLE_LOG_DUMP			1

#define USE_IMWRITE				0

#define PNG_COMPRESSION_RATIO	1

/* always enable opencv support in my case */
#define BUILD_WITH_OPENCV_SUPPORT
//#define USE_MATRIXVISION_DISPLAY_API	

#define GNSS_SUPPORT			0

#define NUM_STOR_THREADS		20

#define NUM_ENC_THREADS			4

#define BUFFER_SIZE				1024*1024*256		// 256Mb

#define LOG_INFO(fmt, ...)		printf(fmt"\n", __VA_ARGS__)

#define DISPLAY					1

#define CONFIG_FILE_DIR			"./mvCapture.json"

#define ENABLE_MANUAL_FOCUS			0

#define ENABLE_ARUCO_DETECTION	1

#endif