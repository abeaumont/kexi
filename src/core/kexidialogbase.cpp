/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>

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

#include "keximainwindow.h"
#include "kexidialogbase.h"
#include "kexicontexthelp_p.h"

#include <kdebug.h>
#include <kapplication.h>

KexiDialogBase::KexiDialogBase(KexiMainWindow *parent, const QString &caption)
 : KMdiChildView(caption, parent, "KexiDialogBase"), KXMLGUIClient(),m_isRegistered(false)
{
	m_contextHelpInfo=new KexiContextHelpInfo();
	m_parentWindow=parent;
	setInstance(parent->instance());
	m_docID = -1;
}


KexiDialogBase::~KexiDialogBase()
{
}


void KexiDialogBase::registerDialog() {
	m_parentWindow->registerChild(this);
	m_isRegistered=true;
	m_parentWindow->addWindow((KMdiChildView *)this);
	show();
	m_parentWindow->activeWindowChanged(this);
}

bool KexiDialogBase::isRegistered(){
	return m_isRegistered;
}

KXMLGUIClient *KexiDialogBase::guiClient() {
	return this;
}

void
KexiDialogBase::setDocID(int id)
{
	kdDebug() << "KexiDialogBase::setDocID(): id = " << id << endl;
	m_docID = id;
//	m_parentWindow->registerChild(this);
}

void KexiDialogBase::setContextHelp(const QString& caption, const QString& text, const QString& iconName) {
	m_contextHelpInfo->caption=caption;
	m_contextHelpInfo->text=text;
	m_contextHelpInfo->text=iconName;
	updateContextHelp();

}


#include "kexidialogbase.moc"

