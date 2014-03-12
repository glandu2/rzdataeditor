#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "HashToName.h"
#include "NameToHash.h"
#include "RappelzDecrypt.h"
#include "RappelzDataManager.h"
#include "signature.h"

#define BUFFER_SIZE 16384

#define debug_printf(format, ...) printf("RappelzDataManager: " format, ## __VA_ARGS__)

#define NON_ENCRYPTED 0
#define ENCRYPTED 1

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif

/**
 * Ouvre un fichier index de Rappelz.
 * \param data000filename Le fichier index (data.000 le plus souvent)
 * \param error Pointeur vers une entier, utilisé pour indiquer le type de l'erreur. L'utilisateur peut mettre ce paramètre a NULL s'il n'en a pas besoin.
 * \return Un handle identifiant les données contenue dans le fichier:
 * \sa saveRappelzData retriveRappelzFile addRappelzFile removeRappelzFile renameRappelzFile closeRappelzData
 */

RPZDATAHANDLE openRappelzData(const char *data000filename, int *error) {
	RPZDATAHANDLE hdl;
	char baseFileName[100];
	char *p;
	int i, dummy;

	if(!error) error = &dummy;

	if(!data000filename) {
		*error = EINVAL;
		return NULL;
	}

	*error = 0;

	debug_printf("Opening file %s\n", data000filename);

	hdl = (RPZDATAHANDLE) malloc(sizeof(struct rpzDataHStruct));
	if(!hdl) {
		*error = ENOMEM;
		debug_printf("Failed to alloc %d bytes\n", sizeof(struct rpzDataHStruct));
		return NULL;
	}

	strcpy(hdl->data00[0], data000filename);

	strcpy(baseFileName, data000filename);
	p = strrchr(baseFileName, '.');
	if(p) {
		*p = 0;
		i = 1;
	} else {  //no extension, use as basename
		i = 0;
	}

	for(; i<9; i++) {
		sprintf(hdl->data00[i], "%s.00%d", baseFileName, i);
		debug_printf("Using data.00%d from file %s\n", i, baseFileName);
	}

	*error = retrievePackedFiles(hdl);
	if(*error != 0) {
		free(hdl);
		return NULL;
	}

	return hdl;
}

/**
 * Sauvegarde un fichier index de Rappelz.
 * \param hdl Handle du fichier ouvert
 * \param data000filename Le fichier index cible (data.000 le plus souvent)
 * \return Un code d'erreur ou 0:
 * \li \c EINVAL hdl est NULL
 * \li \c EIO Le fichier n'a pas pu être ouvert en écriture
 * \sa openRappelzData retriveRappelzFile addRappelzFile removeRappelzFile renameRappelzFile closeRappelzData
 */
int saveRappelzData(RPZDATAHANDLE hdl, const char *data000filename) {
	FILE *data000file;
	RPZFILERECORD *p;
	int hashSize, tempInt;
	char temp[256];

	if(!hdl || !data000filename) return EINVAL;

	data000file = fopen(data000filename, "wb");
	if(!data000file) return EIO;

	p = hdl->dataList;
	while(p) {
		tempInt = hashSize = strlen(p->hash);
		cryptfwrite(&tempInt, 1, 1, data000file);	//WARNING cryptfwrite use the user buffer to decrypt/encrypt data so data became invalid after call to this function
		strcpy(temp, p->hash);
		cryptfwrite(temp, 1, hashSize, data000file);
		tempInt = p->beginAddress;
		cryptfwrite(&tempInt, 1, 4, data000file);
		tempInt = p->dataSize;
		cryptfwrite(&tempInt, 1, 4, data000file);

		p = p->next;
	}

	fclose(data000file);

	return 0;
}

long getFileSize(const char *filename) {
	FILE *file;
	long size = 0;

	file = fopen(filename, "rb");
	if(!file) return 0;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fclose(file);

	return size;
}

