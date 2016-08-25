#include "stdafx.h"
#include "VideoWriter.h"


VideoWriter::VideoWriter(int width, int height, int outputWidth, int outputHeight)
{
	mfVideoWriter = new MFVideoWriter(width, height, outputWidth, outputHeight);
	OnCompletedDelegate^ onCompletedDelegate = gcnew OnCompletedDelegate(this, &VideoWriter::OnCompleted);
	onCompletedDelegateHandle = GCHandle::Alloc(onCompletedDelegate, GCHandleType::Normal);
	IntPtr callback = Marshal::GetFunctionPointerForDelegate(onCompletedDelegate);
	marshal_context ctx;
	mfVideoWriter->SetCompletedCallback(ctx.marshal_as<void *>(callback));
}

VideoWriter::~VideoWriter()
{
	onCompletedDelegateHandle.Free();
	delete mfVideoWriter;
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

void VideoWriter::OnCompleted()
{
	Completed(this,  System::EventArgs::Empty);
}
