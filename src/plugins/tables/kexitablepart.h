/* This file is part of the KDE project
   Copyright (C) 2002   Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003   Joseph Wenninger<jowenn@kde.org>

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

#ifndef KEXITABLEPART_H
#define KEXITABLEPART_H

#include "kexiprojecthandler.h"
#include "kexidataprovider.h"

class QPixmap;
class KexiTablePartProxy;
class KexiTableFilterManager;

namespace KexiDB
{
	class Curosr;
}

class KEXI_HAND_TBL_EXPORT KexiTablePart : public KexiProjectHandler, public KexiDataProvider
{
	Q_OBJECT

	friend class KexiTablePartProxy;

	public:
		KexiTablePart(QObject *project,const char *,const QStringList &);

		virtual QString				name();
		virtual QString				groupName();
		virtual QString				mime();
		virtual bool				visible();

		virtual void hookIntoView(KexiView *view);
		virtual QWidget *embeddReadOnly(QWidget *, KexiView *);

		virtual void store (KoStore *){;}
		virtual void load  (KoStore *){getTables();}

		virtual QPixmap				groupPixmap();
		virtual QPixmap				itemPixmap();

		void				getTables();

		virtual QStringList datasets(QWidget*);
		virtual QStringList datasetNames(QWidget*);
		virtual QStringList fields(QWidget*,const QString& identifier);
		virtual KexiDB::Cursor* records(QWidget*,const QString& identifier,Parameters params);
		virtual ParameterList parameters(QWidget*,const QString &/*identifier*/)
					{ return ParameterList(); }

		KexiTableFilterManager *filters() { return m_filters; }

		virtual KexiDataProvider	*provider() { return this; }

	private:
		QStringList m_tableNames;
		KexiTableFilterManager *m_filters;
};

#endif
