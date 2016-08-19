#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif

// Windows Header Files
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <Mfreadwrite.h>
#include <mferror.h>
#include <comdef.h>
#include <queue>
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mfuuid")
template<class Interface>
inline void SafeRelease(Interface *& pInterfaceToRelease)
{
	if (pInterfaceToRelease != NULL)
	{
		pInterfaceToRelease->Release();
		pInterfaceToRelease = NULL;
	}
}
class ComException {
public:
	HRESULT hresult;
	ComException(const HRESULT& hr) :hresult(hr)
	{}
};

inline void HR(HRESULT hr)
{
	if (!SUCCEEDED(hr))
	{
		_com_error err(hr);
		LPCTSTR errMsg = err.ErrorMessage();
		throw err;
	}
}
inline void Verify(BOOL success)
{
	if (!success)
	{
		throw success;
	}
}

const UINT32 VIDEO_FPS = 30;
const UINT64 VIDEO_FRAME_DURATION = 10 * 1000 * 1000 / VIDEO_FPS;
const UINT32 VIDEO_BIT_RATE = 800000;
//const GUID   VIDEO_ENCODING_FORMAT = MFVideoFormat_WMV3;
//const GUID   VIDEO_INPUT_FORMAT = MFVideoFormat_RGB32;
using namespace std;
using namespace System::Diagnostics;
typedef void(__stdcall *CompletedCallback)();
class MFVideoWriter
{
private:
	IMFSinkWriter*			m_pSinkWriter;
	DWORD					m_stream;
	LONGLONG				m_rtStart;
	int						m_width;
	int						m_height;
	float					m_resizeRatio;
	HANDLE					m_workThread;
	HANDLE					m_workEvent;
	CRITICAL_SECTION		m_criticalSection;
	queue<IMFMediaBuffer*>		m_samples;
	volatile bool			m_stillRecording;
	CompletedCallback		m_compltedCallback;

public:
	MFVideoWriter(int width, int height, float resizeRatio);
	~MFVideoWriter();
	HRESULT					WriteFrame(BYTE* pImage);
	void					ReadFrame();

	HRESULT					StartRecord(LPCWSTR filename);
	HRESULT					StopRecord();
	void					SetCompletedCallback(void* callback);

};

