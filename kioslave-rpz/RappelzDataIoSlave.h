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

#ifndef RAPPELZDATAIOSLAVE_H
#define RAPPELZDATAIOSLAVE_H

#include <sys/types.h>

#include <kio/global.h>
#include <kio/slavebase.h>

#include "RappelzDataManager.h"

class QFileInfo;

class RappelzDataIoSlave : public KIO::SlaveBase
{
public:
	RappelzDataIoSlave(const QByteArray &pool, const QByteArray &app);
	virtual ~RappelzDataIoSlave();

	virtual void listDir(const KUrl & url);
	virtual void stat(const KUrl & url);
	virtual void get(const KUrl & url);
	void error( int _errid, const QString &_text );

private:
	struct TCRPZHANDLE {
		KIO::UDSEntry archiveEntry;
		RPZDATAHANDLE rappelzData;
	};

	bool splitPath(const QString& fullPath, QString* realPath, QString* rappelzFilename);
	void createRootUDSEntry(KIO::UDSEntry & entry );
	void createUDSEntry(const RPZFILERECORD *entryName, KIO::UDSEntry &entry);

	TCRPZHANDLE *openArchive(const QString &realPath, int &errorNum);
	int closeArchive(TCRPZHANDLE *hdl);

	TCRPZHANDLE *lastArchive;
};

#endif // RAPPELZDATAIOSLAVE_H
