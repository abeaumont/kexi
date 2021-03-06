/* This file is part of the KDE project
   Copyright (C) 2004,2006 Jarosław Staniek <staniek@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#ifndef KEXIUTILS_P_H
#define KEXIUTILS_P_H

#include <QPointer>
#include <QTimer>
#include <QTreeWidget>

/*! @internal */
class DelayedCursorHandler : public QObject
{
    Q_OBJECT
public:
    DelayedCursorHandler(QWidget *widget = nullptr);
    void start(bool noDelay);
    void stop();
    bool startedOrActive; //!< true if ounting started or the cursor is active
protected Q_SLOTS:
    void show();
protected:
    QPointer<QWidget> m_widget;
    QTimer m_timer;
    bool m_handleWidget; //!< Needed because m_widget can disappear
};

/*! @internal KDb Debug Tree */
class KexiDBDebugTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit KexiDBDebugTreeWidget(QWidget *parent = 0);
public Q_SLOTS:
    void copy();
};

/*! @internal Debug window */
class DebugWindow : public QWidget
{
    Q_OBJECT
public:
    explicit DebugWindow(QWidget * parent = 0);
};

#endif
