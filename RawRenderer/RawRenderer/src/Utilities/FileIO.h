#pragma once
#include "Core/Window.h"

namespace FileIO {

	class ReadFromFile
	{
	public:
		ReadFromFile(const wchar_t* name);

		void Read(void* to, unsigned int numOfBytesToRead);

		template<typename T>
		void Read(T* to)
		{
			Read(to, sizeof(T));
		}

		void Close()
		{
			CloseHandle(hFile);
			bClosed = true;
		}

		bool IsOpen() const
		{
			return !bClosed;
		}

		~ReadFromFile()
		{
			if (!bClosed)
				CloseHandle(hFile);
		}

	private:
		HANDLE hFile;
		DWORD dwBytesRead = 0;
		bool bClosed = false;
	};

	class WriteToFile
	{
	public:
		WriteToFile(const wchar_t* name);
		WriteToFile(const WriteToFile&) = delete;

		void Write(void* from, unsigned int numOfBytesToWrite);

		template<typename T>
		void Write(T* from)
		{
			Write(from, sizeof(T));
		}

		void Close()
		{
			CloseHandle(hFile);
			closed = true;
		}

		~WriteToFile()
		{
			if (!closed)
			{
				CloseHandle(hFile);
			}
		}

	private:
		HANDLE hFile;
		DWORD dwBytesWritten = 0;
		bool closed = false;
	};

	template<typename T>
	void TSaveToFile(T* data, std::wstring fileName)
	{
		WriteToFile{ std::wstring{L"bin/" + fileName + L".bin"}.c_str() }.Write(data, sizeof(T));
	}

	template<typename T>
	void TLoadFromFile(T* data, std::wstring fileName)
	{
		FileIO::ReadFromFile(std::wstring{ L"bin/" + fileName + L".bin" }.c_str()).Read(data, sizeof(T));
	}

}

class RStoredInFileObject
{
protected:

	RStoredInFileObject(const std::wstring& fileName)
		: FileName(fileName)
	{

	}

	void SaveToFile(void* from, unsigned int numBytes)
	{
		FileIO::WriteToFile{ FileName.c_str() }.Write(from, numBytes);
	}
	template<typename T>
	void TSaveToFile(T* data)
	{
		SaveToFile(data, sizeof(T));
	}

	void LoadFromFile(void* into, unsigned int numBytes )
	{
		FileIO::ReadFromFile(FileName.c_str()).Read(into, numBytes);
	}
	template<typename T>
	void TLoadFromFile(T* into)
	{
		LoadFromFile(into, sizeof(T));
	}

	std::wstring FileName;

};








