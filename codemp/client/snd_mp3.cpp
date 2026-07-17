/*
===========================================================================
Copyright (C) 1999 - 2005, Id Software, Inc.
Copyright (C) 2000 - 2013, Raven Software, Inc.
Copyright (C) 2001 - 2013, Activision, Inc.
Copyright (C) 2013 - 2015, SerenityJediEngine2026 contributors

This file is part of the SerenityJediEngine2026 source code.

SerenityJediEngine2026 is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, see <http://www.gnu.org/licenses/>.
===========================================================================
*/

// Filename:-	cl_mp3.cpp
//
// (The interface module between all the MP3 stuff and the game)

#include "client.h"
#include "snd_mp3.h"					// only included directly by a few snd_xxxx.cpp files plus this one
#include "mp3code/mp3struct.h"	// keep this rather awful file secret from the rest of the program

// expects data already loaded, filename arg is for error printing only
//
// returns success/fail
//
qboolean MP3_IsValid(const char* psLocalFilename, void* pvData, const int iDataLen,
	const qboolean bStereoDesired /* = qfalse */)
{
	char* psError = C_MP3_IsValid(pvData, iDataLen, bStereoDesired);

	if (psError)
	{
		Com_Printf(va(S_COLOR_RED"%s(%s)\n", psError, psLocalFilename));
	}

	return static_cast<qboolean>(!psError);
}

// expects data already loaded, filename arg is for error printing only
//
// returns unpacked length, or 0 for errors (which will be printed internally)
//
int MP3_GetUnpackedSize(const char* psLocalFilename, void* pvData, const int iDataLen,
	qboolean qbIgnoreID3Tag /* = qfalse */
	, const qboolean bStereoDesired /* = qfalse */
)
{
	int iUnpackedSize = 0;

	// always do this now that we have fast-unpack code for measuring output size... (much safer than relying on tags that may have been edited, or if MP3 has been re-saved with same tag)
	//
	if (true) //qbIgnoreID3Tag || !MP3_ReadSpecialTagInfo((byte *)pvData, iDataLen, NULL, &iUnpackedSize))
	{
		char* psError = C_MP3_GetUnpackedSize(pvData, iDataLen, &iUnpackedSize, bStereoDesired);

		if (psError)
		{
			Com_Printf(va(S_COLOR_RED"%s\n(File: %s)\n", psError, psLocalFilename));
			return 0;
		}
	}

	return iUnpackedSize;
}

// expects data already loaded, filename arg is for error printing only
//
// returns byte count of unpacked data (effectively a success/fail bool)
//
int MP3_UnpackRawPCM(const char* psLocalFilename, void* pvData, const int iDataLen, byte* pbUnpackBuffer,
	const qboolean bStereoDesired /* = qfalse */)
{
	int iUnpackedSize;
	char* psError = C_MP3_UnpackRawPCM(pvData, iDataLen, &iUnpackedSize, pbUnpackBuffer, bStereoDesired);

	if (psError)
	{
		Com_Printf(va(S_COLOR_RED"%s\n(File: %s)\n", psError, psLocalFilename));
		return 0;
	}

	return iUnpackedSize;
}

// psLocalFilename is just for error reporting (if any)...
//
qboolean MP3Stream_InitPlayingTimeFields(const LP_MP3STREAM lpMP3Stream, const char* psLocalFilename, void* pvData,
	const int iDataLen, const qboolean bStereoDesired /* = qfalse */)
{
	qboolean bRetval = qfalse;

	int iRate, iWidth, iChannels;

	char* psError = C_MP3_GetHeaderData(pvData, iDataLen, &iRate, &iWidth, &iChannels, bStereoDesired);
	if (psError)
	{
		Com_Printf(va(S_COLOR_RED"MP3Stream_InitPlayingTimeFields(): %s\n(File: %s)\n", psError, psLocalFilename));
	}
	else
	{
		const int iUnpackLength = MP3_GetUnpackedSize(psLocalFilename, pvData, iDataLen, qfalse,
			// qboolean qbIgnoreID3Tag
			bStereoDesired);
		if (iUnpackLength)
		{
			lpMP3Stream->iTimeQuery_UnpackedLength = iUnpackLength;
			lpMP3Stream->iTimeQuery_SampleRate = iRate;
			lpMP3Stream->iTimeQuery_Channels = iChannels;
			lpMP3Stream->iTimeQuery_Width = iWidth;

			bRetval = qtrue;
		}
	}

	return bRetval;
}

