#include "stdafx.h"
#include "MFVideoWriter.h"


MFVideoWriter::MFVideoWriter(int width, int height, float resizeRatio) :
	m_pSinkWriter(NULL),
	m_pVideoTransfrom(NULL),
	m_pBuffer(NULL),
	m_pResizeBuffer(NULL),
	m_stream(0),
	m_rtStart(0),
	m_width(width),
	m_height(height),
	m_resizeRatio(resizeRatio)
{
}


MFVideoWriter::~MFVideoWriter()
{
	SafeRelease(m_pSinkWriter);
	SafeRelease(m_pVideoTransfrom);
	SafeRelease(m_pBuffer);
	SafeRelease(m_pResizeBuffer);
}

HRESULT MFVideoWriter::WriteFrame(BYTE * pImage)
{
	IMFSample *pSample = NULL;
	const LONG cbWidth = 4 * m_width;
	const DWORD cbBuffer = cbWidth * m_height;

	BYTE *pData = NULL;
	HR( m_pBuffer->Lock(&pData, NULL, NULL));
	HR(MFCopyImage(
			pData,                      // Destination buffer.
			cbWidth,                    // Destination stride.
			pImage,    // First row in source image.
			cbWidth,                    // Source stride.
			cbWidth,                    // Image width in bytes.
			m_height                // Image height in pixels.
		));
	HR(m_pBuffer->Unlock());

	// Set the data length of the buffer.
	HR(m_pBuffer->SetCurrentLength(cbBuffer));
	HR(MFCreateSample(&pSample));
	HR( pSample->AddBuffer(m_pBuffer));
	HR(pSample->SetSampleTime(m_rtStart));
	HR(pSample->SetSampleDuration(VIDEO_FRAME_DURATION));
	
	if (m_pVideoTransfrom)
	{
		HR(m_pVideoTransfrom->ProcessInput(m_stream, pSample, 0));
		IMFSample *pOutSample = NULL;
		HR(MFCreateSample(&pOutSample));
		DWORD outBufferSize = cbBuffer * m_resizeRatio * m_resizeRatio;
		HR(m_pResizeBuffer->SetCurrentLength(outBufferSize));
		HR(pOutSample->AddBuffer(m_pResizeBuffer));

		MFT_OUTPUT_DATA_BUFFER IYUVOutputDataBuffer;
		IYUVOutputDataBuffer.pSample = pOutSample;
		IYUVOutputDataBuffer.dwStreamID = m_stream;
		IYUVOutputDataBuffer.dwStatus = 0;
		IYUVOutputDataBuffer.pEvents = NULL;
		DWORD dwDSPStatus = 0;

		HR(m_pVideoTransfrom->ProcessOutput(0, 1, &IYUVOutputDataBuffer, &dwDSPStatus));
		HR(m_pVideoTransfrom->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0));

		// Send the sample to the Sink Writer.
		HR(m_pSinkWriter->WriteSample(m_stream, pOutSample));
		SafeRelease(pOutSample);
	}
	else
	{
		HR(m_pSinkWriter->WriteSample(m_stream, pSample));
	}
	m_rtStart += VIDEO_FRAME_DURATION;
	SafeRelease(pSample);

	return S_OK;
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

HRESULT InitializeSinkWriter(LPCWSTR filename, int width, int height, float resizeRatio, IMFSinkWriter **ppWriter, IMFTransform* videoTransform)
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
	
	if (videoTransform)
	{
		HR(CreateMediaType(width * resizeRatio, height *  resizeRatio, MFVideoFormat_RGB32, &pMediaTypeInOut));
		HR(videoTransform->SetInputType(streamIndex, pMediaTypeIn, NULL));
		HR(videoTransform->SetOutputType(streamIndex, pMediaTypeInOut, NULL));
		HR(pSinkWriter->SetInputMediaType(streamIndex, pMediaTypeInOut, NULL));
	}
	else 
	{
		HR(pSinkWriter->SetInputMediaType(streamIndex, pMediaTypeIn, NULL));
	}

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
	HR(MFCreateMemoryBuffer(4 * m_width * m_height, &m_pBuffer));
	if (m_resizeRatio != 1)
	{
		HR(MFCreateMemoryBuffer(m_width * m_resizeRatio * m_height * m_resizeRatio * 4 , &m_pResizeBuffer));
		HR(CreateVideoTransform(&m_pVideoTransfrom));
		IMFVideoProcessorControl* tronsfromControl;
		HR(m_pVideoTransfrom->QueryInterface(&tronsfromControl));
	}

	HR(InitializeSinkWriter(filename,m_width, m_height, m_resizeRatio,  &m_pSinkWriter, m_pVideoTransfrom));

	return S_OK;
}

HRESULT MFVideoWriter::StopRecord()
{
	HRESULT	hr = m_pSinkWriter->Finalize();
	SafeRelease(m_pVideoTransfrom);
	SafeRelease(m_pBuffer);
	SafeRelease(m_pResizeBuffer);
	SafeRelease(m_pSinkWriter);
	MFShutdown();
	CoUninitialize();
	return hr;
}
