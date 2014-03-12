#ifdef WIN32

#include <windows.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "wcxhead.h"

#include "RappelzDataManager.h"

#define EXPORT __export __stdcall

typedef struct {
	RPZDATAHANDLE rappelzData;
	RPZFILERECORD *currentRecord;
	unsigned long int currentIndexFile;
} TCRPZHANDLE;

tProcessDataProc pProcessDataProc;	//progress callBack

TCRPZHANDLE *lastOpenedArchive;	//avoid slow opening when navigating in an archive (TC call severals times OpenArchive/CloseArchive when user is working with 1 archive)

HANDLE EXPORT OpenArchive (tOpenArchiveData *ArchiveData) {
	TCRPZHANDLE *hdl;
	int error;
	char debug[300];

	if(lastOpenedArchive) {
		if(!strcmp(lastOpenedArchive->rappelzData->data00[0], ArchiveData->ArcName)) {	//TC want to reopen last opened archive
			OutputDebugString("Using last archive\n");
			lastOpenedArchive->currentRecord = lastOpenedArchive->rappelzData->dataList;
			lastOpenedArchive->currentIndexFile = 0;
			return lastOpenedArchive;
		} else {
			closeRappelzData(lastOpenedArchive->rappelzData);
			free(lastOpenedArchive);
			lastOpenedArchive = NULL;
		}
	}

	
	sprintf(debug, "Opening file %s with open mode %d\n", ArchiveData->ArcName, ArchiveData->OpenMode);
	OutputDebugString(debug);

	
	hdl = (TCRPZHANDLE*) malloc(sizeof(TCRPZHANDLE));

	hdl->rappelzData = openRappelzData(ArchiveData->ArcName, &error);
	if(!hdl->rappelzData) {
		sprintf(debug, "ERROR no %d\n", ArchiveData->ArcName, ArchiveData->OpenMode, error);
		switch(error) {
			case ENOMEM: ArchiveData->OpenResult = E_NO_MEMORY; break;
			default: ArchiveData->OpenResult = E_EOPEN; break;
		}
		free(hdl);
		OutputDebugString(debug);
		return NULL;
	}

	hdl->currentRecord = hdl->rappelzData->dataList;
	hdl->currentIndexFile = 0;

	lastOpenedArchive = hdl;
	return hdl;
}

int EXPORT ReadHeader (HANDLE hArcData, tHeaderData *HeaderData) {
	TCRPZHANDLE *hdl = (TCRPZHANDLE*)hArcData;

	if(!hdl->currentRecord) return E_END_ARCHIVE;
	
	strcpy(HeaderData->ArcName, hdl->rappelzData->data00[0]);
	strcpy(HeaderData->FileName, hdl->currentRecord->filename);
	HeaderData->Flags = 0; //what to do here ???
	HeaderData->UnpSize = HeaderData->PackSize = hdl->currentRecord->dataSize;
	HeaderData->HostOS = 0;
	HeaderData->FileCRC = 0;
	HeaderData->UnpVer = 1;
	HeaderData->FileAttr = 0;

	return 0;
}

void progressInfoCallback(char *filename, unsigned long processed, unsigned long totalSize) {
	if(pProcessDataProc && (totalSize > 100)) {
		pProcessDataProc(filename, -(processed/(totalSize/100)));
	}
}

int EXPORT ProcessFile (HANDLE hArcData, int Operation, char *DestPath, char *DestName) {
	TCRPZHANDLE *hdl = (TCRPZHANDLE*)hArcData;
	char fullFilename[300];
	int error;
	char debug[300];
	
	if(!hdl->currentRecord) return E_END_ARCHIVE;

	switch(Operation) {
		case PK_TEST:	//no integry check
		case PK_SKIP:
			break;

		case PK_EXTRACT:
			if(DestPath) sprintf(fullFilename, "%s\\%s", DestPath, DestName);
			else strcpy(fullFilename, DestName);
			sprintf(debug, "Extracting %s in %s\n", hdl->currentRecord->filename, fullFilename);
			OutputDebugString(debug);
			error = unpackRappelzFile(fullFilename, hdl->rappelzData->data00[hdl->currentRecord->dataFileID], hdl->currentRecord->beginAddress, hdl->currentRecord->dataSize, progressInfoCallback);
			if(hdl->rappelzData->fileNum > 100) pProcessDataProc(hdl->rappelzData->data00[0], -(hdl->currentIndexFile/(hdl->rappelzData->fileNum/100)) - 1000);
			OutputDebugString("Extrating Done\n");
			switch(error) {
				case 0: break;
				case ENOMEM: return E_NO_MEMORY;
				case ENOENT: return E_EOPEN;
				default: return E_BAD_ARCHIVE;
			}
			break;
	}
	
	hdl->currentRecord = hdl->currentRecord->next;
	hdl->currentIndexFile++;

	return 0;
}


