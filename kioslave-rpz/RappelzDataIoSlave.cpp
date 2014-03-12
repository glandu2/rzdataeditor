/*  This file is part of the KDE libraries
		Copyright (C) 2000 David Faure <faure@kde.org>

		This library is free software; you can redistribute it and/or
		modify it under the terms of the GNU Library General Public
		License as published by the Free Software Foundation; either
		version 2 of the License, or (at your option) any later version.

		This library is distributed in the hope that it will be useful,
		but WITHOUT ANY WARRANTY; without even the implied warranty of
		MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
		Library General Public License for more details.

		You should have received a copy of the GNU Library General Public License
		along with this library; see the file COPYING.LIB.  If not, write to
		the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
		Boston, MA 02110-1301, USA.
	*/

#include "RappelzDataIoSlave.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>

#include <QFile>
#include <QDir>
#include <QDateTime>

#include <kglobal.h>
#include <kurl.h>
#include <kdebug.h>
#include <kcomponentdata.h>
#include <kmimetype.h>
#include <klocale.h>
#include <kde_file.h>
#include <kio/global.h>

#include <kuser.h>

using namespace KIO;

extern "C" { int KDE_EXPORT kdemain(int argc, char **argv); }

int kdemain( int argc, char **argv )
{
	KComponentData componentData( "kio_rappelz" );

	kDebug() << "Starting" << getpid();

	if (argc != 4)
	{
		fprintf(stderr, "Usage: kio_rappelz protocol domain-socket1 domain-socket2\n");
		exit(-1);
	}

	RappelzDataIoSlave slave(argv[2], argv[3]);
	slave.dispatchLoop();

	kDebug() << "Done";
	return 0;
}

RappelzDataIoSlave::RappelzDataIoSlave( const QByteArray &pool, const QByteArray &app ) : SlaveBase( "rappelz", pool, app )
{
	kDebug() << "RappelzDataIoSlave::RappelzDataIoSlave";
	lastArchive = 0;
}

RappelzDataIoSlave::~RappelzDataIoSlave()
{
	if(lastArchive)
		closeRappelzData(lastArchive->rappelzData);
	delete lastArchive;
}

void RappelzDataIoSlave::error( int _errid, const QString &_text ) {
	kDebug() << "Error " << _errid << " text: " << _text;
	KIO::SlaveBase::error(_errid, _text);
}

bool RappelzDataIoSlave::splitPath(const QString& fullPath, QString* realPath, QString* rappelzFilename) {
	int pos = 0;
	KDE_struct_stat statbuf;
	statbuf.st_mode = 0;

	QString dummy;

	if(realPath == 0)
		realPath = &dummy;
	if(rappelzFilename == 0)
		rappelzFilename = &dummy;

	while ( (pos=fullPath.indexOf( '/', pos+1 )) != -1 )
	{
		QString tryPath = fullPath.left( pos );
		kDebug() << fullPath << "trying" << tryPath;
		if ( ::stat( QFile::encodeName(tryPath), &statbuf ) == -1 )
		{
			// We are not in the file system anymore, either we have already enough data or we will never get any useful data anymore
			kDebug() << "Stat failed on " << tryPath;
			return false;
		}
		if ( !S_ISDIR(statbuf.st_mode) )
		{
			int len;
			*realPath = tryPath;
			*rappelzFilename = fullPath.mid( pos + 1 );
			kDebug() << "fullPath=" << *realPath << " path=" << *rappelzFilename;
			len = rappelzFilename->length();
			if ( len > 1 )
			{
				if ( (*rappelzFilename)[ len - 1 ] == '/' )
					rappelzFilename->truncate( len - 1 );
			}
			else
				*rappelzFilename = QString::fromLatin1("/");
			kDebug() << "Found. archiveFile=" << *realPath << " path=" << *rappelzFilename;
			return true;
		}
	}

	*realPath = fullPath;
	*rappelzFilename = QString("");
	kDebug() << "Cannot split " << fullPath;

	return true;
}

void RappelzDataIoSlave::createRootUDSEntry( KIO::UDSEntry & entry )
{
	if(lastArchive)
		entry = lastArchive->archiveEntry;
	else {
		entry.clear();
		kDebug() << "createRootUDSEntry without archive !";
	}
}

void RappelzDataIoSlave::createUDSEntry(const RPZFILERECORD* entryInfo, UDSEntry & entry)
{
	entry.clear();
	entry.insert( KIO::UDSEntry::UDS_NAME, QString(entryInfo->filename));
	entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
	entry.insert( KIO::UDSEntry::UDS_ACCESS, 0777);
	entry.insert( KIO::UDSEntry::UDS_SIZE, entryInfo->dataSize);
	//entry.insert( KIO::UDSEntry::UDS_DISPLAY_NAME, QString(entryInfo->filename));
}

