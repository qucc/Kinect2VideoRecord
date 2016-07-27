#include "stdafx.h"
#include "MFVideoWriter.h"


MFVideoWriter::MFVideoWriter():
	m_pSinkWriter(NULL),
	m_pVideoTransfrom(NULL),
	m_pBuffer(NULL),
	m_pResizeBuffer(NULL),
	m_stream(0),
	m_rtStart(0)
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

	const LONG cbWidth = 4 * VIDEO_WIDTH;
	const DWORD cbBuffer = cbWidth * VIDEO_HEIGHT;

	BYTE *pData = NULL;
	HRESULT	hr = m_pBuffer->Lock(&pData, NULL, NULL);
	if (SUCCEEDED(hr))
	{
		hr = MFCopyImage(
			pData,                      // Destination buffer.
			cbWidth,                    // Destination stride.
			pImage,    // First row in source image.
			cbWidth,                    // Source stride.
			cbWidth,                    // Image width in bytes.
			VIDEO_HEIGHT                // Image height in pixels.
		);
	}
	m_pBuffer->Unlock();

	// Set the data length of the buffer.
	if (SUCCEEDED(hr))
	{
		hr = m_pBuffer->SetCurrentLength(cbBuffer);
	}

	// Create a media sample and add the buffer to the sample.
	if (SUCCEEDED(hr))
	{
		hr = MFCreateSample(&pSample);
	}
	if (SUCCEEDED(hr))
	{
		hr = pSample->AddBuffer(m_pBuffer);
	}

	// Set the time stamp and the duration.
	if (SUCCEEDED(hr))
	{
		hr = pSample->SetSampleTime(m_rtStart);
	}
	if (SUCCEEDED(hr))
	{
		hr = pSample->SetSampleDuration(VIDEO_FRAME_DURATION);
	}
	if (SUCCEEDED(hr))
	{
		hr = m_pVideoTransfrom->ProcessInput(m_stream, pSample, 0);
	}

	IMFSample *pOutSample = NULL;
	hr = MFCreateSample(&pOutSample);
	DWORD outBufferSize = cbBuffer / 4;
	m_pResizeBuffer->SetCurrentLength(outBufferSize);
	hr = pOutSample->AddBuffer(m_pResizeBuffer);
	MFT_OUTPUT_DATA_BUFFER IYUVOutputDataBuffer;
	IYUVOutputDataBuffer.pSample = pOutSample;
	IYUVOutputDataBuffer.dwStreamID = m_stream;
	IYUVOutputDataBuffer.dwStatus = 0;
	IYUVOutputDataBuffer.pEvents = NULL;
	if (SUCCEEDED(hr))
	{
		DWORD dwDSPStatus = 0;
		hr = m_pVideoTransfrom->ProcessOutput(0, 1, &IYUVOutputDataBuffer, &dwDSPStatus);
		hr = m_pVideoTransfrom->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH, 0);
		int b = 0;
	}

	// Send the sample to the Sink Writer.
	if (SUCCEEDED(hr))
	{
		hr = m_pSinkWriter->WriteSample(m_stream, pOutSample);
		m_rtStart += VIDEO_FRAME_DURATION;
	}
	SafeRelease(pOutSample);
	SafeRelease(pSample);
	return hr;
}



