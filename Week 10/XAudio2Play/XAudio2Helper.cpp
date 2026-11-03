// !{! XAudio2Helper 구현 코드. 원본 코드의 위치는 XAudio2Play/XAudio2Helper.cpp.
#include "XAudio2Helper.h"


//---------------------------------------------------------------------------
// 디버깅 편의용 윈도우 함수
//---------------------------------------------------------------------------
#if defined(TRACE_WIN_REQUIRE_IMPL)
#include <tchar.h> //_vstprintf_s
// 메시지를 출력하고 E_FAIL을 리턴함
HRESULT TRACE_WIN(LPCTSTR lpszFormat, ...)
{
    TCHAR lpszBuffer[0x160]; //버퍼 크기.
    va_list fmtList;
    va_start(fmtList, lpszFormat);
    _vstprintf_s(lpszBuffer, lpszFormat, fmtList);
    va_end(fmtList);
    OutputDebugString(lpszBuffer);
    return E_FAIL;
}
// 메시지를 출력하고 입력값 hr을 그대로 리턴함
HRESULT TRACE_WIN(HRESULT hr, LPCTSTR lpszFormat, ...)
{
    TCHAR lpszBuffer[0x160]; //버퍼 크기.
    va_list fmtList;
    va_start(fmtList, lpszFormat);
    _vstprintf_s(lpszBuffer, lpszFormat, fmtList);
    va_end(fmtList);
    OutputDebugString(lpszBuffer);
    return hr;
}
#undef TRACE_WIN_REQUIRE_IMPL
#endif



//---------------------------------------------------------------------------
// Uses XAudio2 to playback sounds
// Original Version(2008) from directx-sdk-samples/XAudio2/XAudio2BasicSound/XAudio2BasicSound.cpp
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Name: PlayPCM
// Desc: Plays a wave and blocks until the wave finishes playing
//---------------------------------------------------------------------------
HRESULT PlayPCM(IXAudio2* pXaudio2, LPCWSTR szFilename)
{
    HRESULT hr = S_OK;

    //
    // Locate the wave file
    //
    WCHAR strFilePath[MAX_PATH];
    if (FAILED(hr = FindMediaFileCch(strFilePath, MAX_PATH, szFilename)))
    {
        wprintf(L"Failed to find media file: %s\n", szFilename);
        return hr;
    }

    //
    // Read in the wave file
    //
    CWaveFile wav;
    if (FAILED(hr = wav.Open(strFilePath, NULL, WAVEFILE_READ)))
    {
        wprintf(L"Failed reading WAV file: %#X (%s)\n", hr, strFilePath);
        return hr;
    }

    // Get format of wave file
    WAVEFORMATEX* pwfx = wav.GetFormat();

    // Calculate how many bytes and samples are in the wave
    DWORD cbWaveSize = wav.GetSize();

    // Read the sample data into memory
    BYTE* pbWaveData = new BYTE[cbWaveSize];

    if (FAILED(hr = wav.Read(pbWaveData, cbWaveSize, &cbWaveSize)))
    {
        wprintf(L"Failed to read WAV data: %#X\n", hr);
        SAFE_DELETE_ARRAY(pbWaveData);
        return hr;
    }

    //
    // Play the wave using a XAudio2SourceVoice
    //

    // Create the source voice
    IXAudio2SourceVoice* pSourceVoice;
    if (FAILED(hr = pXaudio2->CreateSourceVoice(&pSourceVoice, pwfx)))
    {
        wprintf(L"Error %#X creating source voice\n", hr);
        SAFE_DELETE_ARRAY(pbWaveData);
        return hr;
    }

    // Submit the wave sample data using an XAUDIO2_BUFFER structure
    XAUDIO2_BUFFER buffer = { 0 };
    buffer.pAudioData = pbWaveData;
    buffer.Flags = XAUDIO2_END_OF_STREAM;  // tell the source voice not to expect any data after this buffer
    buffer.AudioBytes = cbWaveSize;

    if (FAILED(hr = pSourceVoice->SubmitSourceBuffer(&buffer)))
    {
        wprintf(L"Error %#X submitting source buffer\n", hr);
        pSourceVoice->DestroyVoice();
        SAFE_DELETE_ARRAY(pbWaveData);
        return hr;
    }

    hr = pSourceVoice->Start(0);

    // Let the sound play
    BOOL isRunning = TRUE;
    while (SUCCEEDED(hr) && isRunning)
    {
        XAUDIO2_VOICE_STATE state;
        pSourceVoice->GetState(&state);
        isRunning = (state.BuffersQueued > 0) != 0;

        // Wait till the escape key is pressed
        if (GetAsyncKeyState(VK_ESCAPE))
            break;

        Sleep(10);
    }

    // Wait till the escape key is released
    while (GetAsyncKeyState(VK_ESCAPE))
        Sleep(10);

    pSourceVoice->DestroyVoice();
    SAFE_DELETE_ARRAY(pbWaveData);

    return hr;
}


