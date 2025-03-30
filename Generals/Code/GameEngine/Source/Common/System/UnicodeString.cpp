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

// FILE: UnicodeString.cpp 
//-----------------------------------------------------------------------------
//                                                                          
//                       Westwood Studios Pacific.                          
//                                                                          
//                       Confidential Information					         
//                Copyright (C) 2001 - All Rights Reserved                  
//                                                                          
//-----------------------------------------------------------------------------
//
// Project:    RTS3
//
// File name:  UnicodeString.cpp
//
// Created:    Steven Johnson, October 2001
//
// Desc:       General-purpose string classes
//
//-----------------------------------------------------------------------------
///////////////////////////////////////////////////////////////////////////////
#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine

#include "Common/CriticalSection.h"

#ifdef _INTERNAL
// for occasional debugging...
//#pragma optimize("", off)
//#pragma MESSAGE("************************************** WARNING, optimization disabled for debugging purposes")
#endif

// -----------------------------------------------------

/*static*/ UnicodeString UnicodeString::TheEmptyString;

// -----------------------------------------------------
#ifdef _DEBUG
void UnicodeString::validate() const
{
	if (!m_data) return;
	DEBUG_ASSERTCRASH(m_data->m_refCount > 0, ("m_refCount is zero"));
	DEBUG_ASSERTCRASH(m_data->m_numCharsAllocated > 0, ("m_numCharsAllocated is zero"));
	DEBUG_ASSERTCRASH(wcslen(m_data->peek())+1 <= m_data->m_numCharsAllocated,("str is too long for storage"));
}
#endif

int char16_compare(const WideChar *s1, const WideChar *s2) {
	auto s1Len = std::char_traits<WideChar>::length(s1);
	auto s2Len = std::char_traits<WideChar>::length(s2);
	auto count = min(s1Len, s2Len);
	return std::char_traits<WideChar>::compare(s1, s2, count);
}

int char16_compare_no_case(const WideChar *s1, const WideChar *s2) {
	auto s1Len = std::char_traits<WideChar>::length(s1);
	auto s2Len = std::char_traits<WideChar>::length(s2);
	auto count = min(s1Len, s2Len);

	while (count-- != 0)
	{
		WideChar c1 = *s1;
		WideChar c2 = *s2;
		if (c1 >= u'a' && c1 <= u'z') {
			c1 -= 32;
		}
		if (c2 >= u'a' && c2 <= u'z') {
			c2 -= 32;
		}
		if (c1 < c2)
			return -1;
		if (c1 > c2)
			return 1;
		++s1;
		++s2;
	}
	return s1Len > s2Len ? 1 : (s1Len < s2Len ? -1 : 0);
}

// -----------------------------------------------------
UnicodeString::UnicodeString(const UnicodeString& stringSrc) : m_data(stringSrc.m_data)
{
	ScopedCriticalSection scopedCriticalSection(TheUnicodeStringCriticalSection);
	if (m_data)
		++m_data->m_refCount;
	validate();
}

// -----------------------------------------------------
void UnicodeString::ensureUniqueBufferOfSize(int numCharsNeeded, Bool preserveData, const WideChar* strToCopy, const WideChar* strToCat)
{
	validate();

	if (m_data &&
			m_data->m_refCount == 1 &&
			m_data->m_numCharsAllocated >= numCharsNeeded)
	{
		// no buffer manhandling is needed (it's already large enough, and unique to us)
		if (strToCopy)
			wcscpy(m_data->peek(), strToCopy);
		if (strToCat)
			wcscat(m_data->peek(), strToCat);
		return;
	}

	int minBytes = sizeof(UnicodeStringData) + numCharsNeeded*sizeof(WideChar);
	if (minBytes > MAX_LEN)
		throw ERROR_OUT_OF_MEMORY;

	int actualBytes = TheDynamicMemoryAllocator->getActualAllocationSize(minBytes);
	UnicodeStringData* newData = (UnicodeStringData*)TheDynamicMemoryAllocator->allocateBytesDoNotZero(actualBytes, "STR_UnicodeString::ensureUniqueBufferOfSize");
	newData->m_refCount = 1;
	newData->m_numCharsAllocated = (actualBytes - sizeof(UnicodeStringData))/sizeof(WideChar);
#if defined(_DEBUG) || defined(_INTERNAL)
	newData->m_debugptr = newData->peek();	// just makes it easier to read in the debugger
#endif

	if (m_data && preserveData)
		wcscpy(newData->peek(), m_data->peek());
	else
		newData->peek()[0] = 0;

	// do these BEFORE releasing the old buffer, so that self-copies
	// or self-cats will work correctly.
	if (strToCopy)
		wcscpy(newData->peek(), strToCopy);
	if (strToCat)
		wcscat(newData->peek(), strToCat);

	releaseBuffer();
	m_data = newData;

	validate();
}


// -----------------------------------------------------
void UnicodeString::releaseBuffer()
{
	ScopedCriticalSection scopedCriticalSection(TheUnicodeStringCriticalSection);

	validate();
	if (m_data)
	{
		if (--m_data->m_refCount == 0)
		{
			TheDynamicMemoryAllocator->freeBytes(m_data);
		}
		m_data = 0;
	}
}

