/* This file is part of the KDE project
   Copyright (C) 2002   Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2002   Joseph Wenninger <jowenn@kde.org>

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

#include <qpixmap.h>

#include <koApplication.h>

#include <kiconloader.h>
#include <kdebug.h>
#include <klocale.h>
#include <klineeditdlg.h>
#include <kmessagebox.h>
#include <kaction.h>

#include "kexitablepartproxy.h"
#include "kexitablepart.h"
#include "kexiprojecthandleritem.h"
#include "kexidatatable.h"
#include "kexialtertable.h"
#include "tables/kexitablefiltermanager.h"
#include "tables/kexitableimportfilter.h"

KexiTablePartProxy::KexiTablePartProxy(KexiTablePart *part,KexiView *view)
 : KexiProjectHandlerProxy(part,view),KXMLGUIClient()
{
	m_tablePart=part;
	kdDebug() << "KexiTablePartProxy::KexiTablePartProxy()" << endl;

	(void*) new KAction(i18n("Create &Table..."), 0,
		this,SLOT(slotCreate()), actionCollection(), "tablepart_create");

	setXMLFile("kexitablepartui.rc");

	view->insertChildClient(this);
}

KexiPartPopupMenu*
KexiTablePartProxy::groupContext()
{
	kdDebug() << "KexiTablePart::groupContext()" << endl;
	KexiPartPopupMenu *m = new KexiPartPopupMenu(this);
	m->insertAction(i18n("Create Table..."), SLOT(slotCreate()));
	Filters f = m_tablePart->filters()->importFilters();
	if(f.count() > 0)
	{
		QPopupMenu *menuImport = new QPopupMenu(m);
		for(KexiTableFilterMeta *it = f.first(); it; it = f.next())
		{
			menuImport->insertItem(it->comment());
		}
		m->insertItem("Import", menuImport);
		connect(menuImport, SIGNAL(activated(int)), this, SLOT(slotImport(int)));
	}

	return m;
}

KexiPartPopupMenu*
KexiTablePartProxy::itemContext(const QString& identifier)
{
	kdDebug() << "KexiTablePart::itemContext()" << endl;
	KexiPartPopupMenu *m = new KexiPartPopupMenu(this);
	m->insertAction(i18n("Open Table"), SLOT(slotOpen(const QString&)));
	m->insertAction(i18n("Alter Table"), SLOT(slotAlter(const QString&)));
	m->insertAction(i18n("Delete Table"), SLOT(slotDrop(const QString&)));
	m->insertSeparator();
	m->insertAction(i18n("Create Table..."), SLOT(slotCreate()));

	return m;
}

void
KexiTablePartProxy::slotCreate()
{
	KexiProjectHandler::ItemList *list=m_tablePart->items();
	bool ok = false;
	QString name = KLineEditDlg::getText(i18n("New Table"), i18n("Table name:"), "", &ok, 0);

	if(ok && name.length() > 0)
	{
		KexiAlterTable* kat = new KexiAlterTable(kexiView(), 0, name, true, "alterTable");
		kat->show();
		list->insert("kexi/table" + name, new KexiProjectHandlerItem(part(), name, "kexi/table",  name));
		emit m_tablePart->itemListChanged(part());
	}
}

void
KexiTablePartProxy::slotOpen(const QString& identifier)
{
	if(kexiView()->activateWindow(identifier))
		return;

	kdDebug() << "KexiTablePartProxy::slotOpen(): indentifier = " << identifier << endl;
	kdDebug() << "KexiTablePartProxy::slotOpen(): kexiView = " << kexiView() << endl;

	//trying to get data
	KexiDBRecordSet  *data=m_tablePart->records(kexiView(),
		identifier,KexiDataProvider::Parameters());
	if (!data) {
		 kdDebug() <<"KexitablePartProxy::slotOpen(): error while retrieving data, aborting"<<endl;
		 return;
	}

	KexiDataTable *kt = new KexiDataTable(kexiView(), 0, identifier, "table");
	kdDebug() << "KexiTablePart::slotOpen(): indentifier = " << identifier << endl;
	kt->setDataSet(data);
}

void
KexiTablePartProxy::slotAlter(const QString& identifier)
{
	KexiAlterTable* kat = new KexiAlterTable(kexiView(), 0, part()->items()->find(identifier)->name(), false, "alterTable");
	kat->show();
}

void
KexiTablePartProxy::slotDrop(const QString& identifier)
{
        KexiProjectHandlerItem * item = part()->items()->find(identifier);

        if (!item)
        {
          KMessageBox::sorry( 0, i18n( "Table not found" ) );
          return;
        }

	QString rI = item->name();
	int ans = KMessageBox::questionYesNo(kexiView(),
		i18n("Do you really want to delete %1?").arg(rI), i18n("Delete Table?"));

	if(ans == KMessageBox::Yes)
	{
		if(kexiView()->project()->db()->query("DROP TABLE " + rI))
		{
			// FIXME: Please implement a less costly solution.
			m_tablePart->getTables();
		}
	}
}

void
KexiTablePartProxy::executeItem(const QString& identifier)
{
	slotOpen(identifier);
}

void
KexiTablePartProxy::slotImport(int filter)
{
	KexiTableFilterMeta *meta = m_tablePart->filters()->importFilters().at(0);
	kdDebug() << "KexiTablePartProxy::slotImport(): f=" << filter << "meta: " << meta << endl;

	if(!meta)
		return;

	KexiTableImportFilter *f = static_cast<KexiTableImportFilter*>(m_tablePart->filters()->getFilter(meta));
	f->open(m_tablePart->kexiProject()->db());
	m_tablePart->getTables();
}

#include "kexitablepartproxy.moc"
