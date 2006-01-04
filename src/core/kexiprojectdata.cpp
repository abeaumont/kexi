/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003-2004 Jaroslaw Staniek <js@iidea.pl>

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
 * Boston, MA 02110-1301, USA.
*/

#include <sys/types.h>
#include <unistd.h>

#include <qdom.h>
#include <qdir.h>
#include <qfile.h>
#include <qregexp.h>

#include <kglobal.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kio/netaccess.h>
#include <kurl.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <kexidb/drivermanager.h>
#include "kexiprojectdata.h"


//! @internal
class KexiProjectDataPrivate
{
public:
	KexiProjectDataPrivate()
	 : finalMode(false)
	{}
	
	KexiDB::ConnectionData connData;
	QDateTime lastOpened;
	bool finalMode : 1;
};

//---------------------------------------

KexiProjectData::KexiProjectData()
 : QObject(0, "KexiProjectData")
 , KexiDB::SchemaData()
 , d( new KexiProjectDataPrivate() )
{
}

KexiProjectData::KexiProjectData( 
	const KexiDB::ConnectionData &cdata, const QString& dbname, const QString& caption )
 : QObject(0, "KexiProjectData")
 , KexiDB::SchemaData()
 , d( new KexiProjectDataPrivate() )
{
	d->connData = cdata;
	setDatabaseName(dbname);
	setCaption(caption);
}

KexiProjectData::KexiProjectData( const KexiProjectData& pdata )
 : QObject(0, "KexiProjectData"), KexiDB::SchemaData()
 , d( 0 )
{
	*this = pdata;
/*
	d->connData = *pdata.connectionData();
	setDatabaseName(pdata.databaseName());
	setCaption(pdata.caption());*/
}

KexiProjectData::~KexiProjectData()
{
	delete d;
}

KexiProjectData& KexiProjectData::operator=(const KexiProjectData& pdata)
{
	delete d; //this is old
	static_cast<KexiDB::SchemaData&>(*this) = static_cast<const KexiDB::SchemaData&>(pdata);
	//deep copy
	d = new KexiProjectDataPrivate();
	*d = *pdata.d;
//	d->connData = *pdata.constConnectionData();
//	setDatabaseName(pdata.databaseName());
//	setCaption(pdata.caption());
//	setDescription(pdata.description());
	return *this;
}

KexiDB::ConnectionData* KexiProjectData::connectionData()
{
	return &d->connData;
}

const KexiDB::ConnectionData* KexiProjectData::constConnectionData() const
{
	return &d->connData;
}

QString KexiProjectData::databaseName() const
{
	return KexiDB::SchemaData::name();
}

void KexiProjectData::setDatabaseName(const QString& dbName)
{
	KexiDB::SchemaData::setName(dbName);
}

bool KexiProjectData::finalMode() const
{
	return d->finalMode;
}

QDateTime KexiProjectData::lastOpened() const
{
	return d->lastOpened;
}

void KexiProjectData::setLastOpened(const QDateTime& lastOpened)
{
	d->lastOpened=lastOpened;

}
QString KexiProjectData::description() const
{
	return KexiDB::SchemaData::description();
}

void KexiProjectData::setDescription(const QString& desc)
{
	return KexiDB::SchemaData::setDescription(desc);
}

QString KexiProjectData::infoString(bool nobr) const
{
	QString string;
	if (constConnectionData()->fileName().isEmpty()) { //server-based
		string = QString(nobr ? "<nobr>" : "") + QString("\"%1\"").arg(objectName()) + (nobr ? "</nobr>" : "")
			+ (nobr ? " <nobr>" : " ") + i18n("database connection", "(connection %1)")
		.arg(constConnectionData()->serverInfoString()) + (nobr ? "</nobr>" : "");
	}
	else {//file-based
		string = QString(nobr ? "<nobr>" : "") 
			+ QString("\"%1\"").arg(constConnectionData()->fileName()) + (nobr ? "</nobr>" : "");
	}
	return string;
}