RappelzDataIoSlave::TCRPZHANDLE *RappelzDataIoSlave::openArchive(const QString& realPath, int &errorNum) {
	TCRPZHANDLE *hdl;

	if(lastArchive) {
		if(!qstrcmp(lastArchive->rappelzData->data00[0], realPath.toLocal8Bit().constData())) {	//TC want to reopen last opened archive
			kDebug() << "Using last archive " << realPath;
			return lastArchive;
		} else {
			closeRappelzData(lastArchive->rappelzData);
			delete lastArchive;
			lastArchive = 0;
		}
	}


	kDebug() << "Opening file " << realPath;


	hdl = new TCRPZHANDLE;

	hdl->rappelzData = openRappelzData(realPath.toLocal8Bit().constData(), &errorNum);
	if(!hdl->rappelzData) {
		kError() << "ERROR no " << errorNum;
		delete hdl;
		return NULL;
	}
	kDebug() << "file opened";

	lastArchive = hdl;
	lastArchive->archiveEntry.clear();
	lastArchive->archiveEntry.insert( KIO::UDSEntry::UDS_NAME, ".");
	lastArchive->archiveEntry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
	lastArchive->archiveEntry.insert( KIO::UDSEntry::UDS_ACCESS, 0777);
	lastArchive->archiveEntry.insert( KIO::UDSEntry::UDS_SIZE, 0L);
	lastArchive->archiveEntry.insert( KIO::UDSEntry::UDS_COMMENT, QString("Truc"));
	return hdl;
}

int RappelzDataIoSlave::closeArchive(TCRPZHANDLE *hdl) {
	kDebug() << "Closing data archive " << hdl->rappelzData->data00[0];

	/*closeRappelzData(hdl->rappelzData);
	free(hdl);*/
	//do nothing, keep open last archive, OpenArchive will close it if TC want to open another archive

	return 0;
}

void RappelzDataIoSlave::listDir( const KUrl & url )
{
	int errorNum;
	QString path = url.path(KUrl::RemoveTrailingSlash) + '/';
	QString realPath, rappelzFile;
	RPZFILERECORD *currentRecord;

	kDebug() << "RappelzDataIoSlave::listDir" << url.url();

	if ( path.isEmpty() )
	{
		KUrl redir( QString::fromLatin1( "file:/") );
		kDebug() << "url.path()=" << url.path();
		redir.setPath( url.path() + QString::fromLatin1("/") );
		kDebug() << "RappelzDataIoSlave::listDir: redirection" << redir.url();
		redirection(redir);
		finished();
		return;
	}

	if(splitPath(path, &realPath, &rappelzFile) == false) {
		KUrl redir( QString::fromLatin1( "file:/") );
		kDebug() << "url.path()=" << url.path();
		redir.setPath( url.path() + QString::fromLatin1("/") );
		kDebug() << "RappelzDataIoSlave::listDir: redirection to real dir" << redir.url();
		redirection(redir);
		finished();
		return;
	}

	if(!rappelzFile.isEmpty() && rappelzFile != "/") {
		error(KIO::ERR_CANNOT_ENTER_DIRECTORY, url.path());
		return;
	}

	TCRPZHANDLE *hdl = openArchive(realPath, errorNum);

	if(!hdl) {
		switch(errorNum) {
			case ENOMEM:
				error(KIO::ERR_OUT_OF_MEMORY, url.path());
				return;

			default:
				error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.path());
				return;
		}
	}

	kDebug() << "File count : " << hdl->rappelzData->fileNum;

	totalSize(hdl->rappelzData->fileNum);

	UDSEntry entry;

	createRootUDSEntry(entry);
	listEntry(entry, false);

	kDebug() << "Listing entries ...";
	unsigned long i = 0;
	for(currentRecord = hdl->rappelzData->dataList; currentRecord; currentRecord = currentRecord->next) {
		createUDSEntry(currentRecord, entry);
		listEntry(entry, false);
		//kDebug() << currentRecord->filename;
		i++;
		if(i % 1000 == 0)
			listEntry(entry, true); // force flush entries every 1000 files
	}

	kDebug() << "done " << i << " files";

	listEntry(entry, true); // ready

	finished();

	kDebug() << "RappelzDataIoSlave::listDir done now";
}

