#pragma once
#include <stdint.h>
#include <mutex>
#include <vector>
#include "opencv2/core.hpp"
#include <mvIMPACT_CPP/mvIMPACT_acquire.h>
#include <mvIMPACT_CPP/mvIMPACT_acquire_GenICam.h>
#include <iostream>
#include "config.h"

using namespace mvIMPACT::acquire;
using namespace mvIMPACT::acquire::GenICam;

enum __CAMERA_RUNNING_MODE {
	MONOCULAR = 1,
	STEREO = 2,
	ARUCO = 3,
	CIRCULAR_PATTERN = 4,
	NUM_MODE,
};

//-----------------------------------------------------------------------------
//DeviceData class associated to each image acquisition thread.
class DeviceData {
public:
	Device* pDev_;
	FunctionInterface* pFI_;
	DigitalIOControl* pDIOC_;
	Statistics* pSS_;
	int lastRequestNr_;

	explicit DeviceData(Device* p) :
		pDev_(p), pFI_(0), pDIOC_(0), pSS_(0), lastRequestNr_(INVALID_ID) {
	}
	~DeviceData() {
		delete pFI_;
		delete pSS_;
		delete pDIOC_;
		if (pFI_->isRequestNrValid(lastRequestNr_)) {
			pFI_->imageRequestUnlock(lastRequestNr_);
			lastRequestNr_ = INVALID_ID;
		}
	}
	void init(void) {
		pDev_->open();
		CameraSettingsBase cs(pDev_);
		// A buffers timeout shall never elapse
		cs.imageRequestTimeout_ms.write(0);
		pFI_ = new FunctionInterface(pDev_);
		pDIOC_ = new DigitalIOControl(pDev_);
		pSS_ = new Statistics(pDev_);
	}
	Device* device(void) const {
		return pDev_;
	}
	FunctionInterface* functionInterface(void) const {
		return pFI_;
	}
	DigitalIOControl* DIOC(void) const {
		return pDIOC_;
	}
	Statistics* statistics(void) const {
		return pSS_;
	}
};


#pragma pack(1)
typedef struct __imageHeader_t
{
	uint32_t width;
	uint32_t height;
	uint32_t lineWidth;
	uint32_t bytesPerPixel;
	__imageHeader_t() :width(0), height(0), lineWidth(0), bytesPerPixel(0) {}
	~__imageHeader_t() {}

	__imageHeader_t& operator=(const __imageHeader_t &arg)
	{
		{
			this->width = arg.width;
			this->height = arg.height;
			this->lineWidth = arg.lineWidth;
			this->bytesPerPixel = arg.bytesPerPixel;
		}
		return *this;
	}
}imageHeader_t;

#pragma pack()

//struct used fo queue content. Each frame has a cam number (0= left, 1=right)
// and an ID relative to the current frame count.
struct frame_t {
	unsigned int isSave;
	unsigned int cam;
	unsigned int id;
	unsigned int decim;
	//cv::Mat data;
	std::vector<uchar> data;
};

typedef struct __raw_frame_t
{
	unsigned int isSave;
	unsigned int cam;
	unsigned int id;
	unsigned int decim;
	cv::Mat data;
}raw_frame_t;

////////////
//Multiple threads with fstream access
//Source: http://stackoverflow.com/questions/15973428/what-are-the-options-for-safe-use-of-fstream-with-many-threads
class OFStreamThread {
	std::ofstream *f;
	std::mutex mtx;
public:
	OFStreamThread(void) :
		f(0) {
	}

	void init(std::ofstream *fp) {
		f = fp;
	}
	template<typename T>
	OFStreamThread &operator<<(const T &x) {
		std::lock_guard<std::mutex> lock(mtx);
		(*f) << x;
		return *this;
		//mutex is locked; will be unlocked when lock guard goes out of scope
	}
};

////////////
//multiple threads with access to cout
//Source: http://stackoverflow.com/questions/18277304/using-stdcout-in-multiple-threads proposed by user "Conchylicultor"
class CoutThread : public std::ostringstream {
public:
	CoutThread() = default;

	~CoutThread() {
		std::lock_guard<std::mutex> lock(mutexPrint);
		std::cout << this->str();
		//mutex is locked; will be unlocked when lock guard goes out of scope
	}

private:
	static std::mutex mutexPrint;
};

