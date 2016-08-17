#include "stdafx.h"
#include <msclr\marshal.h>
#include <msclr/marshal_cppstd.h>
#include "VideoWriter.h"
using namespace System::Runtime::InteropServices;
using namespace msclr::interop;
VideoWriter::VideoWriter(int width, int height, float resizeRatio)
{
	mfVideoWriter = new MFVideoWriter(width, height, resizeRatio);
	
}

void VideoWriter::StartRecord(String ^ filename)
{
	marshal_context ctx;
	mfVideoWriter->StartRecord(ctx.marshal_as<LPCWSTR>(filename));
}

void VideoWriter::StopRecord()
{
	mfVideoWriter->StopRecord();
}

void VideoWriter::WriteFrame(IntPtr pImage)
{
	mfVideoWriter->WriteFrame(reinterpret_cast<BYTE *>(pImage.ToPointer()));
}
