/*
**	Command & Conquer Generals(tm)
**	Copyright 2025 Electronic Arts Inc.
**
**	This program is free software: you can redistribute it and/or modify
**	it under the terms of the GNU General Public License as published by
**	the Free Software Foundation, either version 3 of the License, or
**	(at your option) any later version.
**
**	This program is distributed in the hope that it will be useful,
**	but WITHOUT ANY WARRANTY; without even the implied warranty of
**	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**	GNU General Public License for more details.
**
**	You should have received a copy of the GNU General Public License
**	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

////////////////////////////////////////////////////////////////////////////////
//																																						//
//  (c) 2001-2003 Electronic Arts Inc.																				//
//																																						//
////////////////////////////////////////////////////////////////////////////////

// Win32OSDisplay.cpp //////////////////////////////////////
// John McDonald, December 2002
////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Common/OSDisplay.h"

#include "Common/SubsystemInterface.h"
#include "Common/STLTypedefs.h"
#include "Common/AsciiString.h"
#include "Common/SystemInfo.h"
#include "Common/UnicodeString.h"
#include "GameClient/GameText.h"

#ifdef _INTERNAL
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif


extern HWND ApplicationHWnd;

//-------------------------------------------------------------------------------------------------
static void RTSFlagsToOSFlags(UnsignedInt buttonFlags, UnsignedInt otherFlags, UnsignedInt& outWindowsFlags)
{
	outWindowsFlags = 0;

	if (BitTest(buttonFlags, OSDBT_OK)) {
		outWindowsFlags |= MB_OK;
	}
	
	if (BitTest(buttonFlags, OSDBT_CANCEL)) {
		outWindowsFlags |= MB_OKCANCEL;
	}

	//-----------------------------------------------------------------------------------------------
	if (BitTest(otherFlags, OSDOF_SYSTEMMODAL)) {
		outWindowsFlags |= MB_SYSTEMMODAL;
	}

	if (BitTest(otherFlags, OSDOF_APPLICATIONMODAL)) {
		outWindowsFlags |= MB_APPLMODAL;
	}

	if (BitTest(otherFlags, OSDOF_TASKMODAL)) {
		outWindowsFlags |= MB_TASKMODAL;
	}

	if (BitTest(otherFlags, OSDOF_EXCLAMATIONICON)) {
		outWindowsFlags |= MB_ICONEXCLAMATION;
	}

	if (BitTest(otherFlags, OSDOF_INFORMATIONICON)) {
		outWindowsFlags |= MB_ICONINFORMATION;
	}

	if (BitTest(otherFlags, OSDOF_ERRORICON)) {
		outWindowsFlags |= MB_ICONERROR;
	}

	if (BitTest(otherFlags, OSDOF_STOPICON)) {
		outWindowsFlags |= MB_ICONSTOP;
	}

}

//-------------------------------------------------------------------------------------------------
OSDisplayButtonType OSDisplayWarningBox(AsciiString p, AsciiString m, UnsignedInt buttonFlags, UnsignedInt otherFlags)
{
	if (!TheGameText) {
		return OSDBT_ERROR;
	}

	UnicodeString promptStr = TheGameText->fetch(p);
	UnicodeString mesgStr = TheGameText->fetch(m);

	UnsignedInt windowsOptionsFlags = 0;
	RTSFlagsToOSFlags(buttonFlags, otherFlags, windowsOptionsFlags);
	
	// @todo Make this return more than just ok/cancel - jkmcd
	// (we need a function to translate back the other way.)
	Int returnResult = 0;
	if (TheSystemIsUnicode) 
	{
		returnResult = ::MessageBoxW(NULL, mesgStr.str(), promptStr.str(), windowsOptionsFlags);
	} 
	else 
	{
		// However, if we're using the default version of the message box, we need to 
		// translate the string into an AsciiString
		AsciiString promptA, mesgA;
		promptA.translate(promptStr);
		mesgA.translate(mesgStr);
		//Make sure main window is not TOP_MOST
		::SetWindowPos(ApplicationHWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		returnResult = ::MessageBoxA(NULL, mesgA.str(), promptA.str(), windowsOptionsFlags);
	}

	if (returnResult == IDOK) {
		return OSDBT_OK;
	} 

	return OSDBT_CANCEL;
}
