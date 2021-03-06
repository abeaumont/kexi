/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2008-2011 Jarosław Staniek <staniek@kde.org>

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

#include <QPainter>
#include <QRect>
#include <QEvent>
#include <QLayout>
#include <QCursor>
#include <QGridLayout>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDebug>

#include "utils.h"
#include "container.h"
#include "widgetlibrary.h"
#include "objecttree.h"
#include "form.h"
#include "commands.h"
#include "events.h"
#include "FormWidget.h"
#include <kexiutils/utils.h>

using namespace KFormDesigner;

EventEater::EventEater(QWidget *widget, QObject *container)
        : QObject(container)
{
    m_widget = widget;
    m_container = container;

    KexiUtils::installRecursiveEventFilter(m_widget, this);
}

bool
EventEater::eventFilter(QObject *o, QEvent *ev)
{
    if (!m_container)
        return false;
    if (ev->type() == QEvent::MouseButtonPress && o->inherits("QTabBar")) {
        QMouseEvent *mev = static_cast<QMouseEvent*>(ev);
        if (mev->button() == Qt::RightButton) {
            // (because of tab widget specifics) block right-click for tab widget's tab bar,
            // otherwise form will be selected!
            return true;
        }
    }

    return m_container->eventFilter(m_widget, ev);
}

EventEater::~EventEater()
{
    if (m_widget)
        KexiUtils::removeRecursiveEventFilter(m_widget, this);
}

void EventEater::setContainer(QObject *container)
{
    m_container = container;
}

// Container itself

class Q_DECL_HIDDEN Container::Private
{
public:
    Private(Container* toplevel_, QWidget *container)
      : state(DoingNothing)
      , idOfPropertyCommand(0)
      , toplevel(toplevel_)
      , widget(container)
      , layout(0)
      , layType(Form::NoLayout)
      , moving(0)
      , tree(0)
      , mousePressEventReceived(false)
      , mouseReleaseEvent(QEvent::None, QPoint(), Qt::NoButton, Qt::NoButton, Qt::NoModifier)
      , insertBegin(-1, -1)
    {
        if (toplevel)
            form = toplevel->form();
    }

    ~Private()
    {
    }

    //! for inserting/selection
    void startSelectionOrInsertingRectangle(const QPoint& begin)
    {
        insertBegin = begin;
    }
    void stopSelectionRectangleOrInserting()
    {
        insertBegin = QPoint(-1, -1);
        insertRect = QRect();
    }
    void updateSelectionOrInsertingRectangle(const QPoint& end)
    {
        if (!selectionOrInsertingStarted()) {
            stopSelectionRectangleOrInserting();
            return;
        }
        QRect oldInsertRect( insertRect );
        insertRect.setTopLeft( QPoint(
            qMin(insertBegin.x(), end.x()),
            qMin(insertBegin.y(), end.y()) ) );
        insertRect.setBottomRight( QPoint(
            qMax(insertBegin.x(), end.x()) - 1,
            qMax(insertBegin.y(), end.y()) - 1) ); // minus 1 to make the size correct
        QRect toUpdate( oldInsertRect.united(insertRect) );
        toUpdate.setWidth(toUpdate.width()+1);
        toUpdate.setHeight(toUpdate.height()+1);
        widget->update(toUpdate);
    }
    bool selectionOrInsertingStarted() const
    {
        return insertBegin != QPoint(-1, -1);
    }
    QRect selectionOrInsertingRectangle() const {
        return insertRect;
    }
    QPoint selectionOrInsertingBegin() const {
        return insertBegin;
    }

    void widgetDeleted() {
        widget = 0;
    }

    QPointer<Form> form;

    enum State {
      DoingNothing,
      DrawingSelectionRect,
      CopyingWidget,
      MovingWidget,
      InlineEditing
    };
    State state;

    int idOfPropertyCommand;

    //! the watched container and it's toplevel one...
    QPointer<Container> toplevel;
    QPointer<QWidget> widget;

    // Layout
    QLayout *layout;
    Form::LayoutType layType;
    int margin, spacing;

    QPoint grab;
    QPointer<QWidget> moving;

    //inserting
    ObjectTreeItem *tree;

    bool mousePressEventReceived;
    QMouseEvent mouseReleaseEvent;
    QPointer<QObject> objectForMouseReleaseEvent;

    QPoint insertBegin;
    QRect insertRect;
};

//---------------------------------------------------------------------------------------------------------------------

Container::Container(Container *toplevel, QWidget *container, QObject *parent)
        : QObject(parent)
        , d(new Private(toplevel, container))
{
    QByteArray classname = container->metaObject()->className();
    if ((classname == "HBox") || (classname == "Grid") || (classname == "VBox") ||
            (classname == "HFlow")  || (classname == "VFlow"))
        d->margin = 4; // those containers don't have frames, so little margin
    else
        d->margin = d->form ? d->form->defaultMargin() : 0;
    d->spacing = d->form ? d->form->defaultSpacing() : 0;

    if (toplevel) {
        //qDebug() << "Creating ObjectTreeItem:";
        ObjectTreeItem *it = new ObjectTreeItem(d->form->library()->displayName(classname),
                                                widget()->objectName(), widget(), this, this);
        setObjectTree(it);

        if (parent->isWidgetType()) {
            QString n = parent->objectName();
            ObjectTreeItem *parent = d->form->objectTree()->lookup(n);
            d->form->objectTree()->addItem(parent, it);
        }
        else {
            d->form->objectTree()->addItem(toplevel->objectTree(), it);
        }

        connect(toplevel, SIGNAL(destroyed()), this, SLOT(widgetDeleted()));
    }

    connect(container, SIGNAL(destroyed()), this, SLOT(widgetDeleted()));
}

