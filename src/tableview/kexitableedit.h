/* This file is part of the KDE project
   Copyright (C) 2002   Peter Simonsson <psn@linux.se>

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

#ifndef _KEXITABLEEDIT_H_
#define _KEXITABLEEDIT_H_

#include <qvariant.h>
#include <qwidget.h>

class KexiTableEdit : public QWidget
{
	public:
		KexiTableEdit(QWidget* parent = 0, const char* name = 0);
		virtual QVariant value() = 0;
		virtual void resize(int w, int h);
		virtual bool eventFilter(QObject* watched, QEvent* e);
	protected:
		QWidget* m_view;
};

#endif
