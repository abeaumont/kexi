/* This file is part of the KDE project
   Copyright (C) 2002   Lucijan Busch <lucijan@gmx.at>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
 */

#include <qwidget.h>

#include <klocale.h>
#include <kmessagebox.h>

#include "kexidberror.h"

KexiDBError::KexiDBError(int errno, QString text)
{
	m_errno = errno;
	m_text = text;
}

void
KexiDBError::toUser(QWidget *parent)
{
	KMessageBox::error(parent, m_text, i18n("Database Error"));
}

int
KexiDBError::errno()
{
	return m_errno;
}

QString
KexiDBError::message()
{
	return m_text;
}

KexiDBError::~KexiDBError()
{
}