Container::~Container()
{
    delete d;
}

Form* Container::form() const
{
    return d->form;
}

QWidget* Container::widget() const
{
    return d->widget;
}

void
Container::setForm(Form *form)
{
    d->form = form;
    d->margin = d->form ? d->form->defaultMargin() : 0;
    d->spacing = d->form ? d->form->defaultSpacing() : 0;
}

bool
Container::eventFilter(QObject *s, QEvent *e)
{
#ifdef KFD_SIGSLOTS
    const bool connecting = d->form->state() == Form::Connecting;
#else
    const bool connecting = false;
#endif
    switch (e->type()) {
    case QEvent::MouseButtonPress: {
        d->stopSelectionRectangleOrInserting();
        d->mousePressEventReceived = true;

        //qDebug() << "sender object =" << s->objectName() << "of type =" << s->metaObject()->className()
        //    << "this =" << this->objectName() << metaObject()->className();

        d->moving = static_cast<QWidget*>(s);
        d->idOfPropertyCommand++; // this will create another PropertyCommand
        if (d->moving->parentWidget() && KexiUtils::objectIsA(d->moving->parentWidget(), "QStackedWidget")) {
            //qDebug() << "widget is a stacked widget's page";
            d->moving = d->moving->parentWidget(); // widget is a stacked widget's page
        }
        if (d->moving->parentWidget() && d->moving->parentWidget()->inherits("QTabWidget")) {
            //qDebug() << "widget is a tab widget page";
            d->moving = d->moving->parentWidget(); // widget is a tab widget page
        }
        QMouseEvent *mev = static_cast<QMouseEvent*>(e);
        d->grab = QPoint(mev->x(), mev->y());

#ifdef KFD_SIGSLOTS
        // we are drawing a connection
        if (d->form->state() == Form::Connecting)  {
            drawConnection(mev);
            return true;
        }
#endif

        if ((mev->modifiers() == Qt::ControlModifier || mev->modifiers() == Qt::ShiftModifier)
                && d->form->state() != Form::WidgetInserting) { // multiple selection mode
            if (d->form->selectedWidgets()->contains(d->moving)) { // widget is already selected
                if (d->form->selectedWidgets()->count() > 1) // we remove it from selection
                    deselectWidget(d->moving);
                else { // the widget is the only selected, so it means we want to copy it
                    d->state = Private::CopyingWidget;
                }
            }
            else {
                // the widget is not yet selected, we add it
                Form::WidgetSelectionFlags flags = Form::AddToPreviousSelection
                    | Form::LastSelection;
                if (mev->button() == Qt::RightButton) {
                    flags |= Form::DontRaise;
                }
                selectWidget(d->moving, flags);
            }
        }
        else if ((d->form->selectedWidgets()->count() > 1)) { // more than one widget selected
            if (!d->form->selectedWidgets()->contains(d->moving)) {
                // widget is not selected, it becomes the only selected widget
                Form::WidgetSelectionFlags flags
                    = Form::ReplacePreviousSelection | Form::LastSelection;
                if (mev->button() == Qt::RightButton) {
                    flags |= Form::DontRaise;
                }
                selectWidget(d->moving, flags);
            }
            // If the widget is already selected, we do nothing (to ease widget moving, etc.)
        }
        else {// if(!d->form->manager()->isInserting())
            Form::WidgetSelectionFlags flags
                = Form::ReplacePreviousSelection | Form::LastSelection;
            if (mev->button() == Qt::RightButton) {
                flags |= Form::DontRaise;
            }
            selectWidget(d->moving, flags);
        }

        // we are inserting a widget or drawing a selection rect in the form
        if ((d->form->state() == Form::WidgetInserting) || ((s == widget()) && !d->toplevel)) {
            int tmpx, tmpy;
            if (    !d->form->isSnapToGridEnabled()
                 || d->form->state() != Form::WidgetInserting
                 || (mev->buttons() == Qt::LeftButton && mev->modifiers() == (Qt::ControlModifier | Qt::AltModifier))
               )
            {
                tmpx = mev->x();
                tmpy = mev->y();
            }
            else {
                int grid = d->form->gridSize();
                tmpx = alignValueToGrid(mev->x(), grid);
                tmpy = alignValueToGrid(mev->y(), grid);
            }
            d->startSelectionOrInsertingRectangle( (static_cast<QWidget*>(s))->mapTo(widget(), QPoint(tmpx, tmpy)) );
            if (d->form->state() != Form::WidgetInserting) {
                d->state = Private::DrawingSelectionRect;
            }
        }
        else {
            if (s->inherits("QTabWidget")) // to allow changing page by clicking tab
                return false;
        }

        if (d->objectForMouseReleaseEvent) {
            const bool res = handleMouseReleaseEvent(d->objectForMouseReleaseEvent, &d->mouseReleaseEvent);
            d->objectForMouseReleaseEvent = 0;
            return res;
        }
        return true;
    }

    case QEvent::MouseButtonRelease: {
        QMouseEvent *mev = static_cast<QMouseEvent*>(e);
        if (!d->mousePressEventReceived) {
            d->mouseReleaseEvent = *mev;
            d->objectForMouseReleaseEvent = s;
            return true;
        }
        d->mousePressEventReceived = false;
        d->objectForMouseReleaseEvent = 0;
        return handleMouseReleaseEvent(s, mev);
    }

    case QEvent::MouseMove: {
        QMouseEvent *mev = static_cast<QMouseEvent*>(e);
        if (d->selectionOrInsertingStarted() && d->form->state() == Form::WidgetInserting
            && (   (mev->buttons() == Qt::LeftButton)
                || (mev->buttons() == Qt::LeftButton && mev->modifiers() == Qt::ControlModifier)
                || (mev->buttons() == Qt::LeftButton && mev->modifiers() == (Qt::ControlModifier | Qt::AltModifier))
                || (mev->buttons() == Qt::LeftButton && mev->modifiers() == Qt::ShiftModifier)
               )
           )
        {
            QPoint realPos;
            if (d->form->isSnapToGridEnabled()) {
                const int gridX = d->form->gridSize();
                const int gridY = d->form->gridSize();
                realPos = QPoint(
                    alignValueToGrid(mev->pos().x(), gridX),
                    alignValueToGrid(mev->pos().y(), gridY));
            }
            else {
                realPos = mev->pos();
            }
            d->updateSelectionOrInsertingRectangle(realPos);
            return true;
        }
#ifdef KFD_SIGSLOTS
        // Creating a connection, we highlight sender and receiver, and we draw a link between them
        else if (connecting && !FormManager::self()->createdConnection()->sender().isNull()) {
            ObjectTreeItem *tree = d->form->objectTree()->lookup(
                FormManager::self()->createdConnection()->sender());
            if (!tree || !tree->widget())
                return true;

            if (d->form->formWidget() && (tree->widget() != s))
                d->form->formWidget()->highlightWidgets(tree->widget(), static_cast<QWidget*>(s));
        }
#endif
        else if (d->selectionOrInsertingStarted()
                   && s == widget()
                   && !d->toplevel
                   && (mev->modifiers() != Qt::ControlModifier)
                   && !connecting
            )
        { // draw the selection rect
            if ((mev->buttons() != Qt::LeftButton) || d->state == Private::InlineEditing)
                return true;
            d->updateSelectionOrInsertingRectangle(mev->pos());

//! @todo?            if (d->form->formWidget())
//! @todo?                d->form->formWidget()->drawRect(r, 1);

            d->state = Private::DoingNothing;
            return true;
        }
        else if (mev->buttons() == Qt::LeftButton && mev->modifiers() == Qt::ControlModifier) {
            //! @todo draw the insert rect for the copied widget
//            if (s == widget()) {
//                return true;
//            }
            return true;
        }
        else if ( (   (mev->buttons() == Qt::LeftButton && mev->modifiers() == Qt::NoModifier)
                   || (mev->buttons() == Qt::LeftButton && mev->modifiers() == (Qt::ControlModifier | Qt::AltModifier))
                  )
                  && d->form->state() != Form::WidgetInserting
                  && d->state != Private::CopyingWidget
                )
        {
            // we are dragging the widget(s) to move it
            if (!d->toplevel && d->moving == widget()) // no effect for form
                return false;
            if (!d->moving || !d->moving->parentWidget())
                return true;

            moveSelectedWidgetsBy(mev->x() - d->grab.x(), mev->y() - d->grab.y());
            d->state = Private::MovingWidget;
        }

        return true; // eat
    }

    case QEvent::Paint: { // Draw the dotted background
        if (s != widget())
            return false;
        if (widget()->inherits("ContainerWidget")) {
            QWidget *parentContainer = widget()->parentWidget()->parentWidget()->parentWidget();
            if (parentContainer->inherits("KexiDBForm") || parentContainer->inherits("ContainerWidget")) {
                // do not display grid on ContainerWidget (e.g. inside of tab widget) becasue it's already done at higher level
                return false;
            }
        }
        QPaintEvent* pe = static_cast<QPaintEvent*>(e);
        QPainter p(widget());
        p.setRenderHint(QPainter::Antialiasing, false);
#if 1 // grid
//#define DEBUG_PAINTER
#ifdef DEBUG_PAINTER
    qDebug() << "would draw grid" << pe->rect();
    QTime t;
    t.start();
    long points= 0;
#endif
        int gridX = d->form->gridSize();
        int gridY = d->form->gridSize();

        QColor c1(Qt::white);
        c1.setAlpha(100);
        QColor c2(Qt::black);
        c2.setAlpha(100);
        int cols = widget()->width() / gridX;
        int rows = widget()->height() / gridY;
        const QRect r( pe->rect() );
        // for optimization, compute the start/end row and column to paint
        int startRow = (r.top()-1) / gridY;
        startRow = qMax(startRow, 1);
        int endRow = (r.bottom()+1) / gridY;
        endRow = qMin(endRow, rows);
        int startCol = (r.left()-1) / gridX;
        startCol = qMax(startCol, 1);
        int endCol = (r.right()+1) / gridX;
        endCol = qMin(endCol, cols);
        QVector<QPoint> gridpoints;
        for (int rowcursor = startRow; rowcursor <= endRow; ++rowcursor) {
            for (int colcursor = startCol; colcursor <= endCol; ++colcursor) {
                  gridpoints << QPoint(colcursor * gridX - 1,rowcursor * gridY - 1);
#ifdef DEBUG_PAINTER
    points++;
#endif
            }
        }

//! @todo when container's background is not solid color, find better grid color,
//!       e.g. buffer the background for pixmaps or gradients
        p.setPen(KexiUtils::contrastColor(widget()->palette().background().color()));
        p.drawPoints(gridpoints);
#ifdef DEBUG_PAINTER
    qDebug() << "millisecs:" << t.elapsed() << "points:" << points;
#endif
#endif
        if (d->selectionOrInsertingRectangle().isValid()) {
            QColor sc1(Qt::white);
            sc1.setAlpha(220);
            QColor sc2(Qt::black);
            sc2.setAlpha(200);
            QPen selPen1(sc2, 1.0);
            QPen selPen2(sc1, 1.0, Qt::CustomDashLine);
            QVector<qreal> dashes;
            dashes << 2 << 2;
            selPen2.setDashPattern(dashes);
            QRect selectionOrInsertingRectangle(d->selectionOrInsertingRectangle());
            selectionOrInsertingRectangle.setSize(
                selectionOrInsertingRectangle.size() - QSize(1,1)); // -(1,1) because the rect is painted
                                                                    // up to the next pixel in Qt
            p.setPen(selPen1);
            p.drawRect(selectionOrInsertingRectangle);
            p.setPen(selPen2);
            p.drawRect(selectionOrInsertingRectangle);
        }
        return false;
    }

    case QEvent::Resize: { // we are resizing a widget, so we enter MovingWidget state
                           // -> the layout will be reloaded when releasing mouse
        if (d->form->interactiveMode())
            d->state = Private::MovingWidget;
        break;
    }

    //case QEvent::AccelOverride:
    case QEvent::KeyPress: {
        QKeyEvent *kev = static_cast<QKeyEvent*>(e);

        if (kev->key() == Qt::Key_F2) { // pressing F2 == double-clicking
            QWidget *w;

            // try to find the widget which was clicked last and should be edited
            if (d->form->selectedWidgets()->count() == 1)
                w = d->form->selectedWidgets()->first();
            else if (d->form->selectedWidgets()->contains(d->moving))
                w = d->moving;
            else
                w = d->form->selectedWidgets()->last();
            if (d->form->library()->startInlineEditing(w->metaObject()->className(), w, this)) {
                d->state = Private::InlineEditing;
            }
        }
        else if (kev->key() == Qt::Key_Escape) {
            if (false) {
            }
#ifdef KFD_SIGSLOTS
            else if (connecting) {
                d->form->abortCreatingConnection();
            }
#endif
            else if (d->form->state() == Form::WidgetInserting) {
                d->form->abortWidgetInserting();
            }
            return true;
        }
        else if ((kev->key() == Qt::Key_Control) && (d->state == Private::MovingWidget)) {
            if (!d->moving)
                return true;
            // we simulate a mouse move event to update screen
            QMouseEvent *mev = new QMouseEvent(QEvent::MouseMove,
                                               d->moving->mapFromGlobal(QCursor::pos()),
                                               Qt::NoButton,
                                               Qt::LeftButton,
                                               Qt::ControlModifier);
            eventFilter(d->moving, mev);
            delete mev;
        }
        else if (kev->key() == Qt::Key_Delete) {
            d->form->deleteWidget();
            return true;
        }
        // directional buttons move the widget
        else if (kev->key() == Qt::Key_Left) { // move the widget of gridX to the left
            moveSelectedWidgetsBy(-form()->gridSize(), 0);
            return true;
        }
        else if (kev->key() == Qt::Key_Right) { // move the widget of gridX to the right
            moveSelectedWidgetsBy(form()->gridSize(), 0);
            return true;
        }
        else if (kev->key() == Qt::Key_Up) { // move the widget of gridY to the top
            moveSelectedWidgetsBy(0, - form()->gridSize());
            return true;
        }
        else if (kev->key() == Qt::Key_Down) { // move the widget of gridX to the bottom
            moveSelectedWidgetsBy(0, form()->gridSize());
            return true;
        }
        else if ((kev->key() == Qt::Key_Tab) || (kev->key() == Qt::Key_Backtab)) {
            ObjectTreeItem *item = form()->objectTree()->lookup(
                form()->selectedWidgets()->first()->objectName()
            );
            if (!item || !item->parent())
                return true;
            ObjectTreeList *list = item->parent()->children();
            if (list->count() == 1)
                return true;
            int index = list->indexOf(item);

            if (kev->key() == Qt::Key_Backtab) {
                if (index == 0) // go back to the last item
                    index = list->count() - 1;
                else
                    index = index - 1;
            }
            else  {
                if (index == int(list->count() - 1)) // go back to the first item
                    index = 0;
                else
                    index = index + 1;
            }

            ObjectTreeItem *nextItem = list->at(index);
            if (nextItem && nextItem->widget()) {
                form()->selectWidget(nextItem->widget());
            }
        }
        return true;
    }

    case QEvent::KeyRelease: {
        QKeyEvent *kev = static_cast<QKeyEvent*>(e);
        if ((kev->key() == Qt::Key_Control) && (d->state == Private::CopyingWidget)) {
            // cancel copying
        }
        return true;
    }

    case QEvent::MouseButtonDblClick: { // editing
        //qDebug() << "Mouse dbl click for widget" << s->objectName();
        QWidget *w = static_cast<QWidget*>(s);
        if (!w)
            return false;

        if (d->form->library()->startInlineEditing(w->metaObject()->className(), w, this)) {
            d->state = Private::InlineEditing;
        }
        return true;
    }

    case QEvent::ContextMenu: {
        QContextMenuEvent* cme = static_cast<QContextMenuEvent*>(e);
        d->moving = 0; // clear this otherwise mouse dragging outside
                       // of the popup menu would drag the selected widget(s) randomly
        // target widget is the same as selected widget if this is context key event
        QWidget *widgetTarget = cme->reason() == QContextMenuEvent::Mouse
                ? static_cast<QWidget*>(s) : d->form->selectedWidget();
        if (widgetTarget) {
            // target widget is the same as selected widget if this is context key event
            QPoint pos;
            if (cme->reason() == QContextMenuEvent::Mouse) {
                pos = cme->pos();
            }
            else {
                if (widgetTarget == topLevelWidget()) {
                    pos = QPoint(20, 20);
                }
                else {
                    pos = QPoint(widgetTarget->width() / 2, widgetTarget->height() / 2);
                }
            }
            d->form->createContextMenu(widgetTarget, this, pos, Form::FormContextMenuTarget);
            return true;
        }
        break;
    }
    case QEvent::Enter:
    case QEvent::Leave:
    case QEvent::FocusIn:
    case QEvent::FocusOut:
        return true; // eat them

    default:
        break;
    }
    return false; // let the widget do the rest ...
}

