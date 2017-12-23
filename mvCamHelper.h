//-----------------------------------------------------------------------------
//mvCamHelper.h
//	Author: Johannes Linde s113579
//	Based on getDeviceFromUserInput in exampleHelper.h
//	<apps/Common/exampleHelper.h>
//-----------------------------------------------------------------------------
#ifndef mvCamHelperH
#define mvCamHelperH

#if _WIN32
#include <mvIMPACT_CPP/mvIMPACT_acquire.h>
#endif

//TODO: List which functions that are from matrix vision and which functions that arent.
typedef bool(*SUPPORTED_DEVICE_CHECK)(const mvIMPACT::acquire::Device* const);

unsigned int getNumberFromUser(void);

bool isDeviceSupportedBySample(const Device* const pDev);

unsigned int listAllDevices(const mvIMPACT::acquire::DeviceManager& devMgr, int isDump);

std::set<unsigned int> listDevices(const mvIMPACT::acquire::DeviceManager& devMgr, bool UseGenICam = true);//, bool silent = true);

mvIMPACT::acquire::Device* getDeviceFromUserInputId(unsigned int cameraId, const mvIMPACT::acquire::DeviceManager& devMgr, bool UseGenICam = true);

mvIMPACT::acquire::Device* getMasterDeviceFromUserInput(const mvIMPACT::acquire::DeviceManager& devMgr, int &masterNr,
	SUPPORTED_DEVICE_CHECK pSupportedDeviceCheckFn, bool boSilent, bool boAutomaticallyUseGenICamInterface);

#endif // mvCamHelperH