float MP3Stream_GetPlayingTimeInSeconds(const LP_MP3STREAM lpMP3Stream)
{
	if (lpMP3Stream->iTimeQuery_UnpackedLength) // fields initialised?
		return static_cast<float>(static_cast<double>(lpMP3Stream->iTimeQuery_UnpackedLength) / static_cast<double>(
			lpMP3Stream->iTimeQuery_SampleRate) / static_cast<double>(lpMP3Stream->iTimeQuery_Channels) / static_cast<
			double>(lpMP3Stream->iTimeQuery_Width));

	return 0.0f;
}

float MP3Stream_GetRemainingTimeInSeconds(const LP_MP3STREAM lpMP3Stream)
{
	if (lpMP3Stream->iTimeQuery_UnpackedLength) // fields initialised?
		return static_cast<float>(static_cast<double>(lpMP3Stream->iTimeQuery_UnpackedLength - lpMP3Stream->
			iBytesDecodedTotal * (lpMP3Stream->
				iTimeQuery_SampleRate / dma.speed)) / static_cast<double>(lpMP3Stream->iTimeQuery_SampleRate) /
			static_cast<double>(lpMP3Stream->
				iTimeQuery_Channels) / static_cast<double>(lpMP3Stream->iTimeQuery_Width));

	return 0.0f;
}

// expects data already loaded, filename arg is for error printing only
//
qboolean MP3_FakeUpWAVInfo(const char* psLocalFilename, void* pvData, const int iDataLen, const int iUnpackedDataLength,
	int& format, int& rate, int& width, int& channels, int& samples, int& dataofs,
	const qboolean bStereoDesired /* = qfalse */
)
{
	// some things can be done instantly...
	//
	format = 1; // 1 for MS format
	dataofs = 0; // will be 0 for me (since there's no header in the unpacked data)

	// some things need to be read...  (though the whole stereo flag thing is crap)
	//
	char* psError = C_MP3_GetHeaderData(pvData, iDataLen, &rate, &width, &channels, bStereoDesired);
	if (psError)
	{
		Com_Printf(va(S_COLOR_RED"%s\n(File: %s)\n", psError, psLocalFilename));
	}

	// and some stuff needs calculating...
	//
	samples = iUnpackedDataLength / width;

	return static_cast<qboolean>(!psError);
}

const char sKEY_MAXVOL[] = "#MAXVOL"; // formerly #defines
const char sKEY_UNCOMP[] = "#UNCOMP"; //    "        "

// returns qtrue for success...
//
qboolean MP3_ReadSpecialTagInfo(byte* pbLoadedFile, const int iLoadedFileLen,
	id3v1_1** ppTAG /* = NULL */,
	int* piUncompressedSize /* = NULL */,
	float* pfMaxVol /* = NULL */
)
{
	qboolean qbError = qfalse;

	auto pTAG = reinterpret_cast<id3v1_1*>(pbLoadedFile + iLoadedFileLen - sizeof(id3v1_1)); // sizeof = 128

	if (!Q_strncmp(pTAG->id, "TAG", 3))
	{
		// TAG found...
		//

		// read MAXVOL key...
		//
		if (Q_strncmp(pTAG->comment, sKEY_MAXVOL, strlen(sKEY_MAXVOL)))
		{
			qbError = qtrue;
		}
		else
		{
			if (pfMaxVol)
			{
				*pfMaxVol = atof(pTAG->comment + strlen(sKEY_MAXVOL));
			}
		}

		//
		// read UNCOMP key...
		//
		if (Q_strncmp(pTAG->album, sKEY_UNCOMP, strlen(sKEY_UNCOMP)))
		{
			qbError = qtrue;
		}
		else
		{
			if (piUncompressedSize)
			{
				*piUncompressedSize = atoi(pTAG->album + strlen(sKEY_UNCOMP));
			}
		}
	}
	else
	{
		pTAG = nullptr;
	}

	if (ppTAG)
	{
		*ppTAG = pTAG;
	}

	return static_cast<qboolean>(pTAG && !qbError);
}

