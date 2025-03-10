﻿/*  This file is part of UKNCBTL.
    UKNCBTL is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    UKNCBTL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
UKNCBTL. If not, see <http://www.gnu.org/licenses/>. */

// Common.h

#pragma once

//////////////////////////////////////////////////////////////////////
// Assertions checking - MFC-like ASSERT macro

#ifdef _DEBUG

BOOL AssertFailedLine(LPCSTR lpszFileName, int nLine);
#define ASSERT(f)          (void) ((f) || !AssertFailedLine(__FILE__, __LINE__) || (DebugBreak(), 0))
#define VERIFY(f)          ASSERT(f)

#else   // _DEBUG

#define ASSERT(f)          ((void)0)
#define VERIFY(f)          ((void)f)

#endif // !_DEBUG


//////////////////////////////////////////////////////////////////////
// Alerts

void AlertInfo(LPCTSTR sMessage);
void AlertWarning(LPCTSTR sMessage);
void AlertWarningFormat(LPCTSTR sFormat, ...);
BOOL AlertOkCancel(LPCTSTR sMessage);


//////////////////////////////////////////////////////////////////////
// DebugPrint

void DebugPrint(LPCTSTR message);
void DebugPrintFormat(LPCTSTR pszFormat, ...);
void DebugLogClear();
void DebugLogCloseFile();
void DebugLog(LPCTSTR message);
void DebugLogFormat(LPCTSTR pszFormat, ...);


//////////////////////////////////////////////////////////////////////


// Processor register names
extern const TCHAR* REGISTER_NAME[8];

const int UKNC_SCREEN_WIDTH = 640;
const int UKNC_SCREEN_HEIGHT = 288;


void PrintOctalValue(TCHAR* buffer, WORD value);
void PrintHexValue(TCHAR* buffer, WORD value);
void PrintBinaryValue(TCHAR* buffer, WORD value);
bool ParseOctalValue(const char* text, uint16_t* pValue);

void CopyTextToClipboard(LPCTSTR text);

TCHAR Translate_KOI8R(BYTE ch);

BOOL ShowSaveDialog(HWND hwndOwner, LPCTSTR strTitle, LPCTSTR strFilter, LPCTSTR strDefExt, TCHAR* bufFileName);
BOOL ShowOpenDialog(HWND hwndOwner, LPCTSTR strTitle, LPCTSTR strFilter, TCHAR* bufFileName);

LPCTSTR GetFileNameFromFilePath(LPCTSTR lpfilepath);


//////////////////////////////////////////////////////////////////////