int checkEncryptedFile(const char *filename) {
	char *p;

	p = strrchr(filename, '.');
	if(p) {
		if(!strcmp(p+1, "dds")) return NON_ENCRYPTED;
		else if(!strcmp(p+1, "cob")) return NON_ENCRYPTED;
		else if(!strcmp(p+1, "naf")) return NON_ENCRYPTED;
		else if(!strcmp(p+1, "nx3")) return NON_ENCRYPTED;
		else if(!strcmp(p+1, "nfm")) return NON_ENCRYPTED;
		else if(!strcmp(p+1, "tga")) return NON_ENCRYPTED;
	}
	return ENCRYPTED;
}

int resizeFileRecordList(RPZDATAHANDLE hdl, unsigned long int newNumber) {
	RPZFILERECORD* oldList = hdl->dataList;
	unsigned long int oldSize = hdl->allocatedRecords;

	if(oldSize == 0)
		hdl->allocatedRecords = newNumber;
	else {
		while(newNumber > hdl->allocatedRecords) {
			hdl->allocatedRecords = hdl->allocatedRecords * 1.1;
			if(hdl->allocatedRecords == oldSize) hdl->allocatedRecords = hdl->allocatedRecords * 2;	//if there are less than 100 files, *1.1 will do nothing
		}
	}
	hdl->dataList = realloc(oldList, sizeof(RPZFILERECORD) * hdl->allocatedRecords);
	if(!hdl->dataList) {
		hdl->dataList = oldList;
		hdl->allocatedRecords = oldSize;
		return ENOMEM;
	}

	return 0;
}

RPZFILEHANDLE * rpzopen(RPZDATAHANDLE hdl, const char* filename, const char* mode) {
	RPZFILERECORD *p;
	RPZFILEHANDLE *fileHandle;
	FILE *dataHandle;

	if(!hdl || !filename || !mode) return NULL;

	p = hdl->dataList;
	while(p) {
		if(!strcmp(p->filename, filename) || !strcmp(p->hash, filename)) break;
		p = p->next;
	}

	if(!p) {
		return NULL;
	}

	dataHandle = fopen(hdl->data00[p->dataFileID], mode);
	if(dataHandle == NULL)
		return NULL;

	fileHandle = (RPZFILEHANDLE*) malloc(sizeof(RPZFILEHANDLE));
	if(fileHandle == NULL)
		return NULL;

	fileHandle->fileRecord = p;
	fileHandle->isCrypted = checkEncryptedFile(filename);
	fileHandle->currentOffset = 0;
	fileHandle->dataFile = dataHandle;

	return fileHandle;
}

int rpzwrite(const void *buffer, int size, RPZFILEHANDLE *file) {
	int nbWriten;

	if(size + file->currentOffset > file->fileRecord->dataSize)
		size = file->fileRecord->dataSize - file->currentOffset;

	fseek(file->dataFile, file->currentOffset + file->fileRecord->beginAddress, SEEK_SET);

	if(file->isCrypted == NON_ENCRYPTED) {
		nbWriten = fwrite(buffer, 1, size, file->dataFile);
	} else {	//ENCRYPTED
		void *tempBuffer;

		tempBuffer = malloc(size);
		memcpy(tempBuffer, buffer, size);

		nbWriten = cryptfwrite(tempBuffer, 1, size, file->dataFile);
	}

	file->currentOffset += nbWriten;

	return nbWriten;
}

int rpzread(void *buffer, int size, RPZFILEHANDLE *file) {
	int nbRead;

	if(size + file->currentOffset > file->fileRecord->dataSize)
		size = file->fileRecord->dataSize - file->currentOffset;

	fseek(file->dataFile, file->currentOffset + file->fileRecord->beginAddress, SEEK_SET);

	if(file->isCrypted == NON_ENCRYPTED) {
		nbRead = fread(buffer, 1, size, file->dataFile);
	} else {	//ENCRYPTED
		//nbRead = cryptfread(buffer, 1, size, file->dataFile);

		nbRead = fread(buffer, 1, size, file->dataFile);
		decryptRappelzData((char*)buffer, (char*)buffer, nbRead*1, file->currentOffset);
	}

	file->currentOffset += nbRead;

	return nbRead;
}

void rpzclose(RPZFILEHANDLE *file) {
	fclose(file->dataFile);
	free(file);
}

int rpzeof(RPZFILEHANDLE *file) {
	return file->currentOffset >= file->fileRecord->dataSize;
}


