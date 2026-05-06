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

#pragma once

#ifndef __SDL3BIGFILESYSTEM_H
#define __SDL3BIGFILESYSTEM_H

#include "Common/ArchiveFileSystem.h"

class SDL3BIGFileSystem : public ArchiveFileSystem
{
public:
	SDL3BIGFileSystem();
	virtual ~SDL3BIGFileSystem();

	virtual void init(void);
	virtual void update(void);
	virtual void reset(void);
	virtual void postProcessLoad(void);

	virtual void closeAllArchiveFiles(void);

	virtual ArchiveFile *openArchiveFile(const Char *filename);
	virtual void closeArchiveFile(const Char *filename);
	virtual void closeAllFiles(void);

	virtual Bool loadBigFilesFromDirectory(AsciiString dir, AsciiString fileMask, Bool overwrite = FALSE);
};

#endif // __SDL3BIGFILESYSTEM_H