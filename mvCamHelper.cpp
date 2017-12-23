/*
* mvCamHelper.cpp
*
*  Created on: Apr X, 2017
*      Author: jlinde
*/
#include "stdafx.h"

#include <iostream>
#include <set>
#include <string>
#include <stdexcept>

#include <apps/Common/exampleHelper.h>
#include "mvCamHelper.h"

using namespace std;

//-----------------------------------------------------------------------------
unsigned int getNumberFromUser(void)
//-----------------------------------------------------------------------------
{
	unsigned int nr = 0;
	cin >> nr;
	// remove the '\n' from the stream
	cin.get();
	return nr;
}

//-----------------------------------------------------------------------------
bool isDeviceSupportedBySample(const Device* const pDev)
//-----------------------------------------------------------------------------
{
	return (match(pDev->product.read(), string("mvBlueFOX3*"), '*') == 0);
}

unsigned int listAllDevices(const mvIMPACT::acquire::DeviceManager& devMgr, int isDump) {
	const unsigned int devCnt = devMgr.deviceCount();
	std::cout << devMgr.deviceCount() << " devices have been detected (both valid/invalid):" << std::endl;
	for (unsigned int i = 0; i < devCnt; i++)
	{
		Device* pDev = devMgr[i];
		if (pDev)
		{
			if (isDump == 1)
			{
				std::cout << "  [" << i << "]: " << pDev->serial.read() << " (" << pDev->product.read() << ", " << pDev->family << ")" << std::endl;
			}
		}
	}
	return devCnt;
}

//std::set<unsigned int> listDevices( const mvIMPACT::acquire::DeviceManager& devMgr, SUPPORTED_DEVICE_CHECK pSupportedDeviceCheckFn, bool UseGenICam, bool silent)
std::set<unsigned int> listDevices(const mvIMPACT::acquire::DeviceManager& devMgr, bool UseGenICam)
{
	const unsigned int devCnt = devMgr.deviceCount();
	std::set<unsigned int> validDeviceNumbers;
	bool isSupportedDevice;

	if (devCnt == 0)
	{
		std::cout << "No compatible device found!" << std::endl;
		//return 0;
		return validDeviceNumbers;
	}
	// prepare/display every device detected that matches
	for (unsigned int i = 0; i < devCnt; i++)
	{
		Device* pDev = devMgr[i];
		//verify if device is compatible, i.e. mvBlueFOX3
		isSupportedDevice = (match(pDev->product.read(), string("mvBlueFOX3*"), '*') == 0);
		if (pDev)
		{
			if (isSupportedDevice) {
				//if(!silent){
				std::cout << "  [" << i << "]: " << pDev->serial.read() << " (" << pDev->product.read() << ", " << pDev->family;
				//}
				if (pDev->interfaceLayout.isValid())
				{
					if (UseGenICam)
					{
						// if this device offers the 'GenICam' interface switch it on, as this will
						// allow are better control over GenICam compliant devices
						conditionalSetProperty(pDev->interfaceLayout, dilGenICam, true);
					}
				}
				if (pDev->acquisitionStartStopBehaviour.isValid())
				{
					// if this device offers a user defined acquisition start/stop behaviour
					// enable it as this allows finer control about the streaming behaviour
					conditionalSetProperty(pDev->acquisitionStartStopBehaviour, assbUser, true);
				}
				if (pDev->interfaceLayout.isValid() && !pDev->interfaceLayout.isWriteable())
				{
					if (pDev->isInUse())
					{
						std::cout << ", !!!ALREADY IN USE!!!";
					}
				}
				//if(!silent){
				std::cout << ")" << std::endl;
				//}
				validDeviceNumbers.insert(i);
			}
		}
	}

	if (validDeviceNumbers.empty())
	{
		std::cout << devMgr.deviceCount() << " devices have been detected:" << std::endl;
		for (unsigned int i = 0; i < devCnt; i++)
		{
			Device* pDev = devMgr[i];
			if (pDev)
			{
				std::cout << "  [" << i << "]: " << pDev->serial.read() << " (" << pDev->product.read() << ", " << pDev->family << ")" << std::endl;
			}
		}
		std::cout << "However none of these devices seems to be supported by this sample." << std::endl << std::endl;
		//return 0;
		return validDeviceNumbers;
	}

	return validDeviceNumbers;
}

