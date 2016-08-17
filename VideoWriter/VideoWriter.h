#pragma once
#include "MFVideoWriter.h"
using namespace System;
	public ref class VideoWriter
	{
	public:
		VideoWriter(int width, int height, float resizeRatio);
		void StartRecord(String^ filename);
		void StopRecord();
		void WriteFrame(IntPtr pImage);
	private:
		MFVideoWriter* mfVideoWriter;
	};