bool
Container::handleMouseReleaseEvent(QObject *s, QMouseEvent *mev)
{
    if (d->form->state() == Form::WidgetInserting) {
        if (mev->button() == Qt::LeftButton) { // insert the widget at cursor pos
            Command *com = new InsertWidgetCommand(*this);
            d->form->addCommand(com);
            d->stopSelectionRectangleOrInserting();
        }
        else { // right button, etc.
            d->form->abortWidgetInserting();
        }
        return true;
    }
    else if (   s == widget()
             && !d->toplevel
             && (mev->button() != Qt::RightButton)
             && d->selectionOrInsertingRectangle().isValid() )
    {
        // we are still drawing a rect to select widgets
        selectionWidgetsForRectangle(mev->pos());
        return true;
    }

    if (mev->button() == Qt::RightButton) {
        // Right-click -> context menu
    }
    else if (mev->button() == Qt::LeftButton && mev->modifiers() == Qt::ControlModifier) {
        // copying a widget by Ctrl+dragging
        if (s == widget()) // should have no effect on form
            return true;

        // prevent accidental copying of widget (when moving mouse a little while selecting)
        if (    ((mev->pos().x() - d->grab.x()) < form()->gridSize() && (d->grab.x() - mev->pos().x()) < form()->gridSize())
             && ((mev->pos().y() - d->grab.y()) < form()->gridSize() && (d->grab.y() - mev->pos().y()) < form()->gridSize()) )
        {
            //qDebug() << "The widget has not been moved. No copying.";
            return true;
        }
        QPoint copyToPoint;
        if (d->form->selectedWidgets()->count() > 1) {
            copyToPoint = mev->pos();
        }
        else {
            copyToPoint = static_cast<QWidget*>(s)->mapTo(widget(), mev->pos() - d->grab);
        }

        Command *com = new DuplicateWidgetCommand(*d->form->activeContainer(), *d->form->selectedWidgets(), copyToPoint);
        d->form->addCommand(com);
    }
    else if (mev->button() == Qt::LeftButton && !(mev->buttons() & Qt::LeftButton) && d->state == Private::MovingWidget) {
        // one widget has been moved, so we need to update the layout
        reloadLayout();
    }

    // cancel copying as user released Ctrl before releasing mouse button
    d->stopSelectionRectangleOrInserting();
    d->state = Private::DoingNothing;
    d->moving.clear();
    return true; // eat
}

