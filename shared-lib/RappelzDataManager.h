#ifndef RAPPELZDATALIST_H
#define RAPPELZDATALIST_H

typedef enum {
	AM_UpdateOrAdd,
	AM_UpdateOnly,
	AM_AddOnly
} eAddMode;

struct rpzFileRecord {
	char filename[259];
	char hash[257];
	
	int dataFileID;
	unsigned long beginAddress;
	unsigned long dataSize;

	struct rpzFileRecord *next;
};
typedef struct rpzFileRecord RPZFILERECORD;

typedef struct rpzDataHStruct{
	RPZFILERECORD *dataList;
	char data00[9][100];	//data.000 to data.008
	unsigned long int fileNum;
	unsigned long int allocatedRecords;
} *RPZDATAHANDLE;

typedef struct {
	RPZFILERECORD *fileRecord;
	int currentOffset;
	int isCrypted;
	FILE* dataFile;
} RPZFILEHANDLE;

typedef void (*ProgressInfoCallbackType)(const char *filename, unsigned long processed, unsigned long totalSize);


#ifdef __cplusplus
extern "C" {
#endif

RPZDATAHANDLE openRappelzData(const char *data000filename, int *error);
int saveRappelzData(RPZDATAHANDLE hdl, const char *data000filename);
int retrieveRappelzFile(RPZDATAHANDLE hdl, const char *filename, const char *filePath);
int addRappelzFile(RPZDATAHANDLE hdl, const char *filename, const char *filePath, eAddMode updateMode);
int removeRappelzFile(RPZDATAHANDLE hdl, const char *filename);
int renameRappelzFile(RPZDATAHANDLE hdl, const char *filename, const char *newFilename);
void closeRappelzData(RPZDATAHANDLE hdl);

int unpackRappelzFile(const char *filename, const char *dataLocation, unsigned long startOffset, unsigned long size, ProgressInfoCallbackType progressCallback);
int packRappelzFile(const char *filename, const char *dataLocation, unsigned long startOffset, unsigned long size, ProgressInfoCallbackType progressCallback);

int retrievePackedFiles(RPZDATAHANDLE hdl);
void freeRecordData(RPZFILERECORD *list);

RPZFILEHANDLE * rpzopen(RPZDATAHANDLE hdl, const char* filename, const char* mode);
int rpzwrite(const void *buffer, int size, RPZFILEHANDLE *file);
int rpzread(void *buffer, int size, RPZFILEHANDLE *file);
void rpzclose(RPZFILEHANDLE *file);
int rpzeof(RPZFILEHANDLE *file);

#ifdef __cplusplus
}
#endif

#endif
