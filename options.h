#pragma once
#include <map>
#include <string>
#include <stdint.h>

static std::map<std::string, std::string> options{
	{ "mono"		 , "Mono Capture: "},
	{ "stereo" 		 , "Stereo Capture: "},
	{ "list_devices" , "The following devices can be used for capture:"},
	{ "ext_trigger"	 , "PARAM: External trigger enabled." },
	{ "autoexp"		 , "PARAM: Auto Exposure enabled." },
	{ "exptime"		 , "PARAM: Exposure time, in uS:" },
	{ "exptime_max"	 , "PARAM: Maximum exposure time, in uS: " },
	{ "mid"			 , "PARAM: Master ID: " },
	{ "mtout"		 , "PARAM: Master TriggerOut Pin: " },
	{ "mtin"		 , "PARAM: Master TriggerIn Pin: " },
	{ "freq"		 , "PARAM: Image capture frame rate (Hz): " },
	{ "nframes"		 , "PARAM: Number of image pairs to acquire: " },
	{ "outdir"		 , "PARAM: Image Output directory: " },
	{ "log_en"		 , "PARAM: Logging to file enabled." },
	{ "bmp_en"		 , "PARAM: Storage to BMP files enabled." },
	{ "log_int"		 , "PARAM: Logging interval: " },
	{ "disp_int"     , "PARAM: Statistics display interval: " },
	{ "png_ratio"    , "PARAM: PNG Compression ratio: " },
	{ "threads"		 , "PARAM: storage threads: " },
	{ "decim"		 , "PARAM: Decimation factor: " },
	{ "gnss_port"	 , "PARAM: GNSS Receiver Portname: " },
	{ "binning"		 , "PARAM: Binning factor: " },
	{ "samplingMode" , "PARAM: samplingMode: " },
};