void Container::selectWidget(QWidget *w, Form::WidgetSelectionFlags flags)
{
    if (!w) {
        d->form->selectWidget(widget());
        return;
    }

    d->form->selectWidget(w, flags);
}

void
Container::deselectWidget(QWidget *w)
{
    if (!w)
        return;

    d->form->deselectWidget(w);
}

Container* Container::toplevel()
{
    if (d->toplevel)
        return d->toplevel;

    return this;
}

QWidget* Container::topLevelWidget() const
{
    if (d->toplevel)
        return d->toplevel->widget();

    return widget();
}

void
Container::deleteWidget(QWidget *w)
{
    if (!w)
        return;
    ObjectTreeItem *itemToRemove = d->form->objectTree()->lookup(w->objectName());
    if (!itemToRemove)
        return;
    QWidget *widgetoRemove = itemToRemove->widget();
    const ObjectTreeItem *parentItemToSelect = itemToRemove->parent()
        ? d->form->library()->selectableItem(itemToRemove->parent()) : 0;
    QWidget *parentWidgetToSelect = parentItemToSelect ? parentItemToSelect->widget() : 0;
    d->form->objectTree()->removeItem(itemToRemove);
    d->form->selectWidget(parentWidgetToSelect);
    delete widgetoRemove;
}