constexpr auto FUZZY_AMOUNT = 5 * 1024; // so it has to be significantly over, not just break even, because of;
// the xtra CPU time versus memory saving

cvar_t* cv_MP3overhead = nullptr;

void MP3_InitCvars(void)
{
	cv_MP3overhead = Cvar_Get("s_mp3overhead", va("%d", sizeof(MP3STREAM) + FUZZY_AMOUNT), CVAR_ARCHIVE);
}

// a file has been loaded in memory, see if we want to keep it as MP3, else as normal WAV...
//
// return = qtrue if keeping as MP3
//
// (note: the reason I pass in the unpacked size rather than working it out here is simply because I already have it)
//
// ============================================================================
// MP3Stream_InitFromFile
// Initializes MP3 stream data for an sfx_t entry. Modernized to include
// safety checks, debug prints, explicit qboolean usage, and zero warnings.
// ============================================================================
qboolean MP3Stream_InitFromFile(
	sfx_t* sfx,
	byte* pbSrcData,
	const int iSrcDatalen,
	const char* psSrcDataFilename,
	const int iMP3UnPackedSize,
	const qboolean bStereoDesired)
{
	// ----------------------------------------------------------------------
	// Safety: validate pointers (fixes MSVC C6011)
	// ----------------------------------------------------------------------
	if (sfx == nullptr)
	{
		Com_Printf(S_COLOR_RED "MP3Stream_InitFromFile ERROR: sfx == NULL\n");
		return qfalse;
	}

	if (pbSrcData == nullptr)
	{
		Com_Printf(S_COLOR_RED "MP3Stream_InitFromFile ERROR: pbSrcData == NULL\n");
		return qfalse;
	}

	// ----------------------------------------------------------------------
	// Decide whether MP3 is worth using based on overhead
	// ----------------------------------------------------------------------
	if (cv_MP3overhead != nullptr &&
		(iSrcDatalen + cv_MP3overhead->integer) < iMP3UnPackedSize)
	{
		float fMaxVol = 128.0f; // default max volume for lip sync

		// Try reading MP3 header tag info
		MP3_ReadSpecialTagInfo(pbSrcData, iSrcDatalen, nullptr, nullptr, &fMaxVol);

		// Fill sfx fields
		sfx->eSoundCompressionMethod = ct_MP3;
		sfx->fVolRange = fMaxVol;

		// Compute sample length
		const int stereoDivisor = (bStereoDesired == qtrue) ? 2 : 1;
		sfx->iSoundLengthInSamples =
			(iMP3UnPackedSize / 2) / (44100 / dma.speed) / stereoDivisor;

		// Allocate raw MP3 data
		sfx->pSoundData = reinterpret_cast<short*>(SND_malloc(iSrcDatalen, sfx));
		if (sfx->pSoundData == nullptr)
		{
			Com_Printf(S_COLOR_RED "MP3Stream_InitFromFile ERROR: SND_malloc failed\n");
			return qfalse;
		}

		memcpy(sfx->pSoundData, pbSrcData, iSrcDatalen);

		// ------------------------------------------------------------------
		// Initialize low-level MP3 stream
		// ------------------------------------------------------------------
		MP3STREAM SFX_MP3Stream = {}; // zero-init
		char* psError = C_MP3Stream_DecodeInit(
			&SFX_MP3Stream,
			pbSrcData,
			iSrcDatalen,
			dma.speed,
			2 * 8,               // width * 8 bits
			bStereoDesired);

		SFX_MP3Stream.pbSourceData = reinterpret_cast<byte*>(sfx->pSoundData);

		if (psError != nullptr)
		{
			Com_Printf(va(S_COLOR_YELLOW "File \"%s\": %s\n", psSrcDataFilename, psError));
			return qfalse;
		}

		// ------------------------------------------------------------------
		// Success: copy stream header into sfx
		// ------------------------------------------------------------------
		sfx->pMP3StreamHeader =
			static_cast<MP3STREAM*>(Z_Malloc(sizeof(MP3STREAM), TAG_SND_MP3STREAMHDR, qfalse));

		if (sfx->pMP3StreamHeader == nullptr)
		{
			Com_Printf(S_COLOR_RED "MP3Stream_InitFromFile ERROR: Z_Malloc failed\n");
			return qfalse;
		}

		memcpy(sfx->pMP3StreamHeader, &SFX_MP3Stream, sizeof(MP3STREAM));

		return qtrue;
	}

	// Not worth using MP3 → return qfalse
	return qfalse;
}