static std::string help_info = " \
No command line parameters specified. \n\n \
AVAILABLE PARAMETERS:\n \
============================================================ \n \
'-list_devices' \t to generate a list of valid MatrixVision BlueFOX3 devices and exit. \n \
'-autoexp' \t to use auto exposure for image capture. \n \
'exptime' \t to set a fixed exposure time (in uS), if not set automatically. \n \
'exptime_max' \t to set a maximum exposure time (in uS), if using auto exposure. Default 20000uS \n \
'mid' \t\t to set the ID for the master camera. \n \
'sid' \t\t to set the ID for the slave camera. \n \
'-ext_trigger'  to use external PPS signal for frame capture sync. Not combined with mtout \n \
'mtout' \t to set trigger line output for the master camera. \n \
'mtin' \t to set trigger line input for the master camera. \n \
'stin' \t to set trigger line input for the slave camera. \n \
'freq' \t to set image capture frequency (in Hz) for both cameras OR to set GNSS Receiver frequency. \n \
'nframes' \t to set number of image pairs to acquire. \n \
'outdir' \t to set output directory for the image pairs. \n \
'log_int' \t to set logging interval for file. \n \
'disp_int' \t to set logging interval for display. Zero to disable statistics functionality. \n \
'threads' \t to set number of threads used for image storage. Default 6 threads. \n \
'-bmp_en'  to use BMP file format for image output. \n \
'png_ratio' \t to set PNG compression ratio. Values ranging from 0-9. Not used with '-bmp_en'. \n \
'decim' \t to set decimation factor used for image resizing. Use 1,2,4 or 8. Default is 2. \n \
\t 'decim=1' will enable output in original size, decim by two and decim by four. \n \
'gnss_port' \t to set gnss receiver port for GNSS logging. \n \
'binning' \t to set binning factor for output image. Use 1,2,4, or 8. NEVER USE THIS WITH decim. Default is 1. \n \
'samplingMode' \t to select sampling mode: 1. with external board; 2. standalone. Default is 2. \n \
EXAMPLE USAGE: \n \n \n \
==========================MONO================================= \n \
Acquire 10 image pairs with 1Hz frequency using internal triggering on Master Pin 5: \n \
\t ./mvMonoCapture mid=0 mtout=1 mtin=5 freq=1 nframes=10 outdir=/media/odroid/KINGSTON/  \n   \n \
As above but with output folder in application root folder and with continuous image capture:  \n \
\t ./mvMonoCapture mid=0 mtout=1 mtin=5 freq=1 outdir=./OUT/  \n \n \
External triggering (from GNSS Receiver) on Master input 4:  \n \
\t ./mvMonoCapture mid=0 -ext_trigger mtin=4 outdir=./OUT/  \n  \n \
Using either dynamic or fixed shutter speed: \n \
\t ./mvMonoCapture -autoexp exptime_max=10000 ... [parameters]  \n \
\t ./mvMonoCapture exptime=10000 ... [parameters]  \n \
Generating a log file during acquisition, logging interval of 10 frames, display interval of 20 frames: \n \
\t ./mvMonoCapture log_int=10 disp_int=20 ... [parameters]  \n \
Logging timestamps from external GNSS receiver with update frequency of 5 Hz, using trigger input pin 4:  \n \
\t ./mvMonoCapture [parameters] ... -ext_trigger mtin=4 stin=4 freq=5 gnss_port=/dev/ttyACM0 outdir=./OUT/  \n \
Reduce image size by a (decimation) factor of four:  \n \
\t ./mvMonoCapture [parameters] ... decim=4 ... outdir=./OUT/  \n  \n \
COMPLETE EXAMPLE (EXTERNAL):  \n \
============================================================ \n \
Using external trigger (GNSS) on Master pin 4, autoexposure with maximum of 10000uS, \n \
Log file uses interval of 10 frames, while statistics are displayed with an interval of 20 frames. \n \
GNSS Receiver (on /dev/ttyACM0) updates with frequency of 5 Hz and generates PPS. \n \
10 threads are used for image storage to avoid growing memory usage. All all images are stored to ./OUT/  \n \
\t ./mvMonoCapture mid=0 -ext_trigger mtin=5 -autoexp exptime_max=10000 log_int=20 disp_int=20 freq=5 \
threads=10 png_ratio=2 gnss_port=/dev/ttyACM0 outdir=./OUT/  \n  \n \
COMPLETE EXAMPLE (INTERNAL):  \n \
============================================================ \n \
Using master triggering (5Hz) on Master/Slave pin 5, auto exposure with maximum of 10000uS, \n \
Log file uses interval of 1 frame, while statistics are displayed with an interval of 20 frames. \n \
10 threads are used for image storage to avoid growing memory usage. All images are stored to ./OUT/  \n \
\t ./mvMonoCapture mid=0 mtout=1 mtin=5 -autoexp exptime_max=10000 log_int=10 disp_int=20 freq=5 threads=10 png_ratio=2 outdir=./OUT/  \n  \n \
Note: All examples assume master/slave ID's according to the output from '-list_devices' \n \n \n \
==========================MONO================================ = \n \
Acquire 10 image pairs with 1Hz frequency using internal triggering on Master/Slave Pin 5: \n \
\t ./mvPairCapture mid=0 sid=1 mtout=1 mtin=5 stin=5 freq=1 nframes=10 outdir=/media/odroid/KINGSTON/ \n \n \
As above but with output folder in application root folder and with continuous image capture: \n \
\t ./mvPairCapture mid=0 sid=1 mtout=1 mtin=5 stin=5 freq=1 outdir=./OUT/ \n \n \
External triggering (from GNSS Receiver) on Master/Slave input 4:\n \
\t ./mvPairCapture mid=0 sid=1 -ext_trigger mtin=4 stin=4 outdir=./OUT/ \n \n \
Using either dynamic or fixed shutter speed: \n \
\t ./mvPairCapture -autoexp exptime_max=10000 ... [parameters] \n \
\t  ./mvPairCapture exptime=10000 ... [parameters] \n \
Generating a log file during acquisition, logging interval of 10 frames, display interval of 20 frames:\n \
\t ./mvPairCapture log_int=10 disp_int=20 ... [parameters] \n \
Logging timestamps from external GNSS receiver with update frequency of 5 Hz, using trigger input pin 4:\n \
\t ./mvPairCapture [parameters] ... -ext_trigger mtin=4 stin=4 freq=5 gnss_port=/dev/ttyACM0 outdir=./OUT/\n \
Reduce image size by a (decimation) factor of four:\n \
\t ./mvPairCapture [parameters] ... decim=4 ... outdir=./OUT/\n \n \
COMPLETE EXAMPLE (EXTERNAL): \n \
============================================================ \n \
Using external trigger (GNSS) on Master/Slave pin 4, autoexposure with maximum of 10000uS,\n \
Log file uses interval of 10 frames, while statistics are displayed with an interval of 20 frames.\n \
GNSS Receiver (on /dev/ttyACM0) updates with frequency of 5 Hz and generates PPS.\n \
10 threads are used for image storage to avoid growing memory usage. All all images are stored to ./OUT/ \n \
\t ./mvPairCapture mid=0 sid=1 -ext_trigger mtin=5 stin=5 -autoexp exptime_max=10000 log_int=20 disp_int=20 freq=5 threads=10 png_ratio=2 gnss_port=/dev/ttyACM0 outdir=./OUT/ \n \n \
COMPLETE EXAMPLE (INTERNAL): \n \
============================================================\n \
Using master triggering (5Hz) on Master/Slave pin 5, auto exposure with maximum of 10000uS,\n \
Log file uses interval of 1 frame, while statistics are displayed with an interval of 20 frames.\n \
10 threads are used for image storage to avoid growing memory usage. All images are stored to ./OUT/\n \
\t ./mvPairCapture mid=0 sid=1 mtout=1 mtin=5 stin=5 -autoexp exptime_max=10000 log_int=10 disp_int=20 freq=5 threads=10 png_ratio=2 outdir=./OUT/ \n \n \
Note: All examples assume master/slave ID's according to the output from '-list_devices' \n \
\n \
\n \
\n \
-------------------------------------------------------------------------- \n \
\n \
\n \
\t\t !!!!! Now default options will be used !!!!! \n";