#define _CRT_SECURE_NO_DEPRECATE

#include "pxd_to_wav.h"

/*	
--------------------------------
		PXD-TO-WAV Converter
--------------------------------
	Authors:

		@DJ_VK		(http://goo.gl/qlloL0) (pxd32d5_d4.dll, rev-engineering)
		@gruena		(http://www.buha.info/member.php?26299-gruena) (rev-engineering)
		@HobbyCoder (http://www.buha.info/member.php?61860-HobbyCoder) (rev-engineering, basic code)
		@Burnett01	(https://github.com/Burnett01/) (removed unecessary stuff, combined raw2wav/converter)

	Based on:
					(http://reversing.be/forum/viewtopic.php?p=3463&sid=55b23b02f5fc369f2f28c5b640bcead0)
					(http://www.buha.info/showthread.php?56934-eJay-Dll-Reversing/)
*/

typedef int(__cdecl *PINIT_FUNC)();
typedef int(__cdecl *RWAVTOTEMP_FUNC)(char*, char*, int, int, int, int, int);
typedef int(__cdecl *PCLOSE_FUNC)();


int main()
{
	char *PXD = "C:\\PATH_TO_PXD.pxd";
	char *TMP = "C:\\PATH_TO_TMP.tmp";
	char *WAV = "C:\\PATH_TO_WAV.wav";

	PINIT_FUNC PInit;
	RWAVTOTEMP_FUNC RWavToTemp;
	PCLOSE_FUNC PClose;
	
	HMODULE hinstLib = LoadLibrary("pxd32d5_d4.dll");
	if (hinstLib != NULL)
	{
		PInit = (PINIT_FUNC)GetProcAddress(hinstLib, "PInit");
		RWavToTemp = (RWAVTOTEMP_FUNC)GetProcAddress(hinstLib, "RWavToTemp");
		PClose = (PCLOSE_FUNC)GetProcAddress(hinstLib, "PClose");

		PInit();

		printf("Creating TMP-File (RAW)...\n");
		DWORD conv = 3;
		memcpy((void*)0x10096cd8, &conv, 4);
		RWavToTemp(PXD, TMP, 0, NULL, 0, 0, 0);
		
		printf("Converting TMP-File (RAW) to (WAV)...\n");
		Raw2Wav(TMP, WAV, 44100);
		printf("WAV-File was successfully created.\n");

		PClose();
		FreeLibrary(hinstLib);

		return 1;

	} else {

		printf("Library couldn't be linked!\n");
		Sleep(1000);
	}
}


int Raw2Wav(const char *rawfn, const char *wavfn, long frequency)
{
	struct _stat sb;
	if (!_stat(rawfn, &sb))            // OK
	{
		long samplecount = sb.st_size / 2;
		long riffsize = samplecount * 2 + 0x24;
		long datasize = samplecount * 2;
		FILE *raw = fopen(rawfn, "rb");
		if (!raw) return -2;
		FILE *wav = fopen(wavfn, "wb");
		if (!wav)
		{
			fclose(raw); return -3;
		}
		fwrite("RIFF", 1, 4, wav);
		fwrite(&riffsize, 4, 1, wav);
		fwrite("WAVEfmt ", 1, 8, wav);
		fwrite(&chunksize, 4, 1, wav);
		fmt.wFormatTag = 1;      // PCM 
		fmt.wChannels = 1;      // MONO
		fmt.dwSamplesPerSec = frequency * 1;
		fmt.dwAvgBytesPerSec = frequency * 1 * 2;      // PCM 16 bits
		fmt.wBlockAlign = 2;
		fmt.wBitsPerSample = 16;
		fwrite(&fmt, sizeof(fmt), 1, wav);
		fwrite("data", 1, 4, wav);
		fwrite(&datasize, 4, 1, wav);
		short buff[1024];
		while (!feof(raw))
		{
			int cnt = fread(buff, 2, 1024, raw);
			if (cnt == 0) break;
			fwrite(buff, 2, cnt, wav);
		}
		fclose(raw);
		fclose(wav);
	}
	else      // File not found?
	{
		return -1;
	}
	return 0;
}