/**
 * Récupère un fichier a partir d'un fichier de données.
 * \param filename Nom du fichier a extraire
 * \param dataLocation Nom du fichier de données ou est stocké le fichier a extraire
 * \param startOffset Emplacement du fichier dans le fichier de données
 * \param size Taille en octet du fichier a extraire
 * \param progressCallback Procédure appelé par la fonction pour indiquer l'état de l'avancement de l'opération (peut être égal a NULL si non utilisé)
 * \return Un code d'erreur ou 0:
 * \li \c EINVAL filename ou dataLocation est NULL
 * \li erreur renvoyé par fopen ou fseek lors des opérations sur les fichiers
 * \sa openRappelzData saveRappelzData retriveRappelzFile addRappelzFile removeRappelzFile renameRappelzFile closeRappelzData packRappelzFile
 */
int unpackRappelzFile(const char *filename, const char *dataLocation, unsigned long startOffset, unsigned long size, ProgressInfoCallbackType progressCallback) {
	FILE *fin;
	FILE *fout;
	int nbRead, encrypt;
	unsigned long bytesRead;
	char *buffer;

	if(!filename || !dataLocation) return EINVAL;

	encrypt = checkEncryptedFile(filename);

	fin = fopen(dataLocation, "rb");
	fout = fopen(filename, "wb");

	if( !fin || !fout) {
		if(fin) fclose(fin);
		if(fout) fclose(fout);
		return errno;
	}

	if(fseek(fin, startOffset, SEEK_SET)) {
		fclose(fin);
		fclose(fout);
		return errno;
	}

	buffer = (char*) malloc(BUFFER_SIZE);
	if(!buffer) {
		fclose(fin);
		fclose(fout);
		return errno;
	}

	bytesRead = 0;
	if(encrypt == NON_ENCRYPTED) {
		while((nbRead = fread(buffer, 1, min(BUFFER_SIZE, size-bytesRead), fin)) > 0) {
			bytesRead += nbRead;
			fwrite(buffer, 1, nbRead, fout);
			if(progressCallback) progressCallback(filename, bytesRead, size);
		}
	} else {	//ENCRYPTED
		while((nbRead = fread(buffer, 1, min(BUFFER_SIZE, size-bytesRead), fin)) > 0) {
			bytesRead += nbRead;
			cryptfwrite(buffer, 1, nbRead, fout);
			if(progressCallback) progressCallback(filename, bytesRead, size);
		}
	}

	fclose(fin);
	fclose(fout);
	free(buffer);

	return 0;
}

/**
 * Ecrit un fichier dans un fichier de données.
 * \param filename Nom du fichier source
 * \param dataLocation Nom du fichier de données ou le fichier source va être stocké
 * \param startOffset Emplacement du fichier dans le fichier de données
 * \param size Taille en octet du fichier a insérer
 * \param progressCallback Procédure appelé par la fonction pour indiquer l'état de l'avancement de l'opération (peut être égal a NULL si non utilisé)
 * \return Un code d'erreur ou 0:
 * \li \c EINVAL filename ou dataLocation est NULL
 * \li erreur renvoyé par fopen ou fseek lors des opérations sur les fichiers
 * \sa openRappelzData saveRappelzData retriveRappelzFile addRappelzFile removeRappelzFile renameRappelzFile closeRappelzData unpackRappelzFile
 */
int packRappelzFile(const char *filename, const char *dataLocation, unsigned long startOffset, unsigned long size, ProgressInfoCallbackType progressCallback) {
	FILE *fin;
	FILE *fout;
	int nbRead, encrypted;
	char *buffer;

	if(!filename || !dataLocation) return EINVAL;

	encrypted = checkEncryptedFile(filename);

	fin = fopen(filename, "rb");
	if(!fin) return errno;

	fout = fopen(dataLocation, "rb+");
	if(!fout && errno == ENOENT) fout = fopen(dataLocation, "wb");	//create data file if it does not exist
	if(!fout) return errno;

	if(fseek(fout, startOffset, SEEK_SET)) {
		fclose(fin);
		fclose(fout);
		return errno;
	}

	buffer = (char*) malloc(BUFFER_SIZE);
	if(!buffer) {
		fclose(fin);
		fclose(fout);
		return errno;
	}

	if(encrypted == NON_ENCRYPTED) {
		while((nbRead = fread(buffer, 1, min(BUFFER_SIZE, size), fin)) > 0) {
			size -= nbRead;
			fwrite(buffer, 1, nbRead, fout);
		}
	} else {	//ENCRYPTED
		while((nbRead = cryptfread(buffer, 1, min(BUFFER_SIZE, size), fin)) > 0) {
			size -= nbRead;
			fwrite(buffer, 1, nbRead, fout);
		}
	}

	fclose(fin);
	fclose(fout);
	free(buffer);

	return 0;
}