int EXPORT CloseArchive (HANDLE hArcData) {
	TCRPZHANDLE *hdl = (TCRPZHANDLE*)hArcData;
	char debug[300];

	sprintf(debug, "Closing data archive %s\n", hdl->rappelzData->data00[0]);
	OutputDebugString(debug);

	hdl->currentRecord = hdl->rappelzData->dataList;

	/*closeRappelzData(hdl->rappelzData);
	free(hdl);*/
	//do nothing, keep open last archive, OpenArchive will close it if TC want to open another archive

	return 0;
}

void EXPORT SetChangeVolProc(HANDLE hArcData, tChangeVolProc pChangeVolProc1) {
}

void EXPORT SetProcessDataProc (HANDLE hArcData, tProcessDataProc pPDProc) {
	pProcessDataProc = pPDProc;
}


int EXPORT GetPackerCaps() {
	return PK_CAPS_SEARCHTEXT | PK_CAPS_NEW | PK_CAPS_MODIFY | PK_CAPS_MULTIPLE | PK_CAPS_DELETE;
}

int EXPORT PackFiles(char *PackedFile, char *SubPath, char *SrcPath, char *AddList, int Flags) {
	RPZDATAHANDLE rappelzData = NULL;
	int closeArchiveAtTheEnd = 0;
	char fullFilePath[200];
	
	if(lastOpenedArchive) {
		if(!strcmp(lastOpenedArchive->rappelzData->data00[0], PackedFile)) {	//use already opened file
			OutputDebugString("Using last archive\n");
			rappelzData = lastOpenedArchive->rappelzData;
		}
	}

	if(!rappelzData) {
		rappelzData = openRappelzData(PackedFile, NULL);
		if(!rappelzData) return E_EOPEN;
		closeArchiveAtTheEnd = 1;
	}

	while(*AddList) {
		sprintf(fullFilePath, "%s\\%s", SrcPath, AddList);
		OutputDebugString("Adding to archive ");
		OutputDebugString(fullFilePath);
		addRappelzFile(rappelzData, strrchr(fullFilePath, '\\')+1, fullFilePath, AM_UpdateOrAdd);
		while(*AddList) AddList++;
		AddList++;
	}

	saveRappelzData(rappelzData, PackedFile);

	if(closeArchiveAtTheEnd) closeRappelzData(rappelzData);

	return 0;
}

int EXPORT DeleteFiles (char *PackedFile, char *DeleteList) {
	RPZDATAHANDLE rappelzData = NULL;
	int closeArchiveAtTheEnd = 0;
	
	if(lastOpenedArchive) {
		if(!strcmp(lastOpenedArchive->rappelzData->data00[0], PackedFile)) {	//use already opened file
			OutputDebugString("Using last archive\n");
			rappelzData = lastOpenedArchive->rappelzData;
		}
	}

	if(!rappelzData) {
		rappelzData = openRappelzData(PackedFile, NULL);
		if(!rappelzData) return E_EOPEN;
		closeArchiveAtTheEnd = 1;
	}

	while(*DeleteList) {
		OutputDebugString("Removing from archive ");
		OutputDebugString(DeleteList);
		removeRappelzFile(rappelzData, DeleteList);
		while(*DeleteList) DeleteList++;
		DeleteList++;
	}

	saveRappelzData(rappelzData, PackedFile);

	if(closeArchiveAtTheEnd) closeRappelzData(rappelzData);

	return 0;
}

/*
void EXPORT ConfigurePacker (HWND Parent, HINSTANCE DllInstance);

int EXPORT StartMemPack (int Options, char *FileName);
int EXPORT PackToMem (int hMemPack,char* BufIn,int InLen,int* Taken,char* BufOut, int OutLen,int* Written,int SeekBy);
int EXPORT DoneMemPack (int hMemPack);
BOOL EXPORT CanYouHandleThisFile (char *FileName);
*/



BOOL WINAPI DllMain(HINSTANCE hInst, DWORD fdwReason, LPVOID lpVoid) {
    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
            // attach to process
            // return FALSE to fail DLL load
			lastOpenedArchive = NULL;
			pProcessDataProc = NULL;
            break;

        case DLL_PROCESS_DETACH:
            // detach from process
			if(lastOpenedArchive) closeRappelzData(lastOpenedArchive->rappelzData);
			free(lastOpenedArchive);
			lastOpenedArchive = NULL;
            break;

        case DLL_THREAD_ATTACH:
            // attach to thread
            break;

        case DLL_THREAD_DETACH:
            // detach from thread
            break;
    }
    return TRUE; // succesful
}

#endif
