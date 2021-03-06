/* This file is part of the KDE project
   Copyright (C) 2004 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2004 Cedric Pasteur <cedric.pasteur@free.fr>
   Copyright (C) 2005-2017 Jarosław Staniek <staniek@kde.org>

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

#include "kexiformpart.h"
#include <KexiIcon.h>
#include <KexiView.h>
#include <KexiWindow.h>
#include <KexiMainWindowIface.h>
#include <kexiproject.h>
#include <kexipartitem.h>
#include <widget/KexiDataSourceComboBox.h>
#include <kexiutils/utils.h>
#include <kexi_global.h>
#include <formeditor/form.h>
#include <formeditor/formIO.h>
#include <formeditor/WidgetTreeWidget.h>
#include "widgets/kexidbform.h"
#include "kexiformscrollview.h"
#include "kexiformmanager.h"
#include "kexidatasourcepage.h"

#include <KDbConnection>

#include <KProperty>

#include <KActionMenu>

#include <QVBoxLayout>
#include <QDomDocument>
#include <QTabWidget>
#include <QAction>
#include <QDebug>

//! @todo #define KEXI_SHOW_SPLITTER_WIDGET

//! @internal
class Q_DECL_HIDDEN KexiFormPart::Private
{
public:
    Private() {
    }
    ~Private() {
        delete static_cast<QWidget*>(widgetTreeWidget);
        delete static_cast<KexiDataSourcePage*>(dataSourcePage);
    }
    QPointer<KexiDataSourcePage> dataSourcePage;
    QPointer<KFormDesigner::WidgetTreeWidget> widgetTree;
    QPointer<QWidget> widgetTreeWidget;
};

KexiFormPart::KexiFormPart(QObject *parent, const QVariantList &l)
  : KexiPart::Part(parent,
        xi18nc("Translate this word using only lowercase alphanumeric characters (a..z, 0..9). "
              "Use '_' character instead of spaces. First character should be a..z character. "
              "If you cannot use latin characters in your language, use english word.",
              "form"),
        xi18nc("tooltip", "Create new form"),
        xi18nc("what's this", "Creates new form."),
        l)
  , d(new Private)
{
    setInternalPropertyValue("newObjectsAreDirty", true);

    // Only create form manager if it's not yet created.
    // KexiReportPart could have created it already.

    KexiFormManager::self()->init(this, d->widgetTree); // this should create KexiFormManager singleton
}

KexiFormPart::~KexiFormPart()
{
    delete d;
}

void KexiFormPart::initPartActions()
{
}

void KexiFormPart::initInstanceActions()
{
    //connect actions provided by widget factories
    createSharedAction(Kexi::DesignViewMode, xi18n("Clear Widget Contents"),
                       koIconName("edit-clear"), QKeySequence(), "formpart_clear_contents");
    createSharedAction(Kexi::DesignViewMode, xi18n("Edit Tab Order..."),
                       KexiIconName("widgets-tab-order"), QKeySequence(), "formpart_taborder");
//! @todo createSharedAction(Kexi::DesignViewMode, xi18n("Edit Pixmap Collection"), koIconName("icons"), 0, "formpart_pixmap_collection");
//! @todo createSharedAction(Kexi::DesignViewMode, xi18n("Edit Form Connections"), koIconName("connections"), 0, "formpart_connections");

    createSharedAction(Kexi::DesignViewMode, xi18n("Bring Widget to Front"), koIconName("object-order-front"),
                       QKeySequence(), "formpart_format_raise");
    createSharedAction(Kexi::DesignViewMode, xi18n("Send Widget to Back"), koIconName("object-order-back"),
                       QKeySequence(), "formpart_format_lower");

#ifdef KEXI_SHOW_UNFINISHED
    createSharedAction(Kexi::DesignViewMode, futureI18n("Other Widgets"), QString(),
                       QKeySequence(), "other_widgets_menu", "KActionMenu");
#endif

    QAction *action = createSharedAction(Kexi::DesignViewMode, xi18n("Align Widgets Position"),
                                koIconName("align-horizontal-left"), QKeySequence(), "formpart_align_menu", "KActionMenu");
    KActionMenu *menu = static_cast<KActionMenu*>(action);
    menu->addAction(createSharedAction(Kexi::DesignViewMode, xi18n("To Left"), koIconName("align-horizontal-left"),
                                       QKeySequence(), "formpart_align_to_left"));
    menu->addAction(createSharedAction(Kexi::DesignViewMode, xi18n("To Right"), koIconName("align-horizontal-right"),
                                       QKeySequence(), "formpart_align_to_right"));
    menu->addAction(createSharedAction(Kexi::DesignViewMode, xi18n("To Top"), koIconName("align-vertical-top"),
                                       QKeySequence(), "formpart_align_to_top"));
    menu->addAction(createSharedAction(Kexi::DesignViewMode, xi18n("To Bottom"), koIconName("align-vertical-bottom"),
                                       QKeySequence(), "formpart_align_to_bottom"));
    menu->addAction(createSharedAction(Kexi::DesignViewMode, xi18n("To Grid"), koIconName("align-grid"),
                                       QKeySequence(), "formpart_align_to_grid"));

    action = createSharedAction(Kexi::DesignViewMode, xi18n("Adjust Widgets Size"), koIconName("fit-grid"),
                                QKeySequence(), "formpart_adjust_size_menu", "KActionMenu");
    menu = static_cast<KActionMenu*>(action);
    menu->addAction(createSharedAction(Kexi::DesignViewMode, xi18n("To Fit"), koIconName("fit-contents"),
                                       QKeySequence(), "formpart_adjust_to_fit"));
    menu->addAction(createSharedAction(Kexi::DesignViewMode, xi18n("To Grid"), koIconName("fit-grid"),
                                       QKeySequence(), "formpart_adjust_size_grid"));
    menu->addAction(createSharedAction(Kexi::DesignViewMode, xi18n("To Shortest"), koIconName("fit-shortest"),
                                       QKeySequence(), "formpart_adjust_height_small"));
    menu->addAction(createSharedAction(Kexi::DesignViewMode, xi18n("To Tallest"), koIconName("fit-tallest"),
                                       QKeySequence(), "formpart_adjust_height_big"));
    menu->addAction(createSharedAction(Kexi::DesignViewMode, xi18n("To Narrowest"), koIconName("fit-narrowest"),
                                       QKeySequence(), "formpart_adjust_width_small"));
    menu->addAction(createSharedAction(Kexi::DesignViewMode, xi18n("To Widest"), koIconName("fit-widest"),
                                       QKeySequence(), "formpart_adjust_width_big"));
}

KexiWindowData* KexiFormPart::createWindowData(KexiWindow* window)
{
    KexiMainWindowIface *win = KexiMainWindowIface::global();
    return new KexiFormPartTempData(window, win->project()->dbConnection());
}

KexiView* KexiFormPart::createView(QWidget *parent, KexiWindow* window,
                                   KexiPart::Item *item, Kexi::ViewMode viewMode, QMap<QString, QVariant>*)
{
    Q_ASSERT(item);
    Q_UNUSED(window);
    Q_UNUSED(viewMode);
    KexiMainWindowIface *win = KexiMainWindowIface::global();
    if (!win || !win->project() || !win->project()->dbConnection())
        return 0;

    KexiFormView *view = new KexiFormView(parent, win->project()->dbConnection());
    view->setObjectName(item->name().toLatin1());
    return view;
}

#ifndef KEXI_NO_FORM_DATASOURCE_WIZARD
void
KexiFormPart::generateForm(KDbFieldList *list, QDomDocument &domDoc)
{
    //this form generates a .ui from KDbFieldList list
    //basically that is a Label and a LineEdit for each field
    domDoc = QDomDocument("UI");
    QDomElement uiElement = domDoc.createElement("UI");
    domDoc.appendChild(uiElement);
    uiElement.setAttribute("version", "3.1");
    uiElement.setAttribute("stdsetdef", 1);

    QDomElement baseClass = domDoc.createElement("class");
    uiElement.appendChild(baseClass);
    QDomText baseClassV = domDoc.createTextNode("QWidget");
    baseClass.appendChild(baseClassV);
    QDomElement baseWidget = domDoc.createElement("widget");
    baseWidget.setAttribute("class", "QWidget");

    int y = 0;

    for (unsigned int i = 0; i < list->fieldCount(); i++) {
        QDomElement lclass = domDoc.createElement("widget");
        baseWidget.appendChild(lclass);
        lclass.setAttribute("class", "QLabel");
        QDomElement lNameProperty = domDoc.createElement("property");
        lNameProperty.setAttribute("name", "name");
        QDomElement lType = domDoc.createElement("cstring");
        QDomText lClassN = domDoc.createTextNode(QString("l%1").arg(list->field(i)->name()));
        lType.appendChild(lClassN);
        lNameProperty.appendChild(lType);
        lclass.appendChild(lNameProperty);

        QDomElement gNameProperty = domDoc.createElement("property");
        gNameProperty.setAttribute("name", "geometry");
        QDomElement lGType = domDoc.createElement("rect");

        QDomElement lx = domDoc.createElement("x");
        QDomText lxV = domDoc.createTextNode("10");
        lx.appendChild(lxV);
        QDomElement ly = domDoc.createElement("y");
        QDomText lyV = domDoc.createTextNode(QString::number(y + 10));
        ly.appendChild(lyV);
        QDomElement lWidth = domDoc.createElement("width");
        QDomText lWidthV = domDoc.createTextNode("100");
        lWidth.appendChild(lWidthV);
        QDomElement lHeight = domDoc.createElement("height");
        QDomText lHeightV = domDoc.createTextNode("20");
        lHeight.appendChild(lHeightV);

        lGType.appendChild(lx);
        lGType.appendChild(ly);
        lGType.appendChild(lWidth);
        lGType.appendChild(lHeight);

        gNameProperty.appendChild(lGType);
        lclass.appendChild(gNameProperty);

        QDomElement tNameProperty = domDoc.createElement("property");
        tNameProperty.setAttribute("name", "text");
        QDomElement lTType = domDoc.createElement("string");
        QDomText lTextV = domDoc.createTextNode(list->field(i)->name());
        lTType.appendChild(lTextV);
        tNameProperty.appendChild(lTType);
        lclass.appendChild(tNameProperty);


        ///line edit!


        QDomElement vclass = domDoc.createElement("widget");
        baseWidget.appendChild(vclass);
        vclass.setAttribute("class", "QLineEdit");
        QDomElement vNameProperty = domDoc.createElement("property");
        vNameProperty.setAttribute("name", "name");
        QDomElement vType = domDoc.createElement("cstring");
        QDomText vClassN = domDoc.createTextNode(list->field(i)->name());
        vType.appendChild(vClassN);
        vNameProperty.appendChild(vType);
        vclass.appendChild(vNameProperty);

        QDomElement vgNameProperty = domDoc.createElement("property");
        vgNameProperty.setAttribute("name", "geometry");
        QDomElement vGType = domDoc.createElement("rect");

        QDomElement vx = domDoc.createElement("x");
        QDomText vxV = domDoc.createTextNode("110");
        vx.appendChild(vxV);
        QDomElement vy = domDoc.createElement("y");
        QDomText vyV = domDoc.createTextNode(QString::number(y + 10));
        vy.appendChild(vyV);
        QDomElement vWidth = domDoc.createElement("width");
        QDomText vWidthV = domDoc.createTextNode("200");
        vWidth.appendChild(vWidthV);
        QDomElement vHeight = domDoc.createElement("height");
        QDomText vHeightV = domDoc.createTextNode("20");
        vHeight.appendChild(vHeightV);

        vGType.appendChild(vx);
        vGType.appendChild(vy);
        vGType.appendChild(vWidth);
        vGType.appendChild(vHeight);

        vgNameProperty.appendChild(vGType);
        vclass.appendChild(vgNameProperty);

        y += 20;
    }

    QDomElement lNameProperty = domDoc.createElement("property");
    lNameProperty.setAttribute("name", "name");
    QDomElement lType = domDoc.createElement("cstring");
    QDomText lClassN = domDoc.createTextNode("DBForm");
    lType.appendChild(lClassN);
    lNameProperty.appendChild(lType);
    baseWidget.appendChild(lNameProperty);

    QDomElement wNameProperty = domDoc.createElement("property");
    wNameProperty.setAttribute("name", "geometry");
    QDomElement wGType = domDoc.createElement("rect");

    QDomElement wx = domDoc.createElement("x");
    QDomText wxV = domDoc.createTextNode("0");
    wx.appendChild(wxV);
    QDomElement wy = domDoc.createElement("y");
    QDomText wyV = domDoc.createTextNode("0");
    wy.appendChild(wyV);
    QDomElement wWidth = domDoc.createElement("width");
    QDomText wWidthV = domDoc.createTextNode("340");
    wWidth.appendChild(wWidthV);
    QDomElement wHeight = domDoc.createElement("height");
    QDomText wHeightV = domDoc.createTextNode(QString::number(y + 30));
    wHeight.appendChild(wHeightV);

    wGType.appendChild(wx);
    wGType.appendChild(wy);
    wGType.appendChild(wWidth);
    wGType.appendChild(wHeight);

    wNameProperty.appendChild(wGType);
    baseWidget.appendChild(wNameProperty);

    uiElement.appendChild(baseWidget);
}
#endif

KLocalizedString KexiFormPart::i18nMessage(
    const QString& englishMessage, KexiWindow* window) const
{
    Q_UNUSED(window);
    if (englishMessage == "Design of object <resource>%1</resource> has been modified.")
        return kxi18nc(I18NC_NOOP("@info", "Design of form <resource>%1</resource> has been modified."));
    if (englishMessage == "Object <resource>%1</resource> already exists.")
        return kxi18nc(I18NC_NOOP("@info", "Form <resource>%1</resource> already exists."));

    return Part::i18nMessage(englishMessage, window);
}

KexiDataSourcePage* KexiFormPart::dataSourcePage() const
{
    return d->dataSourcePage;
}

KFormDesigner::WidgetTreeWidget* KexiFormPart::widgetTreePage() const
{
    return d->widgetTree;
}

void KexiFormPart::setupCustomPropertyPanelTabs(QTabWidget *tab)
{
    if (!d->dataSourcePage) {
        d->dataSourcePage = new KexiDataSourcePage(0);
        d->dataSourcePage->setObjectName("dataSourcePage");
        connect(d->dataSourcePage,
                SIGNAL(jumpToObjectRequested(QString,QString)),
                KexiMainWindowIface::global()->thisWidget(),
                SLOT(highlightObject(QString,QString)));
        connect(d->dataSourcePage,
                SIGNAL(formDataSourceChanged(QString,QString)),
                KexiFormManager::self(),
                SLOT(setFormDataSource(QString,QString)));
        connect(d->dataSourcePage,
                SIGNAL(dataSourceFieldOrExpressionChanged(QString,QString,KDbField::Type)),
                KexiFormManager::self(),
                SLOT(setDataSourceFieldOrExpression(QString,QString,KDbField::Type)));
#ifdef KEXI_AUTOFIELD_FORM_WIDGET_SUPPORT
        connect(d->dataSourcePage,
                SIGNAL(insertAutoFields(QString,QString,QStringList)),
                KexiFormManager::self(),
                SLOT(insertAutoFields(QString,QString,QStringList)));
#endif
    }

    KexiProject *prj = KexiMainWindowIface::global()->project();
    d->dataSourcePage->setProject(prj);

    tab->addTab(d->dataSourcePage, koIcon("server-database"), QString());
    tab->setTabToolTip(tab->indexOf(d->dataSourcePage), xi18n("Data Source"));

    if (!d->widgetTreeWidget) {
        d->widgetTreeWidget = new QWidget;
        QVBoxLayout *lyr = new QVBoxLayout(d->widgetTreeWidget);
        lyr->setContentsMargins(2, 2, 2, 2);
        d->widgetTree = new KFormDesigner::WidgetTreeWidget;
        d->widgetTree->setObjectName("KexiFormPart:WidgetTreeWidget");
        lyr->addWidget(d->widgetTree);
    }
    tab->addTab(d->widgetTreeWidget, KexiIcon("widgets"), QString());
    tab->setTabToolTip(tab->indexOf(d->widgetTreeWidget), xi18n("Widgets"));
}

//----------------

class Q_DECL_HIDDEN KexiFormPartTempData::Private
{
public:
    Private(KexiFormPartTempData *temp)
        : q(temp)
    {
    }

    void unregisterForChanges()
    {
        if (dataSource.isEmpty()) {
            return;
        }
        if (pluginId == "org.kexi-project.table") {
            KDbTableSchema *table = conn->tableSchema(dataSource);
            if (!table) {
                return;
            }
            KDbTableSchemaChangeListener::unregisterForChanges(conn, table);
        } else if (pluginId == "org.kexi-project.query") {
            KDbQuerySchema *query = conn->querySchema(dataSource);
            if (!query) {
                return;
            }
            KDbTableSchemaChangeListener::unregisterForChanges(conn, query);
        } else {
            return;
        }
    }

    void registerForChanges(const QString &newPluginId, const QString &newDataSource)
    {
        if (newPluginId == "org.kexi-project.table") {
            KDbTableSchema *table = conn->tableSchema(newDataSource);
            if (!table) {
                return;
            }
            KDbTableSchemaChangeListener::registerForChanges(conn, q, table);
        } else if (pluginId == "org.kexi-project.query") {
            KDbQuerySchema *query = conn->querySchema(newDataSource);
            if (!query) {
                return;
            }
            KDbTableSchemaChangeListener::registerForChanges(conn, q, query);
        } else {
            return;
        }
        pluginId = newPluginId;
        dataSource = newDataSource;
    }

    KDbConnection *conn;
    QString pluginId;
    QString dataSource;

private:
    KexiFormPartTempData * const q;
};

KexiFormPartTempData::KexiFormPartTempData(KexiWindow* parent, KDbConnection *conn)
        : KexiWindowData(parent), d(new Private(this))
{
    d->conn = conn;
    setName(KexiUtils::localizedStringToHtmlSubstring(
        kxi18nc("@info", "Form <resource>%1</resource>").subs(parent->partItem()->name())));
}

KexiFormPartTempData::~KexiFormPartTempData()
{
    KDbTableSchemaChangeListener::unregisterForChanges(d->conn, this);
    delete d;
}

void KexiFormPartTempData::setDataSource(const QString &pluginId, const QString &dataSource)
{
    if (d->pluginId != pluginId || d->dataSource != dataSource) {
        d->unregisterForChanges();
        d->registerForChanges(pluginId, dataSource);
    }
}

tristate KexiFormPartTempData::closeListener()
{
    KexiWindow* window = static_cast<KexiWindow*>(parent());
    qDebug() << window->partItem()->name();
    return KexiMainWindowIface::global()->closeWindow(window);
}
