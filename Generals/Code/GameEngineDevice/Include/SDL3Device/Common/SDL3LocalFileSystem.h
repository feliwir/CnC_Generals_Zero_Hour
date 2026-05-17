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

#ifndef __SDL3LOCALFILESYSTEM_H
#define __SDL3LOCALFILESYSTEM_H

#include "Common/LocalFileSystem.h"

class SDL3LocalFileSystem : public LocalFileSystem
{
public:
	SDL3LocalFileSystem();
	virtual ~SDL3LocalFileSystem();

	virtual void init();
	virtual void reset();
	virtual void update();

	virtual File *openFile(const Char *filename, Int access = 0);
	virtual Bool doesFileExist(const Char *filename) const;

	virtual void getFileListInDirectory(const AsciiString &currentDirectory,
	                                    const AsciiString &originalDirectory,
	                                    const AsciiString &searchName,
	                                    FilenameList &filenameList,
	                                    Bool searchSubdirectories) const;
	virtual Bool getFileInfo(const AsciiString &filename, FileInfo *fileInfo) const;

	virtual Bool createDirectory(AsciiString directory);
protected:
	// List of files that we can use for case-insensitive matching.
	FilenameList m_fileList;
};

#endif // __SDL3LOCALFILESYSTEM_H