HRESULT InitializeSinkWriter(LPCWSTR filename, IMFSinkWriter **ppWriter, DWORD *pStreamIndex, IMFTransform* videoTransform)
{
	*ppWriter = NULL;
	*pStreamIndex = NULL;

	IMFSinkWriter   *pSinkWriter = NULL;
	IMFMediaType    *pMediaTypeOut = NULL;
	IMFMediaType    *pMediaTypeIn = NULL;
	IMFMediaType    *pMediaTypeInOut = NULL;
	DWORD           streamIndex;

	HRESULT hr = MFCreateSinkWriterFromURL(filename, NULL, NULL, &pSinkWriter);

	// Set the output media type.
	if (SUCCEEDED(hr))
	{
		hr = MFCreateMediaType(&pMediaTypeOut);
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetGUID(MF_MT_SUBTYPE, VIDEO_ENCODING_FORMAT);
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetUINT32(MF_MT_AVG_BITRATE, VIDEO_BIT_RATE);
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeSize(pMediaTypeOut, MF_MT_FRAME_SIZE, VIDEO_WIDTH / 4, VIDEO_HEIGHT / 4);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_FRAME_RATE, VIDEO_FPS, 1);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	}
	if (SUCCEEDED(hr))
	{
		hr = pSinkWriter->AddStream(pMediaTypeOut, &streamIndex);
	}

	// Set the input media type.
	if (SUCCEEDED(hr))
	{
		hr = MFCreateMediaType(&pMediaTypeIn);
		hr = MFCreateMediaType(&pMediaTypeInOut);
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		hr = pMediaTypeInOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetGUID(MF_MT_SUBTYPE, VIDEO_INPUT_FORMAT);
		hr = pMediaTypeInOut->SetGUID(MF_MT_SUBTYPE, VIDEO_INPUT_FORMAT);

	}
	if (SUCCEEDED(hr))
	{
		hr = pMediaTypeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
		hr = pMediaTypeInOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeSize(pMediaTypeIn, MF_MT_FRAME_SIZE, VIDEO_WIDTH, VIDEO_HEIGHT);
		hr = MFSetAttributeSize(pMediaTypeInOut, MF_MT_FRAME_SIZE, VIDEO_WIDTH / 4, VIDEO_HEIGHT / 4);

	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_FRAME_RATE, VIDEO_FPS, 1);
		hr = MFSetAttributeRatio(pMediaTypeInOut, MF_MT_FRAME_RATE, VIDEO_FPS, 1);

	}
	if (SUCCEEDED(hr))
	{
		hr = MFSetAttributeRatio(pMediaTypeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
		hr = MFSetAttributeRatio(pMediaTypeInOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);

	}
	if (SUCCEEDED(hr))
	{
		hr = videoTransform->SetInputType(streamIndex, pMediaTypeIn, NULL);
	}
	if (SUCCEEDED(hr))
	{
		hr = videoTransform->SetOutputType(streamIndex, pMediaTypeInOut, NULL);
	}

	if (SUCCEEDED(hr))
	{
		hr = pSinkWriter->SetInputMediaType(streamIndex, pMediaTypeInOut, NULL);
	}

	// Tell the sink writer to start accepting data.
	if (SUCCEEDED(hr))
	{
		hr = pSinkWriter->BeginWriting();
	}

	// Return the pointer to the caller.
	if (SUCCEEDED(hr))
	{
		*ppWriter = pSinkWriter;
		(*ppWriter)->AddRef();
		*pStreamIndex = streamIndex;
	}

	SafeRelease(pMediaTypeInOut);
	SafeRelease(pSinkWriter);
	SafeRelease(pMediaTypeOut);
	SafeRelease(pMediaTypeIn);
	return hr;
}

HRESULT MFVideoWriter::StartRecord(LPCWSTR filename)
{
	HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	if (SUCCEEDED(hr))
	{
		hr = MFStartup(MF_VERSION);
		if (SUCCEEDED(hr))
		{
			m_rtStart = 0;
			hr = MFCreateMemoryBuffer(4 * VIDEO_WIDTH * VIDEO_HEIGHT, &m_pBuffer);
			hr = MFCreateMemoryBuffer(VIDEO_WIDTH * VIDEO_HEIGHT, &m_pResizeBuffer);
			hr = CreateVideoTransform();
			hr = InitializeSinkWriter(filename, &m_pSinkWriter, &m_stream, m_pVideoTransfrom);
			IMFVideoProcessorControl* tronsfromControl;
			hr = m_pVideoTransfrom->QueryInterface(&tronsfromControl);
			MFARGB border{ 100,200,100,255 };
			hr = tronsfromControl->SetBorderColor(&border);
		}
	}
	return hr;
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

HRESULT MFVideoWriter::CreateVideoTransform()
{
	UINT32 count = 0;
	IMFActivate **ppActivate = NULL;
	HRESULT hr = MFTEnumEx(
		MFT_CATEGORY_VIDEO_PROCESSOR,
		MFT_ENUM_FLAG_SYNCMFT | MFT_ENUM_FLAG_LOCALMFT | MFT_ENUM_FLAG_SORTANDFILTER,
		NULL,       // Input type
		NULL,      // Output type
		&ppActivate,
		&count
	);
	if (SUCCEEDED(hr))
	{
		hr = ppActivate[0]->ActivateObject(IID_PPV_ARGS(&m_pVideoTransfrom));
	}
	for (UINT32 i = 0; i < count; i++)
	{
		ppActivate[i]->Release();
	}
	CoTaskMemFree(ppActivate);
	return hr;
}