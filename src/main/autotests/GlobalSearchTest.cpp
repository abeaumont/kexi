/* This file is part of the KDE project
   Copyright (C) 2012-2016 Jarosław Staniek <staniek@kde.org>

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

#include "GlobalSearchTest.h"

#include <core/kexi.h>
#include <core/kexipartitem.h>
#include <core/kexiaboutdata.h>
#include <main/KexiMainWindow.h>
#include <kexiutils/KexiTester.h>
#include <widget/navigator/KexiProjectNavigator.h>

#include <KActionCollection>

#include <QApplication>
#include <QtTest>
#include <QtTest/qtestkeyboard.h>
#include <QtTest/qtestmouse.h>
#include <QFile>
#include <QTreeView>
#include <QLineEdit>
#include <QDebug>

const int GUI_DELAY = 10;
const char *FILES_DATA_DIR = CURRENT_SOURCE_DIR "/data";

void GlobalSearchTest::initTestCase()
{
}

//! Copies 0-th arg and adds second empty
class NewArgs
{
public:
    NewArgs(char *argv[]) {
        count = 2;
        vals = new char*[count];
        vals[0] = qstrdup(argv[0]);
        vals[count - 1] = 0;
    }
    ~NewArgs() {
        for (int i = 0; i < count; i++) {
            delete [] vals[i];
        }
        delete [] vals;
    }

    int count;
    char **vals;
};

//! A helper that creates Kexi main window, and manages its lifetime.
//! It is needed because lifetime of QApplication, main window and command line arguments
//! should be synchronized. This class is designed for using on a stack of test blocks.
class KexiMainWindowCreator
{
public:
    //! A helper that creates Kexi main window, passing @a filename to it.
    //! @a testObject (required) is used only to obtain test object name for debugging.
    KexiMainWindowCreator(char *argv[], const QString& filename, QObject *testObject)
        : args(argv)
    {
        Q_ASSERT(testObject);
        args.vals[args.count - 1] = qstrdup(QFile::encodeName(filename).constData());
        result = KexiMainWindow::create(args.count, args.vals,
                                        testObject->metaObject()->className());
    }

    //! Executes action @a name. @return true if action was found.
    bool executeAction(const QString &name) {
        KActionCollection *actionCollection = KexiMainWindowIface::global()->actionCollection();
        QAction *a = actionCollection->action(name);
        if (a) {
            a->trigger();
        }
        return a;
    }

    ~KexiMainWindowCreator() {
        delete KexiMainWindowIface::global();
        delete qApp;
    }
    int result;

private:
    NewArgs args;
};

GlobalSearchTest::GlobalSearchTest(int &argc, char **argv, bool goToEventLoop)
 : m_argc(argc), m_argv(argv), m_goToEventLoop(goToEventLoop)
{
}

void GlobalSearchTest::testGlobalSearch()
{
    const QString filename(QFile::decodeName(FILES_DATA_DIR) + "/GlobalSearchTest.kexi");
    qDebug() << filename;
    KexiMainWindowCreator windowCreator(m_argv, filename, this);
    QVERIFY(qApp);
    QCOMPARE(windowCreator.result, 0);

    QLineEdit *lineEdit = kexiTester().widget<QLineEdit*>("globalSearch.lineEdit");
    QVERIFY(lineEdit);
    QTreeView *treeView = kexiTester().widget<QTreeView*>("globalSearch.treeView");
    QVERIFY(treeView);

    lineEdit->setFocus();
    // enter "cars", expect 4 completion items
    QTest::keyClicks(lineEdit, "cars");
    QVERIFY(treeView->isVisible());
    int globalSearchCompletionListRows = treeView->model()->rowCount();
    QCOMPARE(globalSearchCompletionListRows, 4);

    // add "x", expect no completion items and hidden list
    QTest::keyClicks(lineEdit, "x");
    QVERIFY(!treeView->isVisible());
    globalSearchCompletionListRows = treeView->model()->rowCount();
    QCOMPARE(globalSearchCompletionListRows, 0);

    // Escape should clear
    QTest::keyClick(lineEdit, Qt::Key_Escape,  Qt::NoModifier, GUI_DELAY);
    QVERIFY(lineEdit->text().isEmpty());

    QTest::keyClicks(lineEdit, "cars");
    QVERIFY(treeView->isVisible());
    treeView->setFocus();
    // no highlight initially
    KexiProjectNavigator *projectNavigator = kexiTester().widget<KexiProjectNavigator*>("KexiProjectNavigator");
    QVERIFY(projectNavigator);
    QVERIFY(!projectNavigator->partItemWithSearchHighlight());

    QTest::keyPress(treeView, Qt::Key_Down, Qt::NoModifier, GUI_DELAY);

    // selecting 1st row should highlight "cars" table
    KexiPart::Item* highlightedPartItem = projectNavigator->partItemWithSearchHighlight();
    QVERIFY(highlightedPartItem);
    QCOMPARE(highlightedPartItem->name(), QLatin1String("cars"));
    QCOMPARE(highlightedPartItem->pluginId(), QLatin1String("org.kexi-project.table"));

    QTest::keyPress(treeView, Qt::Key_Down, Qt::NoModifier, GUI_DELAY);
    QTest::keyPress(treeView, Qt::Key_Down, Qt::NoModifier, GUI_DELAY);

    // 3rd row should be "cars" form
    QModelIndexList selectedIndices = treeView->selectionModel()->selectedRows();
    QCOMPARE(selectedIndices.count(), 1);
    QCOMPARE(treeView->model()->data(selectedIndices.first(), Qt::DisplayRole).toString(), QLatin1String("cars"));

    // check if proper entry of Project Navigator is selected
    QTest::keyPress(treeView, Qt::Key_Enter, Qt::NoModifier, GUI_DELAY);

    KexiPart::Item* selectedPartItem = projectNavigator->selectedPartItem();
    QVERIFY(selectedPartItem);
    QCOMPARE(selectedPartItem->name(), QLatin1String("cars"));
    QCOMPARE(selectedPartItem->pluginId(), QLatin1String("org.kexi-project.form"));

    if (m_goToEventLoop) {
        const int result = qApp->exec();
        QCOMPARE(result, 0);
    }
}

void GlobalSearchTest::cleanupTestCase()
{
}

int main(int argc, char *argv[])
{
    // Pull off custom options
    bool goToEventLoop = false;
    int realCount = 0;
    char **realVals = new char*[argc];
    for (int i = 0; i < argc; ++i) {
        realVals[i] = 0;
    }
    for (int i = 0; i < argc; ++i) {
        if (0 == qstrcmp(argv[i], "-loop")) {
            goToEventLoop = true;
            continue;
        }
        else {
            if (0 == qstrcmp(argv[i], "-help") || 0 == qstrcmp(argv[i], "--help")) {
                printf(" Options coming from the Kexi test suite:\n -loop : Go to event loop after successful test\n\n");
            }
            realVals[realCount] = qstrdup(argv[i]);
            ++realCount;
        }
    }

    // Actual test
    GlobalSearchTest tc(realCount, realVals, goToEventLoop);
    int result = QTest::qExec(&tc, realCount, realVals);

    // Clean up
    for (int i = 0; i < argc; i++) {
        delete [] realVals[i];
    }
    delete [] realVals;
    return result;
}