// decode one packet of MP3 data only (typical output size is 2304, or 2304*2 for stereo, so input size is less
//
// return is decoded byte count, else 0 for finished
//
int MP3Stream_Decode(const LP_MP3STREAM lpMP3Stream, qboolean bDoingMusic)
{
	lpMP3Stream->iCopyOffset = 0;

	{
		// SOF2 music, or EF1 anything...
		//
		return C_MP3Stream_Decode(lpMP3Stream, qfalse); // bFastForwarding
	}
}

qboolean MP3Stream_SeekTo(channel_t* ch, float fTimeToSeekTo)
{
	MP3Stream_Rewind(ch);
	//
	// sanity... :-)
	//
	const float fTrackLengthInSeconds = MP3Stream_GetPlayingTimeInSeconds(&ch->MP3StreamHeader);
	if (fTimeToSeekTo > fTrackLengthInSeconds)
	{
		fTimeToSeekTo = fTrackLengthInSeconds;
	}

	// now do the seek...
	//
	while (true)
	{
		constexpr float fEpsilon = 0.05f;
		const float fPlayingTimeElapsed = MP3Stream_GetPlayingTimeInSeconds(&ch->MP3StreamHeader) -
			MP3Stream_GetRemainingTimeInSeconds(&ch->MP3StreamHeader);
		const float fAbsTimeDiff = fabs(fTimeToSeekTo - fPlayingTimeElapsed);

		if (fAbsTimeDiff <= fEpsilon)
			return qtrue;

		// when decoding, use fast-forward until within 3 seconds, then slow-decode (which should init stuff properly?)...
		//
		const int iBytesDecodedThisPacket = C_MP3Stream_Decode(&ch->MP3StreamHeader, fAbsTimeDiff > 3.0f);
		// bFastForwarding
		if (iBytesDecodedThisPacket == 0)
			break; // EOS
	}

	return qfalse;
}

// returns qtrue for all ok
//
qboolean MP3Stream_Rewind(channel_t* ch)
{
	ch->iMP3SlidingDecodeWritePos = 0;
	ch->iMP3SlidingDecodeWindowPos = 0;

	/*
		char *psError = C_MP3Stream_Rewind( &ch->MP3StreamHeader );

		if (psError)
		{
			Com_Printf(S_COLOR_YELLOW"%s\n",psError);
			return qfalse;
		}

		return qtrue;
	*/

	// speed opt, since I know I already have the right data setup here...
	//
	memcpy(&ch->MP3StreamHeader, ch->thesfx->pMP3StreamHeader, sizeof ch->MP3StreamHeader);
	return qtrue;
}

