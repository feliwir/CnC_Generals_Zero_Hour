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

#include "SDL3Device/Common/SDL3LocalFileSystem.h"

#include <SDL3/SDL.h>

#include "Common/AsciiString.h"
#include "Common/GameMemory.h"
#include "SDL3Device/Common/SDL3LocalFile.h"

namespace
{
static bool isSubdirectoryMatch(const char *relativePath)
{
	if (relativePath == NULL) {
		return false;
	}

	while (*relativePath != '\0') {
		if (*relativePath == '/' || *relativePath == '\\') {
			return true;
		}
		++relativePath;
	}

	return false;
}
}

SDL3LocalFileSystem::SDL3LocalFileSystem() : LocalFileSystem() {}

SDL3LocalFileSystem::~SDL3LocalFileSystem() {}

File *SDL3LocalFileSystem::openFile(const Char *filename, Int access)
{
	SDL3LocalFile *file = newInstance(SDL3LocalFile);

	if (strlen(filename) <= 0) {
		return NULL;
	}

	if (access & File::WRITE) {
		AsciiString string;
		string = filename;
		AsciiString token;
		AsciiString dirName;
		string.nextToken(&token, "\\/");
		dirName = token;
		while ((token.find('.') == NULL) || (string.find('.') != NULL)) {
			createDirectory(dirName);
			string.nextToken(&token, "\\/");
			dirName.concat('/');
			dirName.concat(token);
		}
	}

	if (file->open(filename, access) == FALSE) {
		file->close();
		file->deleteInstance();
		file = NULL;
	} else {
		file->deleteOnClose();
	}

	return file;
}

void SDL3LocalFileSystem::update() {}

void SDL3LocalFileSystem::init() {}

void SDL3LocalFileSystem::reset() {}

Bool SDL3LocalFileSystem::doesFileExist(const Char *filename) const
{
	SDL_PathInfo pathInfo;
	if (!SDL_GetPathInfo(filename, &pathInfo)) {
		return FALSE;
	}

	return pathInfo.type == SDL_PATHTYPE_FILE ? TRUE : FALSE;
}

void SDL3LocalFileSystem::getFileListInDirectory(const AsciiString &currentDirectory,
	                                               const AsciiString &originalDirectory,
	                                               const AsciiString &searchName,
	                                               FilenameList &filenameList,
	                                               Bool searchSubdirectories) const
{
	AsciiString rootPath = originalDirectory;
	if (rootPath.isEmpty()) {
		rootPath = "./";
	}
	rootPath.concat(currentDirectory);

	int count = 0;
	char **matches = SDL_GlobDirectory(rootPath.str(), searchName.str(), SDL_GLOB_CASEINSENSITIVE, &count);
	if (matches == NULL) {
		return;
	}

	for (int i = 0; i < count; ++i) {
		const char *match = matches[i];
		if (!searchSubdirectories && isSubdirectoryMatch(match)) {
			continue;
		}

		AsciiString fullName = rootPath;
		fullName.concat(match);
		if (filenameList.find(fullName) == filenameList.end()) {
			filenameList.insert(fullName);
		}
	}

	SDL_free(matches);
}

Bool SDL3LocalFileSystem::getFileInfo(const AsciiString &filename, FileInfo *fileInfo) const
{
	if (fileInfo == NULL) {
		return FALSE;
	}

	SDL_PathInfo pathInfo;
	if (!SDL_GetPathInfo(filename.str(), &pathInfo)) {
		return FALSE;
	}

	fileInfo->sizeHigh = static_cast<Int>((pathInfo.size >> 32u) & 0xFFFFFFFFu);
	fileInfo->sizeLow = static_cast<Int>(pathInfo.size & 0xFFFFFFFFu);

	const Uint64 modifyTime = static_cast<Uint64>(pathInfo.modify_time);
	fileInfo->timestampHigh = static_cast<Int>((modifyTime >> 32u) & 0xFFFFFFFFu);
	fileInfo->timestampLow = static_cast<Int>(modifyTime & 0xFFFFFFFFu);

	return TRUE;
}

Bool SDL3LocalFileSystem::createDirectory(AsciiString directory)
{
	if (directory.getLength() <= 0) {
		return FALSE;
	}

	return SDL_CreateDirectory(directory.str()) ? TRUE : FALSE;
}
