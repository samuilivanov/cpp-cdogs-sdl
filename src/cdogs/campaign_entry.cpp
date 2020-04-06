/*
 C-Dogs SDL
 A port of the legendary (and fun) action/arcade cdogs.

 Copyright (c) 2013-2014, Cong Xu
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.
 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following disclaimer in the documentation
 and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */
#include "campaign_entry.h"

#include <stdio.h>

#include <cdogs/files.h>
#include <cdogs/map_new.h>
#include <cdogs/mission.h>
#include <cdogs/utils.h>

static bool IsCampaignOK(const char *path, char **buf, int *numMissions) {
	return MapNewScan(path, buf, numMissions) == 0;
}

void CampaignEntryInit(CampaignEntry *entry, const char *title, GameMode mode) {
	memset(entry, 0, sizeof *entry);
	entry->Info = static_cast<char*>(malloc(strlen(title) + 1));
	if (entry->Info == NULL && strlen(title) + 1 > 0) {
		exit(1);
	}
	strcpy(entry->Info, title);
//	CSTRDUP(entry->Info, title);
	entry->Mode = mode;
}
void CampaignEntryCopy(CampaignEntry *dst, CampaignEntry *src) {
	memcpy(dst, src, sizeof *dst);
	if (src->Filename) {
		dst->Filename = static_cast<char*>(malloc(strlen(src->Filename) + 1));
		if (dst->Filename == NULL && strlen(src->Filename) + 1 > 0) {
			exit(1);
		}
		strcpy(dst->Filename, src->Filename);
//		CSTRDUP(dst->Filename, src->Filename);
	}
	if (src->Path) {
		dst->Path = static_cast<char*>(malloc(strlen(src->Path) + 1));
		if (dst->Path == NULL && strlen(src->Path) + 1 > 0) {
			exit(1);
		}
		strcpy(dst->Path, src->Path);
//		CSTRDUP(dst->Path, src->Path);
	}
	if (src->Info) {
		dst->Info = static_cast<char*>(malloc(strlen(src->Info) + 1));
		if (dst->Info == NULL && strlen(src->Info) + 1 > 0) {
			exit(1);
		}
		strcpy(dst->Info, src->Info);
//		CSTRDUP(dst->Info, src->Info);
	}
}
bool CampaignEntryTryLoad(CampaignEntry *entry, const char *path,
		GameMode mode) {
	char *buf;
	int numMissions;
	if (!IsCampaignOK(path, &buf, &numMissions)) {
		return false;
	}
	// cap length of title
	size_t maxLen = 70;
	if (strlen(buf) > maxLen) {
		buf[maxLen] = '\0';
	}
	char title[256];
	sprintf(title, "%s (%d)", buf, numMissions);
	CampaignEntryInit(entry, title, mode);
	entry->Filename = static_cast<char*>(malloc(
			strlen(PathGetBasename(path)) + 1));
	if (entry->Filename == NULL && strlen(PathGetBasename(path)) + 1 > 0) {
		exit(1);
	}
	strcpy(entry->Filename, PathGetBasename(path));
//	CSTRDUP(entry->Filename, PathGetBasename(path));
	// Get relative path for the campaign entry, so when we transmit it to
	// network clients they can load it regardless of install path
	// Note: relative path is counted from data dir
	char pathBuf[CDOGS_PATH_MAX];
	char dataDirBuf[CDOGS_PATH_MAX];
	GetDataFilePath(dataDirBuf, "");
	RelPath(pathBuf, path, dataDirBuf);
	entry->Path = static_cast<char*>(malloc(strlen(pathBuf) + 1));
	if (entry->Path == NULL && strlen(pathBuf) + 1 > 0) {
		exit(1);
	}
	strcpy(entry->Path, pathBuf);
//	CSTRDUP(entry->Path, pathBuf);
	entry->NumMissions = numMissions;
	CFREE(buf);
	return true;
}
void CampaignEntryTerminate(CampaignEntry *entry) {
	CFREE(entry->Filename);
	CFREE(entry->Path);
	CFREE(entry->Info);
	memset(entry, 0, sizeof *entry);
}