void RappelzDataIoSlave::stat( const KUrl & url )
{
	int errorNum;
	QString realPath, rappelzFile;
	TCRPZHANDLE *hdl;
	RPZFILERECORD *currentRecord;
	QByteArray filename;
	const char *c_filename;

	kDebug() << "RappelzDataIoSlave::stat" << url.url();

	if(splitPath(url.path(KUrl::RemoveTrailingSlash) + '/', &realPath, &rappelzFile) == false || rappelzFile.isEmpty()) {
		KIO::UDSEntry entry;
		entry.insert(KIO::UDSEntry::UDS_NAME, url.fileName());
		KDE_struct_stat buff;
		if (KDE_stat(QFile::encodeName(realPath), &buff) == -1)
		{
			error(KIO::ERR_COULD_NOT_STAT, realPath);
			return;
		}
		entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, buff.st_mode & S_IFMT);
		statEntry(entry);
		finished();
		return;
	}

	hdl = openArchive(realPath, errorNum);
	if(!hdl) {
		switch(errorNum) {
			case ENOMEM:
				error(KIO::ERR_OUT_OF_MEMORY, url.path());
				return;

			default:
				error(KIO::ERR_COULD_NOT_STAT, url.path());
				return;
		}
	}

	if(rappelzFile == "/") {
		KIO::UDSEntry entry;
		entry.insert( KIO::UDSEntry::UDS_NAME, realPath);
		entry.insert( KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
		entry.insert( KIO::UDSEntry::UDS_ACCESS, 0777);
		entry.insert( KIO::UDSEntry::UDS_SIZE, 0L);
		statEntry(entry);
		finished();
		return;
	}

	filename = rappelzFile.toLocal8Bit();
	c_filename = filename.constData();

	kDebug() << "Searching file " << filename << " in " << realPath;

	for(currentRecord = hdl->rappelzData->dataList; currentRecord && qstrcmp(currentRecord->filename, c_filename) && qstrcmp(currentRecord->hash, c_filename); currentRecord = currentRecord->next);

	if(currentRecord) {
		UDSEntry entry;
		createUDSEntry(currentRecord, entry);
		statEntry(entry);
		finished();
		kDebug() << "Found";
	} else {
		kDebug() << "Not found";
		error(KIO::ERR_COULD_NOT_STAT, url.path());
	}
}

void RappelzDataIoSlave::get( const KUrl & url )
{
	kDebug( 7109 ) << "RappelzDataIoSlave::get" << url.url();

	int errorNum;

	TCRPZHANDLE *hdl;
	RPZFILERECORD *currentRecord;
	RPZFILEHANDLE *fileHandle;
	QString realPath, rappelzFile;
	QByteArray filename;
	const char *c_filename;

	if(splitPath(url.path(KUrl::RemoveTrailingSlash) + '/', &realPath, &rappelzFile) == false || rappelzFile.isEmpty()) {
		error(KIO::ERR_IS_DIRECTORY, url.path());
		return;
	}

	hdl = openArchive(realPath, errorNum);
	if(!hdl) {
		switch(errorNum) {
			case ENOMEM:
				error(KIO::ERR_OUT_OF_MEMORY, url.path());
				return;

			default:
				error(KIO::ERR_CANNOT_OPEN_FOR_READING, url.path());
				return;
		}
	}

	filename = rappelzFile.toLocal8Bit();
	c_filename = filename.constData();

	fileHandle = rpzopen(hdl->rappelzData, c_filename, "rb");
	if(!fileHandle) {
		error(KIO::ERR_COULD_NOT_STAT, filename);
		return;
	}

	currentRecord = fileHandle->fileRecord;
	totalSize(currentRecord->dataSize);

	// Size of a QIODevice read. It must be large enough so that the mime type check will not fail
	const unsigned long maxSize = 0x100000; // 1MB

	unsigned long bufferSize = qMin(maxSize, currentRecord->dataSize);
	char* buffer = new char[bufferSize];
	if ( !buffer && bufferSize > 0 )
	{
		// Something went wrong
		error(KIO::ERR_OUT_OF_MEMORY, url.prettyUrl());
		return;
	}

	bool firstRead = true;

	// How much file do we still have to process?
	unsigned long fileSize = currentRecord->dataSize;
	unsigned long processed = 0;

	while ( processed < currentRecord->dataSize && currentRecord->dataSize > 0 )
	{
		unsigned long size = qMin(bufferSize, fileSize);
		const unsigned long read = rpzread(buffer, size, fileHandle);
		if ( read != size )
		{
			kWarning(7109) << "Read" << read << "bytes but expected" << size ;
			error( KIO::ERR_COULD_NOT_READ, url.prettyUrl() );
			delete[] buffer;
			return;
		}
		QByteArray bufferArray(buffer, read);
		if ( firstRead )
		{
			// We use the magic one the first data read
			// (As magic detection is about fixed positions, we can be sure that it is enough data.)
			KMimeType::Ptr mime = KMimeType::findByNameAndContent( url.path(), bufferArray );
			kDebug(7109) << "Emitting mimetype" << mime->name();
			mimeType( mime->name() );
			firstRead = false;
		}
		data( bufferArray );
		processed += read;
		processedSize( processed );
		fileSize -= read;
	}
	rpzclose(fileHandle);
	delete[] buffer;

	data( QByteArray() );

	finished();
}