/**
 * Récupère un fichier dans une archive Rappelz ouvert par un handle.
 * \param hdl Handle du fichier ouvert
 * \param filename Nom du fichier a extraire
 * \param filePath Chemin complet vers la destination
 * \return Un code d'erreur ou 0:
 * \li \c EINVAL hdl, filename ou filePath est NULL
 * \li \c ENOENT Le fichier n'éxiste pas dans l'archive
 * \li Autre erreur renvoyé par \c unpackRappelzFile
 * \sa openRappelzData saveRappelzData addRappelzFile removeRappelzFile renameRappelzFile closeRappelzData unpackRappelzFile
 */
int retriveRappelzFile(RPZDATAHANDLE hdl, const char *filename, const char *filePath) {
	RPZFILERECORD *p;

	if(!hdl || !filename || !filePath) return EINVAL;

	p = hdl->dataList;
	while(p) {
		if(!strcmp(p->filename, filename)) break;
		p = p->next;
	}

	if(p) return unpackRappelzFile(filePath, hdl->data00[p->dataFileID], p->beginAddress, p->dataSize, NULL);

	return ENOENT;
}

//add a file in rappelz data archive, detect if the file already exist and if it should be moved somewhere if not enough space to update it
//args:	dataList: current data.000
//		filename: name of the file in the archive, often the same name as the original file
//		filePath: complete path to the file in filesystem
//note: this documentation may be obsolete, please refer to this one below:
/**
 * Ajoute ou écrase un fichier dans une archive Rappelz ouvert par un handle, (cette fonction peut facilement fragmenter l'archive lors de mise a jour de fichier, si la taille est supérieure a l'ancien fichier, il est ajouté a la fin du fichier de donnée corespondant).
 * \param hdl Handle du fichier ouvert
 * \param filename Nom du fichier à ajouter
 * \param filePath Chemin complet vers la source
 * \return Un code d'erreur ou 0:
 * \li \c EINVAL hdl, filename ou filePath est NULL
 * \li \c EEXIST updateMode vaut AM_AddOnly et le fichier existe déja
 * \li \c ENOENT updateMode vaut AM_UpdateOnly et le fichier n'existe pas
 * \li \c ENOMEM Pas assez de mémoire pour faire l'opération
 * \li \c ENOMEM Pas assez de mémoire pour faire l'opération
 * \li Autre erreur renvoyé par \c packRappelzFile
 * \sa openRappelzData saveRappelzData retriveRappelzFile removeRappelzFile renameRappelzFile closeRappelzData packRappelzFile
 * \todo Verifier qu'il n'y aurait pas de la place dans le fichier de données avant de l'ajouter a la fin, et faire que si le fichier de données est a déja a la fin de pouvoir l'agrandir sans le changer d'emplacement.
 */
