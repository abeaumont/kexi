/* This file is part of the KDE project
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005-2010 Jarosław Staniek <staniek@kde.org>

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

#ifndef KFORMEDITOR_COMMANDS_H
#define KFORMEDITOR_COMMANDS_H

#include "kformdesigner_export.h"
#include "utils.h"
#include "objecttree.h"
#include "form.h"

#include <QHash>
#include <QVariant>

#include <kundo2command.h>

#include <QDebug>

class QPoint;
class QStringList;
class QDomElement;

namespace KFormDesigner
{

class ObjectTreeItem;
class Container;
class Form;

//! Base class for KFormDesigner's commands
class KFORMDESIGNER_EXPORT Command : public KUndo2Command
{
public:
    explicit Command(Command *parent = 0);

    explicit Command(const QString &text, Command *parent = 0);

    virtual ~Command();

    //! Reimplemented to support effect of blockRedoOnce().
    virtual void redo();

    //! Implement instead of redo().
    virtual void execute() = 0;

    virtual void debug() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const Command &c);
protected:
    //! Used to block execution of redo() once, on adding the command to the stack.
    void blockRedoOnce();

    friend class Form;
    bool m_blockRedoOnce; //!< Used to block redo() once
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const Command &c);

//! Command is used when changing a property for one or more widgets.
class KFORMDESIGNER_EXPORT PropertyCommand : public Command
{
public:
   /*! @a oldValue is the old property value for selected widget.
     This enables reverting the change. @a value is the new property value. */
    PropertyCommand(Form& form, const QByteArray &wname, const QVariant &oldValue,
                    const QVariant &value, const QByteArray &propertyName, Command *parent = 0);

   /*! @a oldValues is a QHash of the old property values for every widget,
     to allow reverting the change. @a value is the new property value.
     You can use the simpler constructor for a single widget. */
    PropertyCommand(Form& form, const QHash<QByteArray, QVariant> &oldValues,
                    const QVariant &value, const QByteArray &propertyName, Command *parent = 0);

    virtual ~PropertyCommand();

    Form* form() const;

    virtual int id() const;

    void setUniqueId(int id);

    virtual void execute();

    virtual void undo();

    bool mergeWith(const KUndo2Command * command);

    QByteArray propertyName() const;

    QVariant value() const;

    void setValue(const QVariant &value);

    const QHash<QByteArray, QVariant>& oldValues() const;

    //! @return old value if there is single value, otherwise null value.
    QVariant oldValue() const;

    //! @return widget name in case when there is only one widget
    //! with changed property in this command
    /*! Otherwise empty value is returned. */
    QByteArray widgetName() const;

    virtual void debug() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const PropertyCommand &c);
protected:
    void init();
    class Private;
    Private * const d;
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const PropertyCommand &c);

//! Command used when moving multiples widgets at the same time, while holding Ctrl or Shift.
/*! You need to supply a list of widget names, and the position of the cursor before moving. Use setPos()
  to tell the new cursor pos every time it changes.*/
class KFORMDESIGNER_EXPORT GeometryPropertyCommand : public Command
{
public:
    GeometryPropertyCommand(Form& form, const QStringList &names,
                            const QPoint& oldPos, Command *parent = 0);

    virtual ~GeometryPropertyCommand();

    virtual int id() const;

    virtual void execute();

    virtual void undo();

    void setPos(const QPoint& pos);

    QPoint pos() const;

    QPoint oldPos() const;

    virtual void debug() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const GeometryPropertyCommand &c);
protected:
    class Private;
    Private * const d;
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const GeometryPropertyCommand &c);

//! Command used when an "Align Widgets position" action is activated.
/* You just need to give the list of widget names (the selected ones), and the
  type of alignment (see the enum for possible values). */
class KFORMDESIGNER_EXPORT AlignWidgetsCommand : public Command
{
public:
    AlignWidgetsCommand(Form &form, Form::WidgetAlignment alignment, const QWidgetList &list,
                        Command *parent = 0);

    virtual ~AlignWidgetsCommand();

    virtual int id() const;

    virtual void execute();

    virtual void undo();

    virtual void debug() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const AlignWidgetsCommand &c);
protected:
    class Private;
    Private * const d;
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const AlignWidgetsCommand &c);

//! Command used when an "Adjust Widgets Size" action is activated.
/*! You just need to give the list of widget names (the selected ones),
    and the type of size modification (see the enum for possible values). */