//---------------------------------------------------------------------------
// Helper function to try to find the location of a media file
//---------------------------------------------------------------------------
HRESULT FindMediaFileCch(WCHAR* strDestPath, int cchDest, LPCWSTR strFilename)
{
    bool bFound = false;

    if (NULL == strFilename || strFilename[0] == 0 || NULL == strDestPath || cchDest < 10)
        return E_INVALIDARG;

    // Get the exe name, and exe path
    WCHAR strExePath[MAX_PATH] = { 0 };
    WCHAR strExeName[MAX_PATH] = { 0 };
    WCHAR* strLastSlash = NULL;
    GetModuleFileName(NULL, strExePath, MAX_PATH);
    strExePath[MAX_PATH - 1] = 0;
    strLastSlash = wcsrchr(strExePath, TEXT('\\'));
    if (strLastSlash)
    {
        wcscpy_s(strExeName, MAX_PATH, &strLastSlash[1]);

        // Chop the exe name from the exe path
        *strLastSlash = 0;

        // Chop the .exe from the exe name
        strLastSlash = wcsrchr(strExeName, TEXT('.'));
        if (strLastSlash)
            *strLastSlash = 0;
    }

    wcscpy_s(strDestPath, cchDest, strFilename);
    if (GetFileAttributes(strDestPath) != 0xFFFFFFFF)
        return S_OK;

    // Search all parent directories starting at .\ and using strFilename as the leaf name
    WCHAR strLeafName[MAX_PATH] = { 0 };
    wcscpy_s(strLeafName, MAX_PATH, strFilename);

    WCHAR strFullPath[MAX_PATH] = { 0 };
    WCHAR strFullFileName[MAX_PATH] = { 0 };
    WCHAR strSearch[MAX_PATH] = { 0 };
    WCHAR* strFilePart = NULL;

    GetFullPathName(L".", MAX_PATH, strFullPath, &strFilePart);
    if (strFilePart == NULL)
        return E_FAIL;

    while (strFilePart != NULL && *strFilePart != '\0')
    {
        swprintf_s(strFullFileName, MAX_PATH, L"%s\\%s", strFullPath, strLeafName);
        if (GetFileAttributes(strFullFileName) != 0xFFFFFFFF)
        {
            wcscpy_s(strDestPath, cchDest, strFullFileName);
            bFound = true;
            break;
        }

        swprintf_s(strFullFileName, MAX_PATH, L"%s\\%s\\%s", strFullPath, strExeName, strLeafName);
        if (GetFileAttributes(strFullFileName) != 0xFFFFFFFF)
        {
            wcscpy_s(strDestPath, cchDest, strFullFileName);
            bFound = true;
            break;
        }

        swprintf_s(strSearch, MAX_PATH, L"%s\\..", strFullPath);
        GetFullPathName(strSearch, MAX_PATH, strFullPath, &strFilePart);
    }
    if (bFound)
        return S_OK;

    // On failure, return the file as the path but also return an error code
    wcscpy_s(strDestPath, cchDest, strFilename);

    return HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
}