void
Container::widgetDeleted()
{
    d->widgetDeleted();
    deleteLater();
}

/// Layout functions

QLayout* Container::layout() const
{
    return d->layout;
}

Form::LayoutType Container::layoutType() const
{
    return d->layType;
}

int Container::layoutMargin() const
{
    return d->margin;
}

int Container::layoutSpacing() const
{
    return d->spacing;
}

void Container::setLayoutType(Form::LayoutType type)
{
    if (d->layType == type)
        return;

    delete d->layout;
    d->layout = 0;
    d->layType = type;

    switch (type) {
    case Form::HBox: {
        d->layout = static_cast<QLayout*>( new QHBoxLayout(widget()) );
        d->layout->setContentsMargins(d->margin, d->margin, d->margin, d->margin);
        d->layout->setSpacing(d->spacing);
        createBoxLayout(new HorizontalWidgetList(d->form->toplevelContainer()->widget()));
        break;
    }
    case Form::VBox: {
        d->layout = static_cast<QLayout*>( new QVBoxLayout(widget()) );
        d->layout->setContentsMargins(d->margin, d->margin, d->margin, d->margin);
        d->layout->setSpacing(d->spacing);
        createBoxLayout(new VerticalWidgetList(d->form->toplevelContainer()->widget()));
        break;
    }
    case Form::Grid: {
        createGridLayout();
        break;
    }
    default: {
        d->layType = Form::NoLayout;
        return;
    }
    }
    widget()->setGeometry(widget()->geometry()); // just update layout
    d->layout->activate();
}