typedef struct __mvCaptureParams_t {
public:
	unsigned int captureMode;
	unsigned int isCaptureContinuous;
	unsigned int masterId;
	unsigned int slaveId;
	unsigned int masterTrigOut;
	unsigned int masterTrigIn;
	unsigned int slaveTrigIn;
	unsigned int triggerFrequency_Hz;
	unsigned int nImagePairs;
	unsigned int expTime_us;
	unsigned int expTimeMax_us;
	unsigned int logInterval;
	unsigned int displayInterval;
	unsigned int pngCompression;
	unsigned int decimFactor;
	unsigned int imageThreads;
	unsigned int binningFactor;
	unsigned int samplingMode;
	unsigned int autoSave;

	bool bmpEnabled;
	bool displayStatsEnabled;
	bool writeLogEnabled;
	bool extSyncEnabled;
	bool autoExposureEnabled;
	bool listDevices;

	unsigned int extTrigIn;
	unsigned int extTrigOut;

	std::string gnssPortname;
	std::string outdir;

	float innerdiameter;
	float outterdiameter;
	std::string axisFile;
	float dimx;
	float dimy;
	bool axisSet;
	bool doAxisSet;

	__mvCaptureParams_t(void):
		captureMode(STEREO),
		masterId(0),
		slaveId(1),
		masterTrigOut(1),
		masterTrigIn(5),
		slaveTrigIn(5),
		triggerFrequency_Hz(10),
		nImagePairs(0),
		expTime_us(0),
		expTimeMax_us(100000),
		logInterval(10),
		displayInterval(10),
		pngCompression(1),
		decimFactor(1),
		imageThreads(6),
		binningFactor(1),
		samplingMode(2),
		extTrigIn(4),
		extTrigOut(1), 
		autoSave(0),
		isCaptureContinuous(1),
		bmpEnabled(false), 
		displayStatsEnabled(true),
		writeLogEnabled(true), extSyncEnabled(false), autoExposureEnabled(true),
		outdir(BASE_DIR), gnssPortname(),
		innerdiameter(0.047), outterdiameter(0.114), axisFile(),
		axisSet(false), dimx(0.100 + 0.114), dimy(0.211 + 0.114), doAxisSet(false)
		{}

	void print(void) {
		std::string str;
		str = str + "********************************************************" + "\n" + \
			"captureMode: " + (captureMode == MONOCULAR ? "MONO" : "STEREO") + "\n" \
			"isCaptureContinuous: " + (isCaptureContinuous == 1 ? "Yes" : "No") + "\n" \
			"masterId: " + std::to_string(masterId) + "\n" + \
			"slaveId: " + std::to_string(slaveId) + "\n" + \
			"masterTrigOut: " + std::to_string(masterTrigOut) + "\n" + \
			"masterTrigIn: " + std::to_string(masterTrigIn) + "\n" + \
			"slaveTrigIn: " + std::to_string(slaveTrigIn) + "\n" + \
			"triggerFrequency_Hz: " + std::to_string(triggerFrequency_Hz) + "\n" + \
			"nImagePairs: " + std::to_string(nImagePairs) + "\n" + \
			"expTime_us: " + std::to_string(expTime_us) + "\n" + \
			"expTimeMax_us: " + std::to_string(expTimeMax_us) + "\n" + \
			"logInterval: " + std::to_string(logInterval) + "\n" + \
			"displayInterval: " + std::to_string(displayInterval) + "\n" + \
			"pngCompression: " + std::to_string(pngCompression) + "\n" + \
			"decimFactor: " + std::to_string(decimFactor) + "\n" + \
			"imageThreads: " + std::to_string(imageThreads) + "\n" + \
			"binningFactor: " + std::to_string(binningFactor) + "\n" + \
			"samplingMode: " + std::to_string(samplingMode) + "\n" + \
			"autoSave: " + (autoSave ? "enbale" : "disable") + "\n" + \
			"bmpEnabled: " + (bmpEnabled ? "enbale" : "disable") + "\n" + \
			"displayStatsEnabled: " + (displayStatsEnabled ? "enbale" : "disable") + "\n" + \
			"writeLogEnabled: " + (writeLogEnabled ? "enbale" : "disable") + "\n" + \
			"extSyncEnabled: " + (extSyncEnabled ? "enbale" : "disable") + "\n" + \
			"autoExposureEnabled: " + (autoExposureEnabled ? "enbale" : "disable") + "\n" + \
			"listDevices: " + (listDevices ? "enbale" : "disable") + "\n" + \
			"gnssPortname: " + gnssPortname + "\n" + \
			"outdir: " + outdir + "\n" + \
			"inner diameter: " + std::to_string(innerdiameter) + "\n" + \
			"outter diameter: " + std::to_string(outterdiameter) + "\n" + \
			"axisFile: " + axisFile + "\n" + \
			"axisSet: " + (axisSet?"Yes":"No") + "\n" + \
			"dimx: " + std::to_string(dimx) + "\n" + \
			"dimy: " + std::to_string(dimy) + "\n" + \
			"doAxisSet: "+(doAxisSet?"Yes":"No")+"\n" + \
			"********************************************************" + "\n";
		;
		printf("%s\n", str.c_str());
	}
}mvCaptureParams_t;