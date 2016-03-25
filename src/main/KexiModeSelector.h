/* This file is part of the KDE project
   Copyright (C) 2016 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIMODESELECTOR_H
#define KEXIMODESELECTOR_H

#include <KexiMainWindowIface.h>
#include <KexiListView.h>

#include <QScopedPointer>

//! @internal Global mode selector
class KexiModeSelector : public KexiListView
{
    Q_OBJECT
public:
    explicit KexiModeSelector(QWidget *parent = 0);
    virtual ~KexiModeSelector();

    //! @return current mode
    Kexi::GlobalViewMode currentMode() const;

    //! Sets current mode
    void setCurrentMode(Kexi::GlobalViewMode mode);

Q_SIGNALS:
    void currentModeChanged();

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;

    class Private;
    const QScopedPointer<Private> d;
};

#endif // KEXIMODESELECTOR_H
