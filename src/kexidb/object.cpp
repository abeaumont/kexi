/* This file is part of the KDE project
   Copyright (C) 2003 Jaroslaw Staniek <js@iidea.pl>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <kexidb/object.h>
#include <kexidb/error.h>

#include <klocale.h>
#include <kdebug.h>

using namespace KexiDB;


Object::Object()
: d(0) //empty
{
	clearError();
}

void Object::setError( int code, const QString &msg )
{
	m_errno=code;
	if (m_errno==ERR_OTHER && msg.isNull())
		m_errMsg = i18n("Unspecified error encountered");
	else
		m_errMsg = msg;
	m_hasError = code!=ERR_NONE;
}

void Object::setError( const QString &msg )
{
	m_errno=ERR_OTHER;
	m_errMsg = msg;
	m_hasError = true;
}

void Object::setError( KexiDB::Object *obj )
{
	if (obj) {
		m_errno = obj->errorNum();
		m_errMsg = obj->errorMsg();
		m_hasError = obj->error();
	}
}

void Object::clearError()
{ 
	m_errno = 0;
	m_hasError = false;
	m_errMsg = QString::null;
	drv_clearServerResult();
}

QString Object::serverErrorMsg()
{
	return QString::null;
}

int Object::serverResult()
{
	return 0;
}

QString Object::serverResultName()
{
	return QString::null;
}

Object::~Object()
{
}

void Object::debugError()
{
	if (error()) {
		KexiDBDbg << "KEXIDB ERROR: " << errorMsg() << endl;
		QString s = serverErrorMsg(), sn = serverResultName();
		if (!s.isEmpty())
			KexiDBDbg << "KEXIDB SERVER ERRMSG: " << s << endl;
		if (!sn.isEmpty())
			KexiDBDbg << "KEXIDB SERVER RESULT NAME: " << sn << endl;
		if (serverResult()!=0)
			KexiDBDbg << "KEXIDB SERVER RESULT #: " << serverResult() << endl;
	} else
		KexiDBDbg << "KEXIDB OK." << endl;
}