// -----------------------------------------------------
UnicodeString::UnicodeString(const WideChar* s) : m_data(0)
{
	int len = wcslen(s);
	if (len)
	{
		ensureUniqueBufferOfSize(len + 1, false, s, NULL);
	}
	validate();
}

// -----------------------------------------------------
void UnicodeString::set(const UnicodeString& stringSrc)
{
	ScopedCriticalSection scopedCriticalSection(TheUnicodeStringCriticalSection);

	validate();
	if (&stringSrc != this)
	{
		releaseBuffer();
		m_data = stringSrc.m_data;
		if (m_data)
			++m_data->m_refCount;
	}
	validate();
}

// -----------------------------------------------------
void UnicodeString::set(const WideChar* s)
{
	validate();
	if (!m_data || s != peek())
	{
		int len = s ? wcslen(s) : 0;
		if (len)
		{
			ensureUniqueBufferOfSize(len + 1, false, s, NULL);
		}
		else
		{
			releaseBuffer();
		}
	}
	validate();
}

// -----------------------------------------------------
void UnicodeString::set(const uint16_t* s)
{
	validate();
	clear();
	// Extreme hack that works for ASCII
	Int i;
	for (i = 0; s[i] != '\0'; i++)
		concat((WideChar)s[i]);
	validate();
}

// -----------------------------------------------------
WideChar* UnicodeString::getBufferForRead(Int len)
{
	validate();
	DEBUG_ASSERTCRASH(len>0, ("No need to allocate 0 len strings."));
	ensureUniqueBufferOfSize(len + 1, false, NULL, NULL);
	validate();
	return peek();
}

// -----------------------------------------------------
void UnicodeString::translate(const AsciiString& stringSrc)
{
	validate();
	/// @todo srj put in a real translation here; this will only work for 7-bit ascii
	clear();
	Int len = stringSrc.getLength();
	for (Int i = 0; i < len; i++)
		concat((WideChar)stringSrc.getCharAt(i));
	validate();
}

// -----------------------------------------------------
void UnicodeString::concat(const WideChar* s)
{
	validate();
	int addlen = wcslen(s);
	if (addlen == 0)
		return;	// my, that was easy

	if (m_data)
	{
		ensureUniqueBufferOfSize(getLength() + addlen + 1, true, NULL, s);
	}
	else
	{
		set(s);
	}
	validate();
}

// -----------------------------------------------------
void UnicodeString::trim()
{
	validate();

	if (m_data)
	{
		const WideChar *c = peek();

		//	Strip leading white space from the string.
		while (c && iswspace(*c))
		{
			c++;
		}
		if (c != peek())
		{
			set(c);
		}

		if (m_data) // another check, because the previous set() could erase m_data
		{
			//	Clip trailing white space from the string.
			int len = wcslen(peek());
			for (int index = len-1; index >= 0; index--)
			{
				if (iswspace(getCharAt(index)))
				{
					removeLastChar();
				}
				else
				{
					break;
				}
			}
		}
	}
	validate();
}

// -----------------------------------------------------
void UnicodeString::removeLastChar()
{
	validate();
	if (m_data)
	{
		int len = wcslen(peek());
		if (len > 0)
		{
			ensureUniqueBufferOfSize(len+1, true, NULL, NULL);
			peek()[len - 1] = 0;
		}
	}
	validate();
}

// -----------------------------------------------------
void UnicodeString::format(UnicodeString format, ...)
{
	validate();
	va_list args;
  va_start(args, format);
	format_va(format, args);
  va_end(args);
	validate();
}

// -----------------------------------------------------
void UnicodeString::format(const WideChar* format, ...)
{
	validate();
	va_list args;
  va_start(args, format);
	format_va(format, args);
  va_end(args);
	validate();
}

// -----------------------------------------------------
void UnicodeString::format_va(const UnicodeString& format, va_list args)
{
	validate();
	WideChar buf[MAX_FORMAT_BUF_LEN];
  if (vswprintf(buf, sizeof(buf)/sizeof(WideChar)-1, format.str(), args) < 0)
			throw ERROR_OUT_OF_MEMORY;
	set(buf);
	validate();
}

// -----------------------------------------------------
void UnicodeString::format_va(const WideChar* format, va_list args)
{
	validate();
	WideChar buf[MAX_FORMAT_BUF_LEN];
  if (vswprintf(buf, sizeof(buf)/sizeof(WideChar)-1, format, args) < 0)
			throw ERROR_OUT_OF_MEMORY;
	set(buf);
	validate();
}

//-----------------------------------------------------------------------------
Bool UnicodeString::nextToken(UnicodeString* tok, UnicodeString delimiters)
{
	if (this->isEmpty() || tok == this)
		return false;

	if (delimiters.isEmpty())
		delimiters = UnicodeString(L" \t\n\r");

	Int offset;

	offset = wcsspn(peek(), delimiters.str());
	WideChar* start = peek() + offset;

	offset = wcscspn(start, delimiters.str());
	WideChar* end = start + offset;

	if (end > start)
	{
		Int len = end - start;
		WideChar* tmp = tok->getBufferForRead(len + 1);
		memcpy(tmp, start, len*2);
		tmp[len] = 0;

		this->set(end);

		return true;
	}
	else
	{
		this->clear();
		tok->clear();
		return false;
	}
}