class KFORMDESIGNER_EXPORT AdjustSizeCommand : public Command
{
public:
    enum Adjustment {
        SizeToGrid,
        SizeToFit,
        SizeToSmallWidth,
        SizeToBigWidth,
        SizeToSmallHeight,
        SizeToBigHeight
    };

    AdjustSizeCommand(Form& form, Adjustment type, const QWidgetList &list, Command *parent = 0);

    virtual ~AdjustSizeCommand();

    virtual int id() const;

    virtual void execute();

    virtual void undo();

    virtual void debug() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const AdjustSizeCommand &c);
protected:
    QSize getSizeFromChildren(ObjectTreeItem *item);

protected:
    class Private;
    Private * const d;
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const AdjustSizeCommand &c);

//! Command used when switching the layout of a container.
/*! It remembers the old pos of every widget inside the container. */
class KFORMDESIGNER_EXPORT LayoutPropertyCommand : public PropertyCommand
{
public:
    LayoutPropertyCommand(Form& form, const QByteArray &wname,
                          const QVariant &oldValue, const QVariant &value,
                          Command *parent = 0);

    virtual ~LayoutPropertyCommand();

    virtual int id() const;

    virtual void execute();

    virtual void undo();

    virtual void debug() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const LayoutPropertyCommand &c);
protected:
    class Private;
    Private * const d;
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const LayoutPropertyCommand &c);

//! Command used when inserting a widget using toolbar or menu.
/*! You only have to give the parent Container and the widget pos.
 The other information is taken from the form. */
class KFORMDESIGNER_EXPORT InsertWidgetCommand : public Command
{
public:
    explicit InsertWidgetCommand(const Container& container, Command *parent = 0);

    /*! This ctor allows to set explicit class name and position.
     Used for dropping widgets on the form surface.
     If \a namePrefix is empty, widget's unique name is constructed using
     hint for \a className (WidgetLibrary::namePrefix()),
     otherwise, \a namePrefix is used to generate widget's name.
     This allows e.g. inserting a widgets having name constructed using
     */
    InsertWidgetCommand(const Container& container, const QByteArray& className,
                        const QPoint& pos, const QByteArray& namePrefix = QByteArray(),
                        Command *parent = 0);

    virtual ~InsertWidgetCommand();

    virtual int id() const;

    virtual void execute();

    virtual void undo();

    //! @return inserted widget's name
    QByteArray widgetName() const;

    virtual void debug() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const InsertWidgetCommand &c);
protected:
    void init();

    class Private;
    Private * const d;
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const InsertWidgetCommand &c);

//! @todo add CopyWidgetCommand

//! Command used when pasting widgets.
/*! You need to give the QDomDocument containing
    the widget(s) to paste, and optionally the point where to paste widgets. */
class KFORMDESIGNER_EXPORT PasteWidgetCommand : public Command
{
public:
    PasteWidgetCommand(const QDomDocument &domDoc, const Container& container,
                       const QPoint& p = QPoint(), Command *parent = 0);

    virtual ~PasteWidgetCommand();

    virtual int id() const;

    virtual void execute();

    virtual void undo();

    virtual void debug() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const PasteWidgetCommand &c);

protected:
    /*! Internal function used to change the coordinates of a widget to \a newPos
     before pasting it (to paste it at the position of the contextual menu). It modifies
       the "geometry" property of the QDomElement representing the widget. */
    void changePos(QDomElement &el, const QPoint &newPos);

    /*! Internal function used to fix the coordinates of a widget before pasting it
       (to avoid having two widgets at the same position). It moves the widget by
       (10, 10) increment (several times if there are already pasted widgets at this position). */
    void fixPos(QDomElement &el, Container *container);

    void moveWidgetBy(QDomElement &el, Container *container, const QPoint &p);

    /*! Internal function used to fix the names of the widgets before pasting them.
      It prevents from pasting a widget with
      the same name as an actual widget. The child widgets are also fixed recursively.\n
      If the name of the widget ends with a number (eg "QLineEdit1"), the new name is
      just incremented by one (eg becomes "QLineEdit2"). Otherwise, a "2" is just
      appended at the end of the name (eg "myWidget" becomes "myWidget2"). */
    void fixNames(QDomElement &el);

protected:
    class Private;
    Private * const d;
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const PasteWidgetCommand &c);

