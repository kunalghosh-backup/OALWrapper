/*
 * Copyright 2007-2010 (C) - Frictional Games
 *
 * This file is part of OALWrapper
 *
 * For conditions of distribution and use, see copyright notice in LICENSE
 */
/**
	@file OAL_OggSample.cpp
	@author Luis Rodero
	@date 2006-10-02
	@version 0.1
	Derived class for containing Ogg Vorbis Sample data 
*/

#include "OALWrapper/OAL_OggSample.h"
#include "OALWrapper/OAL_Buffer.h"
#include <stdlib.h>
#include <string>
#include <cstring>

#include <vorbis/vorbisfile.h>

//-------------------------------------------------------------------------------

///////////////////////////////////////////////////////////
// PUBLIC METHODS
///////////////////////////////////////////////////////////

//-------------------------------------------------------------------------------

bool cOAL_OggSample::CreateFromFile(const wstring &asFilename)
{
	DEF_FUNC_NAME("cOAL_OggSample::Load()");
	FUNC_USES_AL;

	if(mbStatus==false)
		return false;

	Reset();
	
	char *pPCMBuffer;
	bool bEOF = false;
	int lOpenResult;
	int lCurrent_section;
	long lDataSize = 0;

	msFilename = asFilename;

	FILE *fileHandle = OpenFileW(asFilename, L"rb");
	
	// If no file is present, set the error status and return
	if(!fileHandle)
	{
		mbStatus = false;
		return false;
	}

	// If not an Ogg file, set status and exit
	OggVorbis_File ovFileHandle;
	if((lOpenResult = ov_open_callbacks(fileHandle, &ovFileHandle, NULL, 0, OV_CALLBACKS_DEFAULT))<0)
	{
		fclose ( fileHandle );
		mbStatus = false;
		return false;
	}

	// Get file info
	vorbis_info *viFileInfo = ov_info ( &ovFileHandle, -1 );
	mlChannels = viFileInfo->channels;
	mFormat = (mlChannels == 2)?AL_FORMAT_STEREO16:AL_FORMAT_MONO16;
	mlFrequency = viFileInfo->rate;
	mlSamples = (long) ov_pcm_total ( &ovFileHandle, -1 );
	mfTotalTime = ov_time_total( &ovFileHandle, -1 );

	// Reserve memory for 'mlChannels' channels of 'mlSamples' * 2 bytes of data each
	int lSizeInBytes = mlSamples * mlChannels * GetBytesPerSample();
	pPCMBuffer = (char *) malloc (lSizeInBytes);
	memset (pPCMBuffer, 0, lSizeInBytes);

	// Loop which loads chunks of decoded data into a buffer
	while(!bEOF)
	{
		long lChunkSize = ov_read ( &ovFileHandle			,										
									pPCMBuffer + lDataSize	, 
									STREAMING_BLOCK_SIZE	, 
									SYS_ENDIANNESS			,
									2, 1, &lCurrent_section );

		if(lChunkSize == 0)
			bEOF = true;
		// If we get a negative value, then something went wrong. Clean up and set error status.
		else if(lChunkSize < 0)
		{
			free(pPCMBuffer);
			ov_clear(&ovFileHandle);
			// ov_clear closes the file handle for us
			mbStatus = false;
			return false;
		}
		else 
			lDataSize += lChunkSize;
	}

	cOAL_Buffer* pBuffer = mvBuffers[0];
	if(lDataSize)
	{
		// If something went wrong, set error status. Clean up afterwards.
		mbStatus = pBuffer->Feed((ALvoid*)pPCMBuffer, lDataSize);
	}
	free(pPCMBuffer);
	ov_clear(&ovFileHandle);
	// ov_clear closes the file handle for us

	return true;
}

