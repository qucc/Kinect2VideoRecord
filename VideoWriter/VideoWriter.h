#pragma once
#include "MFVideoWriter.h"
using namespace System;
	public ref class VideoWriter
	{
	public:
		VideoWriter();
		void StartRecord(String^ filename);
		void StopRecord();
		void WriteFrame(IntPtr pImage);
	private:
		MFVideoWriter* mfVideoWriter;
	};