//---------------------------------------------------------------------------
// class CWaveFile -- from here to the end of the file
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Name: CWaveFile::CWaveFile()
// Desc: Constructs the class.  Call Open() to open a wave file for reading.
//       Then call Read() as needed.  Calling the destructor or Close()
//       will close the file.
//---------------------------------------------------------------------------
CWaveFile::CWaveFile()
{
    m_pwfx = NULL;
    m_hmmio = NULL;
    m_pResourceBuffer = NULL;
    m_dwSize = 0;
    m_bIsReadingFromMemory = FALSE;
}


//---------------------------------------------------------------------------
// Name: CWaveFile::~CWaveFile()
// Desc: Destructs the class
//---------------------------------------------------------------------------
CWaveFile::~CWaveFile()
{
    Close();

    if (!m_bIsReadingFromMemory)
        SAFE_DELETE_ARRAY(m_pwfx);
}


//---------------------------------------------------------------------------
// Name: CWaveFile::Open()
// Desc: Opens a wave file for reading
//---------------------------------------------------------------------------
HRESULT CWaveFile::Open(LPWSTR strFileName, WAVEFORMATEX* pwfx, DWORD dwFlags)
{
    HRESULT hr;

    m_dwFlags = dwFlags;
    m_bIsReadingFromMemory = FALSE;

    if (m_dwFlags == WAVEFILE_READ)
    {
        if (strFileName == NULL)
            return E_INVALIDARG;
        SAFE_DELETE_ARRAY(m_pwfx);

        m_hmmio = mmioOpen(strFileName, NULL, MMIO_ALLOCBUF | MMIO_READ);

        if (NULL == m_hmmio)
        {
            HRSRC hResInfo;
            HGLOBAL hResData;
            DWORD dwSize;
            VOID* pvRes;

            // Loading it as a file failed, so try it as a resource
            if (NULL == (hResInfo = FindResource(NULL, strFileName, L"WAVE")))
            {
                if (NULL == (hResInfo = FindResource(NULL, strFileName, L"WAV")))
                    return TRACE(TEXT("FindResource"));
            }

            if (NULL == (hResData = LoadResource(GetModuleHandle(NULL), hResInfo)))
                return TRACE(TEXT("LoadResource"));

            if (0 == (dwSize = SizeofResource(GetModuleHandle(NULL), hResInfo)))
                return TRACE(TEXT("SizeofResource"));

            if (NULL == (pvRes = LockResource(hResData)))
                return TRACE(TEXT("LockResource"));

            m_pResourceBuffer = new CHAR[dwSize];
            if (m_pResourceBuffer == NULL)
                return TRACE(E_OUTOFMEMORY, TEXT("new"));
            memcpy(m_pResourceBuffer, pvRes, dwSize);

            MMIOINFO mmioInfo;
            ZeroMemory(&mmioInfo, sizeof(mmioInfo));
            mmioInfo.fccIOProc = FOURCC_MEM;
            mmioInfo.cchBuffer = dwSize;
            mmioInfo.pchBuffer = (CHAR*)m_pResourceBuffer;

            m_hmmio = mmioOpen(NULL, &mmioInfo, MMIO_ALLOCBUF | MMIO_READ);
        }

        if (FAILED(hr = ReadMMIO()))
        {
            // ReadMMIO will fail if its an not a wave file
            mmioClose(m_hmmio, 0);
            return TRACE(hr, TEXT("ReadMMIO"));
        }

        if (FAILED(hr = ResetFile()))
            return TRACE(hr, TEXT("ResetFile"));

        // After the reset, the size of the wav file is m_ck.cksize so store it now
        m_dwSize = m_ck.cksize;
    }
    else
    {
        m_hmmio = mmioOpen(strFileName, NULL, MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_CREATE);
        if (NULL == m_hmmio)
            return TRACE(TEXT("mmioOpen"));

        if (FAILED(hr = WriteMMIO(pwfx)))
        {
            mmioClose(m_hmmio, 0);
            return TRACE(hr, TEXT("WriteMMIO"));
        }

        if (FAILED(hr = ResetFile()))
            return TRACE(hr, TEXT("ResetFile"));
    }

    return hr;
}


