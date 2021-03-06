/* This file is part of the KDE project
   Copyright (C) 2003-2015 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIPROJECTSET_H
#define KEXIPROJECTSET_H

#include "kexiprojectdata.h"

#include <KDbConnectionData>
#include <KDbResult>

class KexiProjectSetPrivate;
class KDbMessageHandler;

/*! @short Stores information about multiple kexi project-data items */
class KEXICORE_EXPORT KexiProjectSet : public KDbResultable
{
public:

    /*! Creates empty project set. Use addProjectData to add a project data.
      \a handler can be provided to receive error messages. */
    explicit KexiProjectSet(KDbMessageHandler* handler = 0);

    virtual ~KexiProjectSet();

    /*! Fills the set with all projects found using \a conndata (required).
    Previous set of projects is removed.
    A KDbConnection object is created in this method and immediately deleted afterwards.
    @return false on error during project list retrieving. */
    bool setConnectionData(KDbConnectionData* conndata);

    /*! Adds \a data as project data.
    \a data will be owned by this object. */
    void addProjectData(KexiProjectData *data);

    /*! Takes \a data project data from the set without deleting it.
        @return 0 if there is no such data in this set. */
    KexiProjectData* takeProjectData(KexiProjectData *data);

    //! \return list object
    KexiProjectData::List list() const;

    //! Case insensitive lookup.
    //! \return project data for databased \a dbName or NULL if not found
    KexiProjectData* findProject(const QString &dbName) const;

private:
    Q_DISABLE_COPY(KexiProjectSet)
    KexiProjectSetPrivate * const d;
};

#endif // KEXIPROJECTSET_H

