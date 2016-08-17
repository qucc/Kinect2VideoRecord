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
public :
	HRESULT hresult;
    ComException(const HRESULT& hr):hresult(hr)
	{}
};

inline void HR(HRESULT hr)
{
	if (!SUCCEEDED(hr))
	{
		throw ComException(hr);
	}
}




const UINT32 VIDEO_FPS = 30;
const UINT64 VIDEO_FRAME_DURATION = 10 * 1000 * 1000 / VIDEO_FPS;
const UINT32 VIDEO_BIT_RATE = 800000;
//const GUID   VIDEO_ENCODING_FORMAT = MFVideoFormat_WMV3;
//const GUID   VIDEO_INPUT_FORMAT = MFVideoFormat_RGB32;

class MFVideoWriter
{
private:
	IMFSinkWriter*			m_pSinkWriter;
	IMFTransform*			m_pVideoTransfrom;
	IMFMediaBuffer*			m_pBuffer;
	IMFMediaBuffer*			m_pResizeBuffer;
	DWORD					m_stream;
	LONGLONG				m_rtStart;
	int						m_width;
	int						m_height;
	float					m_resizeRatio;
public:
	MFVideoWriter(int width, int height, float resizeRatio);
	~MFVideoWriter();
	HRESULT					WriteFrame(BYTE* pImage);
	HRESULT					StartRecord(LPCWSTR filename);
	HRESULT					StopRecord();
};