//---------------------------------------------------------------------------
// Name: CWaveFile::OpenFromMemory()
// Desc: copy data to CWaveFile member variable from memory
//---------------------------------------------------------------------------
HRESULT CWaveFile::OpenFromMemory(BYTE* pbData, ULONG ulDataSize,
    WAVEFORMATEX* pwfx, DWORD dwFlags)
{
    m_pwfx = pwfx;
    m_ulDataSize = ulDataSize;
    m_pbData = pbData;
    m_pbDataCur = m_pbData;
    m_bIsReadingFromMemory = TRUE;

    if (dwFlags != WAVEFILE_READ)
        return E_NOTIMPL;

    return S_OK;
}


//---------------------------------------------------------------------------
// Name: CWaveFile::ReadMMIO()
// Desc: Support function for reading from a multimedia I/O stream.
//       m_hmmio must be valid before calling.  This function uses it to
//       update m_ckRiff, and m_pwfx.
//---------------------------------------------------------------------------
HRESULT CWaveFile::ReadMMIO()
{
    MMCKINFO ckIn;           // chunk info. for general use.
    PCMWAVEFORMAT pcmWaveFormat;  // Temp PCM structure to load in.

    memset(&ckIn, 0, sizeof(ckIn));

    m_pwfx = NULL;

    if ((0 != mmioDescend(m_hmmio, &m_ckRiff, NULL, 0)))
        return TRACE(TEXT("mmioDescend"));

    // Check to make sure this is a valid wave file
    if ((m_ckRiff.ckid != FOURCC_RIFF) ||
        (m_ckRiff.fccType != mmioFOURCC('W', 'A', 'V', 'E')))
        return TRACE(TEXT("mmioFOURCC"));

    // Search the input file for for the 'fmt ' chunk.
    ckIn.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (0 != mmioDescend(m_hmmio, &ckIn, &m_ckRiff, MMIO_FINDCHUNK))
        return TRACE(TEXT("mmioDescend"));

    // Expect the 'fmt' chunk to be at least as large as <PCMWAVEFORMAT>;
    // if there are extra parameters at the end, we'll ignore them
    if (ckIn.cksize < (LONG)sizeof(PCMWAVEFORMAT))
        return TRACE(TEXT("sizeof(PCMWAVEFORMAT)"));

    // Read the 'fmt ' chunk into <pcmWaveFormat>.
    if (mmioRead(m_hmmio, (HPSTR)&pcmWaveFormat,
        sizeof(pcmWaveFormat)) != sizeof(pcmWaveFormat))
        return TRACE(TEXT("mmioRead"));

    // Allocate the waveformatex, but if its not pcm format, read the next
    // word, and thats how many extra bytes to allocate.
    if (pcmWaveFormat.wf.wFormatTag == WAVE_FORMAT_PCM)
    {
        m_pwfx = (WAVEFORMATEX*)new CHAR[sizeof(WAVEFORMATEX)];
        if (NULL == m_pwfx)
            return TRACE(TEXT("m_pwfx"));

        // Copy the bytes from the pcm structure to the waveformatex structure
        memcpy(m_pwfx, &pcmWaveFormat, sizeof(pcmWaveFormat));
        m_pwfx->cbSize = 0;
    }
    else
    {
        // Read in length of extra bytes.
        WORD cbExtraBytes = 0L;
        if (mmioRead(m_hmmio, (CHAR*)&cbExtraBytes, sizeof(WORD)) != sizeof(WORD))
            return TRACE(TEXT("mmioRead"));

        m_pwfx = (WAVEFORMATEX*)new CHAR[sizeof(WAVEFORMATEX) + cbExtraBytes];
        if (NULL == m_pwfx)
            return TRACE(TEXT("new"));

        // Copy the bytes from the pcm structure to the waveformatex structure
        memcpy(m_pwfx, &pcmWaveFormat, sizeof(pcmWaveFormat));
        m_pwfx->cbSize = cbExtraBytes;

        // Now, read those extra bytes into the structure, if cbExtraAlloc != 0.
        if (mmioRead(m_hmmio, (CHAR*)(((BYTE*)&(m_pwfx->cbSize)) + sizeof(WORD)), cbExtraBytes) != cbExtraBytes)
        {
            SAFE_DELETE(m_pwfx);
            return TRACE(TEXT("mmioRead"));
        }
    }

    // Ascend the input file out of the 'fmt ' chunk.
    if (0 != mmioAscend(m_hmmio, &ckIn, 0))
    {
        SAFE_DELETE(m_pwfx);
        return TRACE(TEXT("mmioAscend"));
    }

    return S_OK;
}