void Container::setLayout(QLayout* layout)
{
    d->layout = layout;
}

void Container::setLayoutSpacing(int spacing)
{
    d->spacing = spacing;
}

void Container::setLayoutMargin(int margin)
{
    d->margin = margin;
}

void
Container::reloadLayout()
{
    Form::LayoutType type = d->layType;
    setLayoutType(Form::NoLayout);
    setLayoutType(type);
}

void
Container::createBoxLayout(CustomSortableWidgetList* list)
{
    QBoxLayout *layout = static_cast<QBoxLayout*>(d->layout);

    foreach (ObjectTreeItem *titem, *d->tree->children()) {
        list->append(titem->widget());
    }
    list->sort();

    foreach (QWidget *w, *list) {
        layout->addWidget(w);
    }
    delete list;
}

void
Container::createGridLayout(bool testOnly)
{
    //Those lists sort widgets by y and x
    VerticalWidgetList *vlist = new VerticalWidgetList(d->form->toplevelContainer()->widget());
    HorizontalWidgetList *hlist = new HorizontalWidgetList(d->form->toplevelContainer()->widget());
    // The vector are used to store the x (or y) beginning of each column (or row)
    QVector<int> cols;
    QVector<int> rows;
    int end = -1000;
    bool same = false;

    foreach (ObjectTreeItem *titem, *d->tree->children()) {
        vlist->append(titem->widget());
    }
    vlist->sort();

    foreach (ObjectTreeItem *titem, *d->tree->children()) {
        hlist->append(titem->widget());
    }
    hlist->sort();

    // First we need to make sure that two widgets won't be in the same row,
    // ie that no widget overlap another one
    if (!testOnly) {
        for (QWidgetList::ConstIterator it(vlist->constBegin()); it!=vlist->constEnd(); ++it) {
            QWidget *w = *it;
            for (QWidgetList::ConstIterator it2(it); it2!=vlist->constEnd(); ++it2) {
                QWidget *nextw = *it2;
                if ((w->y() >= nextw->y()) || (nextw->y() >= w->geometry().bottom()))
                    break;

                if (!w->geometry().intersects(nextw->geometry()))
                    break;
                // If the geometries of the two widgets intersect each other,
                // we move one of the widget to the rght or bottom of the other
                if ((nextw->y() - w->y()) > qAbs(nextw->x() - w->x()))
                    nextw->move(nextw->x(), w->geometry().bottom() + 1);
                else if (nextw->x() >= w->x())
                    nextw->move(w->geometry().right() + 1, nextw->y());
                else
                    w->move(nextw->geometry().right() + 1, nextw->y());
            }
        }
    }

    // Then we count the number of rows in the layout, and set their beginnings
    for (QWidgetList::ConstIterator it(vlist->constBegin()); it!=vlist->constEnd(); ++it) {
        QWidget *w = *it;
        if (!same) { // this widget will make a new row
            end = w->geometry().bottom();
            rows.append(w->y());
        }
        QWidgetList::ConstIterator it2(it);

        // If same == true, it means we are in the same row as prev widget
        // (so no need to create a new column)
        ++it2;
        if (it2==vlist->constEnd())
            break;

        QWidget *nextw = *it2;
        if (nextw->y() >= end)
            same = false;
        else {
            same = !(same && (nextw->y() >= w->geometry().bottom()));
            if (!same)
                end = w->geometry().bottom();
        }
    }
    //qDebug() << "the new grid will have n rows: n == " << rows.size();

    end = -10000;
    same = false;
    // We do the same thing for the columns
    for (QWidgetList::ConstIterator it(hlist->constBegin()); it!=hlist->constEnd(); ++it) {
        QWidget *w = *it;
        if (!same) {
            end = w->geometry().right();
            cols.append(w->x());
        }

        QWidgetList::ConstIterator it2(it);
        ++it2;
        if (it2==hlist->constEnd())
            break;

        QWidget *nextw = *it2;
        if (nextw->x() >= end)
            same = false;
        else {
            same = !(same && (nextw->x() >= w->geometry().right()));
            if (!same)
                end = w->geometry().right();
        }
    }
    //qDebug() << "the new grid will have n columns: n == " << cols.size();

    // We create the layout ..
    QGridLayout *layout = 0;
    if (!testOnly) {
        layout = new QGridLayout(widget());
        layout->setObjectName("grid");
//! @todo allow for individual margins and spacing
        layout->setContentsMargins(d->margin, d->margin, d->margin, d->margin);
        layout->setSpacing(d->spacing);
        d->layout = static_cast<QLayout*>(layout);
    }

    // .. and we fill it with widgets
    for (QWidgetList::ConstIterator it(vlist->constBegin()); it!=vlist->constEnd(); ++it) {
        QWidget *w = *it;
        QRect r( w->geometry() );
        int wcol = 0, wrow = 0, endrow = 0, endcol = 0;
        int i = 0;

        // We look for widget row(s) ..
        while (r.y() >= rows[i]) {
            if (rows.size() <= (i + 1)) { // we are the last row
                wrow = i;
                break;
            }
            if (r.y() < rows[i+1]) {
                wrow = i; // the widget will be in this row
                int j = i + 1;
                // Then we check if the widget needs to span multiple rows
                while (rows.size() >= (j + 1) && r.bottom() > rows[j]) {
                    endrow = j;
                    j++;
                }

                break;
            }
            i++;
        }
        //qDebug() << "the widget " << w->objectName() << " will be in the row " << wrow <<
        //" and will go to the row " << endrow;

        // .. and column(s)
        i = 0;
        while (r.x() >= cols[i]) {
            if (cols.size() <= (i + 1)) { // last column
                wcol = i;
                break;
            }
            if (r.x() < cols[i+1]) {
                wcol = i;
                int j = i + 1;
                // Then we check if the widget needs to span multiple columns
                while (cols.size() >= (j + 1) && r.right() > cols[j]) {
                    endcol = j;
                    j++;
                }

                break;
            }
            i++;
        }
        //qDebug() << "the widget " << w->objectName() << " will be in the col " << wcol <<
        // " and will go to the col " << endcol;

        ObjectTreeItem *item = d->form->objectTree()->lookup(w->objectName());
        if (item) {
            if (!endrow && !endcol) {
                if (!testOnly)
                    layout->addWidget(w, wrow, wcol);
                item->setGridPos(wrow, wcol, 0, 0);
            }
            else {
                if (!endcol)
                    endcol = wcol;
                if (!endrow)
                    endrow = wrow;
                if (!testOnly)
                    layout->addWidget(w, wrow, wcol, endrow - wrow + 1, endcol - wcol + 1);
                item->setGridPos(wrow, wcol, endrow - wrow + 1, endcol - wcol + 1);
            }
        }
    } //for
}

