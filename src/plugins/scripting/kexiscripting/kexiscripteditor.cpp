/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2004-2005 Jarosław Staniek <staniek@kde.org>
   Copyright (C) 2005 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005 Sebastian Sauer <mail@dipe.org>

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

#include "kexiscripteditor.h"

/// \internal d-pointer class
class Q_DECL_HIDDEN KexiScriptEditor::Private
{
public:
    QString scriptaction;
    Private() {}
};

KexiScriptEditor::KexiScriptEditor(QWidget *parent)
        : KexiEditor(parent)
        , d(new Private())
{
}

KexiScriptEditor::~KexiScriptEditor()
{
    delete d;
}

bool KexiScriptEditor::isInitialized() const
{
    return !d->scriptaction.isEmpty();
}

void KexiScriptEditor::initialize(const QString &scriptaction)
{
    d->scriptaction = scriptaction;

    disconnect(this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));

    if (d->scriptaction.isEmpty()) {
        // If there is no code we just add some information.
///@todo remove after release
#if 0
        code = "# " + QStringList::split("\n", futureI18n(
                                             "This note will appear for a user in the script's source code "
                                             "as a comment. Keep every row not longer than 60 characters and use '\n.'",

                                             "This is Technology Preview (BETA) version of scripting\n"
                                             "support in Kexi. The scripting API may change in details\n"
                                             "in the next KEXI version.\n"
                                             "For more information and documentation see\n%1",
                                             "http://www.kexi-project.org/scripting/"), true).join("\n# ") + "\n";
#endif
    }
    KexiEditor::setText(d->scriptaction);
    // We assume Kross and the HighlightingInterface are using same
    // names for the support languages...
    setHighlightMode("javascript");

    clearUndoRedo();
    KexiEditor::setDirty(false);
    connect(this, SIGNAL(textChanged()), this, SLOT(slotTextChanged()));
}

void KexiScriptEditor::slotTextChanged()
{
    KexiEditor::setDirty(true);
    d->scriptaction = KexiEditor::text();
}

void KexiScriptEditor::setLineNo(long lineno)
{
    setCursorPosition(lineno, 0);
}