//---------------------------------------------------------------------------
// Name: CWaveFile::GetSize()
// Desc: Retuns the size of the read access wave file
//---------------------------------------------------------------------------
DWORD CWaveFile::GetSize()
{
    return m_dwSize;
}


//---------------------------------------------------------------------------
// Name: CWaveFile::ResetFile()
// Desc: Resets the internal m_ck pointer so reading starts from the
//       beginning of the file again
//---------------------------------------------------------------------------
HRESULT CWaveFile::ResetFile()
{
    if (m_bIsReadingFromMemory)
    {
        m_pbDataCur = m_pbData;
    }
    else
    {
        if (m_hmmio == NULL)
            return CO_E_NOTINITIALIZED;

        if (m_dwFlags == WAVEFILE_READ)
        {
            // Seek to the data
            if (-1 == mmioSeek(m_hmmio, m_ckRiff.dwDataOffset + sizeof(FOURCC), SEEK_SET))
                return TRACE(TEXT("mmioSeek"));

            // Search the input file for the 'data' chunk.
            m_ck.ckid = mmioFOURCC('d', 'a', 't', 'a');
            if (0 != mmioDescend(m_hmmio, &m_ck, &m_ckRiff, MMIO_FINDCHUNK))
                return TRACE(TEXT("mmioDescend"));
        }
        else
        {
            // Create the 'data' chunk that holds the waveform samples.
            m_ck.ckid = mmioFOURCC('d', 'a', 't', 'a');
            m_ck.cksize = 0;

            if (0 != mmioCreateChunk(m_hmmio, &m_ck, 0))
                return TRACE(TEXT("mmioCreateChunk"));

            if (0 != mmioGetInfo(m_hmmio, &m_mmioinfoOut, 0))
                return TRACE(TEXT("mmioGetInfo"));
        }
    }

    return S_OK;
}


//---------------------------------------------------------------------------
// Name: CWaveFile::Read()
// Desc: Reads section of data from a wave file into pBuffer and returns
//       how much read in pdwSizeRead, reading not more than dwSizeToRead.
//       This uses m_ck to determine where to start reading from.  So
//       subsequent calls will be continue where the last left off unless
//       Reset() is called.
//---------------------------------------------------------------------------
HRESULT CWaveFile::Read(BYTE* pBuffer, DWORD dwSizeToRead, DWORD* pdwSizeRead)
{
    if (m_bIsReadingFromMemory)
    {
        if (m_pbDataCur == NULL)
            return CO_E_NOTINITIALIZED;
        if (pdwSizeRead != NULL)
            *pdwSizeRead = 0;

        if ((BYTE*)(m_pbDataCur + dwSizeToRead) > (BYTE*)(m_pbData + m_ulDataSize))
        {
            dwSizeToRead = m_ulDataSize - (DWORD)(m_pbDataCur - m_pbData);
        }

#pragma warning(disable: 4616)    // disable warning about warning number '22104' being out of range
#pragma warning(disable: 22104)   // disable PREfast warning during static code analysis
        CopyMemory(pBuffer, m_pbDataCur, dwSizeToRead);
#pragma warning(default: 22104)
#pragma warning(default: 4616)

        if (pdwSizeRead != NULL)
            *pdwSizeRead = dwSizeToRead;

        return S_OK;
    }
    else
    {
        MMIOINFO mmioinfoIn; // current status of m_hmmio

        if (m_hmmio == NULL)
            return CO_E_NOTINITIALIZED;
        if (pBuffer == NULL || pdwSizeRead == NULL)
            return E_INVALIDARG;

        *pdwSizeRead = 0;

        if (0 != mmioGetInfo(m_hmmio, &mmioinfoIn, 0))
            return TRACE(TEXT("mmioGetInfo"));

        UINT cbDataIn = dwSizeToRead;
        if (cbDataIn > m_ck.cksize)
            cbDataIn = m_ck.cksize;

        m_ck.cksize -= cbDataIn;

        for (DWORD cT = 0; cT < cbDataIn; cT++)
        {
            // Copy the bytes from the io to the buffer.
            if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
            {
                if (0 != mmioAdvance(m_hmmio, &mmioinfoIn, MMIO_READ))
                    return TRACE(TEXT("mmioAdvance"));

                if (mmioinfoIn.pchNext == mmioinfoIn.pchEndRead)
                    return TRACE(TEXT("mmioinfoIn.pchNext"));
            }

            // Actual copy.
            *((BYTE*)pBuffer + cT) = *((BYTE*)mmioinfoIn.pchNext);
            mmioinfoIn.pchNext++;
        }

        if (0 != mmioSetInfo(m_hmmio, &mmioinfoIn, 0))
            return TRACE(TEXT("mmioSetInfo"));

        *pdwSizeRead = cbDataIn;

        return S_OK;
    }
}