QString
Container::layoutTypeToString(Form::LayoutType type)
{
    switch (type) {
    case Form::HBox: return "HBox";
    case Form::VBox: return "VBox";
    case Form::Grid: return "Grid";
    case Form::HFlow: return "HFlow";
    case Form::VFlow: return "VFlow";
    default:   return "NoLayout";
    }
}

Form::LayoutType
Container::stringToLayoutType(const QString &name)
{
    if (name == "HBox") return Form::HBox;
    if (name == "VBox") return Form::VBox;
    if (name == "Grid") return Form::Grid;
    if (name == "HFlow")  return Form::HFlow;
    if (name == "VFlow")  return Form::VFlow;
    return Form::NoLayout;
}

#ifdef KFD_SIGSLOTS
void Container::drawConnection(QMouseEvent *mev)
{
    if (mev->button() != Qt::LeftButton) {
        FormManager::self()->resetCreatedConnection();
        return;
    }
    // First click, we select the sender and display menu to choose signal
    if (FormManager::self()->createdConnection()->sender().isNull()) {
        FormManager::self()->createdConnection()->setSender(d->moving->objectName());
        if (d->form->formWidget()) {
            d->form->formWidget()->initBuffer();
            d->form->formWidget()->highlightWidgets(d->moving, 0);
        }
        FormManager::self()->createSignalMenu(d->moving);
        return;
    }
    // the user clicked outside the menu, we cancel the connection
    if (FormManager::self()->createdConnection()->signal().isNull()) {
        FormManager::self()->stopCreatingConnection();
        return;
    }
    // second click to choose the receiver
    if (FormManager::self()->createdConnection()->receiver().isNull()) {
        FormManager::self()->createdConnection()->setReceiver(d->moving->objectName());
        FormManager::self()->createSlotMenu(d->moving);
        widget()->repaint();
        return;
    }
    // the user clicked outside the menu, we cancel the connection
    if (FormManager::self()->createdConnection()->slot().isNull()) {
        FormManager::self()->stopCreatingConnection();
        return;
    }
}
#endif

void Container::selectionWidgetsForRectangle(const QPoint& secondPoint)
{
    //finish drawing unclipped selection rectangle: clear the surface
    d->updateSelectionOrInsertingRectangle(secondPoint);

    selectWidget(0);
    QWidget *widgetToSelect = 0;
    // We check which widgets are in the rect and select them
    foreach (ObjectTreeItem *titem, *d->tree->children()) {
        QWidget *w = titem->widget();
        if (!w)
            continue;
        if (w->geometry().intersects( d->selectionOrInsertingRectangle() ) && w != widget()) {
            if (widgetToSelect) {
                selectWidget(widgetToSelect,
                    Form::AddToPreviousSelection | Form::Raise | Form::MoreWillBeSelected);
            }
            widgetToSelect = w; //select later
        }
    }
    if (widgetToSelect) {
        //the last one left
        selectWidget(widgetToSelect,
            Form::AddToPreviousSelection | Form::Raise | Form::LastSelection);
    }

    d->state = Private::DoingNothing;
    d->stopSelectionRectangleOrInserting();
}

