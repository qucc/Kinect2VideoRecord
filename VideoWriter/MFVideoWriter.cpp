#include "stdafx.h"
#include "MFVideoWriter.h"
#include <stdio.h>
#include <ctime>
MFVideoWriter::MFVideoWriter(int width, int height, float resizeRatio) :
	m_pSinkWriter(NULL),
	m_stream(0),
	m_rtStart(0),
	m_width(width),
	m_height(height),
	m_resizeRatio(resizeRatio),
	m_stillRecording(false)
{

}


MFVideoWriter::~MFVideoWriter()
{
	SafeRelease(m_pSinkWriter);
	Verify(CloseHandle(m_workThread));
	Verify(CloseHandle(m_workEvent));
	DeleteCriticalSection(&m_criticalSection);
	MFShutdown();
	CoUninitialize();
}

HRESULT MFVideoWriter::WriteFrame(BYTE * pImage)
{
	if (m_stillRecording)
	{
		const LONG cbWidth = 4 * m_width;
		const DWORD cbBuffer = cbWidth * m_height;
		BYTE *pData = NULL;
		IMFMediaBuffer* pBuffer;
		(MFCreateMemoryBuffer(m_width * m_height * 4, &pBuffer));
		HR(pBuffer->Lock(&pData, NULL, NULL));
		HR(MFCopyImage(
			pData,                      // Destination buffer.
			cbWidth,                    // Destination stride.
			pImage,    // First row in source image.
			cbWidth,                    // Source stride.
			cbWidth,                    // Image width in bytes.
			m_height                // Image height in pixels.
		));
		HR(pBuffer->Unlock());
		// Set the data length of the buffer.
		HR(pBuffer->SetCurrentLength(cbBuffer));
		EnterCriticalSection(&m_criticalSection);
		m_samples.push(pBuffer);
		if (m_samples.size() == 1)
		{
			SetEvent(m_workEvent);
		}
		LeaveCriticalSection(&m_criticalSection);
	}
	return S_OK;
}


static DWORD WINAPI StaticReadFrame(void* caller)
{
	MFVideoWriter* c = static_cast<MFVideoWriter*>(caller);
	c->ReadFrame();
	return 0;
}

void MFVideoWriter::ReadFrame()
{
	while (true)
	{	
		EnterCriticalSection(&m_criticalSection);
		while (m_samples.empty())
		{
			LeaveCriticalSection(&m_criticalSection);
			WaitForSingleObject(m_workEvent, INFINITE);
			EnterCriticalSection(&m_criticalSection);
		}
		IMFMediaBuffer* buffer = m_samples.front();
		m_samples.pop();
		if (m_samples.empty())
		{
			ResetEvent(m_workEvent);
		}
		if (!m_stillRecording && m_samples.empty())
			break;
		LeaveCriticalSection(&m_criticalSection);
		IMFSample*  sample;
		HR(MFCreateSample(&sample));
		HR(sample->AddBuffer(buffer));
		HR(sample->SetSampleTime(m_rtStart));
		HR(sample->SetSampleDuration(VIDEO_FRAME_DURATION));
		m_rtStart += VIDEO_FRAME_DURATION;
		auto start = clock();
		HR(m_pSinkWriter->WriteSample(m_stream, sample));
		Trace::WriteLine("time elaspd: " + (clock() - start));
		SafeRelease(sample);
		SafeRelease(buffer);
	}
	int i = 0;
	HR(m_pSinkWriter->Finalize());
	
}


HRESULT CreateMediaType(UINT32 width, UINT32 height, GUID encodingFormat, IMFMediaType ** mediaType)
{
	HR(MFCreateMediaType(mediaType));
	auto pMediaTypeOut = *mediaType;
	HR(pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video));
	HR(pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, encodingFormat));
	HR(pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, VIDEO_BIT_RATE));
	HR(pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive));
	HR(MFSetAttributeSize(pMediaTypeOut, MF_MT_FRAME_SIZE, width, height));
	HR(MFSetAttributeRatio(pMediaTypeOut, MF_MT_FRAME_RATE, VIDEO_FPS, 1));
	HR(MFSetAttributeRatio(pMediaTypeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1));
	return S_OK;
}

HRESULT InitializeSinkWriter(LPCWSTR filename, int width, int height, float resizeRatio, IMFSinkWriter **ppWriter)
{
	*ppWriter = NULL;
	IMFSinkWriter   *pSinkWriter = NULL;
	IMFMediaType    *pMediaTypeOut = NULL;
	IMFMediaType    *pMediaTypeIn = NULL;
	IMFMediaType    *pMediaTypeInOut = NULL;
	DWORD           streamIndex = 0;

	HR(CreateMediaType(width, height, MFVideoFormat_RGB32, &pMediaTypeIn));
	HR(CreateMediaType(width * resizeRatio, height *  resizeRatio, MFVideoFormat_WMV3, &pMediaTypeOut));
	
	HR(MFCreateSinkWriterFromURL(filename, NULL, NULL, &pSinkWriter));
	HR(pSinkWriter->AddStream(pMediaTypeOut, &streamIndex));
	
	HR(pSinkWriter->SetInputMediaType(streamIndex, pMediaTypeIn, NULL));
	HR(pSinkWriter->BeginWriting());
	
	// Return the pointer to the caller.
	
	*ppWriter = pSinkWriter;
	(*ppWriter)->AddRef();
	SafeRelease(pMediaTypeInOut);
	SafeRelease(pSinkWriter);
	SafeRelease(pMediaTypeOut);
	SafeRelease(pMediaTypeIn);
	return S_OK;
}

HRESULT CreateVideoTransform(IMFTransform ** videoTransfrom)
{
	UINT32 count = 0;
	IMFActivate **ppActivate = NULL;
	HR(MFTEnumEx(
		MFT_CATEGORY_VIDEO_PROCESSOR,
		MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_LOCALMFT | MFT_ENUM_FLAG_SORTANDFILTER,
		NULL,       // Input type
		NULL,      // Output type
		&ppActivate,
		&count
	));
	HR(ppActivate[0]->ActivateObject(IID_PPV_ARGS(videoTransfrom)));
	
	for (UINT32 i = 0; i < count; i++)
	{
		ppActivate[i]->Release();
	}
	CoTaskMemFree(ppActivate);
	return S_OK;
}

HRESULT MFVideoWriter::StartRecord(LPCWSTR filename)
{
    HR(CoInitializeEx(NULL, COINIT_APARTMENTTHREADED));
	HR(MFStartup(MF_VERSION));
	m_rtStart = 0;
	m_stillRecording = true;
	HR(InitializeSinkWriter(filename,m_width, m_height, m_resizeRatio,  &m_pSinkWriter));
	InitializeCriticalSection(&m_criticalSection);
	DWORD threadId;
	m_workThread = CreateThread(NULL, 0, StaticReadFrame, this, 0, &threadId);
	Verify(m_workThread != NULL);
	m_workEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	Verify(m_workEvent != NULL);
	return S_OK;
}

HRESULT MFVideoWriter::StopRecord()
{
	m_stillRecording = false;
	return S_OK;
}

