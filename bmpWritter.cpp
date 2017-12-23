#include "stdafx.h"
#include "bmpWritter.h"
#include <stdio.h>
#include <stdlib.h>
#pragma warning(disable:4996)

static BMPheader header;

void writeBMP(const ubyte* rawData, int data_width, int data_height, pixel_format fmt, const char *filename)
{
	int pixel_data_size, i, j;
	ubyte *bmpData = NULL;
	FILE *file = fopen(filename, "wb");

	if (NULL == file) {
		return;
	}

	/*assume the rawData are always in format ARGB32*/
	if (fmt == RGB24)
	{
		int stride = ALIGN((data_width * 3), 4);
		pixel_data_size = stride*data_height;
		header.identifier = SWAP16(0x4d42);/*Identifier*/
		header.file_size = SWAP32(0x36 + pixel_data_size);
		header.reserved = 0x0;
		header.data_offset = SWAP32(0x36);
		header.header_size = SWAP32(0x28);
		header.width = SWAP32(data_width);
		header.height = SWAP32(data_height);
		header.planes = 0;
		header.bpp = SWAP16(24);
		header.compression = C_NONE;
		header.data_size = SWAP32(pixel_data_size);
		header.hresolution = 0;
		header.vresolution = 0;
		header.colors = 0;
		header.important_color = 0;
		bmpData = (ubyte*)MALLOC(pixel_data_size);
		/*copy data*/
		for (i = 0; i<data_height; i++) {
			for (j = 0; j<data_width; j++) {
				bmpData[i*stride + j * 4 + RED_HST] = rawData[i*data_width + j + RED_TGT];
				bmpData[i*stride + j * 4 + GREEN_HST] = rawData[i*data_width + j + GREEN_TGT];
				bmpData[i*stride + j * 4 + BLUE_HST] = rawData[i*data_width + j + BLUE_TGT];
			}
		}
	}
	else if (fmt == RGBA32)
	{
		int stride = data_width * 4;
		pixel_data_size = stride*data_height;
		header.identifier = SWAP16(0x4d42);/*BM*/
		header.file_size = SWAP32(0x36 + pixel_data_size);
		header.reserved = 0x0;
		header.data_offset = SWAP32(0x36);
		header.header_size = SWAP32(0x28);
		header.width = SWAP32(data_width);
		header.height = SWAP32(data_height);
		header.planes = 0;
		header.bpp = SWAP16(32);
		header.compression = C_NONE;
		header.data_size = SWAP32(pixel_data_size);
		header.hresolution = 0;
		header.vresolution = 0;
		header.colors = 0;
		header.important_color = 0;
		bmpData = (ubyte*)MALLOC(pixel_data_size);
		/*copy data*/
		for (i = 0; i<data_height; i++) {
			for (j = 0; j<data_width; j++) {
				bmpData[i*stride + j * 4 + RED_HST] = rawData[i*data_width * 4 + j * 4 + RED_TGT];
				bmpData[i*stride + j * 4 + GREEN_HST] = rawData[i*data_width * 4 + j * 4 + GREEN_TGT];
				bmpData[i*stride + j * 4 + BLUE_HST] = rawData[i*data_width * 4 + j * 4 + BLUE_TGT];
				bmpData[i*stride + j * 4 + ALPHA_HST] = rawData[i*data_width * 4 + j * 4 + ALPHA_TGT];
			}
		}
	}

	/*write bmp*/
	fwrite(&header, 1, sizeof(header), file);
	fwrite(bmpData, 1, pixel_data_size, file);
	fclose(file);

	FREE(bmpData);
}