int addRappelzFile(RPZDATAHANDLE hdl, const char *filename, const char *filePath, eAddMode updateMode) {
	RPZFILERECORD *p, *lastFile;
	unsigned long newFileSize;
	int result;

	if(!hdl || !filename || !filePath) return EINVAL;

	//check if the file already exist
	p = hdl->dataList;
	lastFile = NULL;
	while(p) {
		if(!strcmp(p->filename, filename)) break;
		lastFile = p;
		p = p->next;
	}

	if(p && updateMode != AM_AddOnly) {	//update the file
		newFileSize = getFileSize(filePath);
		if(newFileSize <= p->dataSize) {	//update the file at the same location in data.00x
			result = packRappelzFile(filePath, hdl->data00[p->dataFileID], p->beginAddress, newFileSize, NULL);
			if(result) return result;
			p->dataSize = newFileSize;
		} else {	//move the file at the end of current data00x
			result = packRappelzFile(filePath, hdl->data00[p->dataFileID], getFileSize(hdl->data00[p->dataFileID]), newFileSize, NULL);
			if(result) return result;
			p->beginAddress = getFileSize(hdl->data00[p->dataFileID]);	//doing this after ensure that if packRappelzFile fail, datas in *p remain correct
			p->dataSize = newFileSize;
		}
	} else if(!p && updateMode != AM_UpdateOnly) { //add the file at the end of the list
		int hashSize;
		if(hdl->fileNum >= hdl->allocatedRecords) {
			result = resizeFileRecordList(hdl, hdl->fileNum+1);
			if(result)
				return result;
		}

		p = &hdl->dataList[hdl->fileNum];

		hashSize = strlen(filename)+2;

		strcpy(p->filename, filename);

		result = convertNameToHash(filename, p->hash, LEGACY_SEED);
		if(result) return result;

		p->dataFileID = getAssociatedDataFileNumber(p->hash);
		p->dataSize = getFileSize(filePath);

		p->beginAddress = getFileSize(hdl->data00[p->dataFileID]);
		result = packRappelzFile(filePath, hdl->data00[p->dataFileID], p->beginAddress, p->dataSize, NULL);
		if(result) return result;

		p->next = NULL;

		hdl->fileNum++;

		if(lastFile) lastFile->next = p;
		else hdl->dataList = p;
	} else {
		if(updateMode == AM_AddOnly)
			return EEXIST;
		if(updateMode == AM_UpdateOnly)
			return ENOENT;
	}

	return 0;
}

/**
 * Supprime un fichier de l'archive
 * \param hdl Handle du fichier ouvert
 * \param filename Nom du fichier à supprimer
 * \return Un code d'erreur ou 0:
 * \li \c EINVAL hdl, filename est NULL
 * \li \c ENOENT Le fichier n'éxiste pas
 * \sa openRappelzData saveRappelzData retriveRappelzFile addRappelzFile renameRappelzFile closeRappelzData
 */
int removeRappelzFile(RPZDATAHANDLE hdl, const char *filename) {
	RPZFILERECORD *p, *lastFile;

	if(!hdl || !filename) return EINVAL;

	//check if the file already exist
	p = hdl->dataList;
	lastFile = NULL;
	while(p) {
		if(!strcmp(p->filename, filename)) break;
		lastFile = p;
		p = p->next;
	}

	if(p) {
		if(lastFile) lastFile->next = p->next;
		else hdl->dataList = p->next;

		free(p->filename);
		free(p->hash);
		free(p);

		return 0;
	}

	return ENOENT;	//file not found
}

/**
 * Renome un fichier de l'archive (non fonctinnel si le fichier renommé n'est pas dans le meme data.00x)
 * \param hdl Handle du fichier ouvert
 * \param filename Nom du fichier à renomer
 * \param newFilename Nouveau nom de fichier
 * \return Un code d'erreur ou 0:
 * \li \c EINVAL hdl, filename ou newFilename est NULL
 * \li \c ENOMEM Pas assez de mémoire pour faire l'opération
 * \li \c ENOENT Le fichier n'éxiste pas
 * \warning le hash ou le nom du fichier peut devenir erroné si l'erreur est ENOMEM
 * \sa openRappelzData saveRappelzData retriveRappelzFile addRappelzFile removeRappelzFile closeRappelzData convertNameToHash
 */
