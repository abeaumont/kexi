/* This file is part of the KDE project
   Copyright (C) 2011 by Radosław Wicik <radoslaw@wicik.pl>

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

#ifndef MAPBROWSERWIDGET_H
#define MAPBROWSERWIDGET_H

#include "widgetfactory.h"
#include "container.h"
#include "FormWidgetInterface.h"
#include <widget/dataviewcommon/kexiformdataiteminterface.h>
#include <qglobal.h>

#include <marble/MarbleWidget.h>

class MapBrowserWidget : public Marble::MarbleWidget,
                         public KFormDesigner::FormWidgetInterface,
                         public KexiFormDataItemInterface
{
    Q_OBJECT
    Q_PROPERTY(QString dataSource READ dataSource WRITE setDataSource)
    Q_PROPERTY(QString dataSourcePartClass READ dataSourcePluginId WRITE setDataSourcePluginId)
    Q_PROPERTY(bool readOnly READ isReadOnly WRITE setReadOnly)
public:
    explicit MapBrowserWidget(QWidget *parent = 0);
    virtual ~MapBrowserWidget();

    inline QString dataSource() const {
        return KexiFormDataItemInterface::dataSource();
    }
    inline QString dataSourcePluginId() const {
        return KexiFormDataItemInterface::dataSourcePluginId();
    }

    virtual QVariant value();
    virtual bool valueIsNull();
    virtual bool valueIsEmpty();
    virtual bool cursorAtStart();
    virtual bool cursorAtEnd();
    virtual void clear();

    virtual void setInvalidState(const QString&);

    virtual bool isReadOnly() const;

public Q_SLOTS:
    //! Sets the datasource to \a ds
    inline void setDataSource(const QString &ds) {
        KexiFormDataItemInterface::setDataSource(ds);
    }
    inline void setDataSourcePluginId(const QString &pluginId) {
        KexiFormDataItemInterface::setDataSourcePluginId(pluginId);
    }
    void slotMapChanged();
    virtual void setReadOnly(bool);

protected:
    QVariant serializeData(qreal lat, qreal lon, int zoomLevel);
    void deserializeData(const QVariant& serialized);
    virtual void setValueInternal(const QVariant& add, bool removeOld);
    virtual void resizeEvent(QResizeEvent *event);

private:
    //! Used in slotTextChanged()
    bool m_slotMapChanged_enabled; //!< @todo do we need this to be 1 bit? As in some other widgets
    bool m_internalReadOnly;
    int m_defaultZoom;
};

#endif // MAPBROWSERWIDGET_H
