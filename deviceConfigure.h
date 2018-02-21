#pragma once
//-----------------------------------------------------------------------------
#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <mvIMPACT_CPP/mvIMPACT_acquire.h>
#include <mvIMPACT_CPP/mvIMPACT_acquire_GenICam.h>
#include <apps/Common/exampleHelper.h>
#include "datatype.h"

//-----------------------------------------------------------------------------
//Setup image format control
inline void setupImageFormat(DeviceData* pDevData, unsigned int binning = 1, bool decimationEnabled = true)
{
	GenICam::ImageFormatControl ifc(pDevData->device());
	ifc.pixelFormat.writeS("Mono8");

	// since binning and decimation shouldn't be used together
	if (binning != 1)
	{
		ifc.binningHorizontal.write(binning);
		ifc.decimationHorizontal.write(1);
		ifc.decimationVertical.write(1);
	}
	else
	{
		ifc.binningHorizontal.write(1);
		if (decimationEnabled == true) {
			ifc.decimationHorizontal.write(2);
			ifc.decimationVertical.write(2);
		}
		else {
			ifc.decimationHorizontal.write(1);
			ifc.decimationVertical.write(1);
		}
	}
}


//-----------------------------------------------------------------------------
//Setup exposure parameters (i.e. automatic exposure or fixed exposure time (each camera)
inline void setupExposure(DeviceData* pDevData, bool autoEnabled = 1,
	unsigned int exposure_us = 0, unsigned int exposureMax_us = 10000)
	//-----------------------------------------------------------------------------
{
	GenICam::AcquisitionControl ac(pDevData->device());
	//TODO: The max limit should not necessarily be this low as it depends on aperture and other factors.
	unsigned int maxLimit_us = 500000;
	unsigned int minLimit_us = 50;

	//See: https://www.matrix-vision.com/manuals/mvBlueCOUGAR-X/Features_page.html#Features_section_AcquisitionControl
	ac.acquisitionMode.writeS("Continuous");
	if (autoEnabled) {
		ac.exposureMode.writeS("Timed");
		ac.exposureAuto.writeS("Continuous");
		conditionalSetEnumPropertyByString(ac.mvExposureAutoSpeed, "Fast");
		ac.mvExposureAutoLowerLimit.write(minLimit_us);
		if (exposureMax_us > minLimit_us && exposureMax_us <= maxLimit_us) {
			ac.mvExposureAutoUpperLimit.write(exposureMax_us);
			std::cout << "INFO: Using automatic exposure between " << minLimit_us
				<< " and " << exposureMax_us << " uS." << std::endl;
		}
		else {
			ac.mvExposureAutoUpperLimit.write(maxLimit_us);
			std::cout << "INFO: Using automatic exposure between " << minLimit_us
				<< " and " << maxLimit_us << " uS." << std::endl;
		}
		//ac.mvExposureAutoAverageGrey.write(60);								// 
	}
	else
	{
		if (exposure_us > minLimit_us && exposure_us <= maxLimit_us)
		{
			//set exposure time if relevant  (in microseconds)
			ac.exposureMode.writeS("Timed");
			ac.exposureTime.write(exposure_us);
			ac.exposureAuto.writeS("Off");
			std::cout << "INFO: Using a fixed exposure time of " << exposure_us
				<< "uS." << std::endl;
		}
		else
		{
			//Use auto exposure if incorrect exposure selected
			ac.exposureMode.writeS("Timed");
			ac.exposureAuto.writeS("Continuous");
			//ac.exposureTime.write(maxExposure_us);
			ac.mvExposureAutoLowerLimit.write(minLimit_us);
			ac.mvExposureAutoUpperLimit.write(maxLimit_us);
			std::cout << "ERROR: Invalid or no fixed exposure set." << std::endl;
			std::cout << "INFO: Using automatic exposure between " << minLimit_us
				<< " and " << maxLimit_us << " uS." << std::endl;
		}
	}
}

//-----------------------------------------------------------------------------
//Setup trigger input used for image capture (each camera)
inline void setupFrameTriggerInput(DeviceData* pDevData, unsigned int lineIdx)
{
	std::string lineStr("Line" + std::to_string(lineIdx));
	GenICam::AcquisitionControl ac(pDevData->device());
	ac.triggerSelector.writeS("FrameStart");
	ac.triggerMode.writeS("On");
	ac.triggerSource.writeS(lineStr);
	//ac.triggerSource.writeS( "Line5" );
	ac.triggerActivation.writeS("RisingEdge");
}

inline void setupStandaloneFrameTrigger(DeviceData* pDevData)
{
	GenICam::AcquisitionControl ac(pDevData->device());
	ac.triggerSelector.writeS("FrameStart");
	ac.triggerMode.writeS("On");
	ac.triggerSource.writeS("Timer1End");
}

//-----------------------------------------------------------------------------
//Setup trigger input used for external PPS sync.
inline void setupSyncTriggerInput(DeviceData* pDevData, unsigned int lineIdx)
{
	std::string lineStr("Line" + std::to_string(lineIdx));
	GenICam::mvLogicGateControl lgc(pDevData->device());
	lgc.mvLogicGateANDSelector.writeS("mvLogicGateAND1");
	lgc.mvLogicGateANDSource1.writeS(lineStr);
	lgc.mvLogicGateANDSource2.writeS(lineStr);
	lgc.mvLogicGateORSelector.writeS("mvLogicGateOR1");
	lgc.mvLogicGateORSource1.writeS("mvLogicGateAND1Output");
}

//-----------------------------------------------------------------------------
//Setup timer used for individual frames acquired within each sequence.
inline void setupFrameTimer(DeviceData* pDevData, unsigned int frameRate_Hz)
{
	GenICam::CounterAndTimerControl catcMaster(pDevData->device());
	catcMaster.timerSelector.writeS("Timer1");
	catcMaster.timerDelay.write(0.);
	//catcMaster.timerDuration.write( 1000000. ); 				//in uS, 1 S pulse length (1Hz)
	//catcMaster.timerDuration.write( 30000. ); 				//in uS, 1/30 S pulse length (30Hz)
	catcMaster.timerDuration.write(1000000. / frameRate_Hz); //in uS, 1 S pulse length (1Hz)
	catcMaster.timerTriggerSource.writeS("Timer1End");

	catcMaster.timerSelector.writeS("Timer2");
	catcMaster.timerDelay.write(0.);
	catcMaster.timerDuration.write(10000.);		//trigger pulse duration in uS
	catcMaster.timerTriggerSource.writeS("Timer1End");
}

//-----------------------------------------------------------------------------
//Enable trigger signal generation on the Output IO
inline void enableFrameTriggerOutput(DeviceData* pDevData, unsigned int lineIdx, std::string lineSource)
{
	std::string lineStr("Line" + std::to_string(lineIdx));
	GenICam::DigitalIOControl io(pDevData->device());

	//Write Line1 Output when timer2 is active
	io.lineSelector.writeS(lineStr);
	//io.lineSelector.writeS( "Line1" );
	io.lineSource.writeS(lineSource);
}

//-----------------------------------------------------------------------------
//Disable trigger signal generation on the Output IO
inline void disableFrameTriggerOutput(DeviceData* pDevData, unsigned int lineIdx)
{
	std::string lineStr("Line" + std::to_string(lineIdx));
	GenICam::DigitalIOControl io(pDevData->device());

	io.lineSelector.writeS(lineStr);
	//io.lineSelector.writeS( "Line1" );
	io.lineSource.writeS("Off");
}