#include <iostream>
#include <boost/format.hpp>
#include <string>
#include "mdump.h"

char MiniDumper::m_szAppName[_MAX_PATH] = "Application";

MiniDumper::MiniDumper( LPCSTR szAppName )
{	
	if (szAppName)
	{
		sprintf_s(MiniDumper::m_szAppName, szAppName);
	}

	::SetUnhandledExceptionFilter( TopLevelFilter );
}

LONG MiniDumper::TopLevelFilter( struct _EXCEPTION_POINTERS *pExceptionInfo )
{
	std::cout << "Dumping ..." << std::endl;

	LONG retval = EXCEPTION_CONTINUE_SEARCH;
	HWND hParent = NULL;						// find a better value for your app

	// firstly see if dbghelp.dll is around and has the function we need
	// look next to the EXE first, as the one in System32 might be old 
	// (e.g. Windows 2000)
	HMODULE hDll = NULL;

	if (hDll==NULL)
	{
		// load any version we can
		hDll = ::LoadLibrary( "DBGHELP.DLL" );
	}

	if (hDll == NULL)
	{
		std::cerr << "Failed to load DBGHELP.DLL" << std::endl;
		return -1;
	}

	LPCTSTR szResult = NULL;

	if (hDll)
	{
		MINIDUMPWRITEDUMP pDump = (MINIDUMPWRITEDUMP)::GetProcAddress( hDll, "MiniDumpWriteDump" );
		if (pDump)
		{
			char szDumpPath[_MAX_PATH] = ".\\";

			// work out a good place for the dump file
			GetTempPath(_MAX_PATH, szDumpPath);
			std::cerr << "Temp path: " << szDumpPath << std::endl;
			boost::format fmt("%1%%2%.dmp");
			fmt % m_szAppName % std::time(nullptr);
			std::string path(fmt.str());
		
			// create the file
			HANDLE hFile = ::CreateFile(path.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

			if (hFile != INVALID_HANDLE_VALUE)
			{
				_MINIDUMP_EXCEPTION_INFORMATION ExInfo;

				ExInfo.ThreadId = ::GetCurrentThreadId();
				ExInfo.ExceptionPointers = pExceptionInfo;
				ExInfo.ClientPointers = NULL;

				// write the dump
				BOOL bOK = pDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &ExInfo, NULL, NULL);
				if (bOK)
				{
					std::cout << "Dump saved to: " << path << std::endl;
					retval = EXCEPTION_EXECUTE_HANDLER;
				}
				else
				{
					std::cerr << "Failed to save dump file to : " << path << ", Error:" << GetLastError() << std::endl;
				}
				::CloseHandle(hFile);
			}
			else
			{
				std::cerr << "Failed to create dump file : " << path << ", Error:" << GetLastError() << std::endl;
			}
		}
		else
		{
			std::cerr << "DBGHELP.DLL too old" << std::endl;
		}
	}
	else
	{
		std::cerr << "DBGHELP.DLL not found" << std::endl;
	}

	return retval;
}
