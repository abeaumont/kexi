/* This file is part of the KDE project
   Copyright (C) 2003 Jaroslaw Staniek <js@iidea.pl>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <kexidb/roweditbuffer.h>

#include <kdebug.h>

using namespace KexiDB;


RowEditBuffer::RowEditBuffer(bool dbAwareBuffer)
: m_simpleBuffer(dbAwareBuffer ? 0 : new QMap<QString,QVariant>())
, m_dbBuffer(dbAwareBuffer ? new QMap<Field*,QVariant>() : 0)
{
}

RowEditBuffer::~RowEditBuffer()
{
	delete m_simpleBuffer;
	delete m_dbBuffer;
}

QVariant* RowEditBuffer::at( Field& f ) 
{ 
	if (!m_dbBuffer)
		return 0;
	DBMap::Iterator it = m_dbBuffer->find( &f );
	if (it==m_dbBuffer->end())
		return 0;
	return &it.data();
}

QVariant* RowEditBuffer::at( const QString& fname )
{
	if (!m_simpleBuffer)
		return 0;
	SimpleMap::Iterator it = m_simpleBuffer->find( fname );
	if (it==m_simpleBuffer->end())
		return 0;
	return &it.data();
}

void RowEditBuffer::clear() {
	if (m_dbBuffer)
		m_dbBuffer->clear(); 
	if (m_simpleBuffer)
		m_simpleBuffer->clear();
}

bool RowEditBuffer::isEmpty() const
{
	if (m_dbBuffer)
		return m_dbBuffer->isEmpty(); 
	if (m_simpleBuffer)
		return m_simpleBuffer->isEmpty();
	return true;
}

void RowEditBuffer::debug()
{
	if (isDBAware()) {
		kdDebug() << "RowEditBuffer type=DB-AWARE, " << m_dbBuffer->count() <<" items"<< endl;
		for (DBMap::Iterator it = m_dbBuffer->begin(); it!=m_dbBuffer->end(); ++it) {
			kdDebug() << "* field name=" <<it.key()->name()<<" val="
				<< (it.data().isNull ? QString("<NULL>") : it.data().toString()) <<endl;
		}
		return;
	}
	kdDebug() << "RowEditBuffer type=SIMPLE, " << m_simpleBuffer->count() <<" items"<< endl;
	for (SimpleMap::Iterator it = m_simpleBuffer->begin(); it!=m_simpleBuffer->end(); ++it) {
		kdDebug() << "* field name=" <<it.key()<<" val="
			<< (it.data().isNull ? QString("<NULL>") : it.data().toString()) <<endl;
	}
}
