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

#include "SDL3Device/Common/SDL3BIGFileSystem.h"

#include <arpa/inet.h>
#include <strings.h>

#include "Common/ArchiveFile.h"
#include "Common/AudioAffect.h"
#include "Common/File.h"
#include "Common/GameAudio.h"
#include "Common/GameMemory.h"
#include "Common/LocalFileSystem.h"
#include "SDL3Device/Common/SDL3BIGFile.h"

static const char *BIGFileIdentifier = "BIGF";

SDL3BIGFileSystem::SDL3BIGFileSystem() : ArchiveFileSystem() {}

SDL3BIGFileSystem::~SDL3BIGFileSystem() {}

void SDL3BIGFileSystem::init()
{
	DEBUG_ASSERTCRASH(TheLocalFileSystem != NULL, ("TheLocalFileSystem must be initialized before TheArchiveFileSystem."));
	if (TheLocalFileSystem == NULL) {
		return;
	}

	loadBigFilesFromDirectory("", "*.big");
}

void SDL3BIGFileSystem::reset() {}

void SDL3BIGFileSystem::update() {}

void SDL3BIGFileSystem::postProcessLoad() {}

ArchiveFile *SDL3BIGFileSystem::openArchiveFile(const Char *filename)
{
	File *fp = TheLocalFileSystem->openFile(filename, File::READ | File::BINARY);
	AsciiString archiveFileName;
	archiveFileName = filename;
	archiveFileName.toLower();
	Int archiveFileSize = 0;
	Int numLittleFiles = 0;

	ArchiveFile *archiveFile = NEW SDL3BIGFile;

	DEBUG_LOG(("SDL3BIGFileSystem::openArchiveFile - opening BIG file %s\n", filename));

	if (fp == NULL) {
		DEBUG_CRASH(("Could not open archive file %s for parsing", filename));
		return NULL;
	}

	char buffer[1024];
	fp->read(buffer, 4);
	buffer[4] = 0;
	if (strcmp(buffer, BIGFileIdentifier) != 0) {
		DEBUG_CRASH(("Error reading BIG file identifier in file %s", filename));
		fp->close();
		fp = NULL;
		return NULL;
	}

	fp->read(&archiveFileSize, 4);

	DEBUG_LOG(("SDL3BIGFileSystem::openArchiveFile - size of archive file is %d bytes\n", archiveFileSize));

	fp->read(&numLittleFiles, 4);
	numLittleFiles = ntohl(numLittleFiles);

	DEBUG_LOG(("SDL3BIGFileSystem::openArchiveFile - %d are contained in archive\n", numLittleFiles));

	fp->seek(0x10, File::START);
	ArchivedFileInfo *fileInfo = NEW ArchivedFileInfo;

	for (Int i = 0; i < numLittleFiles; ++i) {
		Int filesize = 0;
		Int fileOffset = 0;
		fp->read(&fileOffset, 4);
		fp->read(&filesize, 4);

		filesize = ntohl(filesize);
		fileOffset = ntohl(fileOffset);

		fileInfo->m_archiveFilename = archiveFileName;
		fileInfo->m_offset = fileOffset;
		fileInfo->m_size = filesize;

		Int pathIndex = -1;
		do {
			++pathIndex;
			if (pathIndex >= static_cast<Int>(sizeof(buffer) - 1)) {
				DEBUG_CRASH(("Archive path exceeded parser buffer size."));
				delete fileInfo;
				fp->close();
				return NULL;
			}
			fp->read(buffer + pathIndex, 1);
		} while (buffer[pathIndex] != 0);

		Int filenameIndex = pathIndex;
		while ((buffer[filenameIndex] != '\\') && (buffer[filenameIndex] != '/') && (filenameIndex >= 0)) {
			--filenameIndex;
		}

		fileInfo->m_filename = (char *)(buffer + filenameIndex + 1);
		fileInfo->m_filename.toLower();
		buffer[filenameIndex + 1] = 0;

		AsciiString path;
		path = buffer;

		archiveFile->addFile(path, fileInfo);
	}

	archiveFile->attachFile(fp);

	delete fileInfo;
	fileInfo = NULL;

	return archiveFile;
}

void SDL3BIGFileSystem::closeArchiveFile(const Char *filename)
{
	ArchiveFileMap::iterator it = m_archiveFileMap.find(filename);
	if (it == m_archiveFileMap.end()) {
		return;
	}

	if (strcasecmp(filename, MUSIC_BIG) == 0) {
		TheAudio->stopAudio(AudioAffect_Music);
	}
	DEBUG_ASSERTCRASH(strcasecmp(filename, MUSIC_BIG) == 0,
	                  ("Attempting to close Archive file '%s', need to add code to handle its shutdown correctly.", filename));

	delete (it->second);
	m_archiveFileMap.erase(it);
}

void SDL3BIGFileSystem::closeAllArchiveFiles() {}

void SDL3BIGFileSystem::closeAllFiles() {}

Bool SDL3BIGFileSystem::loadBigFilesFromDirectory(AsciiString dir, AsciiString fileMask, Bool overwrite)
{
	FilenameList filenameList;
	TheLocalFileSystem->getFileListInDirectory(dir, AsciiString(""), fileMask, filenameList, TRUE);

	Bool actuallyAdded = FALSE;
	FilenameListIter it = filenameList.begin();
	while (it != filenameList.end()) {
		ArchiveFile *archiveFile = openArchiveFile((*it).str());

		if (archiveFile != NULL) {
			DEBUG_LOG(("SDL3BIGFileSystem::loadBigFilesFromDirectory - loading %s into the directory tree.\n", (*it).str()));
			loadIntoDirectoryTree(archiveFile, *it, overwrite);
			m_archiveFileMap[(*it)] = archiveFile;
			DEBUG_LOG(("SDL3BIGFileSystem::loadBigFilesFromDirectory - %s inserted into the archive file map.\n", (*it).str()));
			actuallyAdded = TRUE;
		}

		it++;
	}

	return actuallyAdded;
}