//mvIMPACT::acquire::Device* getDeviceFromUserInputId(unsigned int cameraId, const mvIMPACT::acquire::DeviceManager& devMgr, SUPPORTED_DEVICE_CHECK pSupportedDeviceCheckFn, bool boSilent, bool UseGenICam)

mvIMPACT::acquire::Device* getDeviceFromUserInputId(unsigned int cameraId, const mvIMPACT::acquire::DeviceManager& devMgr, bool UseGenICam)
{
	bool isSupportedDevice;
	// get user input if relevant. Else use values from application parameters
	if (cameraId != 255) {
		std::set<unsigned int> validDeviceNumbers = listDevices(devMgr);

		if (validDeviceNumbers.find(cameraId) == validDeviceNumbers.end())
		{
			std::cout << "Camera ID selected by parameter invalid!" << std::endl;
			throw std::invalid_argument("CamIdx");
		}
		else {
			//	std::cout << "Using device [" << devNrPrev << "] for master (left) camera." << std::endl;
			return devMgr[cameraId];
		}
	}
	else {
		const unsigned int devCnt = devMgr.deviceCount();
		if (devCnt == 0)
		{
			std::cout << "No compatible device found!" << std::endl;
			return 0;
		}

		std::set<unsigned int> validDeviceNumbers;

		// display every device detected that matches
		for (unsigned int i = 0; i < devCnt; i++)
		{
			Device* pDev = devMgr[i];
			//verify if device is compatible, i.e. mvBlueFOX3
			isSupportedDevice = (match(pDev->product.read(), string("mvBlueFOX3*"), '*') == 0);
			if (pDev)
			{
				if (isSupportedDevice)
				{
					std::cout << "[" << i << "]: " << pDev->serial.read() << " (" << pDev->product.read() << ", " << pDev->family;
					if (pDev->interfaceLayout.isValid())
					{
						if (UseGenICam)
						{
							// if this device offers the 'GenICam' interface switch it on, as this will
							// allow are better control over GenICam compliant devices
							conditionalSetProperty(pDev->interfaceLayout, dilGenICam, true);
						}
						std::cout << ", interface layout: " << pDev->interfaceLayout.readS();
					}
					if (pDev->acquisitionStartStopBehaviour.isValid())
					{
						// if this device offers a user defined acquisition start/stop behaviour
						// enable it as this allows finer control about the streaming behaviour
						conditionalSetProperty(pDev->acquisitionStartStopBehaviour, assbUser, true);
						std::cout << ", acquisition start/stop behaviour: " << pDev->acquisitionStartStopBehaviour.readS();
					}
					if (pDev->interfaceLayout.isValid() && !pDev->interfaceLayout.isWriteable())
					{
						if (pDev->isInUse())
						{
							std::cout << ", !!!ALREADY IN USE!!!";
						}
					}
					std::cout << ")" << std::endl;
					validDeviceNumbers.insert(i);
				}
			}
		}

		if (validDeviceNumbers.empty())
		{
			std::cout << devMgr.deviceCount() << " devices have been detected:" << std::endl;
			for (unsigned int i = 0; i < devCnt; i++)
			{
				Device* pDev = devMgr[i];
				if (pDev)
				{
					std::cout << "  [" << i << "]: " << pDev->serial.read() << " (" << pDev->product.read() << ", " << pDev->family << ")" << std::endl;
				}
			}
			std::cout << "However none of these devices seems to be supported by this application." << std::endl << std::endl;
			return 0;
		}

		// get user input
		std::cout << std::endl << "Please enter the number in front of the listed device followed by [ENTER] to open it: ";

		unsigned int devNr = 0;
		std::cin >> devNr;
		// remove the '\n' from the stream
		std::cin.get();

		if (validDeviceNumbers.find(devNr) == validDeviceNumbers.end())
		{
			std::cout << "Invalid selection!" << std::endl;
			return 0;
		}

		std::cout << "Using device number " << devNr << "." << std::endl;

		return devMgr[devNr];
	}
}

