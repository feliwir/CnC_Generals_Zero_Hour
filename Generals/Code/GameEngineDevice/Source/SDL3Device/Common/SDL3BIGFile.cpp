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

#include "SDL3Device/Common/SDL3BIGFile.h"

#include "Common/GameMemory.h"
#include "Common/LocalFileSystem.h"
#include "Common/RAMFile.h"
#include "Common/StreamingArchiveFile.h"

SDL3BIGFile::SDL3BIGFile() {}

SDL3BIGFile::~SDL3BIGFile() {}

File *SDL3BIGFile::openFile(const Char *filename, Int access)
{
	const ArchivedFileInfo *fileInfo = getArchivedFileInfo(AsciiString(filename));

	if (fileInfo == NULL) {
		return NULL;
	}

	RAMFile *ramFile = NULL;

	if (BitTest(access, File::STREAMING)) {
		ramFile = newInstance(StreamingArchiveFile);
	} else {
		ramFile = newInstance(RAMFile);
	}

	ramFile->deleteOnClose();
	if (ramFile->openFromArchive(m_file, fileInfo->m_filename, fileInfo->m_offset, fileInfo->m_size) == FALSE) {
		ramFile->close();
		ramFile = NULL;
		return NULL;
	}

	if ((access & File::WRITE) == 0) {
		return ramFile;
	}

	File *localFile = TheLocalFileSystem->openFile(filename, access);
	if (localFile != NULL) {
		ramFile->copyDataToFile(localFile);
	}

	ramFile->close();
	ramFile = NULL;

	return localFile;
}

void SDL3BIGFile::closeAllFiles(void) {}

AsciiString SDL3BIGFile::getName(void)
{
	return m_name;
}

AsciiString SDL3BIGFile::getPath(void)
{
	return m_path;
}

void SDL3BIGFile::setSearchPriority(Int new_priority) {}

void SDL3BIGFile::close(void) {}

Bool SDL3BIGFile::getFileInfo(const AsciiString &filename, FileInfo *fileInfo) const
{
	const ArchivedFileInfo *tempFileInfo = getArchivedFileInfo(filename);

	if (tempFileInfo == NULL) {
		return FALSE;
	}

	TheLocalFileSystem->getFileInfo(AsciiString(m_file->getName()), fileInfo);

	fileInfo->sizeHigh = 0;
	fileInfo->sizeLow = tempFileInfo->m_size;

	return TRUE;
}