//---------------------------------------------------------------------------
// Name: CWaveFile::Close()
// Desc: Closes the wave file
//---------------------------------------------------------------------------
HRESULT CWaveFile::Close()
{
    if (m_dwFlags == WAVEFILE_READ)
    {
        if (m_hmmio != NULL)
        {
            mmioClose(m_hmmio, 0);
            m_hmmio = NULL;
        }
        SAFE_DELETE_ARRAY(m_pResourceBuffer);
    }
    else
    {
        m_mmioinfoOut.dwFlags |= MMIO_DIRTY;

        if (m_hmmio == NULL)
            return CO_E_NOTINITIALIZED;

        if (0 != mmioSetInfo(m_hmmio, &m_mmioinfoOut, 0))
            return TRACE(TEXT("mmioSetInfo"));

        // Ascend the output file out of the 'data' chunk -- this will cause
        // the chunk size of the 'data' chunk to be written.
        if (0 != mmioAscend(m_hmmio, &m_ck, 0))
            return TRACE(TEXT("mmioAscend"));

        // Do this here instead...
        if (0 != mmioAscend(m_hmmio, &m_ckRiff, 0))
            return TRACE(TEXT("mmioAscend"));

        mmioSeek(m_hmmio, 0, SEEK_SET);

        if (0 != (INT)mmioDescend(m_hmmio, &m_ckRiff, NULL, 0))
            return TRACE(TEXT("mmioDescend"));

        m_ck.ckid = mmioFOURCC('f', 'a', 'c', 't');

        if (0 == mmioDescend(m_hmmio, &m_ck, &m_ckRiff, MMIO_FINDCHUNK))
        {
            DWORD dwSamples = 0;
            mmioWrite(m_hmmio, (HPSTR)&dwSamples, sizeof(DWORD));
            mmioAscend(m_hmmio, &m_ck, 0);
        }

        // Ascend the output file out of the 'RIFF' chunk -- this will cause
        // the chunk size of the 'RIFF' chunk to be written.
        if (0 != mmioAscend(m_hmmio, &m_ckRiff, 0))
            return TRACE(TEXT("mmioAscend"));

        mmioClose(m_hmmio, 0);
        m_hmmio = NULL;
    }

    return S_OK;
}


