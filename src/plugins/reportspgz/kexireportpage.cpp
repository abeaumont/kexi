/*
 * Kexi Report Plugin
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
#include "kexireportpage.h"
#include <qwidget.h>
#include <kdebug.h>
#include <qcolor.h>
#include <qpixmap.h>
#include <KoPageFormat.h>
#include <KoUnit.h>
#include <KoGlobal.h>

#include <parsexmlutils.h>
#include <renderobjects.h>
#include <QPainter>
#include <krscreenrender.h>

//#include "backend/common/pagesizeinfo.h"

KexiReportPage::KexiReportPage(QWidget *parent, const char *name, ORODocument *r)
	: QWidget(parent, name, Qt::WNoAutoErase)
{
	kDebug() << "CREATED PAGE" << endl;
	rpt = r;
	page = 1;
	
	//setBackgroundMode(Qt::NoBackground);
	
	QString pageSize = r->pageOptions().getPageSize();
	int pageWidth = 0;
	int pageHeight = 0;
	if ( pageSize == "Custom" )
	{
		// if this is custom sized sheet of paper we will just use those values
		pageWidth = ( int ) ( r->pageOptions().getCustomWidth());
		pageHeight = ( int ) ( r->pageOptions().getCustomHeight());
	}
	else
	{
		// lookup the correct size information for the specified size paper
		pageWidth = r->pageOptions().widthPx();
		pageHeight = r->pageOptions().heightPx();
		/*
		KoUnit pageUnit(KoUnit::Millimeter);
		pageWidth = KoUnit::toInch(pageUnit.fromUserValue(pageWidth)) * KoGlobal::dpiX();
		pageHeight = KoUnit::toInch(pageUnit.fromUserValue(pageHeight)) * KoGlobal::dpiY();
		
		if ( !r->pageOptions().isPortrait() )
		{
			int tmp = pageWidth;
			pageWidth = pageHeight;
			pageHeight = tmp;
		}*/
	}
	

	setFixedSize(pageWidth,pageHeight);
	//setPaletteBackgroundColor(QColor(255,255,255));
	kDebug() << "PAGE IS " << pageWidth << "x" << pageHeight << endl;
	_repaint = true;
	_pm = new QPixmap(pageWidth, pageHeight);
	renderPage(1);
}

void KexiReportPage::paintEvent(QPaintEvent*)
{
	//bitBlt (this, 0, 0, _pm);
	QPainter painter(this);
	painter.drawPixmap(QPoint(0, 0), *_pm);
}

void KexiReportPage::renderPage(int p)
{
	kDebug() << "KexiReportPage::renderPage " << p << endl;
	page = p;
	_pm->fill();
	QPainter qp(_pm);
	KRScreenRender sr;
	sr.setPainter(&qp);
	sr.render(rpt, p-1);
	_repaint = true;
	repaint();
}

KexiReportPage::~KexiReportPage()
{
}


#include "kexireportpage.moc"