#pragma once
#include "MFVideoWriter.h"
#include <msclr\marshal.h>
#include <msclr/marshal_windows.h>
#include <msclr/marshal_cppstd.h>
using namespace System::Runtime::InteropServices;
using namespace msclr::interop;
using namespace System;
	public ref class VideoWriter
	{
	delegate void OnCompletedDelegate();

	public:
		event EventHandler^ Completed;
		VideoWriter(int width, int height, int outputWidth, int outputHeight);
		~VideoWriter();
		void StartRecord(String^ filename);
		void StopRecord();
		void WriteFrame(IntPtr pImage);
	private:
		MFVideoWriter*		 mfVideoWriter;
		GCHandle			 onCompletedDelegateHandle;
	private:
		void OnCompleted();
	};