//---------------------------------------------------------------------------
// Name: CWaveFile::WriteMMIO()
// Desc: Support function for reading from a multimedia I/O stream
//       pwfxDest is the WAVEFORMATEX for this new wave file.
//       m_hmmio must be valid before calling.  This function uses it to
//       update m_ckRiff, and m_ck.
//---------------------------------------------------------------------------
HRESULT CWaveFile::WriteMMIO(WAVEFORMATEX* pwfxDest)
{
    DWORD dwFactChunk; // Contains the actual fact chunk. Garbage until WaveCloseWriteFile.
    MMCKINFO ckOut1;

    memset(&ckOut1, 0, sizeof(ckOut1));

    dwFactChunk = (DWORD)-1;

    // Create the output file RIFF chunk of form type 'WAVE'.
    m_ckRiff.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    m_ckRiff.cksize = 0;

    if (0 != mmioCreateChunk(m_hmmio, &m_ckRiff, MMIO_CREATERIFF))
        return TRACE(TEXT("mmioCreateChunk"));

    // We are now descended into the 'RIFF' chunk we just created.
    // Now create the 'fmt ' chunk. Since we know the size of this chunk,
    // specify it in the MMCKINFO structure so MMIO doesn't have to seek
    // back and set the chunk size after ascending from the chunk.
    m_ck.ckid = mmioFOURCC('f', 'm', 't', ' ');
    m_ck.cksize = sizeof(PCMWAVEFORMAT);

    if (0 != mmioCreateChunk(m_hmmio, &m_ck, 0))
        return TRACE(TEXT("mmioCreateChunk"));

    // Write the PCMWAVEFORMAT structure to the 'fmt ' chunk if its that type.
    if (pwfxDest->wFormatTag == WAVE_FORMAT_PCM)
    {
        if (mmioWrite(m_hmmio, (HPSTR)pwfxDest, sizeof(PCMWAVEFORMAT)) != sizeof(PCMWAVEFORMAT))
            return TRACE(TEXT("mmioWrite"));
    }
    else
    {
        // Write the variable length size.
        if ((UINT)mmioWrite(m_hmmio, (HPSTR)pwfxDest,
            sizeof(*pwfxDest) + pwfxDest->cbSize) !=
            (sizeof(*pwfxDest) + pwfxDest->cbSize))
            return TRACE(TEXT("mmioWrite"));
    }

    // Ascend out of the 'fmt ' chunk, back into the 'RIFF' chunk.
    if (0 != mmioAscend(m_hmmio, &m_ck, 0))
        return TRACE(TEXT("mmioAscend"));

    // Now create the fact chunk, not required for PCM but nice to have.  This is filled
    // in when the close routine is called.
    ckOut1.ckid = mmioFOURCC('f', 'a', 'c', 't');
    ckOut1.cksize = 0;

    if (0 != mmioCreateChunk(m_hmmio, &ckOut1, 0))
        return TRACE(TEXT("mmioCreateChunk"));

    if (mmioWrite(m_hmmio, (HPSTR)&dwFactChunk, sizeof(dwFactChunk)) !=
        sizeof(dwFactChunk))
        return TRACE(TEXT("mmioWrite"));

    // Now ascend out of the fact chunk...
    if (0 != mmioAscend(m_hmmio, &ckOut1, 0))
        return TRACE(TEXT("mmioAscend"));

    return S_OK;
}


//---------------------------------------------------------------------------
// Name: CWaveFile::Write()
// Desc: Writes data to the open wave file
//---------------------------------------------------------------------------
HRESULT CWaveFile::Write(UINT nSizeToWrite, BYTE* pbSrcData, UINT* pnSizeWrote)
{
    UINT cT;

    if (m_bIsReadingFromMemory)
        return E_NOTIMPL;
    if (m_hmmio == NULL)
        return CO_E_NOTINITIALIZED;
    if (pnSizeWrote == NULL || pbSrcData == NULL)
        return E_INVALIDARG;

    *pnSizeWrote = 0;

    for (cT = 0; cT < nSizeToWrite; cT++)
    {
        if (m_mmioinfoOut.pchNext == m_mmioinfoOut.pchEndWrite)
        {
            m_mmioinfoOut.dwFlags |= MMIO_DIRTY;
            if (0 != mmioAdvance(m_hmmio, &m_mmioinfoOut, MMIO_WRITE))
                return TRACE(TEXT("mmioAdvance"));
        }

        *((BYTE*)m_mmioinfoOut.pchNext) = *((BYTE*)pbSrcData + cT);
        (BYTE*)m_mmioinfoOut.pchNext++;

        (*pnSizeWrote)++;
    }

    return S_OK;
}

// !}!