//! Command used when deleting widgets using the "Delete" menu item.
/*! You need to give a QWidgetList of the selected widgets. */
class KFORMDESIGNER_EXPORT DeleteWidgetCommand : public Command
{
public:
    DeleteWidgetCommand(Form& form, const QWidgetList &list, Command *parent = 0);

    virtual ~DeleteWidgetCommand();

    virtual int id() const;

    virtual void execute();

    virtual void undo();

    virtual void debug() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const DeleteWidgetCommand &c);
protected:
    class Private;
    Private * const d;
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const DeleteWidgetCommand &c);

//! Command used when duplicating widgets.
/*! You need to give a QWidgetList of the selected widgets. */
class KFORMDESIGNER_EXPORT DuplicateWidgetCommand : public Command
{
public:
    DuplicateWidgetCommand(const Container& container, const QWidgetList &list,
                           const QPoint& copyToPoint, Command *parent = 0);

    virtual ~DuplicateWidgetCommand();

    virtual int id() const;

    virtual void execute();

    virtual void undo();

    virtual void debug() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const DuplicateWidgetCommand &c);
protected:
    class Private;
    Private * const d;
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const DuplicateWidgetCommand &c);

//! Command used when cutting widgets.
/*! It is basically a DeleteWidgetCommand which also updates the clipboard contents. */
class KFORMDESIGNER_EXPORT CutWidgetCommand : public DeleteWidgetCommand
{
public:
    CutWidgetCommand(Form &form, const QWidgetList &list, Command *parent = 0);

    virtual ~CutWidgetCommand();

    virtual int id() const;

    virtual void execute();

    virtual void undo();

    virtual void debug() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const CutWidgetCommand &c);
protected:
    class Private;
    Private * const d2;
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const CutWidgetCommand &c);

//! Command that holds several PropertyCommand subcommands.
/*! It appears as one to the user and in the command history. */
class KFORMDESIGNER_EXPORT PropertyCommandGroup : public Command
{
public:
    explicit PropertyCommandGroup(const QString &text, Command *parent = 0);

    virtual ~PropertyCommandGroup();

    virtual int id() const;

    virtual void execute();

    virtual void debug() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const PropertyCommandGroup &c);
protected:
    class Private;
    Private * const d;
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const PropertyCommandGroup &c);

//! Command is used when inline text is edited for a single widget.
class KFORMDESIGNER_EXPORT InlineTextEditingCommand : public Command
{
public:
   /*! @a oldValue is the old property value for selected widget.
     This enables reverting the change. @a value is the new property value. */
    InlineTextEditingCommand(
        Form& form, QWidget *widget, const QByteArray &editedWidgetClass,
        const QString &text, Command *parent = 0);

    virtual ~InlineTextEditingCommand();

    virtual int id() const;

    virtual bool mergeWith(const KUndo2Command * command);

    virtual void execute();

    virtual void debug() const;

    virtual void undo();

    Form* form() const;

    QString text() const;

    QString oldText() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const InlineTextEditingCommand &c);

protected:
    class Private;
    Private * const d;
};

//! qDebug() stream operator. Writes command group @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const InlineTextEditingCommand &c);

class KFORMDESIGNER_EXPORT InsertPageCommand : public Command
{
public:
    InsertPageCommand(Container *container, QWidget *widget);

    virtual ~InsertPageCommand();

    virtual int id() const;

    virtual void execute();

    void execute(const QString& pageWidgetName, const QString& pageName, int pageIndex);

    virtual void debug() const;

    virtual void undo();

    void undo(const QString& name);

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const InsertPageCommand &c);

protected:
    class Private;
    Private * const d;
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const InsertPageCommand &c);

class KFORMDESIGNER_EXPORT RemovePageCommand : public Command
{
public:
    RemovePageCommand(Container *container, QWidget *widget);

    virtual ~RemovePageCommand();

    virtual int id() const;

    virtual void execute();

    virtual void debug() const;

    virtual void undo();

    int pageIndex() const;

    QString pageName() const;

    friend KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const RemovePageCommand &c);

protected:
    class Private;
    Private * const d;
};

//! qDebug() stream operator. Writes command @a c to the debug output in a nicely formatted way.
KFORMDESIGNER_EXPORT QDebug operator<<(QDebug dbg, const RemovePageCommand &c);

}

#endif
