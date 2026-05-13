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

// FILE: ThreadUtils.cpp //////////////////////////////////////////////////////
// GameSpy thread utils
// Author: Matthew D. Campbell, July 2002

#include "PreRTS.h"	// This must go first in EVERY cpp file int the GameEngine
#include <unicode/ustring.h>

//-------------------------------------------------------------------------

std::u16string MultiByteToWideCharSingleLine( const char *orig )
{
	Int len;
	UErrorCode err = U_ZERO_ERROR;
	u_strFromUTF8(NULL, 0, &len, orig, -1, &err); // get length
	if (err != U_ZERO_ERROR)
	{
		return std::u16string();
	}

	WideChar *dest = NEW WideChar[len+1];

	// MultiByteToWideChar(CP_UTF8, 0, orig, -1, dest, len);
	u_strFromUTF8(dest, len, NULL, orig, -1, &err);
	if (err != U_ZERO_ERROR)
	{
		delete[] dest;
		return std::u16string();
	}
	WideChar *c = NULL;
	do
	{
		c = u_strchr(dest, u'\n');
		if (c)
		{
			*c = u' ';
		}
	}
	while ( c != NULL );
	do
	{
		c = u_strchr(dest, u'\r');
		if (c)
		{
			*c = u' ';
		}
	}
	while ( c != NULL );

	dest[len] = 0;
	std::u16string ret = dest;
	delete[] dest;
	return ret;
}

std::string WideCharStringToMultiByte( const WideChar *orig )
{
	std::string ret;
	// Int len = WideCharToMultiByte( CP_UTF8, 0, orig, wcslen(orig), NULL, 0, NULL, NULL ) + 1;
	Int len;
	UErrorCode err = U_ZERO_ERROR;
	u_strToUTF8(NULL, 0, &len, orig, -1, &err) + 1;
	if (len > 0 && err == U_ZERO_ERROR)
	{
		char *dest = NEW char[len];
		// WideCharToMultiByte( CP_UTF8, 0, orig, -1, dest, len, NULL, NULL );
		u_strToUTF8(dest, len, NULL, orig, -1, &err);
		if (err != U_ZERO_ERROR)
		{
			delete[] dest;
			return std::string();
		}
		dest[len-1] = 0;
		ret = dest;
		delete dest;
	}
	return ret;
}

//-------------------------------------------------------------------------

