/*
 * OpenRPT report writer and rendering engine
 * Copyright (C) 2001-2007 by OpenMFG, LLC
 * Copyright (C) 2007-2008 by Adam Pigg (adam@piggz.co.uk)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * Please contact info@openmfg.com with any questions on this license.
 */

#include "reportentities.h"

// qt
#include <qpainter.h>
#include <qstring.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qdom.h>
#include <qinputdialog.h>
#include <qslider.h>
#include <qdatastream.h>
#include <qcheckbox.h>
#include <qradiobutton.h>
#include <qsettings.h>
#include <kdebug.h>

#include <koproperty/Property.h>
#include <koproperty/Set.h>
#include <koproperty/EditorView.h>
#include <KoGlobal.h>
#include <krobjectdata.h>

#include "reportentitylabel.h"
#include "reportentityfield.h"
#include "reportentitytext.h"
#include "reportentityline.h"
#include "reportentitybarcode.h"
#include "reportentityimage.h"
#include "reportentitychart.h"

//
// ReportEntity
//
ReportEntity::ReportEntity(ReportDesigner* r)
{
    m_reportDesigner = r;
}

void ReportEntity::buildXML(QGraphicsItem * item, QDomDocument & doc, QDomElement & parent)
{
    ReportEntity *re = 0;
    re = dynamic_cast<ReportEntity*>(item);

    if (re) {
        re->buildXML(doc, parent);
    }

}

void ReportEntity::buildXMLRect(QDomDocument & doc, QDomElement & entity, KRPos *pos, KRSize *siz)
{
    QDomElement element = doc.createElement("report:rect");

    KoUnit unit = pos->unit();
    
    element.setAttribute("svg:x", QString::number(pos->toUnit().x()) + KoUnit::unitName(unit));
    element.setAttribute("svg:y", QString::number(pos->toUnit().y()) + KoUnit::unitName(unit));
    element.setAttribute("svg:width", QString::number(siz->toUnit().width()) + KoUnit::unitName(unit));
    element.setAttribute("svg:height", QString::number(siz->toUnit().height()) + KoUnit::unitName(unit));

    entity.appendChild(element);
}

void ReportEntity::buildXMLTextStyle(QDomDocument & doc, QDomElement & entity, KRTextStyleData ts)
{
    QDomElement element = doc.createElement("report:text-style");

    element.setAttribute("fo:background-color", ts.backgroundColor.name());
    element.setAttribute("fo:foreground-color", ts.foregroundColor.name());
    element.setAttribute("fo:background-opacity", QString::number(ts.backgroundOpacity) + "%");
    element.setAttribute("report:qtfont", ts.font.toString());

    entity.appendChild(element);
}

void ReportEntity::buildXMLLineStyle(QDomDocument & doc, QDomElement & entity, KRLineStyleData ls)
{
    QDomElement element = doc.createElement("report:line-style");

    element.setAttribute("report:line-color", ls.lineColor.name());
    element.setAttribute("report:line-weight", QString::number(ls.weight));

    QString l;
    switch (ls.style) {
    case Qt::NoPen:
        l = "nopen";
        break;
    case Qt::SolidLine:
        l = "solid";
        break;
    case Qt::DashLine:
        l = "dash";
        break;
    case Qt::DotLine:
        l = "dot";
        break;
    case Qt::DashDotLine:
        l = "dashdot";
        break;
    case Qt::DashDotDotLine:
        l = "dashdotdot";
        break;
    default:
        l = "solid";

    }
    element.setAttribute("report:line-style", l);

    entity.appendChild(element);
}


