#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "RappelzDataManager.h"

int patternMatch(char *buffer, char *pattern) {
	char *p1, *p2;

	p1 = buffer;
	p2 = pattern;

	while(1) {
		if(*p2 == '*') {
			if(*(p2+1) == *p1) {
				if(*p1 == 0) return 1;
				else {
					p2+=2;
					p1++;
				}
			} else if(*p1 == 0) return 0;
			else p1++;
		} else {
			if(*p1 != *p2) return 0;
			else if(*p1 == 0) return 1;
			else {
				p1++;
				p2++;
			}
		}
	}
}

#define OPT_ACTION_LIST	0x01
#define OPT_ACTION_EXTRACT 0x02
#define OPT_VERBOSE_OUTPUT 0x04
#define OPT_ACTION_UPDATE 0x08
#define OPT_ACTION_ADD 0x10
#define OPT_ACTION_RENAME 0x20

int main(int argc, char *argv[]) {
	RPZFILERECORD *p;
	RPZDATAHANDLE dataFile;

	char *pattern = "*";
	char *inputDir = ".";
	char *outputDir = ".";
	char *renameFileName = NULL;
	char *renameNewFileName = NULL;
	char *chrPtr;

	char data000path[80];
	char fullDataname[80];
	char fullFilename[80];

	int i, flags, result;


	flags = 0;
	for(i=1; i<argc; i++) {
		if(!strcmp(argv[i], "--list")) {
			flags |= OPT_ACTION_LIST;
			flags &= ~OPT_ACTION_UPDATE;
			flags &= ~OPT_ACTION_ADD;
		} else if(!strcmp(argv[i], "--extract")) {
			flags |= OPT_ACTION_EXTRACT;
			flags &= ~OPT_ACTION_UPDATE;
			flags &= ~OPT_ACTION_ADD;
		} else if(!strcmp(argv[i], "--update")) {	//update a file with size <= oldSize, file must already exist
			flags |= OPT_ACTION_UPDATE;
			flags &= ~OPT_ACTION_ADD;
			flags &= ~OPT_ACTION_LIST;
			flags &= ~OPT_ACTION_EXTRACT;
		} else if(!strcmp(argv[i], "--add")) {
			flags |= OPT_ACTION_ADD;
			flags &= ~OPT_ACTION_UPDATE;
			flags &= ~OPT_ACTION_LIST;
			flags &= ~OPT_ACTION_EXTRACT;
		} else if(!strcmp(argv[i], "--force-add")) {
			flags |= OPT_ACTION_ADD | OPT_ACTION_UPDATE;
			flags &= ~OPT_ACTION_LIST;
			flags &= ~OPT_ACTION_EXTRACT;
		} else if(!strcmp(argv[i], "--rename")) {
			if((i+2) < argc && argv[i+1] && argv[i+2]) {
				flags &= OPT_VERBOSE_OUTPUT;
				flags |= OPT_ACTION_RENAME;
				renameFileName = argv[i+1];
				renameNewFileName = argv[i+2];
			}
		} else if(!strcmp(argv[i], "--verbose")) {
			flags |= OPT_VERBOSE_OUTPUT;
		} else if(!strcmp(argv[i], "--data-dir")) {
			if((i+1) < argc && argv[i+1]) {
				inputDir = argv[i+1];
				i++;
			}
		} else if(!strcmp(argv[i], "--output-dir")) {
			if((i+1) < argc && argv[i+1]) {
				outputDir = argv[i+1];
				i++;
			}
		} else if(argv[i]) pattern = argv[i];
	}

	sprintf(data000path, "%s/data.000", inputDir);

	if(flags & OPT_VERBOSE_OUTPUT) fprintf(stderr, "Processing %s ...\n", data000path);

	dataFile = openRappelzData(data000path, &result);
	if(!dataFile) printf( "Cannot open data file, Error %s\n", strerror(result));
	else puts("File opened");


	if(flags & OPT_ACTION_RENAME) {
		char choice[10];
		int returnCode;
		do {
			printf("Do you want to rename %s to %s ? [yes, no]\n", renameFileName, renameNewFileName);
			scanf("%9s", choice);
		} while(strncmp(choice, "yes", 3) != 0 && strncmp(choice, "no", 2) != 0);
		if(!strncmp(choice, "yes", 3)) {
			returnCode = renameRappelzFile(dataFile, renameFileName, renameNewFileName);
			if(returnCode) printf( "Cannot rename file %s, Error %s\n", renameFileName, strerror(returnCode));
			else saveRappelzData(dataFile, data000path);
		} else puts("Not renaming");
	} else if(flags & OPT_ACTION_UPDATE || flags & OPT_ACTION_ADD) {
		int returnCode;
		chrPtr = strrchr(pattern, '\\');
		if(!chrPtr) chrPtr = pattern;
		else chrPtr++;
		if(flags & OPT_ACTION_UPDATE)
			returnCode = addRappelzFile(dataFile, chrPtr, pattern, AM_UpdateOnly);
		else returnCode = addRappelzFile(dataFile, chrPtr, pattern, AM_AddOnly);
		if(returnCode) printf( "Cannot update/add file, Error %s\n", strerror(returnCode));
		else saveRappelzData(dataFile, data000path);
	} else {
		if(flags & OPT_ACTION_LIST) printf("Filename\tHash\tData Location\tStart offset\tSize\n");
		p = dataFile->dataList;
		while(p) {
			if(patternMatch(p->filename, pattern)) {
				if(flags & OPT_ACTION_LIST) printf("%s\t\"%s\"\t%s/data.%03d\t%u\t%u\n", p->filename, p->hash, inputDir, p->dataFileID, p->beginAddress, p->dataSize);
				if(flags & OPT_ACTION_EXTRACT) {
					sprintf(fullDataname, "%s/data.%03d", inputDir, p->dataFileID);
					sprintf(fullFilename, "%s/%s", outputDir, p->filename);
					if(flags & OPT_VERBOSE_OUTPUT) fprintf(stderr, "Extracting %s ...\n", fullFilename);
					unpackRappelzFile(fullFilename, fullDataname, p->beginAddress, p->dataSize, NULL);
				}
			}
			p = p->next;
		}
	}

	closeRappelzData(dataFile);
	return 0;
}