int renameRappelzFile(RPZDATAHANDLE hdl, const char *filename, const char *newFilename) {
	RPZFILERECORD *p;
	int result;

	if(!hdl || !filename || !newFilename) return EINVAL;

	result = 0;
	p = NULL;

	//check if the file already exist
	/*p = hdl->dataList;
	while(p) {
		if(!strcmp(p->filename, filename)) break;
		p = p->next;
	}

	if(p) {
		int hashSize = strlen(newFilename) + 2;

		free(p->hash);
		p->hash = (char*) malloc(hashSize + 1);
		if(!p->hash) return ENOMEM;
		result = convertNameToHash(newFilename, p->hash, LEGACY_SEED);
		if(result) {
			free(p->hash);
			p->hash = NULL;
			return result;
		}

		free(p->filename);
		p->filename = (char*) malloc(hashSize + 1);
		if(!p->filename) return ENOMEM;
		strcpy(p->filename, newFilename);

		return 0;
	}*/

	return ENOENT;	//file not found
}

/**
 * Ferme un fichier archive Rappelz
 * \warning N'enregistre pas le fichier!
 * \param hdl Handle du fichier ouvert
 * \li \c EINVAL hdl est NULL
 * \sa openRappelzData saveRappelzData retriveRappelzFile addRappelzFile removeRappelzFile renameRappelzFile
 */
void closeRappelzData(RPZDATAHANDLE hdl) {
	if(!hdl) return;
	if(hdl->dataList) free(hdl->dataList);
	free(hdl);
}

int retrievePackedFiles(RPZDATAHANDLE hdl) {
	FILE *fData000;
	RPZFILERECORD *p, *prec;
	int hashSize;
	char *fileBuffer;
	int result = 0;
	unsigned long int fileSize;

	hdl->dataList = NULL;
	hdl->allocatedRecords = 0;
	hdl->fileNum = 0;

	fData000 = fopen(hdl->data00[0], "rb");
	if(!fData000) {
		if(errno != ENOENT) {
			result = errno;
			debug_printf("Cannot open file %s\n", hdl->data00[0]);
			return result;
		} else {
			debug_printf("File does not exist: %s\n", hdl->data00[0]);
			return 0;	//if the file does not exist, report it as an empty file without any record
		}
	}

	fseek(fData000, 0, SEEK_END);
	fileBuffer = (char*)malloc(fileSize = ftell(fData000));
	fseek(fData000, 0, SEEK_SET);


	if(!fileBuffer) {
		debug_printf("Couldn't alloc %d bytes\n", fileSize);
		if(fileSize == 0)
			result = 0;
		else
			result = ENOMEM;
	} else { //fileBuffer != NULL
		char *ptr;

		cryptfread(fileBuffer, 1, fileSize, fData000);
		hdl->allocatedRecords = fileSize/41;

		debug_printf("Found %d records\n", hdl->allocatedRecords);

		hdl->dataList = (RPZFILERECORD*) malloc(sizeof(RPZFILERECORD) * hdl->allocatedRecords);
		if(!hdl->dataList) {
			hdl->allocatedRecords = 0;
			result = ENOMEM;
		} else {
			p = hdl->dataList;
			ptr = fileBuffer;
			prec = NULL;
			while(ptr < fileBuffer+fileSize) {
				if(hdl->fileNum >= hdl->allocatedRecords) {
					result = resizeFileRecordList(hdl, hdl->fileNum+1);
					if(result)
						break;
				}

				p = &hdl->dataList[hdl->fileNum];

				hashSize = *(ptr++);

				memcpy(p->hash, ptr, hashSize);
				ptr += hashSize;

				p->hash[hashSize] = '\0';
				p->dataFileID = getAssociatedDataFileNumber(p->hash);

				memcpy(&p->beginAddress, ptr, 4);
				ptr += 4;
				memcpy(&p->dataSize, ptr, 4);
				ptr += 4;

				if(convertHashToName(p->hash, p->filename)) {
					result = EINVAL;
					break;
				}

				p->next = NULL;

				hdl->fileNum++;
			}
			if(!result) {	//compatibilité avec la liste chainée
				unsigned long int i;
				for(i=0; i < hdl->fileNum; i++) {
					if(i+1 < hdl->fileNum)
						hdl->dataList[i].next = &hdl->dataList[i+1];
				}
			}
		}
		free(fileBuffer);
	}

	fclose(fData000);

	if(result != 0) {
		free(hdl->dataList);
		hdl->dataList = NULL;
		hdl->allocatedRecords = 0;
		hdl->fileNum = 0;
	}

	return result;
}

