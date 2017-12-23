#ifndef _CJSON_COMMON_H_
#define _CJSON_COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include "cJSON.h"

inline CJSON_PUBLIC(char*) read_file(const char *filename)
{
	FILE *file = NULL;
	long length = 0;
	char *content = NULL;
	size_t read_chars = 0;

	/* open in read binary mode */
	fopen_s(&file, filename, "rb");
	if (file == NULL)
	{
		goto cleanup;
	}

	/* get the length */
	if (fseek(file, 0, SEEK_END) != 0)
	{
		goto cleanup;
	}
	length = ftell(file);
	if (length < 0)
	{
		goto cleanup;
	}
	if (fseek(file, 0, SEEK_SET) != 0)
	{
		goto cleanup;
	}

	/* allocate content buffer */
	content = (char*)malloc((size_t)length + sizeof(""));
	if (content == NULL)
	{
		goto cleanup;
	}

	/* read the file into memory */
	read_chars = fread(content, sizeof(char), (size_t)length, file);
	if ((long)read_chars != length)
	{
		free(content);
		content = NULL;
		goto cleanup;
	}
	content[read_chars] = '\0';


cleanup:
	if (file != NULL)
	{
		fclose(file);
	}

	return content;
}

#endif