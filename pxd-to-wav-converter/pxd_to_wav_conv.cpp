#include <windows.h>
#include <stdio.h>
#include <sys/stat.h>

/*	
--------------------------------
		PXD-TO-WAV Converter
--------------------------------
	Authors:

		@DJ_VK		(http://goo.gl/qlloL0) (pxd32d5_d4.dll, rev-engineering)
		@gruena		(http://www.buha.info/member.php?26299-gruena) (rev-engineering)
		@HobbyCoder (http://www.buha.info/member.php?61860-HobbyCoder) (rev-engineering, basic code)
		@AttarSoftware (http://rdsrc.us/S1Bnwu) (Raw2Wav)
		@Burnett01	(https://github.com/Burnett01/) (removed unecessary stuff, combined raw2wav/converter, getExt fn)

	Based on:
					(http://reversing.be/forum/viewtopic.php?p=3463&sid=55b23b02f5fc369f2f28c5b640bcead0)
					(http://www.buha.info/showthread.php?56934-eJay-Dll-Reversing/)
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

typedef int(__cdecl *PINIT_FUNC)();
typedef int(__cdecl *RWAVTOTEMP_FUNC)(char*, char*, int, int, int, int, int);
typedef int(__cdecl *PCLOSE_FUNC)();

PINIT_FUNC PInit;
RWAVTOTEMP_FUNC RWavToTemp;
PCLOSE_FUNC PClose;

bool readDirectoryAndConvert(const char *sDir);
const char *getExt( const char *name );
int convertSingleFile( char *PXD );

const int nCurdirlen = 4096;
char sCurdir[nCurdirlen];


int main()
{
	GetCurrentDirectory( nCurdirlen, sCurdir );
	printf( "CURDIR: %s\n", sCurdir );
	
	HMODULE hinstLib = LoadLibrary( "pxd32d5_d4.dll" );
	if( hinstLib == NULL )
	{
		printf( "Library couldn't be linked!\n" );
		Sleep( 1000 );
		goto doExit;
	}

	PInit = (PINIT_FUNC)GetProcAddress(hinstLib, "PInit");
	RWavToTemp = (RWAVTOTEMP_FUNC)GetProcAddress(hinstLib, "RWavToTemp");
	PClose = (PCLOSE_FUNC)GetProcAddress(hinstLib, "PClose");

	PInit();

	readDirectoryAndConvert(".");
	
	PClose();

	FreeLibrary( hinstLib );


	doExit:
		exit( 0 );
}

// Credits: https://stackoverflow.com/a/2315808/1380486
bool readDirectoryAndConvert(const char *sDir)
{
    WIN32_FIND_DATA fdFile;
    HANDLE hFind = NULL;

    char sPath[2048];
    sprintf(sPath, "%s\\*.*", sDir);

    if((hFind = FindFirstFile(sPath, &fdFile)) == INVALID_HANDLE_VALUE)
    {
        printf("Path not found: [%s]\n", sDir);
        return false;
    }

    do
    {
        if(strcmp( fdFile.cFileName, "." ) != 0
                && strcmp( fdFile.cFileName, ".." ) != 0)
        {

            sprintf( sPath, "%s\\%s", sDir, fdFile.cFileName );

            if( fdFile.dwFileAttributes &FILE_ATTRIBUTE_DIRECTORY )
            {
                readDirectoryAndConvert( sPath );
            }
            else{

				if( strcmp( getExt(sPath), "PXD" ) == 0 )
				{
					printf("File: %s\n", sPath);
					convertSingleFile( sPath );
					// Sleep( 9000 );
				}
            }
        }
    }
    while( FindNextFile( hFind, &fdFile ) );

    FindClose( hFind );

    return true;
}

int convertSingleFile( char *pathPXD )
{
	if( pathPXD[0] == '.' && pathPXD[1] == '\\' )
		pathPXD += 2;

	int pxdLen = strlen( pathPXD );
	int extLen = pxdLen + 4; // 4 = [.xyz]
	int finLen = nCurdirlen + extLen;
	
	char *PXD = (char *)malloc( (finLen + 1) * sizeof( char ) );  // 1 = \0
	if( !PXD ) return 0;

	sprintf( PXD, "%s\\%s", sCurdir, pathPXD );
	PXD[finLen] = '\0';

	char *TMP = (char *)malloc( (finLen + 1) * sizeof( char ) );  // 1 = \0
	if( !TMP ) return 0;

	sprintf( TMP, "%s\\%s.tmp", sCurdir, pathPXD );
	TMP[finLen] = '\0';

	char *WAV = (char *)malloc( (finLen + 1) * sizeof( char ) );  // 1 = \0
	if( !WAV ) return 0;

	sprintf( WAV, "%s\\%s.wav", sCurdir, pathPXD );
	WAV[finLen] = '\0';

	printf("PXD IS: %s\n", PXD);
	printf("TMP IS: %s\n", TMP);
	printf("WAV IS: %s\n", WAV);

	printf( "Creating & Converting TMP-File (RAW) to (WAV)...\n" );

	DWORD conv = 3;
	memcpy((void*)0x10096cd8, &conv, 4);
	RWavToTemp(PXD, TMP, 0, NULL, 0, 0, 0);
	Raw2Wav( TMP, WAV, 44100 );

	Sleep( 2000 );
	printf( "WAV-File was successfully created.\n" );

	if( PXD ) free( PXD );
	if( TMP ) free( TMP );
	if( WAV ) free( WAV );

	return 1;
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

const char *getExt( const char *name )
{
  const char *end = strrchr( name, '.' );

  if( !end || end == name )
  {
    return "";
  }

  return end + 1;
}
