/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>

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

#include <QHeader>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDebug>

#include <KLocalizedString>

#include <KexiIcon.h>
#include <KPropertyEditorView>
#include <KPropertySet>
#include <KProperty>

#include "editlistviewdialog.h"

namespace KFormDesigner
{

//////////////////////////////////////////////////////////////////////////////////
/// A Dialog to edit the contents of a listview /////////////////////
/////////////////////////////////////////////////////////////////////////////////

EditListViewDialog::EditListViewDialog(QWidget *parent)
        : KPageDialog(parent)
{
    setObjectName("editlistview_dialog");
    setModal(true);
    setFaceType(Tabbed);
    setWindowTitle(xi18nc("@title:window", "Edit Listview Contents"));

    ///////// Setup the "Contents" page /////////////////////////////
    m_columnsPageItem = addPage(new QWidget(this), xi18n("Columns"));
    QHBoxLayout *layout = new QHBoxLayout(m_contentsPageItem->widget(), 0, 6);

    //// Setup the icon toolbar /////////////////
    QVBoxLayout *vlayout = new QVBoxLayout(layout, 3);
    QToolButton *newRow = new QToolButton(m_contentsPageItem->widget());
    newRow->setIconSet(koIcon("list-add"));
    newRow->setTextLabel(xi18n("&Add Item"), true);
    vlayout->addWidget(newRow);
    m_buttons.insert(BNewRow, newRow);
    connect(newRow, SIGNAL(clicked()), this, SLOT(newRow()));

    QToolButton *newChild = new QToolButton(m_contentsPageItem->widget());
    newChild->setIconSet(koIcon("arrow-right"));
    newChild->setTextLabel(xi18n("New &Subitem"), true);
    vlayout->addWidget(newChild);
    m_buttons.insert(BNewChild, newChild);
    connect(newChild, SIGNAL(clicked()), this, SLOT(newChildRow()));

    QToolButton *delRow = new QToolButton(m_contentsPageItem->widget());
    delRow->setIconSet(koIcon("list-remove"));
    delRow->setTextLabel(xi18n("&Delete Item"), true);
    vlayout->addWidget(delRow);
    m_buttons.insert(BRemRow, delRow);
    connect(delRow, SIGNAL(clicked()), this, SLOT(removeRow()));

    QToolButton *rowUp = new QToolButton(m_contentsPageItem->widget());
    rowUp->setIconSet(koIcon("arrow-up"));
    rowUp->setTextLabel(xi18n("Move Item &Up"), true);
    vlayout->addWidget(rowUp);
    m_buttons.insert(BRowUp, rowUp);
    connect(rowUp, SIGNAL(clicked()), this, SLOT(MoveRowUp()));

    QToolButton *rowDown = new QToolButton(m_contentsPageItem->widget());
    rowDown->setIconSet(koIcon("arrow-down"));
    rowDown->setTextLabel(xi18n("Move Item &Down"), true);
    vlayout->addWidget(rowDown);
    m_buttons.insert(BRowDown, rowDown);
    connect(rowDown, SIGNAL(clicked()), this, SLOT(MoveRowDown()));
    vlayout->addStretch();

    //// The listview ///////////
    m_listview = new KListView(m_contentsPageItem->widget());
    m_listview->setObjectName("editlistview_listview");
    m_listview->setItemsRenameable(true);
    m_listview->setItemsMovable(true);
    m_listview->setDragEnabled(true);
    m_listview->setAllColumnsShowFocus(true);
    m_listview->setRootIsDecorated(true);
    m_listview->setDropVisualizer(true);
    m_listview->setAcceptDrops(true);
    m_listview->setSorting(-1);
    layout->addWidget(m_listview);
    m_listview->setFocus();
    connect(m_listview, SIGNAL(currentChanged(QListViewItem*)), this, SLOT(updateButtons(QListViewItem*)));
    connect(m_listview, SIGNAL(moved(QListViewItem*,QListViewItem*,QListViewItem*)), this, SLOT(updateButtons(QListViewItem*)));

    /////////////////// Setup the columns page ////////////////
    m_contentsPageItem = addPage(new QWidget(this), xi18n("Contents"));
    QHBoxLayout *hbox = new QHBoxLayout(m_columnsPageItem->widget(), 0, 6);

    // The "item properties" field
    m_editor = new KPropertyEditorView(m_columnsPageItem->widget());
    m_editor->setObjectName("editcolumn_propeditor");
    m_propSet = new KPropertySet(this);
    m_propSet->addProperty(
        new KProperty("caption", "Caption", xi18n("Caption"), xi18n("Caption")));
    m_propSet->addProperty(
        new KProperty("width", 100, xi18n("Width"), xi18n("Width")));
    m_propSet->addProperty(
        new KProperty("clickable", QVariant(true), xi18n("Clickable"), xi18n("Clickable")));
    m_propSet->addProperty(
        new KProperty("resizable", QVariant(true), xi18n("Resizable"), xi18n("Resizable")));
    m_propSet->addProperty(
        new KProperty("fullwidth", QVariant(false), xi18n("Full Width"), xi18n("Full Width")));
    m_editor->changeSet(m_propSet);
    connect(m_propSet, SIGNAL(propertyChanged(KPropertySet&,KProperty&)),
            this, SLOT(changeProperty(KPropertySet&,KProperty&)));

    // Setup the icon toolbar //////////
    QVBoxLayout *vbox = new QVBoxLayout(hbox, 3);
    QToolButton *add = new QToolButton(m_columnsPageItem->widget());
    add->setIconSet(koIcon("list-add"));
    add->setTextLabel(xi18n("&Add Item"), true);
    vbox->addWidget(add);
    m_buttons.insert(BColAdd, add);
    connect(add, SIGNAL(clicked()), this, SLOT(newItem()));

    QToolButton *remove = new QToolButton(m_columnsPageItem->widget());
    remove->setIconSet(koIcon("list-remove"));
    remove->setTextLabel(xi18n("&Delete Item"), true);
    vbox->addWidget(remove);
    m_buttons.insert(BColRem, remove);
    connect(remove, SIGNAL(clicked()), this, SLOT(removeItem()));

    QToolButton *up = new QToolButton(m_columnsPageItem->widget());
    up->setIconSet(koIcon("arrow-up"));
    up->setTextLabel(xi18n("Move Item &Up"), true);
    vbox->addWidget(up);
    m_buttons.insert(BColUp, up);
    connect(up, SIGNAL(clicked()), this, SLOT(MoveItemUp()));

    QToolButton *down = new QToolButton(m_columnsPageItem->widget());
    down->setIconSet(koIcon("arrow-down"));
    down->setTextLabel(xi18n("Move Item &Down"), true);
    vbox->addWidget(down);
    m_buttons.insert(BColDown, down);
    connect(down, SIGNAL(clicked()), this, SLOT(MoveItemDown()));
    vbox->addStretch();

    // The listbox with columns name /////
    m_listbox = new KListBox(m_columnsPageItem->widget(), "editlistview_columns");
    m_listbox->setFocus();
    hbox->insertWidget(0, m_listbox);
    hbox->addWidget(m_editor);
    connect(m_listbox, SIGNAL(currentChanged(QListBoxItem*)), this, SLOT(updateItemProperties(QListBoxItem*)));

    //// Init dialog and display it ////////////////////////
//! @todo KEXI3 not available in kdelibs4: setInitialSize(QSize(500, 300), true);
}

int
EditListViewDialog::exec(QListView *listview)
{
    if (!listview) {
        qWarning() << "EditListViewDialog ERROR: no listview ";
        return 0;
    }

    // We copy the contents of the listview into our listview
    for (int i = 0; i < listview->columns(); i++) {
        m_listview->addColumn(listview->columnText(i), listview->columnWidth(i));
        m_listview->header()->setClickEnabled(listview->header()->isClickEnabled(i), i);
        m_listview->header()->setResizeEnabled(listview->header()->isResizeEnabled(i), i);
        m_listview->header()->setStretchEnabled(listview->header()->isStretchEnabled(i), i);
        m_listview->setRenameable(i, true);
    }
    QListViewItem *item = listview->firstChild();
    while (item)  {
        loadChildNodes(m_listview, item, 0);
        item = item->nextSibling();
    }

    m_listview->setSelected(m_listview->firstChild(), true);
    if (!m_listview->firstChild())
        updateButtons(0);

    for (int i = 0; i < listview->columns(); i++)
        m_listbox->insertItem(listview->columnText(i));
    m_listbox->setSelected(0, true);

    // and we exec the dialog
    int r =  KPageDialog::exec();
    if (r == QDialog::Accepted) {
        listview->clear();
        // We copy the contents of our listview back in the listview
        for (int i = 0; i < m_listview->columns(); i++) {
            if (listview->columns() <= i)
                listview->addColumn(m_listview->columnText(i), m_listview->columnWidth(i));
            else {
                listview->setColumnText(i, m_listview->columnText(i));
                listview->setColumnWidth(i, m_listview->columnWidth(i));
            }
            listview->header()->setClickEnabled(m_listview->header()->isClickEnabled(i), i);
            listview->header()->setResizeEnabled(m_listview->header()->isResizeEnabled(i), i);
            listview->header()->setStretchEnabled(m_listview->header()->isStretchEnabled(i), i);
        }

        QListViewItem *item = m_listview->firstChild();
        while (item) {
            loadChildNodes(listview, item, 0);
            item = item->nextSibling();
        }
    }
    return r;
}

/// Columns page slots ///////
void
EditListViewDialog::changeProperty(KPropertySet& set, KProperty& property)
{
    if (&set != m_propSet)
        return;

    QString name = property.name();
    QVariant value = property.value();
    if (name == "caption") {
        m_propSet->blockSignals(true); // we need to block signals because changeItem will modify selection, and call updateItemProperties
        m_listbox->changeItem(value.toString(), m_listbox->currentItem());
        m_listview->setColumnText(m_listbox->currentItem(), value.toString());
        m_propSet->blockSignals(false);
    } else if (name == "width")
        m_listview->setColumnWidth(m_listbox->currentItem(), value.toInt());
    else if (name == "resizable")
        m_listview->header()->setResizeEnabled(value.toBool(), m_listbox->currentItem());
    else if (name == "clickable")
        m_listview->header()->setClickEnabled(value.toBool(), m_listbox->currentItem());
    else if (name == "fullwidth")
        m_listview->header()->setStretchEnabled(value.toBool(), m_listbox->currentItem());
}

void
EditListViewDialog::updateItemProperties(QListBoxItem *item)
{
    if (!item)
        return;

    int id = m_listbox->index(item);
    if (m_propSet) {
        m_propSet->blockSignals(true); // we don't want changeProperty to be called
        (*m_propSet)["caption"].setValue(m_listview->columnText(id), false);
        (*m_propSet)["width"].setValue(m_listview->columnWidth(id), false);
        (*m_propSet)["clickable"].setValue(QVariant(m_listview->header()->isClickEnabled(id)), false);
        (*m_propSet)["resizable"].setValue(QVariant(m_listview->header()->isResizeEnabled(id)), false);
        (*m_propSet)["fullwidth"].setValue(QVariant(m_listview->header()->isStretchEnabled(id)), false);
        m_propSet->blockSignals(false);
        m_editor->changeSet(m_propSet);
    }

    m_buttons[BColUp]->setEnabled(item->prev());
    m_buttons[BColDown]->setEnabled(item->next());
}

void
EditListViewDialog::newItem()
{
    m_listbox->insertItem(xi18n("New Column"));
    m_listview->addColumn(xi18n("New Column"));
    m_listview->setRenameable(m_listview->columns() - 1, true);
    m_listbox->setCurrentItem(m_listbox->count() - 1);
    m_buttons[BColRem]->setEnabled(true);
}

void
EditListViewDialog::removeItem()
{
    int current = m_listbox->currentItem();
    if (m_listbox->item(current + 1))
        m_listbox->setCurrentItem(current + 1);
    else
        m_listbox->setCurrentItem(current - 1);

    m_listview->removeColumn(current);
    m_listbox->removeItem(current);
    if (m_listbox->count() == 0)
        m_buttons[BColRem]->setEnabled(false);
}

void
EditListViewDialog::MoveItemUp()
{
    int current = m_listbox->currentItem();
    QString text = m_listbox->text(current);
    m_listbox->blockSignals(true);

    m_listbox->changeItem(m_listbox->text(current - 1), current);
    m_listview->setColumnText(current, m_listview->columnText(current - 1));
    m_listview->setColumnWidth(current, m_listview->columnWidth(current - 1));
    m_listview->header()->setClickEnabled(m_listview->header()->isClickEnabled(current - 1), current);
    m_listview->header()->setResizeEnabled(m_listview->header()->isResizeEnabled(current - 1), current);
    m_listview->header()->setStretchEnabled(m_listview->header()->isStretchEnabled(current - 1), current);

    m_listbox->changeItem(text, current - 1);
    m_listview->setColumnText(current - 1, (*m_propSet)["caption"].value().toString());
    m_listview->setColumnWidth(current - 1, (*m_propSet)["width"].value().toBool());
    m_listview->header()->setClickEnabled((*m_propSet)["clickable"].value().toBool(), current - 1);
    m_listview->header()->setResizeEnabled((*m_propSet)["resizable"].value().toBool(), current - 1);
    m_listview->header()->setStretchEnabled((*m_propSet)["fullwidth"].value().toBool(), current - 1);

    m_listbox->blockSignals(false);
    m_listbox->setCurrentItem(current - 1);
}

void
EditListViewDialog::MoveItemDown()
{
    int current = m_listbox->currentItem();
    QString text = m_listbox->text(current);
    m_listbox->blockSignals(true);

    m_listbox->changeItem(m_listbox->text(current + 1), current);
    m_listview->setColumnText(current, m_listview->columnText(current + 1));
    m_listview->setColumnWidth(current, m_listview->columnWidth(current + 1));
    m_listview->header()->setClickEnabled(m_listview->header()->isClickEnabled(current + 1), current);
    m_listview->header()->setResizeEnabled(m_listview->header()->isResizeEnabled(current + 1), current);
    m_listview->header()->setStretchEnabled(m_listview->header()->isStretchEnabled(current + 1), current);

    m_listbox->changeItem(text, current + 1);
    m_listview->setColumnText(current + 1, (*m_propSet)["caption"].value().toString());
    m_listview->setColumnWidth(current + 1, (*m_propSet)["width"].value().toBool());
    m_listview->header()->setClickEnabled((*m_propSet)["clickable"].value().toBool(), current + 1);
    m_listview->header()->setResizeEnabled((*m_propSet)["resizable"].value().toBool(), current + 1);
    m_listview->header()->setStretchEnabled((*m_propSet)["fullwidth"].value().toBool(), current + 1);

    m_listbox->blockSignals(false);
    m_listbox->setCurrentItem(current + 1);
}


/// Contents page slots ////////
void
EditListViewDialog::updateButtons(QListViewItem *item)
{
    if (!item) {
        foreach (QToolButton *button, m_buttons) {
            button->setEnabled(false);
        }
        return;
    }

    m_buttons[BNewChild]->setEnabled(true);
    m_buttons[BRemRow]->setEnabled(true);
    m_buttons[BRowUp]->setEnabled((item->itemAbove() && (item->itemAbove()->parent() == item->parent())));
    m_buttons[BRowDown]->setEnabled(item->nextSibling());
}

void
EditListViewDialog::loadChildNodes(QListView *listview, QListViewItem *item, QListViewItem *parent)
{
    QListViewItem *newItem;
    if (listview->inherits("KListView")) {
        if (parent)
            newItem = new KListViewItem(parent);
        else
            newItem = new KListViewItem(listview);
    } else {
        if (parent)
            newItem = new QListViewItem(parent);
        else
            newItem = new QListViewItem(listview);
    }

    // We need to move the item at the end, which is the expected behaviour (by default it is inserted at the beginning)
    QListViewItem *last;
    if (parent)
        last = parent->firstChild();
    else
        last = listview->firstChild();

    while (last->nextSibling())
        last = last->nextSibling();
    newItem->moveItem(last);

    // We copy the text of all the columns
    for (int i = 0; i < listview->columns(); i++)
        newItem->setText(i, item->text(i));

    QListViewItem *child = item->firstChild();
    if (child)
        newItem->setOpen(true);
    while (child)  {
        loadChildNodes(listview, child, newItem);
        child = child->nextSibling();
    }
}

void
EditListViewDialog::newRow()
{
    KListViewItem *parent = (KListViewItem*)m_listview->selectedItem();
    if (parent)
        parent = (KListViewItem*)parent->parent();
    KListViewItem *item;
    if (parent)
        item = new KListViewItem(parent, m_listview->selectedItem());
    else
        item = new KListViewItem(m_listview, m_listview->selectedItem());
    item->setText(0, xi18n("New Item"));
    m_listview->setCurrentItem(item);
}

void
EditListViewDialog::newChildRow()
{
    KListViewItem *parent = (KListViewItem*)m_listview->currentItem();
    KListViewItem *item;
    if (parent)
        item = new KListViewItem(parent);
    else
        item = new KListViewItem(m_listview, m_listview->currentItem());
    item->setText(0, xi18n("Sub Item"));

    m_listview->setCurrentItem(item);
    if (parent)
        parent->setOpen(true);
}

void
EditListViewDialog::removeRow()
{
    delete m_listview->currentItem();
}

void
EditListViewDialog::MoveRowUp()
{
    QListViewItem *item = m_listview->currentItem()->itemAbove();
    item->moveItem(m_listview->currentItem());
    updateButtons(m_listview->currentItem());
}

void
EditListViewDialog::MoveRowDown()
{
    QListViewItem *before = m_listview->currentItem();
    before->moveItem(before->nextSibling());
    updateButtons(before);
}

}

//! @todo KEXI3 noi18n # added to disable message extraction in Messages.sh
