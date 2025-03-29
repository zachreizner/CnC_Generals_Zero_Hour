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
//                                                                            //
//  (c) 2001-2003 Electronic Arts Inc.                                        //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////

// FILE: MiniLog.cpp ///////////////////////////////////////////////////////////
// Alternative logging
// Author: Matthew D. Campbell, January 2003
////////////////////////////////////////////////////////////////////////////////

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine
#include "Common/MiniLog.h"

#ifdef DEBUG_LOGGING

LogClass::LogClass(const char *fname) : m_fname(fname)
{
#if 0
	char buffer[ _MAX_PATH ];
	GetModuleFileName( NULL, buffer, sizeof( buffer ) );
	char *pEnd = buffer + strlen( buffer );
	while( pEnd != buffer )
	{
		if( *pEnd == '\\' )
		{
			*pEnd = 0;
			break;
		}
		pEnd--;
	}
	AsciiString fullPath;
	fullPath.format("%s\\%s", buffer, fname);
	m_fp = fopen(fullPath.str(), "wt");
#endif
}

LogClass::~LogClass()
{
	// if (m_fp)
	// {
	// 	fclose(m_fp);
	// }
}

extern "C" {
void emit_mini_log(const char *fname, const char *buf);
}

void LogClass::log(const char *fmt, ...)
{
	static char buf[1024];
	static Int lastFrame = 0;
	static Int lastIndex = 0;
	if (lastFrame != TheGameLogic->getFrame())
	{
		lastFrame = TheGameLogic->getFrame();
		lastIndex = 0;
	}

	va_list va;
	va_start( va, fmt );
	vsnprintf(buf, 1024, fmt, va );
	buf[1023] = 0;
	va_end( va );

	char *tmp = buf;
	while (tmp && *tmp)
	{
		if (*tmp == '\r' || *tmp == '\n')
		{
			*tmp = ' ';
		}
		++tmp;
	}

	emit_mini_log(m_fname, buf);

	// fprintf(m_fp, "%d:%d %s\n", lastFrame, lastIndex++, buf);
	// fflush(m_fp);
}

#endif // DEBUG_LOGGING
