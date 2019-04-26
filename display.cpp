// display.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <iostream>
#include <mutex>
#include <windows.h>
#include <chrono>
#include <io.h>
#include <direct.h>
#include "mvCamHelper.h"
#include "options.h"
#include "config.h"
#include "safe_queue.h"
#include "mvMonocapture.h"
#include "mvStereocapture.h"
#include "imageEncoding.h"
#include "imageStorage.h"
#include "datatype.h"
#include "deviceConfigure.h"
#include "imageCircularPatternDetection.h"
#include "mvCaptureConfigParse.h"

#if 0
#include "imageCaptureLive.h"
#include "circularBuffer.h"
#endif

#ifdef _WIN32
#   include <windows.h>
#	include <cstdalign>
#   include <process.h>
#elif defined(linux) || defined(__linux) || defined(__linux__)
#   if defined(__x86_64__) || defined(__aarch64__) || defined(__powerpc64__) // -m64 makes GCC define __powerpc64__
typedef uint64_t UINT_PTR;
#   elif defined(__i386__) || defined(__arm__) || defined(__powerpc__) // and -m32 __powerpc__
typedef uint32_t UINT_PTR;
#   endif
#endif // #ifdef _WIN32

#if _DEBUG
#pragma comment(lib, "opencv_core310d.lib")
#pragma comment(lib, "opencv_highgui310d.lib")
#pragma comment(lib, "opencv_imgproc310d.lib")
#pragma comment(lib, "opencv_imgcodecs310d.lib")
#pragma comment(lib, "opencv_calib3d310d.lib")

#else
#pragma comment(lib, "opencv_core310.lib")
#pragma comment(lib, "opencv_highgui310.lib")
#pragma comment(lib, "opencv_imgproc310.lib")
#pragma comment(lib, "opencv_imgcodecs310.lib")
#pragma comment(lib, "opencv_calib3d310.lib")
#endif

using namespace std;

//-----------------------------------------------------------------------------
//Source: http://stackoverflow.com/questions/18100097/portable-way-to-check-if-directory-exists-windows-linux-c
bool dirExists(string path) {
	struct stat info;

	if (stat(path.c_str(), &info) != 0)
		return false;
	else if (info.st_mode & S_IFDIR)
		return true;
	else
		return false;
}

bool getFolderID(const string &baseDir, string &storeDir)
{
	if (dirExists(baseDir) == false)
	{
		char dir[128];
		sprintf_s(dir, "%s", baseDir.c_str());
		_mkdir(dir);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		if (dirExists(baseDir) == false)
			return false;
	}

	string file = baseDir + "/id.txt";
	int folderId = 0;
	if (_access(file.c_str(), 0) == 0)
	{
		FILE *fp = NULL;
		fopen_s(&fp, file.c_str(), "r");
		if (fp != NULL)
		{
			fscanf_s(fp, "%d", &folderId);
			fclose(fp);
			fp = NULL;
		}

		folderId++;

		fopen_s(&fp, file.c_str(), "w");
		if (fp != NULL)
		{
			rewind(fp);
			fprintf_s(fp, "%d", folderId);
			fclose(fp);
			fp = NULL;
		}
		else
		{
			return false;
		}
	}
	else
	{
		FILE *fp = NULL;
		fopen_s(&fp, file.c_str(), "w");
		if (fp != NULL)
		{
			fprintf_s(fp, "%d", folderId);
			fclose(fp);
			fp = NULL;
		}
		else
		{
			return false;
		}
	}
	char dir[128];
	sprintf_s(dir, "%s/%d", baseDir.c_str(), folderId);
	_mkdir(dir);
	storeDir = string(dir);
	return true;
}

