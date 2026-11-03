// !{! XAudio2Helper 선언 코드. 원본 코드의 위치는 XAudio2Play/XAudio2Helper.h.
#pragma once

#include <windows.h>
#include <mmsystem.h>
#pragma comment (lib, "winmm.lib") //for many mmio* functions
#include <xaudio2.h> //library XAudio2


//--------------------------------------------------------------------------------------
// 도움 매크로
//--------------------------------------------------------------------------------------
#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

//--------------------------------------------------------------------------------------
// 디버깅 편의용 윈도우 함수
//--------------------------------------------------------------------------------------
#if defined(_DEBUG) && defined(_WINDOWS) && !defined(_AFX) && !defined(_AFXDLL) && !defined(TRACE)
#define TRACE	TRACE_WIN
HRESULT TRACE_WIN(LPCTSTR lpszFormat, ...);
HRESULT TRACE_WIN(HRESULT hr, LPCTSTR lpszFormat, ...);
#define TRACE_WIN_REQUIRE_IMPL
#endif



//--------------------------------------------------------------------------------------
// WAV 파일을 읽기 위한 함수들 원형
//--------------------------------------------------------------------------------------
HRESULT PlayPCM(IXAudio2* pXaudio2, LPCWSTR szFilename);
HRESULT FindMediaFileCch(WCHAR* strDestPath, int cchDest, LPCWSTR strFilename);



//-----------------------------------------------------------------------------
// class CWaveFile -- from here to the end of the file
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Typing macros 
//-----------------------------------------------------------------------------
#define WAVEFILE_READ   1
#define WAVEFILE_WRITE  2

//-----------------------------------------------------------------------------
// Name: class CWaveFile
// Desc: Encapsulates reading or writing sound data to or from a wave file
//-----------------------------------------------------------------------------
class CWaveFile
{
public:
	WAVEFORMATEX*	m_pwfx; // Pointer to WAVEFORMATEX structure
	HMMIO			m_hmmio; // MM I/O handle for the WAVE
	MMCKINFO		m_ck; // Multimedia RIFF chunk
	MMCKINFO		m_ckRiff; // Use in opening a WAVE file
	DWORD			m_dwSize; // The size of the wave file
	MMIOINFO		m_mmioinfoOut;
	DWORD			m_dwFlags;
	BOOL			m_bIsReadingFromMemory;
	BYTE*			m_pbData;
	BYTE*			m_pbDataCur;
	ULONG			m_ulDataSize;
	CHAR*			m_pResourceBuffer;

protected:
	HRESULT ReadMMIO();
	HRESULT WriteMMIO(WAVEFORMATEX* pwfxDest);

public:
	CWaveFile();
	~CWaveFile();

	HRESULT Open(LPTSTR strFileName, WAVEFORMATEX* pwfx, DWORD dwFlags);
	HRESULT OpenFromMemory(BYTE* pbData, ULONG ulDataSize, WAVEFORMATEX* pwfx, DWORD dwFlags);
	HRESULT Close();

	HRESULT Read(BYTE* pBuffer, DWORD dwSizeToRead, DWORD* pdwSizeRead);
	HRESULT Write(UINT nSizeToWrite, BYTE* pbData, UINT* pnSizeWrote);

	DWORD GetSize();
	HRESULT ResetFile();
	WAVEFORMATEX* GetFormat() { return m_pwfx; };
};

// !}!