// Other functions used by eventFilter
void Container::moveSelectedWidgetsBy(int realdx, int realdy, QMouseEvent *mev)
{
    if (d->form->selectedWidget() == d->form->widget())
        return; //do not move top-level widget

    const int gridX = d->form->gridSize();
    const int gridY = d->form->gridSize();
    int dx = realdx, dy = realdy;

    foreach (QWidget *w, *d->form->selectedWidgets()) {
        if (!w->parent() || w->parent()->inherits("QTabWidget") || w->parent()->inherits("QStackedWidget"))
            continue;

        if (w->parentWidget() && KexiUtils::objectIsA(w->parentWidget(), "QStackedWidget")) {
            w = w->parentWidget(); // widget is a stacked widget's page
        }
        if (w->parentWidget() && w->parentWidget()->inherits("QTabWidget")) {
            w = w->parentWidget(); // widget is a tab widget page
        }

        int tmpx = w->x() + realdx;
        int tmpy = w->y() + realdy;
        if (tmpx < 0)
            dx = qMax(0 - w->x(), dx); // because dx is <0
        else if (tmpx > w->parentWidget()->width() - gridX)
            dx = qMin(w->parentWidget()->width() - gridX - w->x(), dx);

        if (tmpy < 0)
            dy = qMax(0 - w->y(), dy); // because dy is <0
        else if (tmpy > w->parentWidget()->height() - gridY)
            dy = qMin(w->parentWidget()->height() - gridY - w->y(), dy);
    }

    foreach (QWidget *w, *d->form->selectedWidgets()) {
        // Don't move tab widget pages (or widget stack pages)
        if (!w->parent() || w->parent()->inherits("QTabWidget") || w->parent()->inherits("QStackedWidget"))
            continue;

        if (w->parentWidget() && KexiUtils::objectIsA(w->parentWidget(), "QStackedWidget")) {
            w = w->parentWidget(); // widget is WidgetStack page
            if (w->parentWidget() && w->parentWidget()->inherits("QTabWidget")) // widget is a tab widget page
                w = w->parentWidget();
        }

        int tmpx, tmpy;
        if (   !d->form->isSnapToGridEnabled()
            || (mev && mev->buttons() == Qt::LeftButton && mev->modifiers() == (Qt::ControlModifier | Qt::AltModifier))
           )
        {
            tmpx = w->x() + dx;
            tmpy = w->y() + dy;
        }
        else {
            tmpx = alignValueToGrid(w->x() + dx, gridX);
            tmpy = alignValueToGrid(w->y() + dy, gridY);
        }

        if (tmpx != w->x() || tmpy != w->y()) {
            QRect g(w->geometry());
            g.moveTo(tmpx, tmpy);
            if (d->form->selectedWidget()) {
                // single widget
                d->form->addPropertyCommand(w->objectName().toLatin1(), w->geometry(),
                                            g, "geometry", Form::ExecuteCommand,
                                            d->idOfPropertyCommand);
                w->move(tmpx, tmpy);
            }
            else {
                // multiple widgets: group them
                w->move(tmpx, tmpy);
            }
        }
    }
}

void Container::startChangingGeometryPropertyForSelectedWidget()
{
    ++d->idOfPropertyCommand;
}

void Container::setGeometryPropertyForSelectedWidget(const QRect &newGeometry)
{
    QWidget *w = d->form->selectedWidget();
    if (!w) {
        return;
    }
    d->form->addPropertyCommand(w->objectName().toLatin1(), w->geometry(),
                                newGeometry, "geometry", Form::ExecuteCommand,
                                d->idOfPropertyCommand);
}

void Container::stopInlineEditing()
{
    d->state = Private::DoingNothing;
}

QRect Container::selectionOrInsertingRectangle() const
{
    return d->selectionOrInsertingRectangle();
}

QPoint Container::selectionOrInsertingBegin() const
{
    return d->selectionOrInsertingBegin();
}

ObjectTreeItem* Container::objectTree() const
{
    return d->tree;
}

void Container::setObjectTree(ObjectTreeItem *t)
{
    d->tree = t;
}

////////////

class Q_DECL_HIDDEN DesignTimeDynamicChildWidgetHandler::Private
{
public:
    Private() : item(0) {}

    ObjectTreeItem* item;
};

DesignTimeDynamicChildWidgetHandler::DesignTimeDynamicChildWidgetHandler()
        : d(new Private)
{
}

DesignTimeDynamicChildWidgetHandler::~DesignTimeDynamicChildWidgetHandler()
{
    delete d;
}

void
DesignTimeDynamicChildWidgetHandler::childWidgetAdded(QWidget* w)
{
    if (d->item) {
        KexiUtils::installRecursiveEventFilter(w, d->item->eventEater());
    }
}

void DesignTimeDynamicChildWidgetHandler::assignItem(ObjectTreeItem* item)
{
    d->item = item;
}

