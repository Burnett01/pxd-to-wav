#pragma once

#include <stdio.h>
#include <windows.h>
#include <sys/stat.h>

/*
	Raw2Wav  
	~Credits @AttarSoftware:  http://rdsrc.us/S1Bnwu
*/

long chunksize = 0x10;

struct
{
	WORD    wFormatTag;
	WORD    wChannels;
	DWORD   dwSamplesPerSec;
	DWORD   dwAvgBytesPerSec;
	WORD    wBlockAlign;
	WORD    wBitsPerSample;
} fmt;

int Raw2Wav(const char *rawfn, const char *wavfn, long frequency);