//-----------------------------------------------------------------------------
int main(int argc, char* argv[])
//-----------------------------------------------------------------------------
{
	DeviceManager devMgr;
#if 1
	mvCaptureParams_t capParams;
	SafeQueue<frame_t> imageQueue;
	SafeQueue<raw_frame_t> rawImageQueue;

	unsigned int syncSaveCnt = 0;

	string config_file = CONFIG_FILE_DIR;

	// scan command line parameters
#define PARSE_ARGC_ARGV(arg, key, value, paramout) \
{\
	string param(arg);\
	paramout = param;\
	string::size_type keyEnd = param.find_first_of("=");\
	if ((keyEnd == string::npos) || (keyEnd == param.length() - 1))\
	{\
		keyEnd = param.find_first_of("-");\
	}\
	if ((keyEnd == string::npos) || (keyEnd == param.length() - 1))\
	{\
		cout << "Invalid command line parameter: '" << param << "' (ignored)." << endl;\
	}\
	else \
	{\
		key = param.substr(0, keyEnd);\
		value = param.substr(keyEnd + 1);\
	}\
}

	if (argc == 2)
	{
		string param, key, value;
		PARSE_ARGC_ARGV(argv[1], key, value, param)
		if (key == "config")
		{
			LOG_INFO("config file is %s", value.c_str());
			config_file = value;
		}
	}

	if (argc > 2)
	{
		for (int i = 1; i < argc; i++)
		{
			string param, key, value;
			PARSE_ARGC_ARGV(argv[i], key, value, param)
			{
				if (value == "list_devices") {
					LOG_INFO("%s", "The following devices can be used for capture:");
					listDevices(devMgr, true);
					return 0;
				}
				else if (value == "ext_trigger") {
					LOG_INFO("%s", "PARAM: External trigger enabled.");
					capParams.extSyncEnabled = true;
				}
				else if (value == "autoexp") {
					LOG_INFO("%s", "PARAM: Auto Exposure enabled.");
					capParams.autoExposureEnabled = true;
				}
				else if ((key == "exptime")) {
					std::cout << "PARAM: Exposure time, in uS: " << value << endl;
					capParams.expTime_us = stoi(value);
				}
				else if ((key == "exptime_max")) {
					std::cout << "PARAM: Maximum exposure time, in uS: " << value << endl;
					capParams.expTimeMax_us = stoi(value);
				}
				else if ((key == "mid")) {
					std::cout << "PARAM: Master ID: " << value << endl;
					capParams.masterId = stoi(value);
				}
				else if ((key == "mtout")) {
					std::cout << "PARAM: Master TriggerOut Pin: " << value << endl;
					capParams.masterTrigOut = stoi(value);
				}
				else if ((key == "mtin")) {
					std::cout << "PARAM: Master TriggerIn Pin: " << value << endl;
					capParams.masterTrigIn = stoi(value);
				}
				else if (key == "freq") {
					std::cout << "PARAM: Image capture frame rate (Hz): " << value << endl;
					capParams.triggerFrequency_Hz = stoi(value);
				}
				else if (key == "nframes") {
					std::cout << "PARAM: Number of image pairs to acquire: " << value << endl;
					capParams.nImagePairs = stoi(value);
				}
				else if (value == "log_en") {
					std::cout << "PARAM: Logging to file enabled." << endl;
					capParams.writeLogEnabled = true;
				}
				else if (value == "bmp_en") {
					std::cout << "PARAM: Storage to BMP files enabled." << endl;
					capParams.bmpEnabled = true;
				}
				else if (key == "log_int") {
					std::cout << "PARAM: Logging interval: " << value << endl;
					capParams.logInterval = stoi(value);
				}
				else if (key == "disp_int") {
					std::cout << "PARAM: Statistics display interval: " << value << endl;
					capParams.displayInterval = stoi(value);
				}
				else if (key == "threads") {
					std::cout << "PARAM: Image threads: " << value << endl;
					capParams.imageThreads = stoi(value);
				}
				else if (key == "png_ratio") {
					std::cout << "PARAM: PNG Compression ratio: " << value << endl;
					capParams.pngCompression = stoi(value);
				}
				else if (key == "decim") {
					std::cout << "PARAM: Decimation factor: " << value << endl;
					capParams.decimFactor = stoi(value);
				}
				else if (key == "binning") {
					std::cout << "PARAM: Binning factor: " << value << endl;
					capParams.binningFactor = stoi(value);
				}
				else if (key == "samplingMode") {
					std::cout << "PARAM: samplingMode: " << value << endl;
					capParams.samplingMode = stoi(value);
				}
				else if (key == "captureMode") {
					std::cout << "PARAM: captureMode: " << value << endl;
					capParams.captureMode = stoi(value);
				} 
				else if (key == "sid") {
					std::cout << "PARAM: Slave ID: " << value << endl;
					capParams.slaveId = stoi(value);
				} 
				else if ((key == "stin")) {
					std::cout << "PARAM: Slave TriggerIn Pin: " << value << endl;
					capParams.slaveTrigIn = stoi(value);
				}
				else {
					std::cout << "Invalid command line parameter: '" << param
						<< "' (ignored)." << endl;
				}
			}
		}
	}
	else
	{
		printf("%s", help_info.c_str());

		//try load json file
		char json_file[128] = { 0 };
		sprintf_s(json_file,"%s", config_file.c_str());
		if (_access(json_file, 0) == 0)
		{
			mvCaptureConfigParse(json_file, capParams);
			capParams.print();
		}
		else
		{
			std::cout << "Cannot initialize with mvCapture.json, press [enter] to exit" << std::endl;
			std::cin.get();
			exit(0);
		}
	}

	if (capParams.captureMode < MONOCULAR && capParams.captureMode >= NUM_MODE)
	{
		// get user input
		std::cout << endl << "Please enter the mode you want to run (" << MONOCULAR << "-monocular, "
			<< STEREO << "-stereo vision, makesure you have 2 cameras): ";
		unsigned int modeNr = 0;
		std::cin >> modeNr;
		// remove the '\n' from the stream
		std::cin.get();
		if (modeNr == MONOCULAR || modeNr == STEREO)
			capParams.captureMode = modeNr;
		else
		{
			std::cout << endl << "unknwon capture mode, exit" << endl;
			exit(0);
		}
	}
	
	std::cout << "Capture Mode: " << (capParams.captureMode != STEREO ? "Mono" : "Stereo") << endl;

	int devCnt = 0;
	//1. List Devices 	//list all devices (including invalid devices)
	if (capParams.listDevices)
		devCnt = listAllDevices(devMgr, 1);
	else
		devCnt = listAllDevices(devMgr, 0);

	if (devCnt == 0)
	{
		std::cout << "Cannot initialize with valid mv device " << devCnt << ", press [enter] to exit" << std::endl;
		std::cin.get();
		exit(0);
	}

	//list devices if masterId(i.e. !=255)
	if (capParams.masterId != 255 
	|| (capParams.captureMode==STEREO && capParams.slaveId != 255))
	{
		std::cout << "INFO: The following devices can be used for capture:" << std::endl;
		listDevices(devMgr, true);
	}

	//2. Parameter validation
	//a) verify PNG compression ratio
	if (capParams.pngCompression >= 0 && capParams.pngCompression <= 9) 
	{
		std::cout << "INFO: Using " << capParams.pngCompression << " as PNG compression ratio." << endl;
	}
	else 
	{
		std::cout << "ERROR: Cannot use " << capParams.pngCompression << " as PNG compression ratio." << endl;
		std::cout << "ERROR: Compression rate must be between 0 and 9. Using default rate of 2" << endl;
		capParams.pngCompression = 1;
	}

	//b) verify decimation factor
	if (capParams.decimFactor == 1) {
		std::cout << "INFO: Images are not decimated." << endl;
	}
	else if (capParams.decimFactor == 2 || capParams.decimFactor == 4) {
		std::cout << "INFO: Images are decimated by a factor of " << capParams.decimFactor << endl;
	}
	else if (capParams.decimFactor == 0) {
		std::cout << "INFO: Output images will be stored in original size, decimated by two and decimated by four (decim=1)." << endl;
	}
	else {
		std::cout << "ERROR: Decimation factor of " << capParams.decimFactor << " is invalid. Using 0 instead." << endl;
		capParams.decimFactor = 1;
	}

	//c) set image format
	if (capParams.bmpEnabled == true) {
		std::cout << "INFO: Using BMP file format for images. PNG Ratio ignored." << endl;
	}

	//3. Folder validation
	//verify frame output folder if relevant. Ensure './' is not a valid option...
	//Also ensure that last character is always a '/'
	string baseDir;
	if (capParams.outdir.size() == 0)
	{
		std::cout << "ERROR: no desired output dir! Press [Enter] to exit!" << std::endl;
		std::cin.get();
	}

	if (!getFolderID(capParams.outdir, baseDir))
	{
		std::cout << "Cannot initialize with the folder "<< baseDir << ", press [enter] to exit" << std::endl;
		std::cin.get();
		exit(0);
	}
	
	if (baseDir.back() != '/')
		baseDir = baseDir + "/";

	////////
	//4. Initialisation of log file and statistics
	//a) disable log and statistics if relevant
	if (capParams.displayInterval == 0) {
		std::cout << "INFO: Statistics has been disabled." << endl;
		capParams.displayStatsEnabled = false;
	}
	else {
		std::cout << "INFO: Using a statistics display interval of "
			<< capParams.displayInterval << " frames." << endl;
	}
	//b) logging to file is always enabled
	if (capParams.writeLogEnabled == true) {
		if (capParams.logInterval > 0) {
			std::cout << "INFO: Using a logging interval of " << capParams.logInterval
				<< "frames." << endl;
		}
		else {
			std::cout << "INFO: Using a default logging interval of 10 frames."
				<< endl;
			capParams.logInterval = 10;
		}
	}

#if GNSS_SUPPORT
	//5. GNSS Receiver Configuration
	//Setup GNSS Logging. Can only be used when external triggering is used.
	if (gnssPortname.length() > 1 && capParams.extSyncEnabled) 
	{
		//a) Verify if gnssPortname is valid...
		cout << "INFO: Attempting to open GNSS Receiver port..." << endl;
		fd = open(&gnssPortname[0], O_RDWR | O_NOCTTY | O_SYNC);
		if (fd < 0) {
			cout << "ERROR: Opening " << gnssPortname << " failed with error "
				<< strerror(errno) << endl;
			cout << "Unable to continue!" << endl
				<< "Press [ENTER] to exit the application" << endl << endl;
			cin.get();
			return 0;
		}
		cout << "INFO: GNSS Receiver port successfully opened!" << endl;

		//b) update UART interface parameters (baudrate 38400, 8 bits, no parity, 1 stop bit)
		uart_set_interface_attribs(fd, B38400);

		//c) set ublox8 UART Baud Rate to 115200 to ensure all messages will be received at the current baud rate.
		//Note: UART rate must be changed before receiving ACK and other messages. Disabling NMEA messages
		cout << "INFO: Setting ublox UART baud rate to " << 115200 << " using UBX_CFG_PRT command..." << endl;
		ubx_set_port_rate(fd, 115200, false);
		//update UART interface parameters (baudrate 115200, 8 bits, no parity, 1 stop bit)
		uart_set_interface_attribs(fd, B115200);

		if (ubx_ack_wait(fd, UBX_CFG_PRT_CLASS, UBX_CFG_PRT_ID) != 0) {
			cout
				<< "ERROR: Acknowledge message was not received after setting UBX_CFG_PRT. Exiting..."
				<< endl;
			close(fd);
			//TODO: Ensure application exits gracefully if an error occurs...
			return -1;
		}
		else {
			cout << "INFO: UBX ACK received after setting UBX_CFG_PRT!" << endl;
		}

		//d) Wait for GPS Fix
		do {
			//get ublox8 Receiver Navigation Status using UBX-NAV-PVT command (with polling)
			pvtNavPayload = ubx_get_nav_pvt(fd, true);
			cout << "INFO: Waiting for GNSS fix..." << gnssFixItr << "/"
				<< MAX_GNSS_FIX_ITR << endl;
			gnssFixItr++;
		} while (!pvtNavPayload.gnssFixOk && (gnssFixItr < MAX_GNSS_FIX_ITR));
		if (!pvtNavPayload.gnssFixOk) {
			cout << "ERROR: GNSS Fix could not be obtained before timeout!"
				<< endl;
		}
		//setup GNSS update and pulse frequency. Note UBX update frequency is limited to 15 Hz
		//	for the Ublox8 module while timepulse is limited to approx 30Hz
		if ((triggerFrequency_Hz >= 1)
			&& (triggerFrequency_Hz <= MAX_UBX_UPD_FREQ)) {
			if (writeLogEnabled == true) {
				logfile << "INFO: The GNSS pulse frequency has been set to "
					<< triggerFrequency_Hz << "Hz" << endl;
			}
			cout << "INFO: The GNSS pulse frequency has been set to "
				<< triggerFrequency_Hz << "Hz" << endl;
		}
		else if (triggerFrequency_Hz == 255) {
			bool boRun = true;
			while (boRun) {
				cout
					<< "Please enter the approx. desired GNSS pulse frequency in Hz: ";
				triggerFrequency_Hz = getNumberFromUser();
				if ((triggerFrequency_Hz >= 1)
					&& (triggerFrequency_Hz <= MAX_UBX_UPD_FREQ)) {
					boRun = false;
					continue;
				}
				cout
					<< "ERROR: Invalid Selection. Accepted values range from 1 - "
					<< MAX_UBX_UPD_FREQ << "." << endl;
			}
		}
		else {
			cout << "ERROR: Invalid GNSS  pulse frequency of freq="
				<< triggerFrequency_Hz << "Hz has been set." << endl
				<< "Accepted values range from 1 - " << MAX_UBX_UPD_FREQ
				<< "." << endl;

			bool boRun = true;
			while (boRun) {
				cout
					<< "Please enter the approx. desired GNSS trigger frequency in Hz: ";
				triggerFrequency_Hz = getNumberFromUser();
				if ((triggerFrequency_Hz >= 1)
					&& (triggerFrequency_Hz <= MAX_UBX_UPD_FREQ)) {
					boRun = false;
					continue;
				}
				cout
					<< "ERROR: Invalid Selection. Accepted values range from 1 - "
					<< MAX_UBX_UPD_FREQ << "." << endl;
			}
		}

		//e) set ublox8 timepulse parameters to a specific pulse frequency selected by user (1-15Hz)
		//The timepulse will be used as sync source for cameras if extSyncEnabled == true
		//Default timepulse state is disabled
		cout << "INFO: Setting timepulse to " << triggerFrequency_Hz
			<< " Hz using UBX_CFG_TP5 command..." << endl;
		if (ubx_set_timepulse(fd, triggerFrequency_Hz, true) != 0) {
			cout
				<< "ERROR: Acknowledge message was not received after setting UBX_CFG_TP5. Exiting..."
				<< endl;
			close(fd);
			//TODO: Ensure application exits gracefully if an error occurs...
			return -1;
		}
		else {
			cout << "INFO: UBX ACK received after setting UBX_CFG_TP5!" << endl;
		}
		//set ublox8 NMEA/UBX rate to a frequency matching the one used for the timepulse
		cout << "INFO: Setting NMEA/UBX rate to " << triggerFrequency_Hz
			<< " Hz using UBX_CFG_RATE command..." << endl;
		if (ubx_set_meas_rate(fd, triggerFrequency_Hz) != 0) {
			cout
				<< "ERROR: Acknowledge message was not received after setting UBX_CFG_RATE. Exiting..."
				<< endl;
			close(fd);
			//TODO: Ensure application exits gracefully if an error occurs...
			return -1;
		}
		else {
			cout << "INFO: UBX ACK received after setting UBX_CFG_RATE!"
				<< endl;
		}

		//f) enable UBX-NAV-PVT output on USB port (i.e. virtual com port)
		cout << "INFO: Enabling UBX-NAV-PVT message output on USB(Virtual Com port) port using UBX-CFG-MSG command..." << endl;
		if (ubx_enable_msg(fd, UBX_NAV_PVT_CLASS, UBX_NAV_PVT_ID,
			UBX_CFG_MSG_PRT_USB, UBX_CFG_MSG_RATE) != 0) {
			cout << "ERROR: Acknowledge message was not received after setting UBX-CFG-MSG. Exiting..." << endl;
			close(fd);
			//TODO: Ensure application exits gracefully if an error occurs...
			return -1;
		}
		else {
			cout << "INFO: UBX ACK received after setting UBX-CFG-MSG!" << endl;
		}
		//disable UBX-NAV-SVINFO output on USB port (i.e. virtual com port)
		cout << "INFO: Disabling UBX-NAV-SVINFO message output on USB(Virtual Com port) port using UBX-CFG-MSG command..." << endl;

		if (ubx_enable_msg(fd, UBX_NAV_SVINFO_CLASS, UBX_NAV_SVINFO_ID,
			UBX_CFG_MSG_PRT_USB, UBX_CFG_MSG_RATE_DIS) != 0) {
			cout << "ERROR: Acknowledge message was not received after setting UBX-CFG-MSG. Exiting..." << endl;
			close(fd);
			//TODO: Ensure application exits gracefully if an error occurs...
			return -1;
		}
		else {
			cout << "INFO: UBX ACK received after setting UBX-CFG-MSG!" << endl;
		}
		//disable UBX-NAV-STATUS output on USB port (i.e. virtual com port)
		cout << "INFO: Disabling UBX-NAV-STATUS message output on USB(Virtual Com port) port using UBX-CFG-MSG command..." << endl;

		if (ubx_enable_msg(fd, UBX_NAV_STATUS_CLASS, UBX_NAV_STATUS_ID,
			UBX_CFG_MSG_PRT_USB, UBX_CFG_MSG_RATE_DIS) != 0) {
			cout << "ERROR: Acknowledge message was not received after setting UBX-CFG-MSG. Exiting..." << endl;
			close(fd);
			//TODO: Ensure application exits gracefully if an error occurs...
			return -1;
		}
		else {
			cout << "INFO: UBX ACK received after setting UBX-CFG-MSG!" << endl;
		}

		//g) GNSS fix validation
		if (!pvtNavPayload.gnssFixOk) {
			cout << "ERROR: GNSS Fix could not be obtained before timeout!" << endl;
			cout << "GNSS Logging and timepulse will be disabled." << endl;
			gnssPortValid = false;
			close(fd);
			cout << "INFO: Using internal triggering." << endl;
			extSyncEnabled = false;
		}
		else {
			cout << "INFO: GNSS Port valid. GNSS Logging and timepulse is supported." << endl;
			gnssPortValid = true;
		}
	}
	else if (gnssPortname.length() <= 1 && extSyncEnabled) {
		cout << "INFO: GNSS Port blank or invalid. GNSS Logging and timepulse will be disabled." << endl;
		extSyncEnabled = false;
	}
#endif

	//6. Image Thread Verification
	//verify number of image threads.
	if (capParams.imageThreads > 0 && capParams.imageThreads < NUM_STOR_THREADS) {
		std::cout << "INFO: Using " << capParams.imageThreads << " threads for image writing." << endl;
	}
	else {
		std::cout << "ERROR: Cannot use " << capParams.imageThreads
			<< " threads for image writing." << endl;
		std::cout << "ERROR: Number of threads must be between 1 and "
			<< NUM_STOR_THREADS << ". Using default:6" << endl;
		capParams.imageThreads = 6;
	}

	//7. Camera selection
	//Select master camera (either automatically through command line or manual input)
	vector<mvIMPACT::acquire::Device*> validDevices;
	if (getValidDevices(devMgr, validDevices, isDeviceSupportedBySample) > 2)
	{
		std::cout << "ERROR: Only one device is needed for the application;"
			<< endl << validDevices.size()
			<< " device(s) has/have been detected." << endl
			<< "Unable to continue!" << endl
			<< "Press [ENTER] to end the application" << endl << endl;
		std::cin.get();
		return 0;
	}

	if (capParams.masterId == 255) {
		std::cout << "Please select the MASTER device." << endl
			<< "This device will generate trigger signal for both cameras,"
			<< endl
			<< "unless an external trigger signal is used (-ext_trigger)."
			<< endl;
	}
	Device* pMaster = getDeviceFromUserInputId(capParams.masterId, devMgr, true);
	if (!pMaster) {
		std::cout << "ERROR: Device is invalid. Unable to continue!" << endl
			<< "Press [ENTER] to exit the application" << endl << endl;
		std::cin.get();
		return 0;
	}

	/* select slave device if necessary */
	Device* pSlave = NULL;
	if (capParams.captureMode == STEREO)
	{
		bool slaveInvalid = 1;
		do 
		{
			if (capParams.slaveId == 255) {
				std::cout << "Please select the SLAVE device." << endl << endl;
			}
			Device* p = getDeviceFromUserInputId(capParams.slaveId, devMgr, true);
			if (p == pMaster && capParams.slaveId == 255) {
				std::cout << "ERROR: MASTER and SLAVE must be different. Try again!"
					<< endl;
			}
			else if (p == pMaster && capParams.slaveId < 255) {
				std::cout << "ERROR: MASTER and SLAVE must be different." << endl
					<< "Press [ENTER] to exit the application" << endl << endl;
				std::cin.get();
				return 0;
			}
			else {
				pSlave = p;
				slaveInvalid = 0;
			}
		} while (slaveInvalid);

		if (!pSlave) {
			std::cout << "ERROR: Device is invalid. Unable to continue!" << endl
				<< "Press [ENTER] to exit the application" << endl << endl;
			std::cin.get();
			return 0;
		}
		std::cout << endl;
	}

	//8. System Configuration + 9. Camera configuration
	//create vector of devices
	vector<DeviceData*> devices;
	devices.push_back(new DeviceData(pMaster));
	
	if (capParams.captureMode == STEREO)
		devices.push_back(new DeviceData(pSlave));

	{
		const vector<DeviceData*>::size_type DEV_COUNT = devices.size();
		try {
			for (vector<DeviceData*>::size_type i = 0; i < DEV_COUNT; i++)
			{
				cout << "INFO: Initialising device " << devices[i]->device()->serial.read() << "..." << endl;
				devices[i]->init();
				cout << endl << "Setup the " << ((i == 0) ? "MASTER" : "SLAVE")
					<< " device:" << endl << "===========================" << endl << endl;
				// display information about each individual DigitalIO
				vector<int64_type> validLineSelectorValues;
				devices[i]->DIOC()->lineSelector.getTranslationDictValues(validLineSelectorValues);
				const vector<int64_type>::size_type cnt = validLineSelectorValues.size();

				if (i == 0)
				{
					const unsigned int IOCount = devices[i]->DIOC()->lineSelector.dictSize();
					cout << endl;
					cout << "       This device has " << IOCount << " DigitalIOs" << endl;
					cout << " ------------------------------------------" << endl;
					for (unsigned int j = 0; j < cnt; j++)
					{
						devices[i]->DIOC()->lineSelector.write(validLineSelectorValues[j]);
						cout << " IO " << validLineSelectorValues[j] << ": \t Type: "
							<< devices[i]->DIOC()->lineMode.readS() << " \t Current state: "
							<< (devices[i]->DIOC()->lineStatus.read() ? "ON" : "OFF") << endl;
					}
					//List available output signals
					cout << endl;
					cout << "       The following digital outputs are available" << endl;
					cout << " ------------------------------------------" << endl;
					for (unsigned int j = 0; j < cnt; j++)
					{
						//Define the IO we are interested in
						devices[i]->DIOC()->lineSelector.write(validLineSelectorValues[j]);
						//check whether selected IO is an Output or an Input
						if (devices[i]->DIOC()->lineMode.readS() == "Output") {
							cout << " IO " << validLineSelectorValues[j] << ": \t Type: "
								<< devices[i]->DIOC()->lineMode.readS() << " \t Current state: "
								<< (devices[i]->DIOC()->lineStatus.read() ? "ON" : "OFF") << endl;
						}
					}
					//Only select Master trigger output if relevant (i.e. if ext sync isnt used)
					if (capParams.extSyncEnabled == false)
					{
						if (capParams.masterTrigOut == 255)
						{
							cout << endl
								<< "Select the trigger OUTPUT of the MASTER device("
								<< devices[i]->device()->serial.read()
								<< ") where the trigger pulse shall be generated on: ";
							capParams.masterTrigOut = static_cast<uint32_t>(validLineSelectorValues[getNumberFromUser()]);
							cout << endl;
						}
						else if (capParams.masterTrigOut > 3)
						{
							cout << endl << "ERROR: Parameter mtout="
								<< capParams.masterTrigOut << " is invalid!" << endl
								<< "Select the trigger OUTPUT of the MASTER device("
								<< devices[i]->device()->serial.read()
								<< ") where the trigger pulse shall be generated on: ";
							capParams.masterTrigOut = static_cast<uint32_t>(validLineSelectorValues[getNumberFromUser()]);
							cout << endl;
						}
						else
						{
							if (capParams.writeLogEnabled == true)
							{
#if 0
								logfile
									<< "INFO: The trigger OUTPUT of the MASTER device("
									<< devices[i]->device()->serial.read()
									<< ") has been set to " << mvCapHandler.mvCapParams.masterTrigOut
									<< endl;
#endif
							}
							cout << endl
								<< "INFO: The trigger OUTPUT of the MASTER device("
								<< devices[i]->device()->serial.read()
								<< ") has been set to " << capParams.masterTrigOut
								<< endl;
						}
					}
					else {
						if (capParams.masterTrigOut == 255) {
							cout << "INFO: External Sync Enabled." << endl;
						}
						else {
							cout << "INFO: External Sync Enabled, MASTER trigger parameter ignored." << endl;
						}
					}
				}
				//List available input signals (only do this once)
				cout << endl;
				cout << "       The following digital inputs are available" << endl;
				cout << " ------------------------------------------" << endl;
				for (unsigned int j = 0; j < cnt; j++)
				{
					//Define the IO we are interested in
					devices[i]->DIOC()->lineSelector.write(validLineSelectorValues[j]);
					//check whether selected IO is an Output or an Input
					if (devices[i]->DIOC()->lineMode.readS() == "Input")
					{
						cout << " IO " << validLineSelectorValues[j]
							<< ": \t Type: "
							<< devices[i]->DIOC()->lineMode.readS()
							<< " \t Current state: "
							<< (devices[i]->DIOC()->lineStatus.read() ?
								"ON" : "OFF") << endl;
					}
				}
				if (i == 0)
				{
					//Get trigger input for the master device
					if (capParams.masterTrigIn == 255)
					{
						cout << endl
							<< "Select the trigger INPUT of the MASTER device("
							<< devices[i]->device()->serial.read()
							<< ") used for frame capture: ";
						capParams.masterTrigIn = static_cast<uint32_t>(validLineSelectorValues[getNumberFromUser()]);
						cout << endl;
					}
					else if (capParams.masterTrigIn < 255
						&& (capParams.masterTrigIn > 5 || capParams.masterTrigIn < 4))
					{
						cout << endl << "ERROR: Parameter mtin=" << capParams.masterTrigIn
							<< " is invalid!" << endl
							<< "Select the trigger INPUT of the MASTER device("
							<< devices[i]->device()->serial.read()
							<< ") used for frame capture: ";
						capParams.masterTrigIn = static_cast<uint32_t>(validLineSelectorValues[getNumberFromUser()]);
						cout << endl;
					}
					else
					{
#if 0
						if (mvCapHandler.mvCapParams.writeLogEnabled == true)
						{
							logfile
								<< "INFO: The trigger INPUT of the MASTER device("
								<< devices[i]->device()->serial.read()
								<< ") has been set to " << mvCapHandler.mvCapParams.masterTrigIn << endl;
						}
#endif
						cout << endl
							<< "INFO: The trigger INPUT of the MASTER device("
							<< devices[i]->device()->serial.read()
							<< ") has been set to " << capParams.masterTrigIn << endl;
					}
				}
				else {
					//Get trigger input for the slave device
					if (capParams.slaveTrigIn == 255) {
						cout << endl
							<< "Select the trigger INPUT of the SLAVE device("
							<< devices[i]->device()->serial.read()
							<< ") used for frame capture: ";
						capParams.slaveTrigIn = static_cast<uint32_t>(validLineSelectorValues[getNumberFromUser()]);
						cout << endl;
					}
					else if (capParams.slaveTrigIn < 255
						&& (capParams.slaveTrigIn > 5 || capParams.slaveTrigIn < 4)) {
						cout << endl << "ERROR: Parameter stin=" << capParams.masterTrigIn
							<< " is invalid!" << endl
							<< "Select the trigger INPUT of the SLAVE device("
							<< devices[i]->device()->serial.read()
							<< ") used for frame capture: ";
						capParams.slaveTrigIn = static_cast<uint32_t>(validLineSelectorValues[getNumberFromUser()]);
						cout << endl;
					}
					else {
						if (capParams.writeLogEnabled == true) {
#if 0
							logfile
								<< "INFO: The trigger INPUT of the SLAVE device("
								<< devices[i]->device()->serial.read()
								<< ") has been set to " << slaveTrigIn << endl;
#endif
						}
						cout << endl
							<< "INFO: The trigger INPUT of the SLAVE device("
							<< devices[i]->device()->serial.read()
							<< ") has been set to " << capParams.slaveTrigIn << endl;
					}
				}

				//Setup exposure depending on input parameters (i.e. automatic exposure or fixed exposure time)
				setupExposure(devices[i], capParams.autoExposureEnabled, capParams.expTime_us,
					capParams.expTimeMax_us);

				//Setup image format control
				setupImageFormat(devices[i], capParams.binningFactor, false);
			}
		}
		catch (const ImpactAcquireException& e) {
			// this e.g. might happen if the same device is already opened in another process...
			cout << "ERROR: An error occurred while opening the devices(error code: "
				<< e.getErrorCodeAsString() << ")." << endl
				<< "Press [ENTER] to end the application" << endl << endl;
			cin.get();

			return 0;
		}

		if (capParams.captureMode == MONOCULAR)
		{
			mvMonoCaptureParameters mvMonoCap(capParams, baseDir, rawImageQueue, 1, syncSaveCnt);
			ThreadEncodingParam		encodingParam(rawImageQueue, imageQueue, 2, baseDir, capParams);
			ThreadStorageParam		storageParam(imageQueue, 6, baseDir, capParams);

			bool initSuccess = true;
			initSuccess = runImageStorage(storageParam);
			if (initSuccess == false)
			{
				std::cout << "Image Storage threads init failed, please check!!!!" << std::endl;
				goto failedmono;
			}
			initSuccess = runImageEncoding(encodingParam);
			if (initSuccess == false)
			{
				std::cout << "Image Encoding threads init failed, please check!!!!" << std::endl;
				goto failedmono;
			}
			initSuccess = startMonoImageAcquisitions(mvMonoCap, devices[0]);
			if (initSuccess == false)
			{
				std::cout << "Image Acquisition threads init failed, please check!!!!" << std::endl;
				goto failedmono;
			}
			//12. Image Acquisition Loop
			//wait for user input before continuing.
failedmono:
			cout << (capParams.isCaptureContinuous == 1 ? ("Start Continuous Capture:\n") : ("Press s[S] to acquire image[s].\n")) 
				 << "Press [ENTER] to end the acquisition" << endl << endl;

			clock_t prev = clock();
			clock_t now;
			while (1)
			{
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0) //必要的，我是背下来的 
				if (KEY_DOWN(VK_RETURN))
					break;
				else if (KEY_DOWN(0x53))
				{
					now = clock();
					if ((double(now - prev) / CLOCKS_PER_SEC) > 1)
					{
						prev = now;
						syncSaveCnt++;
						printf("syncSaveCnt %d\n", syncSaveCnt);
						system("cls");
					}
				}
				Sleep(20);
			}
			cout << "INFO: Terminating all threads..." << endl;

			endMonoImageAcquisition(mvMonoCap, devices[0]);
			endImageEncoding(encodingParam);
			endImageStorage(storageParam);
		}
		else if (capParams.captureMode == STEREO)
		{
			mvStereoCaptureParameters mvStereoCap(capParams, baseDir, rawImageQueue, 2);
			ThreadEncodingParam		encodingParam(rawImageQueue, imageQueue, 4, baseDir, capParams);
			ThreadStorageParam		storageParam(imageQueue, 8, baseDir, capParams);

			bool initSuccess = true;

			initSuccess = runImageStorage(storageParam);
			if (initSuccess == false)
			{
				std::cout << "Image Storage threads init failed, please check!!!!" << std::endl;
				goto failedstereo;
			}
			initSuccess = runImageEncoding(encodingParam);
			if (initSuccess == false)
			{
				std::cout << "Image Encoding threads init failed, please check!!!!" << std::endl;
				goto failedstereo;
			}
			initSuccess = startStereoImageAcquisitions(mvStereoCap, devices[0], devices[1]);
			if (initSuccess == false)
			{
				std::cout << "Image Acquisition threads init failed, please check!!!!" << std::endl;
				goto failedstereo;
			}
			//12. Image Acquisition Loop
			//wait for user input before continuing.
failedstereo:
			cout << (capParams.isCaptureContinuous == 1 ? ("Start Continuous Capture:\n") : ("Press s[S] to acquire image[s].\n"))
				<< "Press [ENTER] to end the acquisition" << endl << endl;
			clock_t prev = clock();
			clock_t now;
			while (1)
			{
#define KEY_DOWN(VK_NONAME) ((GetAsyncKeyState(VK_NONAME) & 0x8000) ? 1:0) //必要的，我是背下来的 
				if (KEY_DOWN(VK_RETURN))
					break;
				else if (KEY_DOWN(0x53))
				{
					now = clock();
					if ((double(now - prev) / CLOCKS_PER_SEC) > 1)
					{
						prev = now;
						syncSaveCnt++;
						printf("syncSaveCnt %d\n", syncSaveCnt);
						system("cls");
					}
				}
				Sleep(20);
			}
			cout << "INFO: Terminating all threads..." << endl;
			endStereoImageAcquisition(mvStereoCap, devices[0], devices[1]);
			endImageEncoding(encodingParam);
			endImageStorage(storageParam);
		}
		else if (capParams.captureMode == CIRCULAR_PATTERN)
		{
			mvMonoCaptureParameters mvMonoCap(capParams, baseDir, rawImageQueue, 1, syncSaveCnt);
			ThreadCircularParam		cpParam(rawImageQueue, capParams, baseDir);

			bool initSuccess = true;
			initSuccess = runCircularPatternDetection(cpParam);
			if (initSuccess == false)
			{
				std::cout << "Image Encoding threads init failed, please check!!!!" << std::endl;
				goto failecircularpattern;
			}
			initSuccess = startMonoImageAcquisitions(mvMonoCap, devices[0]);
			if (initSuccess == false)
			{
				std::cout << "Image Acquisition threads init failed, please check!!!!" << std::endl;
				goto failecircularpattern;
			}
			//12. Image Acquisition Loop
			//wait for user input before continuing.
		failecircularpattern:
			cout << (capParams.isCaptureContinuous == 1 ? ("Start Continuous Capture:\n") : ("Press s[S] to acquire image[s].\n"))
				<< "Press [ENTER] to end the acquisition" << endl << endl;

			clock_t prev = clock();
			clock_t now;
			while (1)
			{
				if (KEY_DOWN(VK_RETURN))
					break;
				else if (KEY_DOWN(0x53))
				{
					now = clock();
					if ((double(now - prev) / CLOCKS_PER_SEC) > 1)
					{
						prev = now;
						syncSaveCnt++;
						printf("syncSaveCnt %d\n", syncSaveCnt);
						system("cls");
					}
				}
				Sleep(20);
			}
			cout << "INFO: Terminating all threads..." << endl;

			endMonoImageAcquisition(mvMonoCap, devices[0]);
			endCircularPatternDetection(cpParam);
		}

		cout << "INFO: Threads terminated successfully!" << endl;
	}
	

	return 0;
}
#else
	int modeNr = MONOCULAR;
	if (modeNr == MONOCULAR)
	{
		Device* pDev = getDeviceFromUserInput(devMgr);
		if (!pDev)
		{
			std::cout << "Unable to continue!";
			std::cout << "Press [ENTER] to end the application" << endl;
			std::cin.get();
			return 0;
		}
		std::cout << "Initialising the device. This might take some time..." << endl;
		try
		{
			pDev->open();
		}
		catch (const ImpactAcquireException& e)
		{
			// this e.g. might happen if the same device is already opened in another process...
			std::cout << "An error occurred while opening device " << pDev->serial.read()
				<< "(error code: " << e.getErrorCodeAsString() << ")." << endl
				<< "Press [ENTER] to end the application..." << endl;
			std::cin.get();
			return 0;
		}
#if 0
		CaptureParameter captureParams(pDev, sharedBuf, &header, &g_header_mutex);
		ThreadStorageParam storageParams(sharedBuf, &header, &g_header_mutex);
		//=============================================================================
		//========= Capture loop into memory managed by the driver (default) ==========
		//=============================================================================
		cout << "The device will try to capture continuously into memory automatically allocated be the device driver." << endl
			<< "This is the default behaviour." << endl;
		cout << "Press [ENTER] to end the continuous acquisition." << endl;
		runLiveLoop(captureParams);
		runImageStorage(storageParams);

		cin.get();

		endLiveLoop();
		endImageStorage();
#endif
	}
	else if (modeNr == STEREO)
	{
		std::cout << " TODO issues: because hardware supoort needed!!!!!!" << endl;
		return 0;
#if 0
		vector<mvIMPACT::acquire::Device*> validDevices;
		if (getValidDevices(devMgr, validDevices, isDeviceSupportedBySample) != 2)
		{
			cout << "This sample needs 2 valid devices(one master and one slave). " << validDevices.size() << " device(s) has/have been detected." << endl
				<< "Unable to continue!" << endl
				<< "Press [ENTER] to end the application" << endl;
			cin.get();
			return 0;
		}

		cout << "Please select the MASTER device(the one that will create the trigger for the other device)." << endl << endl;
		int masterNr = -1;
		Device* pMaster = getMasterDeviceFromUserInput(devMgr, masterNr, isDeviceSupportedBySample, true);
		if (!pMaster || masterNr == -1)
		{
			cout << "Master device has not been properly selected. Unable to continue!" << endl
				<< "Press [ENTER] to end the application" << endl;
			cin.get();
			return 0;
		}

		int slaveNr = (masterNr == 0) ? 1 : 0;// only two
		Device* pSlave = devMgr[slaveNr];
		if (pSlave == pMaster)
		{
			cout << "Master and slave must be different. Skipped!" << endl;
		}

		cout << endl;
		std::vector<DeviceData*> devices;
		devices.push_back(new DeviceData(pMaster));
		devices.push_back(new DeviceData(pSlave));

		const std::vector<DeviceData*>::size_type DEV_COUNT = devices.size();
#endif
	}
	return 0;
}
#endif

#if 0
FIFO_release(sharedBuf);

return 0;
#endif