//-----------------------------------------------------------------------------
mvIMPACT::acquire::Device* getMasterDeviceFromUserInput(const mvIMPACT::acquire::DeviceManager& devMgr, int &masterNr,
	SUPPORTED_DEVICE_CHECK pSupportedDeviceCheckFn = 0, bool boSilent = false,
	bool boAutomaticallyUseGenICamInterface = true)
	//-----------------------------------------------------------------------------
{
	const unsigned int devCnt = devMgr.deviceCount();
	if (devCnt == 0)
	{
		std::cout << "No compatible device found!" << std::endl;
		return 0;
	}

	std::set<unsigned int> validDeviceNumbers;

	// display every device detected that matches
	for (unsigned int i = 0; i < devCnt; i++)
	{
		Device* pDev = devMgr[i];
		if (pDev)
		{
			if (!pSupportedDeviceCheckFn || pSupportedDeviceCheckFn(pDev))
			{
				std::cout << "[" << i << "]: " << pDev->serial.read() << " (" << pDev->product.read() << ", " << pDev->family;
				if (pDev->interfaceLayout.isValid())
				{
					if (boAutomaticallyUseGenICamInterface)
					{
						// if this device offers the 'GenICam' interface switch it on, as this will
						// allow are better control over GenICam compliant devices
						conditionalSetProperty(pDev->interfaceLayout, dilGenICam, true);
					}
					std::cout << ", interface layout: " << pDev->interfaceLayout.readS();
				}
				if (pDev->acquisitionStartStopBehaviour.isValid())
				{
					// if this device offers a user defined acquisition start/stop behaviour
					// enable it as this allows finer control about the streaming behaviour
					conditionalSetProperty(pDev->acquisitionStartStopBehaviour, assbUser, true);
					std::cout << ", acquisition start/stop behaviour: " << pDev->acquisitionStartStopBehaviour.readS();
				}
				if (pDev->interfaceLayout.isValid() && !pDev->interfaceLayout.isWriteable())
				{
					if (pDev->isInUse())
					{
						std::cout << ", !!!ALREADY IN USE!!!";
					}
				}
				std::cout << ")" << std::endl;
				validDeviceNumbers.insert(i);
			}
		}
	}

	if (validDeviceNumbers.empty())
	{
		std::cout << devMgr.deviceCount() << " devices have been detected:" << std::endl;
		for (unsigned int i = 0; i < devCnt; i++)
		{
			Device* pDev = devMgr[i];
			if (pDev)
			{
				std::cout << "  [" << i << "]: " << pDev->serial.read() << " (" << pDev->product.read() << ", " << pDev->family << ")" << std::endl;
			}
		}
		std::cout << "However none of these devices seems to be supported by this sample." << std::endl << std::endl;
		return 0;
	}

	// get user input
	std::cout << std::endl << "Please enter the number in front of the listed device followed by [ENTER] to open it: ";
	unsigned int devNr = 0;
	std::cin >> devNr;
	// remove the '\n' from the stream
	std::cin.get();

	if (validDeviceNumbers.find(devNr) == validDeviceNumbers.end())
	{
		masterNr = -1;
		std::cout << "Invalid selection!" << std::endl;
		return 0;
	}

	if (!boSilent)
	{
		std::cout << "Using device number " << devNr << "." << std::endl;
	}
	masterNr = devNr;
	return devMgr[devNr];
}