// returns qtrue while still playing normally, else qfalse for either finished or request-offset-error
//
qboolean MP3Stream_GetSamples(channel_t* ch, int startingSampleNum, int count, short* buf, const qboolean bStereo)
{
	qboolean qbStreamStillGoing = qtrue;

	constexpr int iQuarterOfSlidingBuffer = sizeof ch->MP3SlidingDecodeBuffer / 4;
	constexpr int iThreeQuartersOfSlidingBuffer = sizeof ch->MP3SlidingDecodeBuffer * 3 / 4;

	//	Com_Printf("startingSampleNum %d\n",startingSampleNum);

	count *= 2/* <- = SOF2; ch->sfx->width*/; // count arg was for words, so double it for bytes;

	// convert sample number into a byte offset... (make new variable for clarity?)
	//
	startingSampleNum *= 2 /* <- = SOF2; ch->sfx->width*/ * (bStereo ? 2 : 1);

	if (startingSampleNum < ch->iMP3SlidingDecodeWindowPos)
	{
		// what?!?!?!   smegging time travel needed or something?, forget it
		memset(buf, 0, count);
		return qfalse;
	}

	//	Com_OPrintf("\nRequest: startingSampleNum %d, count %d\n",startingSampleNum,count);
	//	Com_OPrintf("WindowPos %d, WindowWritePos %d\n",ch->iMP3SlidingDecodeWindowPos,ch->iMP3SlidingDecodeWritePos);

	//	qboolean _bDecoded = qfalse;

	while (!
		(
			startingSampleNum >= ch->iMP3SlidingDecodeWindowPos
			&&
			startingSampleNum + count < ch->iMP3SlidingDecodeWindowPos + ch->iMP3SlidingDecodeWritePos
			)
		)
	{
		//		if (!_bDecoded)
		//		{
		//			Com_Printf(S_COLOR_YELLOW"Decode needed!\n");
		//		}
		//		_bDecoded = qtrue;
		//		Com_OPrintf("Scrolling...");

		const int _iBytesDecoded = MP3Stream_Decode(&ch->MP3StreamHeader, bStereo);
		// stereo only for music, so this is safe
		//		Com_OPrintf("%d bytes decoded\n",_iBytesDecoded);
		if (_iBytesDecoded == 0)
		{
			// no more source data left so clear the remainder of the buffer...
			//
			memset(ch->MP3SlidingDecodeBuffer + ch->iMP3SlidingDecodeWritePos, 0,
				sizeof ch->MP3SlidingDecodeBuffer - ch->iMP3SlidingDecodeWritePos);
			//			Com_OPrintf("Finished\n");
			qbStreamStillGoing = qfalse;
			break;
		}
		memcpy(ch->MP3SlidingDecodeBuffer + ch->iMP3SlidingDecodeWritePos, ch->MP3StreamHeader.bDecodeBuffer,
			_iBytesDecoded);

		ch->iMP3SlidingDecodeWritePos += _iBytesDecoded;

		// if reached 3/4 of buffer pos, backscroll the decode window by one quarter...
		//
		if (ch->iMP3SlidingDecodeWritePos > iThreeQuartersOfSlidingBuffer)
		{
			memmove(ch->MP3SlidingDecodeBuffer,
				static_cast<byte*>(ch->MP3SlidingDecodeBuffer) + iQuarterOfSlidingBuffer,
				iThreeQuartersOfSlidingBuffer);
			ch->iMP3SlidingDecodeWritePos -= iQuarterOfSlidingBuffer;
			ch->iMP3SlidingDecodeWindowPos += iQuarterOfSlidingBuffer;
		}
		//		Com_OPrintf("WindowPos %d, WindowWritePos %d\n",ch->iMP3SlidingDecodeWindowPos,ch->iMP3SlidingDecodeWritePos);
	}

	//	if (!_bDecoded)
	//	{
	//		Com_Printf(S_COLOR_YELLOW"No decode needed\n");
	//	}

	assert(startingSampleNum >= ch->iMP3SlidingDecodeWindowPos);
	memcpy(buf, ch->MP3SlidingDecodeBuffer + (startingSampleNum - ch->iMP3SlidingDecodeWindowPos), count);

	//	Com_OPrintf("OK\n\n");

	return qbStreamStillGoing;
}

///////////// eof /////////////