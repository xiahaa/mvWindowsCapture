#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "cJSON.h"
#include "cJSON_common.h"
#include "mvCaptureConfigParse.h"

bool mvCaptureConfigParse(char *configFile, mvCaptureParams_t &capParams)
{
	char *content = read_file(configFile);
	cJSON* root = cJSON_Parse(content);

#define FIND_AND_CONFIGURE_DIGITAL_ITEM(name)	\
do{ \
	cJSON *subitem = cJSON_GetObjectItemCaseSensitive(root, #name); \
	if (subitem != NULL) \
	{ \
		cJSON *description = cJSON_GetObjectItemCaseSensitive(subitem, "description"); \
		cJSON *type = cJSON_GetObjectItemCaseSensitive(subitem, "type"); \
		cJSON *value = cJSON_GetObjectItemCaseSensitive(subitem, "value"); \
		if (strcmp(type->valuestring, "int") == 0) \
		{ \
			capParams.name = value->valueint; \
		} \
		else if (strcmp(type->valuestring, "double") == 0) \
		{ \
			capParams.name = static_cast<decltype(capParams.name)>(value->valuedouble); \
		} \
	} \
	else \
	{ \
		printf("No this config in mvCapture.json!!!!!\n"); \
	} \
}while(0)

#define FIND_AND_CONFIGURE_STRING_ITEM(name)	\
do{ \
	cJSON *subitem = cJSON_GetObjectItemCaseSensitive(root, #name); \
	if (subitem != NULL) \
	{ \
		cJSON *description = cJSON_GetObjectItemCaseSensitive(subitem, "description"); \
		cJSON *type = cJSON_GetObjectItemCaseSensitive(subitem, "type"); \
		cJSON *value = cJSON_GetObjectItemCaseSensitive(subitem, "value"); \
		if (strcmp(type->valuestring, "string") == 0) \
		{ \
			std::string str(value->valuestring); \
			capParams.name = str; \
		} \
	} \
	else \
	{ \
		printf("No this config %s in mvCapture.json!!!!!\n", #name); \
	} \
}while(0)

	FIND_AND_CONFIGURE_DIGITAL_ITEM(captureMode);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(masterId);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(slaveId);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(masterTrigOut);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(masterTrigIn);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(slaveTrigIn);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(triggerFrequency_Hz);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(nImagePairs);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(expTime_us);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(expTimeMax_us);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(logInterval);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(displayInterval);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(pngCompression);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(decimFactor);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(imageThreads);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(binningFactor);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(samplingMode);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(autoSave);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(bmpEnabled);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(displayStatsEnabled);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(writeLogEnabled);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(extSyncEnabled);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(autoExposureEnabled);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(listDevices);
	FIND_AND_CONFIGURE_DIGITAL_ITEM(isCaptureContinuous);

	/* for string item */
	FIND_AND_CONFIGURE_STRING_ITEM(gnssPortname);
	FIND_AND_CONFIGURE_STRING_ITEM(outdir);

	return true;
}