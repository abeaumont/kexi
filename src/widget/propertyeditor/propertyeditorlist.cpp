/* This file is part of the KDE project
   Copyright (C) 2002   Lucijan Busch <lucijan@gmx.at>

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

#include <klocale.h>
#include <qstringlist.h>
#include <qcursor.h>
#include <klistbox.h>
#include <kdebug.h>

#include "propertyeditorlist.h"
#include "kexiproperty.h"

PropComboBox::PropComboBox(QWidget *parent, bool multi)
   : KComboBox(parent)
{
	m_listbox = 0;
	if(multi)
	{
	m_listbox = new KListBox(this);
	m_listbox->setSelectionMode(QListBox::Multi);
	setEditable(true);
	setListBox(m_listbox);
	
	disconnect(m_listbox, 0, this, 0);
	connect(m_listbox, SIGNAL(selected(QListBoxItem*)), this, SLOT(updateEdit()));
	connect(m_listbox, SIGNAL(returnPressed(QListBoxItem *)), this, SLOT(hideList()));
	}
}

bool
PropComboBox::eventFilter(QObject *o, QEvent *e)
{
	if(o == lineEdit())
	{
	if(e->type() == QEvent::KeyPress)
	{
		QKeyEvent* ev = static_cast<QKeyEvent*>(e);
		if((ev->key()==Key_Up || ev->key()==Key_Down) && ev->state()!=ControlButton)
		{
			parentWidget()->eventFilter(o, e);
			return true;
		}
	}
	}
	if(o==m_listbox)
	{
	if(e->type() == QEvent::Show)
	{
		QString s = lineEdit()->text();
		setSelected(QStringList::split("|",s));
	}
	}
	
	return KComboBox::eventFilter(o, e);
}

void
PropComboBox::setSelected(const QStringList &list)
{
	QStringList strlist(list);
	m_listbox->clearSelection();
	for(QStringList::iterator it = strlist.begin(); it != strlist.end(); ++it)
	{
		QListBoxItem *item = m_listbox->findItem(*it, Qt::ExactMatch);
		if(item)
			m_listbox->setSelected(item,true);
	}
	setEditText(list.join("|"));
}

QStringList
PropComboBox::getSelected()
{
	QStringList list;

	for(uint i=0; i < m_listbox->count(); i++)
	{
		if(m_listbox->isSelected(i))
			list.append(m_listbox->text(i));
	}
	return list;
}

void
PropComboBox::updateEdit()
{
	QStringList list = getSelected();
	if(!list.isEmpty())
	{
		setEditText(list.join("|"));
	}
	else
		setEditText("");
	emit activated(1);
}

void
PropComboBox::hideList()
{
	m_listbox->hide();
	lineEdit()->setFocus();
}

//EDITOR

PropertyEditorList::PropertyEditorList(QWidget *parent, KexiProperty *property, const char *name)
 : KexiPropertySubEditor(parent, property, name)
{
	m_combo = new PropComboBox(this, false);

	m_combo->setGeometry(frameGeometry());
	m_combo->setEditable(true);
	m_combo->setInsertionPolicy(QComboBox::NoInsertion);
	m_combo->setAutoCompletion(true);
	if(property->list())
	{
	m_combo->insertStringList(*(property->list()));
	m_combo->setCurrentText(property->value().asString());
	KCompletion *comp = m_combo->completionObject();
	comp->insertItems(*(property->list()));
	}
	m_combo->show();

	setWidget(m_combo);
	connect(m_combo, SIGNAL(activated(int)), SLOT(valueChanged()));
}

QVariant
PropertyEditorList::getValue()
{
	return QVariant(m_combo->currentText());
}

void
PropertyEditorList::setValue(const QVariant &value)
{
	m_combo->setCurrentText(value.toString());
	emit changed(this);
}

void
PropertyEditorList::setList(QStringList l)
{
	m_combo->insertStringList(l);
}

void
PropertyEditorList::valueChanged()
{
	emit changed(this);
}

//Multiple selection editor (for OR'ed values)

PropertyEditorMultiList::PropertyEditorMultiList(QWidget *parent, KexiProperty *property, const char *name)
 : KexiPropertySubEditor(parent, property, name)
{
	m_combo = new PropComboBox(this, true);

	m_combo->setGeometry(frameGeometry());
	m_combo->setInsertionPolicy(QComboBox::NoInsertion);
	m_combo->setAutoCompletion(true);
	if(property->list())
	{
	m_combo->insertStringList(*(property->list()));
	m_combo->setSelected(property->value().asStringList());
	m_combo->setEditText(property->value().toStringList().join("|"));
	KCompletion *comp = m_combo->completionObject();
	comp->insertItems(*(property->list()));
	}
	m_combo->show();

	setWidget(m_combo);
	connect(m_combo, SIGNAL(activated(int)), SLOT(valueChanged()));
}

QVariant
PropertyEditorMultiList::getValue()
{
	return QVariant(m_combo->getSelected());
}

void
PropertyEditorMultiList::setValue(const QVariant &value)
{
	m_combo->setSelected(value.toStringList());
	emit changed(this);
}

void
PropertyEditorMultiList::valueChanged()
{
	emit changed(this);
}

void
PropertyEditorMultiList::setList(QStringList l)
{
	m_combo->insertStringList(l);
}


// Cursor Editor

PropertyEditorCursor::PropertyEditorCursor(QWidget *parent, KexiProperty *property, const char *name)
   : PropertyEditorList(parent, property, name)
{
	m_combo->setEditable(false);
	m_combo->insertItem(i18n("Arrow"), Qt::ArrowCursor);
	m_combo->insertItem(i18n("Up Arrow"), Qt::UpArrowCursor);
	m_combo->insertItem(i18n("Cross"), Qt::CrossCursor);
	m_combo->insertItem(i18n("Waiting"), Qt::WaitCursor);
	m_combo->insertItem(i18n("iBeam"), Qt::IbeamCursor);
	m_combo->insertItem(i18n("Size Vertical"), Qt::SizeVerCursor);
	m_combo->insertItem(i18n("Size Horizontal"), Qt::SizeHorCursor);
	m_combo->insertItem(i18n("Size Slash"), Qt::SizeBDiagCursor);
	m_combo->insertItem(i18n("Size Backslash"), Qt::SizeFDiagCursor);
	m_combo->insertItem(i18n("Size All"), Qt::SizeAllCursor);
	m_combo->insertItem(i18n("Blank"), Qt::BlankCursor);
	m_combo->insertItem(i18n("Split Vertical"), Qt::SplitVCursor);
	m_combo->insertItem(i18n("Split Horizontal"), Qt::SplitHCursor);
	m_combo->insertItem(i18n("Pointing Hand"), Qt::PointingHandCursor);
	m_combo->insertItem(i18n("Forbidden"), Qt::ForbiddenCursor);
	m_combo->insertItem(i18n("Whats this"), Qt::WhatsThisCursor);
	
	m_combo->setCurrentItem(property->value().toCursor().shape());
}

QVariant
PropertyEditorCursor::getValue()
{
	return QCursor(m_combo->currentItem());
}

void
PropertyEditorCursor::setValue(const QVariant &value)
{
	m_combo->setCurrentItem(value.toCursor().shape());
	emit changed(this);
}

#include "propertyeditorlist.moc"
