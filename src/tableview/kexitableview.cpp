/* This file is part of the KDE project
   Copyright (C) 2002 Till Busch <till@bux.at>
   Copyright (c) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2003 Jaroslaw Staniek <js@iidea.pl>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Original Author:  Till Busch <till@bux.at>
   Original Project: buX (www.bux.at)
*/

#include <qpainter.h>
#include <qkeycode.h>
#include <qheader.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qwmatrix.h>
#include <qtimer.h>
#include <qpopupmenu.h>
#include <qcursor.h>
#include <qstyle.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qwhatsthis.h>

#include <unistd.h>

#include <config.h>

#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>
#include <kapp.h>
#include <kiconloader.h>

#ifndef KEXI_NO_PRINT
# include <kprinter.h>
//# include <kdeprint/driver.h>
#endif

#include "kexitablerm.h"
#include "kexitableview.h"

#ifdef USE_KDE
#include "kexidatetableedit.h"
#endif

#include "kexitableedit.h"
#include "kexiinputtableedit.h"
#include "kexicomboboxtableedit.h"
#include "kexiblobtableedit.h"

#include "kexitableview_p.h"


KexiTableView::KexiTableView(KexiTableViewData* data, QWidget* parent, const char* name)
:QScrollView(parent, name, /*Qt::WRepaintNoErase | */Qt::WStaticContents /*| Qt::WResizeNoErase*/)
	,m_data(0)
	,m_owner(false)
	,d( new KexiTableViewPrivate() )
{
	setResizePolicy(Manual);
	viewport()->setBackgroundMode(NoBackground);
//	viewport()->setFocusPolicy(StrongFocus);
	viewport()->setFocusPolicy(WheelFocus);

	//setup colors defaults
	setBackgroundMode(PaletteBackground);
	setEmptyAreaColor(palette().active().color(QColorGroup::Base));

	d->baseColor = colorGroup().base();
	d->altColor = KGlobalSettings::alternateBackgroundColor();

	setLineWidth(1);
	horizontalScrollBar()->raise();
	verticalScrollBar()->raise();
	
	// setup scrollbar tooltip
	d->scrollBarTip = new QLabel("abc",0, "scrolltip",WStyle_Customize |WStyle_NoBorder|WX11BypassWM|WStyle_StaysOnTop|WStyle_Tool);
	d->scrollBarTip->setPalette(QToolTip::palette());
	d->scrollBarTip->setMargin(2);
	d->scrollBarTip->setIndent(0);
	d->scrollBarTip->setAlignment(AlignCenter);
	d->scrollBarTip->setFrameStyle( QFrame::Plain | QFrame::Box );
	d->scrollBarTip->setLineWidth(1);
	connect(verticalScrollBar(),SIGNAL(sliderReleased()),this,SLOT(vScrollBarSliderReleased()));
	connect(&d->scrollBarTipTimer,SIGNAL(timeout()),this,SLOT(scrollBarTipTimeout()));
	
	//context menu
	d->pContextMenu = new QPopupMenu(this);
	d->menu_id_addRecord = d->pContextMenu->insertItem(i18n("Add Record"), this, SLOT(addRecord()), CTRL+Key_Insert);
	d->menu_id_removeRecord = d->pContextMenu->insertItem(
		kapp->iconLoader()->loadIcon("button_cancel", KIcon::Small),
		i18n("Remove Record"), this, SLOT(removeRecord()), CTRL+Key_Delete);

#ifdef Q_WS_WIN
	d->rowHeight = fontMetrics().lineSpacing() + 4;
#else
	d->rowHeight = fontMetrics().lineSpacing() + 1;
#endif

	if(d->rowHeight < 17)
		d->rowHeight = 17;

	d->pUpdateTimer = new QTimer(this);

//	d->pColumnTypes = new QMemArray<QVariant::Type>;
//	d->pColumnModes = new QMemArray<int>;
//	d->pColumnDefaults = new QPtrList<QVariant>;
//	deletionPolicy = NoDelete;
	//m_additionPolicy = NoAdd;
	setAdditionPolicy( NoAdd );

//	setMargins(14, fontMetrics().height() + 4, 0, 0);

	// Create headers
	d->pTopHeader = new QHeader(this, "topHeader");
	d->pTopHeader->setOrientation(Horizontal);
	d->pTopHeader->setTracking(false);
	d->pTopHeader->setMovingEnabled(false);

	d->pVerticalHeader = new KexiTableRM(this);
	d->pVerticalHeader->setCellHeight(d->rowHeight);
//	d->pVerticalHeader->setFixedWidth(d->rowHeight);
	d->pVerticalHeader->setCurrentRow(-1);

	setMargins(
		QMIN(d->pTopHeader->sizeHint().height(), d->rowHeight),
		d->pTopHeader->sizeHint().height(), 0, 0);

	setMinimumHeight(horizontalScrollBar()->height() + d->rowHeight + topMargin());

	setupNavigator();

//	navPanelLyr->addStretch(25);
	
//	d->navPanel->updateGeometry();

/*	d->pVerticalHeader = new KexiTableHeader(this);
	
	d->pVerticalHeader->setOrientation(Vertical);
	d->pVerticalHeader->setTracking(false);
	d->pVerticalHeader->setMovingEnabled(false);
	d->pVerticalHeader->setStretchEnabled(false);
	d->pVerticalHeader->setResizeEnabled(false);
	d->pVerticalHeader->setCurrentRow(false);
*/

//	enableClipper(true);

	setData( data );

	d->scrollTimer = new QTimer(this);
	connect(d->scrollTimer, SIGNAL(timeout()), this, SLOT(slotAutoScroll()));

	setBackgroundAltering(true);

	// Connect header, table and scrollbars
	connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), d->pTopHeader, SLOT(setOffset(int)));
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)),	d->pVerticalHeader, SLOT(setOffset(int)));
	connect(d->pTopHeader, SIGNAL(sizeChange(int, int, int)), this, SLOT(columnWidthChanged(int, int, int)));
	connect(d->pTopHeader, SIGNAL(clicked(int)), this, SLOT(sortColumnInternal(int)));

	connect(d->pUpdateTimer, SIGNAL(timeout()), this, SLOT(slotUpdate()));
	
//	horizontalScrollBar()->show();
//	updateScrollBars();
}

//! Setup navigator widget
void KexiTableView::setupNavigator()
{
	updateScrollBars();
	
	d->navPanel = new QFrame(this, "navPanel");
	d->navPanel->setFrameStyle(QFrame::Panel|QFrame::Raised);
	d->navPanel->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
	QHBoxLayout *navPanelLyr = new QHBoxLayout(d->navPanel,0,0,"nav_lyr");
	navPanelLyr->setAutoAdd(true);

	new QLabel(QString(" ")+i18n("Row:")+" ",d->navPanel);
	
	int bw = 6+SmallIcon("navigator_first").width(); //QMIN( horizontalScrollBar()->height(), 20);
	QFont f = d->navPanel->font();
	f.setPixelSize((bw > 12) ? 12 : bw);
	QFontMetrics fm(f);
	int fw = fm.width("888888");
	
	d->navBtnFirst = new QToolButton(d->navPanel);
	d->navBtnFirst->setMaximumWidth(bw);
	d->navBtnFirst->setFocusPolicy(NoFocus);
	d->navBtnFirst->setIconSet( SmallIconSet("navigator_first") );
	QToolTip::add(d->navBtnFirst, i18n("First row"));
	
	d->navBtnPrev = new QToolButton(d->navPanel);
	d->navBtnPrev->setMaximumWidth(bw);
	d->navBtnPrev->setFocusPolicy(NoFocus);
	d->navBtnPrev->setIconSet( SmallIconSet("navigator_prev") );
	QToolTip::add(d->navBtnPrev, i18n("Previous row"));
	
	QWidget *spc = new QFrame(d->navPanel);
	spc->setFixedWidth(6);
	
	d->navRowNumber = new KLineEdit(d->navPanel);
	d->navRowNumber->setAlignment(AlignRight | AlignVCenter);
	d->navRowNumber->setFocusPolicy(ClickFocus);
	d->navRowNumber->setFixedWidth(fw);
	d->navRowNumberValidator = new QIntValidator(1, 1, this);
	d->navRowNumber->setValidator(d->navRowNumberValidator);
	d->navRowNumber->installEventFilter(this);
	QToolTip::add(d->navRowNumber, i18n("Current row number"));
	
	KLineEdit *lbl_of = new KLineEdit(i18n("of"), d->navPanel);
	lbl_of->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
	lbl_of->setMaximumWidth(fm.width(lbl_of->text()+10));
	lbl_of->setReadOnly(true);
	lbl_of->setLineWidth(0);
	lbl_of->setFocusPolicy(NoFocus);
	lbl_of->setAlignment(AlignCenter);
	
	d->navRowCount = new KLineEdit(d->navPanel);
	d->navRowCount->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
	d->navRowCount->setMaximumWidth(fw);
	d->navRowCount->setReadOnly(true);
	d->navRowCount->setLineWidth(0);
	d->navRowCount->setFocusPolicy(NoFocus);
	d->navRowCount->setAlignment(AlignLeft | AlignVCenter);

	d->navBtnNext = new QToolButton(d->navPanel);
	d->navBtnNext->setMaximumWidth(bw);
	d->navBtnNext->setFocusPolicy(NoFocus);
	d->navBtnNext->setIconSet( SmallIconSet("navigator_next") );
	QToolTip::add(d->navBtnNext, i18n("Next row"));
	
	d->navBtnLast = new QToolButton(d->navPanel);
	d->navBtnLast->setMaximumWidth(bw);
	d->navBtnLast->setFocusPolicy(NoFocus);
	d->navBtnLast->setIconSet( SmallIconSet("navigator_last") );
	QToolTip::add(d->navBtnLast, i18n("Last row"));
	
	spc = new QWidget(d->navPanel);
	spc->setFixedWidth(6);
	d->navBtnNew = new QToolButton(d->navPanel);
	d->navBtnNew->setMaximumWidth(bw);
	d->navBtnNew->setFocusPolicy(NoFocus);
	d->navBtnNew->setIconSet( SmallIconSet("navigator_new") );
	QToolTip::add(d->navBtnNew, i18n("New row"));
	d->navBtnNext->setEnabled(isInsertingEnabled());
	
	spc = new QFrame(d->navPanel);
	spc->setFixedWidth(6);

	lbl_of->setFont(f);
	d->navRowCount->setFont(f);
	d->navPanel->setFont(f);

	connect(d->navRowNumber,SIGNAL(returnPressed(const QString&)),
		this,SLOT(navRowNumber_ReturnPressed(const QString&)));
	connect(d->navRowNumber,SIGNAL(lostFocus()),
		this,SLOT(navRowNumber_lostFocus()));
	connect(d->navBtnPrev,SIGNAL(clicked()),this,SLOT(navBtnPrevClicked()));
	connect(d->navBtnNext,SIGNAL(clicked()),this,SLOT(navBtnNextClicked()));
	connect(d->navBtnLast,SIGNAL(clicked()),this,SLOT(navBtnLastClicked()));
	connect(d->navBtnFirst,SIGNAL(clicked()),this,SLOT(navBtnFirstClicked()));
	connect(d->navBtnNew,SIGNAL(clicked()),this,SLOT(navBtnNewClicked()));
	connect(verticalScrollBar(),SIGNAL(valueChanged(int)),
		this,SLOT(vScrollBarValueChanged(int)));
}

KexiTableView::~KexiTableView()
{
	cancelEditor();

	if (m_owner)
		delete m_data;
	delete d;
}

void KexiTableView::setData( KexiTableViewData *data, bool owner )
{
	if (m_owner && m_data) {
		kdDebug(44021) << "KexiTableView::setData(): destroying old data (owned)" << endl;
		delete m_data; //destroy old data
		m_data = 0;
	}
	m_owner = owner;
	if(!data) {
		m_data = new KexiTableViewData();
		m_owner = true;
		clearData();
	}
	else {
		m_data = data;
		m_owner = owner;
		kdDebug(44021) << "KexiTableView::setData(): using shared data" << endl;
		//add columns
		d->pTopHeader->setUpdatesEnabled(false);
		{
			KexiTableViewColumnList::const_iterator it;
			for (it=m_data->columns.begin(); it!=m_data->columns.end(); ++it) {
				d->pTopHeader->addLabel((*it).caption, (*it).width);
			}
		}
		d->pTopHeader->setUpdatesEnabled(true);
		//add rows
//		triggerUpdate();
		d->pVerticalHeader->addLabels(m_data->count());
	}
	
	if (!d->pInsertItem) {//first setData() call - add 'insert' item
		d->pInsertItem = new KexiTableItem(cols());
	}
	else {//just reinit
		d->pInsertItem->init(cols());
	}
	
	//set current row:
	d->pCurrentItem = 0;
	int curRow = -1, curCol = -1;
	d->curRow = -1;
	d->curCol = -1;
	if (m_data->columnsCount()>0) {
		if (rows()>0) {
			d->pCurrentItem = m_data->first();
			curRow = 0;
			curCol = 0;
		}
		else {//no data
			if (isInsertingEnabled()) {
				d->pCurrentItem = d->pInsertItem;
				curRow = 0;
				curCol = 0;
			}
		}
	}
	
	QSize s(tableSize());
	resizeContents(s.width(),s.height());
	
	updateRowCountInfo();
	
	setCursor(curRow, curCol);
	updateContents();
}

void KexiTableView::addDropFilter(const QString &filter)
{
	d->dropFilters.append(filter);
}

void KexiTableView::setFont(const QFont &f)
{
	QWidget::setFont(f);
#ifdef Q_WS_WIN
  d->rowHeight = fontMetrics().lineSpacing() + 4;
#else
  d->rowHeight = fontMetrics().lineSpacing() + 2;
#endif
	if(d->rowHeight < 22)
		d->rowHeight = 22;
	setMargins(14, d->rowHeight, 0, 0);
	d->pVerticalHeader->setCellHeight(d->rowHeight);
	updateContents();
}

void KexiTableView::remove(KexiTableItem *item, bool moveCursor)
{
	if(m_data->removeRef(item))
	{
		d->pVerticalHeader->removeLabel();//TODO
		if(moveCursor)
			setCursor(d->curRow);
//		selectPrev();
		d->pUpdateTimer->start(1,true);
	}
}

void KexiTableView::removeCurrentRecord()
{
	if (d->deletionPolicy == NoDelete || d->pCurrentItem == d->pInsertItem)
		return;
	if (d->deletionPolicy == ImmediateDelete && d->pCurrentItem) {
		remove(d->pCurrentItem);
	} else if (d->deletionPolicy == AskDelete) {
		//TODO(js)
	} else if (d->deletionPolicy == SignalDelete) {
		emit itemRemoveRequest(d->pCurrentItem);
		emit currentItemRemoveRequest();
	}
}

/*void KexiTableView::addRecord()
{
	emit addRecordRequest();
}*/

void KexiTableView::clearData(bool repaint)
{
	cancelEditor();
	d->pVerticalHeader->clear();
	m_data->clear();

	d->clearVariables();
	d->pVerticalHeader->setCurrentRow(-1);

//	d->pUpdateTimer->start(1,true);
	if (repaint)
		viewport()->repaint();
}

void KexiTableView::clearColumns(bool repaint)
{
	clearData(false);
	while(d->pTopHeader->count()>0)
		d->pTopHeader->removeLabel(0);

	if (repaint)
		viewport()->repaint();

/*	for(int i=0; i < rows(); i++)
	{
		d->pVerticalHeader->removeLabel();
	}

	editorCancel();
	m_contents->clear();

	d->clearVariables();
	d->numCols = 0;

	while(d->pTopHeader->count()>0)
		d->pTopHeader->removeLabel(0);

	d->pVerticalHeader->setCurrentRow(-1);

	viewport()->repaint();

//	d->pColumnTypes.resize(0);
//	d->pColumnModes.resize(0);
//	d->pColumnDefaults.clear();*/
}

#if 0 //todo
int KexiTableView::findString(const QString &string)
{
	int row = 0;
	int col = sorting();
	if(col == -1)
		return -1;
	if(string.isEmpty())
	{
		setCursor(0, col);
		return 0;
	}

	QPtrListIterator<KexiTableItem> it(*m_contents);

	if(string.at(0) != QChar('*'))
	{
		switch(columnType(col))
		{
			case QVariant::String:
			{
				QString str2 = string.lower();
				for(; it.current(); ++it)
				{
					if(it.current()->at(col).toString().left(string.length()).lower().compare(str2)==0)
					{
						center(columnPos(col), rowPos(row));
						setCursor(row, col);
						return row;
					}
					row++;
				}
				break;
			}
			case QVariant::Int:
			case QVariant::Bool:
				for(; it.current(); ++it)
				{
					if(QString::number(it.current()->at(col).toInt()).left(string.length()).compare(string)==0)
					{
						center(columnPos(col), rowPos(row));
						setCursor(row, col);
						return row;
					}
					row++;
				}
				break;

			default:
				break;
		}
	}
	else
	{
		QString str2 = string.mid(1);
		switch(columnType(col))
		{
			case QVariant::String:
				for(; it.current(); ++it)
				{
					if(it.current()->at(col).toString().find(str2,0,false) >= 0)
					{
						center(columnPos(col), rowPos(row));
						setCursor(row, col);
						return row;
					}
					row++;
				}
				break;
			case QVariant::Int:
			case QVariant::Bool:
				for(; it.current(); ++it)
				{
					if(QString::number(it.current()->at(col).toInt()).find(str2,0,true) >= 0)
					{
						center(columnPos(col), rowPos(row));
						setCursor(row, col);
						return row;
					}
					row++;
				}
				break;

			default:
				break;
		}
	}
	return -1;
}
#endif

void KexiTableView::slotUpdate()
{
	kdDebug(44021) << " KexiTableView::slotUpdate() -- " << endl;
//	QSize s(tableSize());
//	viewport()->setUpdatesEnabled(false);
///	resizeContents(s.width(), s.height());
//	viewport()->setUpdatesEnabled(true);
	updateGeometries();
	updateContents(0, 0, viewport()->width(), contentsHeight());
	updateGeometries();
}

bool KexiTableView::isSortingEnabled() const
{
	return d->isSortingEnabled;
}

void KexiTableView::setSortingEnabled(bool set)
{
	if (d->isSortingEnabled && !set)
		setSorting(-1);
	d->isSortingEnabled = set;
}

int KexiTableView::sortedColumn()
{
	if (d->isSortingEnabled)
		return m_data->sortedColumn();
	return -1;
}

bool KexiTableView::sortingAscending() const
{ 
	return m_data->sortingAscending();
}

void KexiTableView::setSorting(int col, bool ascending/*=true*/)
{
	if (!d->isSortingEnabled)
		return;
//	d->sortOrder = ascending;
//	d->sortedColumn = col;
	d->pTopHeader->setSortIndicator(col, ascending);//d->sortOrder);
	m_data->setSorting(col, ascending); //d->sortOrder);
//	sort();
//	columnSort(col);
}

void KexiTableView::sort()
{
	if (!d->isSortingEnabled || rows() < 2)
		return;

	if (d->pEditor)
		cancelRowEdit();
			
	if (m_data->sortedColumn()!=-1)
		m_data->sort();

	//locate current record
	if (!d->pCurrentItem) {
		d->pCurrentItem = m_data->first();
		d->curRow = 0;
		if (!d->pCurrentItem)
			return;
	}
	if (d->pCurrentItem != d->pInsertItem) {
		d->curRow = m_data->findRef(d->pCurrentItem);
	}

//	d->pCurrentItem = m_data->at(d->curRow);

	int cw = columnWidth(d->curCol);
	int rh = rowHeight();

//	d->pVerticalHeader->setCurrentRow(d->curRow);
	center(columnPos(d->curCol) + cw / 2, rowPos(d->curRow) + rh / 2);
//	updateCell(oldRow, d->curCol);
//	updateCell(d->curRow, d->curCol);
	d->pVerticalHeader->setCurrentRow(d->curRow);
//	slotUpdate();

	updateContents();
//	d->pUpdateTimer->start(1,true);
}

void KexiTableView::sortColumnInternal(int col)
{
//	bool i = false;
//js	QVariant hint;
	//-select sorting 
	bool asc;
	if (col==sortedColumn())
		asc = !sortingAscending(); //inverse sorting for this column
	else
		asc = true;
	
	setSorting( col, asc );
	
	//-perform sorting 
	sort();
	
#if 0//todo
	if(d->pInsertItem)
	{
		i = true;
//js		hint = d->pInsertItem->getHint();
//		delete d->pInsertItem;
		remove(d->pInsertItem);
		d->pInsertItem = 0;
//		d->pVerticalHeader->removeLabel(rows());
	}

	if(m_data->sortedColumn() == col) {
		m_data->setSorting(col, !m_data->sortingAscending());
	}
	else {
		m_data->setSorting(col);
	}
//js	else
//js		d->sortOrder = true;
//	d->sortedColumn = col;
	d->pTopHeader->setSortIndicator(col, m_data->sortingAscending());
	sort();

	if(i)
	{
		KexiTableItem *insert = new KexiTableItem(cols());
//js		insert->setHint(hint);
//js		insert->setInsertItem(true);
		d->pInsertItem = insert;

	}
//	updateContents( 0, 0, viewport()->width(), viewport()->height());
#endif
	emit sortedColumnChanged(col);
}

QSizePolicy KexiTableView::sizePolicy() const
{
	// this widget is expandable
	return QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

QSize KexiTableView::sizeHint() const
{
	const QSize &ts = tableSize();
	return QSize(ts.width() + leftMargin() + 2*2, //+ QMIN(d->pVerticalHeader->width(),d->rowHeight) + margin()*2,
		QMAX( ts.height()+topMargin()+horizontalScrollBar()->sizeHint().height(), 
		      minimumSizeHint().height() ) 
		);
//		QMAX(ts.height() + topMargin(), minimumSizeHint().height()) );
}

QSize KexiTableView::minimumSizeHint() const
{
	return QSize(columnWidth(1), d->rowHeight*3 + d->pTopHeader->height());
}

void KexiTableView::createBuffer(int width, int height)
{
	if(!d->pBufferPm)
		d->pBufferPm = new QPixmap(width, height);
	else
		if(d->pBufferPm->width() < width || d->pBufferPm->height() < height)
			d->pBufferPm->resize(width, height);
//	d->pBufferPm->fill();
}

//internal
inline void KexiTableView::paintRow(KexiTableItem *item,
	QPainter *pb, int r, int rowp, int cx, int cy, 
	int colfirst, int collast, int maxwc)
{
	// Go through the columns in the row r
	// if we know from where to where, go through [colfirst, collast],
	// else go through all of them
	int colp=0, colw;
	colp = 0;
	int transly = rowp-cy;
	int c;

	if(d->bgAltering && (r%2 != 0))
		pb->fillRect(0, transly, maxwc, d->rowHeight - 1, d->altColor);
	else
		pb->fillRect(0, transly, maxwc, d->rowHeight - 1, d->baseColor);

	for(c = colfirst; c <= collast; c++)
	{
		// get position and width of column c
		colp = columnPos(c);
		colw = columnWidth(c);
		int translx = colp-cx;

		// Translate painter and draw the cell
		pb->saveWorldMatrix();
		pb->translate(translx, transly);
			paintCell( pb, item, c, QRect(colp, rowp, colw, d->rowHeight));
		pb->restoreWorldMatrix();
	}
}

void KexiTableView::drawContents( QPainter *p, int cx, int cy, int cw, int ch)
{
	if (d->disableDrawContents)
		return;
	int colfirst = columnAt(cx);
	int rowfirst = rowAt(cy);
	int collast = columnAt(cx + cw-1);
	int rowlast = rowAt(cy + ch-1);
	bool inserting = isInsertingEnabled();
	bool plus1row = false; //true if we should show 'inserting' row at the end
	bool paintOnlyInsertRow = false;

	kdDebug(44021) << QString(" KexiTableView::drawContents(cx:%1 cy:%2 cw:%3 ch:%4)")
			.arg(cx).arg(cy).arg(cw).arg(ch) << endl;

 	if (rowlast == -1) {
		rowlast = rows() - 1;
		plus1row = inserting;
		if (rowfirst == -1) {
			if (rowAt(cy - d->rowHeight) != -1) {
				paintOnlyInsertRow = true;
				kdDebug(44021) << "-- paintOnlyInsertRow --" << endl;
			}
		}
	}
	kdDebug(44021) << "rowfirst="<<rowfirst<<" rowlast="<<rowlast<<" rows()="<<rows()<<endl;
	kdDebug(44021)<<" plus1row=" << plus1row<<endl;
	
	if ( collast == -1 )
		collast = cols() - 1;

// 	qDebug("cx:%3d cy:%3d w:%3d h:%3d col:%2d..%2d row:%2d..%2d tsize:%4d,%4d", 
//    cx, cy, cw, ch, colfirst, collast, rowfirst, rowlast, tableSize().width(), tableSize().height());

//	triggerUpdate();

	if (rowfirst == -1 || colfirst == -1) {
		if (!paintOnlyInsertRow) {
			paintEmptyArea(p, cx, cy, cw, ch);
			return;
		}
	}

	createBuffer(cw, ch);
	if(d->pBufferPm->isNull())
		return;
	QPainter *pb = new QPainter(d->pBufferPm, this);

//	pb->fillRect(0, 0, cw, ch, colorGroup().base());


//	int maxwc = QMIN(cw, (columnPos(d->numCols - 1) + columnWidth(d->numCols - 1)));
	int maxwc = columnPos(cols() - 1) + columnWidth(cols() - 1);
//	kdDebug(44021) << "KexiTableView::drawContents(): maxwc: " << maxwc << endl;

// 	pb->fillRect(maxwc, 0, cw - maxwc, ch, colorGroup().base());
// 	pb->fillRect(0, rowPos(rowlast) + d->rowHeight, cw, ch, colorGroup().base());
	pb->fillRect(cx, cy, cw, ch, colorGroup().base());

//	QColor baseCol = colorGroup().base();
//	QColor altCol = KGlobalSettings::alternateBackgroundColor();

	int rowp;
	int r;
	if (paintOnlyInsertRow) {
		r = rows();
		rowp = rowPos(r); // 'insert' row's position
	}
	else {
		QPtrListIterator<KexiTableItem> it(*m_data);
		it += rowfirst;//move to 1st row
		rowp = rowPos(rowfirst); // row position 
		for (r = rowfirst;r <= rowlast; r++, ++it, rowp+=d->rowHeight) {
			paintRow(it.current(), pb, r, rowp, cx, cy, colfirst, collast, maxwc);
		}
	}

	if (plus1row) { //additional - 'insert' row
		paintRow(d->pInsertItem, pb, r, rowp, cx, cy, colfirst, collast, maxwc);
	}
	
/*	if (d->paintAfterInsertRow) { //'insert' row is edited now: paint also 1 row after this
		paintRow(d->pAfterInsertItem, pb, r, rowp, cx, cy, colfirst, collast, maxwc, baseCol, altCol);
	}*/

	//draw current cell
/*	int colp, colw, rowp;
	rowp = rowPos(d->curRow);
	colp = columnPos(d->curCol);
	colw = columnWidth(d->curCol);
	int translx = colp-cx;
	int transly = rowp-cy;
	// Translate painter and draw the cell
	pb->saveWorldMatrix();
	pb->translate(translx, transly);
		paintCell( pb, d->pCurrentItem, d->curCol, QRect(colp, rowp, colw, d->rowHeight));
	pb->restoreWorldMatrix();
*/

	delete pb;

	p->drawPixmap(cx,cy,*d->pBufferPm, 0,0,cw,ch);

  //(js)
	paintEmptyArea(p, cx, cy, cw, ch);
}

void KexiTableView::paintCell(QPainter* p, KexiTableItem *item, int col, const QRect &cr, bool print)
{
	Q_UNUSED(print);
	int w = cr.width();
	int h = cr.height();
	int x2 = w - 1;
	int y2 = h - 1;

//	p->setPen(colorGroup().button());

/*	if(d->bgAltering && !print && d->contents->findRef(item)%2 != 0)
	{
		QPen originalPen(p->pen());
		QBrush originalBrush(p->brush());

		p->setBrush(KGlobalSettings::alternateBackgroundColor());
		p->setPen(KGlobalSettings::alternateBackgroundColor());
		p->drawRect(0, 0, x2, y2);

		p->setPen(originalPen);
		p->setBrush(originalBrush);
	}
*/
	//	Draw our lines
	QPen pen(p->pen());
//	if(!print)
	QColor gray(200,200,200);
	p->setPen(gray);
//	else
//		p->setPen(black);

//	p->drawRect( -1, -1, w+1, h+1 );
//	p->drawRect( 0, 0, w, h );

	p->drawLine( x2, 0, x2, y2 );	// right
	p->drawLine( 0, y2, x2, y2 );	// bottom
	p->setPen(pen);

	//	If we are in the focus cell, draw indication
	if(d->pCurrentItem == item && col == d->curCol && m_data->columns[col].readOnly) //js: && !d->recordIndicator)
	{
		if (!hasFocus() && !viewport()->hasFocus()) {
			QPen gray_pen(p->pen());
			gray_pen.setColor(gray);
			p->setPen(gray_pen);
		}
		p->drawRect(0, 0, x2, y2);
//			p->drawRect(-1, -1, w+1, h+1);
	}
/*	if(d->pCurrentItem == item && d->recordIndicator)
	{
		p->setBrush(colorGroup().highlight());
		p->setPen(colorGroup().highlight());
		p->drawRect(0, 0, x2, y2);
	}
*/

//	int iOffset = 0;
	QPen fg(colorGroup().text());
	p->setPen(fg);
#if 0 //todo(js)
	if(item->isInsertItem())
	{
//		QFont f = p->font();
//		f.setBold(true);
//		p->setFont(f);
//		p->setPen(QPen(colorGroup().mid()));
		p->setPen(QColor(190,190,190));
//		iOffset = 3;
	}
#endif

#ifdef Q_WS_WIN
	int x = 1;
	int y_offset = 1;
#else
	int x = 1;
//	int y_offset = 2;
	int y_offset = 1;
#endif

	switch(columnType(col))
	{
		case QVariant::UInt:
		case QVariant::Int:
		{
			int num = item->at(col).toInt();
//			if(num < 0)
//				p->setPen(red);
//			p->drawText(x - (x+x) - 2, 2, w, h, AlignRight, QString::number(num));
//			qDebug("KexiTableView::paintCell(): mode: %i", d->pColumnModes->at(col));
#ifdef Q_WS_WIN
//			y_offset = 1;
			x = 1;
#else
			x = 0;
#endif
#if 0 //todo(js)
			if(item->isInsertItem() && m_data->columns[col]......... d->pColumnModes.at(col) == 3)  //yes that isn't beautiful
			{
				p->drawText(x, y_offset, w - (x+x) - 6, h, AlignRight, "[Auto]");
			}
			else
			{
				p->drawText(x, y_offset, w - (x+x) - 6, h, AlignRight, QString::number(num));
			}
#else
				p->drawText(x, y_offset, w - (x+x) - 6, h, AlignRight, QString::number(num));
#endif
//			p->drawRect(x - 1, 1, w - (x+x) - 1, h + 1);
			break;
		}
		case QVariant::Double:
		{
#ifdef Q_WS_WIN
#else
			x = 0;
#endif
			QString f = KGlobal::locale()->formatNumber(item->at(col).toDouble());
#if 0 //todo(js)
			if(item->isInsertItem() && d->pColumnModes.at(col) == 3)  //yes that isn't beautiful
			{
				p->drawText(x, y_offset, w - (x+x) - 6, h, AlignRight, "[Auto]");
			}
			else
			{
				p->drawText(x, y_offset, w - (x+x) - 6, h, AlignRight, f);
			}
#else
				p->drawText(x, y_offset, w - (x+x) - 6, h, AlignRight, f);
#endif
			break;
		}
		case QVariant::Bool:
		{
/*			QRect r(w/2 - style().pixelMetric(QStyle::PM_IndicatorWidth)/2 + x-1, 1, style().pixelMetric(QStyle::PM_IndicatorWidth), style().pixelMetric(QStyle::PM_IndicatorHeight));
			QPen pen(p->pen());		// bug in KDE HighColorStyle
			style().drawControl(QStyle::CE_CheckBox, p, this, r, colorGroup(), (item->getInt(col) ? QStyle::Style_On : QStyle::Style_Off) | QStyle::Style_Enabled);
			p->setPen(pen); */
			int s = QMAX(h - 5, 12);
			QRect r(w/2 - s/2 + x, h/2 - s/2, s, s);
			p->setPen(QPen(colorGroup().text(), 1));
			p->drawRect(r);
			if(item->at(col).asBool())
			{
				p->drawLine(r.x() + 2, r.y() + 2, r.right() - 1, r.bottom() - 1);
				p->drawLine(r.x() + 2, r.bottom() - 2, r.right() - 1, r.y() + 1);
			}

			break;
		}
		case QVariant::Date:
		{
#ifdef Q_WS_WIN
			x = 5;
			y_offset = -1;
#else
			x = 5;
			y_offset = 0;
#endif
			QString s = "";

			if(item->at(col).toDate().isValid())
			{
				#ifdef USE_KDE
				s = KGlobal::locale()->formatDate(item->at(col).toDate(), true);
				#else
				s = item->getDate(col).toString(Qt::LocalDate);
				#endif
				p->drawText(x, y_offset, w - (x+x), h, AlignLeft | SingleLine | AlignVCenter, s);
//				p->drawText(x, -1, w - (x+x), h, AlignLeft | SingleLine | AlignVCenter, s);
				break;
			}
			break;
		}
		case QVariant::StringList:
		{
#if 0 //todo(js)
			QStringList sl = d->pColumnDefaults.at(col)->toStringList();
			p->drawText(x, y_offset, w - (x+x), h, AlignLeft | SingleLine | AlignVCenter,
				sl[item->at(col).toInt()]);
#endif
			break;
		}
		case QVariant::String:
		default:
		{
#ifdef Q_WS_WIN
			x = 5;
			y_offset = -1;
#else
			x = 5;
			y_offset = 0;
#endif
			if(d->pCurrentItem == item && col == d->curCol && !m_data->columns[col].readOnly && viewport()->hasFocus()) //js: && !d->recordIndicator)
			{
				QRect bound=fontMetrics().boundingRect(x, y_offset, w - (x+x), h, AlignLeft | SingleLine | AlignVCenter, item->at(col).toString());
				p->setPen(colorGroup().highlightedText());
				p->fillRect(bound, colorGroup().highlight());
			}

			p->drawText(x, y_offset, w - (x+x), h, AlignLeft | SingleLine | AlignVCenter, item->at(col).toString());
		}
	}
	p->setPen(fg);
#if 0 //todo(js)
	if(item->isInsertItem())
	{
		QFont f = p->font();
		f.setBold(false);
		p->setFont(f);
//		iOffset = 3;
	}
#endif
}

QPoint KexiTableView::contentsToViewport2( const QPoint &p )
{
    return QPoint( p.x() - contentsX(),
		   p.y() - contentsY() );
}

void KexiTableView::contentsToViewport2( int x, int y, int& vx, int& vy )
{
    const QPoint v = contentsToViewport2( QPoint( x, y ) );
    vx = v.x();
    vy = v.y();
}

QPoint KexiTableView::viewportToContents2( const QPoint& vp )
{
    return QPoint( vp.x() + contentsX(),
		   vp.y() + contentsY() );
}

void KexiTableView::paintEmptyArea( QPainter *p, int cx, int cy, int cw, int ch )
{
//  qDebug("%s: paintEmptyArea(x:%d y:%d w:%d h:%d)", (const char*)parentWidget()->caption(),cx,cy,cw,ch);

    // Regions work with shorts, so avoid an overflow and adjust the
    // table size to the visible size
    QSize ts( tableSize() );
    ts.setWidth( QMIN( ts.width(), visibleWidth() ) );
	ts.setHeight( QMIN( ts.height() - (d->navPanel ? d->navPanel->height() : 0), visibleHeight()) );
	kdDebug(44021) << QString(" (cx:%1 cy:%2 cw:%3 ch:%4)")
			.arg(cx).arg(cy).arg(cw).arg(ch) << endl;
	kdDebug(44021) << QString(" (w:%3 h:%4)")
			.arg(ts.width()).arg(ts.height()) << endl;
	
	// Region of the rect we should draw, calculated in viewport
    // coordinates, as a region can't handle bigger coordinates
    contentsToViewport2( cx, cy, cx, cy );
    QRegion reg( QRect( cx, cy, cw, ch ) );

    // Subtract the table from it
    reg = reg.subtract( QRect( QPoint( 0, 0 ), ts ) );

    // And draw the rectangles (transformed inc contents coordinates as needed)
    QMemArray<QRect> r = reg.rects();
    for ( int i = 0; i < (int)r.count(); ++i ) {
		QRect rect( viewportToContents2(r[i].topLeft()), r[i].size() );
		kdDebug(44021) << QString("- pEA: p->fillRect(x:%1 y:%2 w:%3 h:%4)")
			.arg(rect.x()).arg(rect.y())
			.arg(rect.width()).arg(rect.height()) << endl;
//		p->fillRect( QRect(viewportToContents2(r[i].topLeft()),r[i].size()), d->emptyAreaColor );
		p->fillRect( rect, d->emptyAreaColor );
//		p->fillRect( QRect(viewportToContents2(r[i].topLeft()),r[i].size()), viewport()->backgroundBrush() );
	}
}

void KexiTableView::contentsMouseDoubleClickEvent(QMouseEvent *e)
{
	kdDebug(44021) << "KexiTableView::contentsMouseDoubleClickEvent()" << endl;
	d->contentsMousePressEvent_dblClick = true;
	contentsMousePressEvent(e);
	d->contentsMousePressEvent_dblClick = false;

	if(d->pCurrentItem)
	{
		if(d->editOnDoubleClick && columnEditable(d->curCol) && columnType(d->curCol) != QVariant::Bool)
		{
			createEditor(d->curRow, d->curCol, QString::null);
		}

		emit itemDblClicked(d->pCurrentItem, d->curCol);
	}
}

void KexiTableView::contentsMousePressEvent( QMouseEvent* e )
{
	kdDebug(44021) << "KexiTableView::contentsMousePressEvent() ??" << endl;
	if(m_data->count()==0 && !isInsertingEnabled())
		return;

	if (columnAt(e->pos().x())==-1) //outside a colums
		return;
	
//	d->contentsMousePressEvent_ev = *e;
//	d->contentsMousePressEvent_enabled = true;
//	QTimer::singleShot(2000, this, SLOT( contentsMousePressEvent_Internal() ));
//	d->contentsMousePressEvent_timer.start(100,true);
	
//	if (!d->contentsMousePressEvent_enabled)
//		return;
//	d->contentsMousePressEvent_enabled=false;
	
	// remember old focus cell
	int oldRow = d->curRow;
	int oldCol = d->curCol;
	kdDebug(44021) << "oldRow=" << oldRow <<" oldCol=" << oldCol <<endl;
	bool onInsertItem = false;

	int newrow, newcol;
	//compute clicked row nr
	if (isInsertingEnabled()) {
		if (rowAt(e->pos().y())==-1) {
			newrow = rowAt(e->pos().y() - d->rowHeight);
			if (newrow==-1)
				return;
			newrow++;
			kdDebug(44021) << "Clicked just on 'insert' row." << endl;
			onInsertItem=true;
		}
		else {
			// get new focus cell
			newrow = rowAt(e->pos().y());
		}
	}
	else {
		if (rowAt(e->pos().y())==-1 || columnAt(e->pos().x())==-1)
			return; //clicked outside a grid
		// get new focus cell
		newrow = rowAt(e->pos().y());
	}
	newcol = columnAt(e->pos().x());

/*		// get rid of editor
		if (d->pEditor)
			acceptEditor();*/

/*	if(d->curRow == -1)
		d->curRow = d->numRows-1;

	if(d->curCol == -1)
		d->curCol = d->numCols-1;
*/
	setCursor(newrow,newcol);

/*js:
	//	if we have a new focus cell, repaint
	if (( d->curRow != oldRow) || (d->curCol != oldCol ))
	{
		kdDebug(44021) << "KexiTableView::contentsMousePressEvent(): new cell: reapaint!" << endl;
		int cw = columnWidth( d->curCol );
		int rh = rowHeight();
		int cp = columnPos( d->curCol );
		
		kdDebug(44021) << "colpos=" << columnPos( d->curCol ) << " cw="<<cw
		<<" contentsWidth()="<<contentsWidth()<<" contentsX()="<<contentsX()
		<<" visibleWidth()="<<visibleWidth()<<endl;

		d->pVerticalHeader->setCurrentRow(d->curRow);
		if (onInsertItem) {
			d->pCurrentItem = d->pInsertItem;
		}
		else {
			d->pCurrentItem = itemAt(d->curRow);
		}
//		d->disableDrawContents = true;
		ensureVisible( columnPos( d->curCol ) + cw / 2, rowPos( d->curRow ) + rh / 2, cw / 2, rh / 2 );
//		d->disableDrawContents = false;
		updateCell( oldRow, oldCol );
		
		//update newly visible area:
		if ((cp+cw) >= visibleWidth()) {
			//full column update, because it was hidden (at least partially)
			updateContents( QRect(cp, 0, cw, visibleHeight()) );
		} 
		else {
			//update just new cell
			updateCell( d->curRow, d->curCol );
		}
//		d->pVerticalHeader->setCurrentRow(d->curRow);
//		d->pCurrentItem = itemAt(d->curRow);
//		kdDebug(44021)<<"void KexiTableView::contentsMousePressEvent( QMouseEvent* e ) trying to get the current item"<<endl;
		if (!onInsertItem) {
			emit itemSelected(d->pCurrentItem);
		}
	}*/

//	kdDebug(44021)<<"void KexiTableView::contentsMousePressEvent( QMouseEvent* e ) by now the current items should be set, if not -> error + crash"<<endl;
	if(e->button() == RightButton)
	{
		showContextMenu(e->globalPos());
	}
	else if(e->button() == LeftButton)
	{
		if(columnType(d->curCol) == QVariant::Bool && columnEditable(d->curCol))
		{
			boolToggled();
			updateCell( d->curRow, d->curCol );
		}
		else if(columnType(d->curCol) == QVariant::StringList && columnEditable(d->curCol))
		{
			createEditor(d->curRow, d->curCol);
		}
	}
}

void KexiTableView::showContextMenu(QPoint pos)
{
	if (pos==QPoint(-1,-1)) {
		pos = viewport()->mapToGlobal( QPoint( columnPos(d->curCol), rowPos(d->curRow) + d->rowHeight ) );
	}
	//show own context menu if configured
	if (updateContextMenu()) {
		selectRow(d->curRow);
		d->pContextMenu->exec(pos);
	}
	else {
		//request other context menu
		emit contextMenuRequested(d->pCurrentItem, d->curCol, pos);
	}
}

void KexiTableView::contentsMouseMoveEvent( QMouseEvent *e )
{
	// do the same as in mouse press
	int x,y;
	contentsToViewport(e->x(), e->y(), x, y);

	if(y > visibleHeight())
	{
		d->needAutoScroll = true;
		d->scrollTimer->start(70, false);
		d->scrollDirection = ScrollDown;
	}
	else if(y < 0)
	{
		d->needAutoScroll = true;
		d->scrollTimer->start(70, false);
		d->scrollDirection = ScrollUp;
	}
	else if(x > visibleWidth())
	{
		d->needAutoScroll = true;
		d->scrollTimer->start(70, false);
		d->scrollDirection = ScrollRight;
	}
	else if(x < 0)
	{
		d->needAutoScroll = true;
		d->scrollTimer->start(70, false);
		d->scrollDirection = ScrollLeft;
	}
	else
	{
		d->needAutoScroll = false;
		d->scrollTimer->stop();
		contentsMousePressEvent(e);
	}

}

void KexiTableView::startEditCurrentCell()
{
	if(columnType(d->curCol) != QVariant::Bool && columnEditable(d->curCol)) {
		ensureVisible(columnPos(d->curCol), rowPos(d->curRow)+rowHeight(), columnWidth(d->curCol), rowHeight());
		createEditor(d->curRow, d->curCol, QString::null, false);
	}
}

void KexiTableView::contentsMouseReleaseEvent(QMouseEvent *)
{
	if(d->needAutoScroll)
	{
		d->scrollTimer->stop();
	}
}

void KexiTableView::keyPressEvent(QKeyEvent* e)
{
//	kdDebug(44021) << "KexiTableView::KeyPressEvent()" << endl;

	if (d->skipKeyPress) {
		d->skipKeyPress=false;
		e->ignore();
		return;
	}
	
	if(d->pCurrentItem == 0 && m_data->count() > 0)
	{
		setCursor(0,0);
/*js		d->curCol = d->curRow = 0;
		int cw = columnWidth(d->curCol);

		ensureVisible(columnPos(d->curCol) + cw / 2, rowPos(d->curRow) + d->rowHeight / 2, cw / 2, d->rowHeight / 2);
		updateCell(d->curRow, d->curCol);
		d->pVerticalHeader->setCurrentRow(d->curRow);
		d->pCurrentItem = itemAt(d->curRow);
		emit itemSelected(d->pCurrentItem);*/
	}
	else if(m_data->count() == 0)
	{
		return;
	}

	// if a cell is just editing, do some special stuff
	if(d->pEditor)
	{
		if (e->key() == Key_Escape) {
			cancelEditor();
		} else if (e->key() == Key_Return || e->key() == Key_Enter) {
			acceptEditor();
		}
		return;
	}

	if(e->key() == Key_Return || e->key() == Key_Enter)
	{
		emit itemReturnPressed(d->pCurrentItem, d->curCol);
	}

	int curRow = d->curRow;
	int curCol = d->curCol;

	switch (e->key())
	{
	case Key_Delete:
		if (e->state()==Qt::ControlButton) {//remove current row
			removeCurrentRecord();
		}
		else {//remove all chars in the current cell
			if(columnType(curCol) != QVariant::Bool && columnEditable(curCol))
				createEditor(curRow, curCol, QString::null, false);
			if (d->pEditor && d->pEditor->isA("KexiInputTableEdit")) {
				static_cast<KexiInputTableEdit*>(d->pEditor)->clear();
			}
		}
		break;

	case Key_Shift:
	case Key_Alt:
	case Key_Control:
	case Key_Meta:
		e->ignore();
		break;

	case Key_Left:
		curCol = QMAX(0, curCol - 1);
		break;
	case Key_Right:
	case Key_Tab:
		curCol = QMIN(cols() - 1, curCol + 1);
		break;
	case Key_Up:
		curRow = QMAX(0, curRow - 1);
		break;
	case Key_Down:
		curRow = QMIN(rows() - 1 + (isInsertingEnabled()?1:0), curRow + 1);
		break;
	case Key_Prior:
		curRow -= visibleHeight() / d->rowHeight;
		curRow = QMAX(0, curRow);
		break;
	case Key_Next:
		curRow += visibleHeight() / d->rowHeight;
		curRow = QMIN(rows() - 1 + (isInsertingEnabled()?1:0), curRow);
		break;
	case Key_Home:
		curRow = 0;
		break;
	case Key_End:
		curRow = m_data->count()-1+(isInsertingEnabled()?1:0);
		break;

	case Key_Enter:
	case Key_Return:
	case Key_F2:
		startEditCurrentCell();
		return;
	
	case Key_Backspace:
		if(columnType(curCol) != QVariant::Bool && columnEditable(curCol))
			createEditor(curRow, curCol, QString::null, true);
		break;
	case Key_Space:
		if(columnType(curCol) == QVariant::Bool && columnEditable(curCol))
		{
			boolToggled();
			break;
		}
	case Key_Escape:
		if (d->rowEditing) {
			cancelRowEdit();
			return;
		}
		break;
	default:
		//others:
		if (e->key()==KGlobalSettings::contextMenuKey()) { //Key_Menu:
			showContextMenu();
		}
		else {
			qDebug("KexiTableView::KeyPressEvent(): default");
			if (e->text().isEmpty() || !e->text().isEmpty() && !e->text()[0].isPrint() ) {
				kdDebug(44021) << "NOT PRINTABLE: 0x0" << QString("%1").arg(e->key(),0,16) <<endl;
				e->ignore();
				return;
			}
//			if(columnType(curCol) != QVariant::Bool && columnEditable(curCol))
			KexiTableViewColumn &colinfo = m_data->columns[curCol];
			if (colinfo.acceptsFirstChar(e->text()[0])) {
				kdDebug(44021) << "KexiTableView::KeyPressEvent(): ev pressed: acceptsFirstChar()==true";
	//			if (e->text()[0].isPrint())
				createEditor(curRow, curCol, e->text(), true);
			}
			else 
				kdDebug(44021) << "KexiTableView::KeyPressEvent(): ev pressed: acceptsFirstChar()==false";
		}
	}
	// if focus cell changes, repaint
	setCursor(curRow, curCol);

/*js	if ( d->curRow != oldRow || d->curCol != oldCol )
	{
		int cw = columnWidth(d->curCol);

		ensureVisible(columnPos(d->curCol) + cw / 2, rowPos(d->curRow) + d->rowHeight / 2, cw / 2, d->rowHeight / 2);
		updateCell(oldRow, oldCol);
		updateCell(d->curRow, d->curCol);
		d->pVerticalHeader->setCurrentRow(d->curRow);

		if (isInsertingEnabled() && d->curRow == rows()) {
			d->pCurrentItem = d->pInsertItem;
		}
		else {
			d->pCurrentItem = itemAt(d->curRow);
			emit itemSelected(d->pCurrentItem);
		}
	}*/
}

/*bool KexiTableView::columnAcceptsFirstChar( int col, const QString& ch )
{
	if (columnType(col) == QVariant::Bool)
		return false;
	
}*/

void KexiTableView::emitSelected()
{
	if(d->pCurrentItem)
		emit itemSelected(d->pCurrentItem);
}

void KexiTableView::boolToggled()
{
	int s = d->pCurrentItem->at(d->curCol).toInt();
	QVariant oldValue=d->pCurrentItem->at(d->curCol);
	(*d->pCurrentItem)[d->curCol] = QVariant(s ? 0 : 1);
	updateCell(d->curRow, d->curCol);
	emit itemChanged(d->pCurrentItem, d->curCol,oldValue);
	emit itemChanged(d->pCurrentItem, d->curCol);
}

void KexiTableView::selectNextRow()
{
//	int oldRow = d->curRow;
//	d->curRow = QMIN( rows() - 1, d->curRow + 1 );
	selectRow( QMIN( rows() - 1 +(isInsertingEnabled()?1:0), d->curRow + 1 ) );
/*js
	if(d->curRow != oldRow)
	{
		int rh = rowHeight(d->curRow);

		ensureVisible(0, rowPos(d->curRow) + rh / 2, 0, rh / 2);
		if (hasFocus() || viewport()->hasFocus())
		{
			updateCell(oldRow, d->curCol);
			updateCell(d->curRow, d->curCol);
		}
		d->pVerticalHeader->setCurrentRow(d->curRow);
		d->pCurrentItem = itemAt(d->curRow);
		emit itemSelected(d->pCurrentItem);
	}*/
}

void KexiTableView::selectFirstRow()
{
	selectRow(0);
}

void KexiTableView::selectLastRow()
{
	selectRow(rows() - 1 + (isInsertingEnabled()?1:0));
}

void KexiTableView::selectRow(int row)
{
	setCursor(row, -1);
/*(js)	int oldRow = d->curRow;

	if(d->curRow != row)
	{
		d->curRow = row;
		int rh = rowHeight();

		ensureVisible(0, rowPos(d->curRow) + rh / 2, 0, rh / 2);
		if (hasFocus() || viewport()->hasFocus())
		{
			updateCell(oldRow, d->curCol);
			updateCell(d->curRow, d->curCol);
		}
		d->pVerticalHeader->setCurrentRow(d->curRow);
		d->pCurrentItem = itemAt(d->curRow);
		emit itemSelected(d->pCurrentItem);
	}
*/
}

void KexiTableView::selectPrevRow()
{
//	int oldRow = d->curRow;

//	d->curRow = QMAX( 0, d->curRow - 1 );
	selectRow( QMAX( 0, d->curRow - 1 ) );
	/*js
	if(d->curRow != oldRow)
	{
		int rh = rowHeight(d->curRow);

		ensureVisible(0, rowPos(d->curRow) + rh / 2, 0, rh / 2);
		if (hasFocus() || viewport()->hasFocus())
		{
			updateCell(oldRow, d->curCol);
			updateCell(d->curRow, d->curCol);
		}
		d->pVerticalHeader->setCurrentRow(d->curRow);
		d->pCurrentItem = itemAt(d->curRow);
		emit itemSelected(d->pCurrentItem);
	}*/
}

/*void KexiTableView::gotoNext()
{
	int oldCol = d->curCol;
	d->curCol = QMIN( cols() - 1, d->curCol + 1 );

	if(d->curCol != oldCol)
	{
		if (hasFocus() || viewport()->hasFocus())
		{
			updateCell(d->curRow, oldCol);
			updateCell(d->curRow, d->curCol);
		}
	}
	else
	{
		selectNext();
	}
}*/

void KexiTableView::createEditor(int row, int col, const QString& addText, bool removeOld)
{
	kdDebug(44021) << "KexiTableView::createEditor('"<<addText<<"',"<<removeOld<<")"<<endl;
	if (m_data->columns[col].readOnly)//d->pColumnModes.at(d->numCols-1) & ColumnReadOnly)
		return;
	
//	QString val;
	QVariant val;
	if (!removeOld) {
		val = d->pCurrentItem->at(d->curCol);
	}
/*	switch(columnType(col))
	{
		case QVariant::Date:
			#ifdef USE_KDE
//			val = KGlobal::locale()->formatDate(d->pCurrentItem->getDate(col), true);

			#else
//			val = d->pCurrentItem->getDate(col).toString(Qt::LocalDate);
			#endif
			break;

		default:
//			val = d->pCurrentItem->getText(d->curCol);
			val = d->pCurrentItem->getValue(d->curCol);

			break;
	}*/

	if (!d->rowEditing) {
		d->rowEditing = true;
		//indicate on the vheader that we are editing:
		d->pVerticalHeader->setEditRow(d->curRow);
		if (isInsertingEnabled() && d->pCurrentItem==d->pInsertItem) {
			//we should know that we are in state "new row editing"
			d->newRowEditing = true;
			//'insert' row editing: show another row after that:
			m_data->append( d->pInsertItem );
			//new empty insert item
			d->pInsertItem = new KexiTableItem(cols());
//			updateContents();
			d->pVerticalHeader->addLabel();
			QSize s(tableSize());
			resizeContents(s.width(), s.height());
			updateContents(columnPos(0), rowPos(row+1), viewport()->width(), d->rowHeight);
			qApp->processEvents(500);
			ensureVisible(columnPos(d->curCol), rowPos(row+1)+d->rowHeight-1, columnWidth(d->curCol), d->rowHeight);
		}
	}	
//	else {//just reinit
//		d->pAfterInsertItem->init(cols());
//			d->paintAfterInsertRow = true;
//	}


	//it's getting ugly :)

	int t = columnType(col);
	if (t==KexiDB::Field::BLOB) {
			d->pEditor = new KexiBlobTableEdit(val.toByteArray(), viewport(), "inPlaceEd");
			d->pEditor->resize(columnWidth(d->curCol)-1, 150);
			moveChild(d->pEditor, columnPos(d->curCol), rowPos(d->curRow));
	} 
#if 0 //todo(js)
		case QVariant::StringList:
			d->pEditor = new KexiComboBoxTableEdit(static_cast<KexiDB::Field::Type>(val.toInt()),
				QStringList(), viewport(), "inPlaceEd");
			break;
		case QVariant::Date:
			d->pEditor = new KexiDateTableEdit(val, viewport(), "inPlaceEd");
			kdDebug(44021) << "date editor created..." << endl;
			break;
#endif
	else {//default editor
#if 0 //todo(js)
			d->pEditor = new KexiInputTableEdit(val, columnType(col), addText, false, viewport(), "inPlaceEd",
			 d->pColumnDefaults.at(col)->toStringList());
#else
			d->pEditor = new KexiInputTableEdit(val, t, addText, false, viewport(), "inPlaceEd",
			 QStringList());
#endif
			static_cast<KexiInputTableEdit*>(d->pEditor)->end(false);
//			if(backspace)
//				static_cast<KexiInputTableEdit*>(d->pEditor)->backspace();

	}

#ifdef Q_WS_WIN
//TODO
	if (columnType(col)==QVariant::Date) {
		moveChild(d->pEditor, columnPos(d->curCol), rowPos(d->curRow)-1);
		d->pEditor->resize(columnWidth(d->curCol)-1, rowHeight()+1);
	}
	else {
		moveChild(d->pEditor, columnPos(d->curCol), rowPos(d->curRow)-1);
		d->pEditor->resize(columnWidth(d->curCol)-1, rowHeight());
	}
#else
//	if (columnType(col)==QVariant::String) {
		//moveChild(d->pEditor, columnPos(d->curCol)+5, rowPos(d->curRow));
		//d->pEditor->resize(columnWidth(d->curCol)-1, rowHeight(d->curRow)-1);
	//}
	if (columnType(col)==QVariant::Date) {
		moveChild(d->pEditor, columnPos(d->curCol)-0, rowPos(d->curRow)-2);
		d->pEditor->resize(columnWidth(d->curCol)+0, rowHeight()+3);
	}
	else {
		moveChild(d->pEditor, columnPos(d->curCol), rowPos(d->curRow));
		d->pEditor->resize(columnWidth(d->curCol)-2, rowHeight()-1);
	}
#endif
//	moveChild(d->pEditor, columnPos(d->curCol), rowPos(d->curRow));
	d->pEditor->show();
	d->pEditor->setFocus();
}

void KexiTableView::focusInEvent(QFocusEvent*)
{
	updateCell(d->curRow, d->curCol);
}


void KexiTableView::focusOutEvent(QFocusEvent*)
{
	d->scrollBarTipTimer.stop();
	d->scrollBarTip->hide();
	
	updateCell(d->curRow, d->curCol);
}

bool KexiTableView::focusNextPrevChild(bool next)
{
	if (d->pEditor)
		return true;
	return QScrollView::focusNextPrevChild(next);
}

void KexiTableView::resizeEvent(QResizeEvent *e)
{
	QScrollView::resizeEvent(e);
	updateGeometries();
	
	if (d->navPanel) {
		QRect g = d->navPanel->geometry();
		kdDebug(44021) << "**********"<< g.top() << " " << g.left() <<endl;
		d->navPanel->setGeometry(
			frameWidth(),
			height() - horizontalScrollBar()->sizeHint().height()-frameWidth(),
			d->navPanel->sizeHint().width(), // - verticalScrollBar()->sizeHint().width() - horizontalScrollBar()->sizeHint().width(),
			horizontalScrollBar()->sizeHint().height()
		);
//		updateContents();
//		d->navPanel->setGeometry(1,horizontalScrollBar()->pos().y(),
	//		d->navPanel->width(), horizontalScrollBar()->height());
	}
//	updateContents(0,0,1000,1000);//js
//	erase(); repaint();
}

void KexiTableView::viewportResizeEvent( QResizeEvent *e )
{
    QScrollView::viewportResizeEvent( e );
    updateGeometries();
//	erase(); repaint();
}

void KexiTableView::showEvent(QShowEvent *e)
{
	QScrollView::showEvent(e);
	QSize s(tableSize());

//	QRect r(cellGeometry(rows() - 1 + (isInsertingEnabled()?1:0), cols() - 1 ));
//	resizeContents(r.right() + 1, r.bottom() + 1);
	resizeContents(s.width(),s.height());
	updateGeometries();
}


void KexiTableView::contentsDragMoveEvent(QDragMoveEvent *e)
{
	for(QStringList::Iterator it = d->dropFilters.begin(); it != d->dropFilters.end(); it++)
	{
		if(e->provides((*it).latin1()))
		{
			e->acceptAction(true);
			return;
		}
	}
	e->acceptAction(false);
}

void KexiTableView::contentsDropEvent(QDropEvent *ev)
{
	emit dropped(ev);
}

void KexiTableView::updateCell(int row, int col)
{
	kdDebug(44021) << "updateCell("<<row<<", "<<col<<")"<<endl;

//	if(!d->recordIndicator)
//	{
		updateContents(cellGeometry(row, col));
	/*}
	else
	{
		for(int i = 0; i < (int)m_data->columnsCount(); i++)
		{
			updateContents(cellGeometry(row, i));
		}
	}*/

}

void KexiTableView::columnWidthChanged( int, int, int )
{
	QSize s(tableSize());
	int w = contentsWidth();
	viewport()->setUpdatesEnabled(false);
	resizeContents( s.width(), s.height() );
	viewport()->setUpdatesEnabled(true);
	if (contentsWidth() < w)
		updateContents(contentsX(), 0, viewport()->width(), contentsHeight());
//		repaintContents( s.width(), 0, w - s.width() + 1, contentsHeight(), TRUE );
	else
	//	updateContents( columnPos(col), 0, contentsWidth(), contentsHeight() );
		updateContents(contentsX(), 0, viewport()->width(), contentsHeight());
	//	viewport()->repaint();

//	updateContents(0, 0, d->pBufferPm->width(), d->pBufferPm->height());
	if (d->pEditor)
	{
		d->pEditor->resize(columnWidth(d->curCol)-1, rowHeight()-1);
		moveChild(d->pEditor, columnPos(d->curCol), rowPos(d->curRow));
	}
	updateGeometries();
}

void KexiTableView::updateGeometries()
{
	QSize ts = tableSize();
	if (d->pTopHeader->offset() && ts.width() < (d->pTopHeader->offset() + d->pTopHeader->width()))
		horizontalScrollBar()->setValue(ts.width() - d->pTopHeader->width());

//	d->pVerticalHeader->setGeometry(1, topMargin() + 1, leftMargin(), visibleHeight());
	d->pTopHeader->setGeometry(leftMargin() + 1, 1, visibleWidth(), topMargin());
	d->pVerticalHeader->setGeometry(1, topMargin() + 1, leftMargin(), visibleHeight());
}

int KexiTableView::columnWidth(int col) const
{
    return d->pTopHeader->sectionSize(col);
}

int KexiTableView::rowHeight() const
{
	return d->rowHeight;
}

int KexiTableView::columnPos(int col) const
{
    return d->pTopHeader->sectionPos(col);
}

int KexiTableView::rowPos(int row) const
{
	return d->rowHeight*row;
}

int KexiTableView::columnAt(int pos) const
{
    return d->pTopHeader->sectionAt(pos);
}

int KexiTableView::rowAt(int pos, bool ignoreEnd) const
{
	pos /=d->rowHeight;
	if (pos < 0)
		return 0;
	if ((pos >= (int)m_data->count()) && !ignoreEnd)
		return -1;
	return pos;
}

QRect KexiTableView::cellGeometry(int row, int col) const
{
    return QRect(columnPos(col), rowPos(row),
		  columnWidth(col), rowHeight());
}

QSize KexiTableView::tableSize() const
{
	if (rows() > 0 && cols() > 0)
		return QSize( 
			columnPos( cols() - 1 ) + columnWidth( cols() - 1 ),
			rowPos( rows()-1+(isInsertingEnabled()?1:0)) + d->rowHeight
			+horizontalScrollBar()->sizeHint().height() + margin() );
	return QSize(0,0);
}

int KexiTableView::rows() const
{
	return m_data->count();
}

int KexiTableView::cols() const
{
	return m_data->columns.count();
}

void KexiTableView::setCursor(int row, int col/*=-1*/)
{
	int newrow = row;
	int newcol = col;
	if(rows() <= 0)
	{
		d->curRow=0;
		d->pCurrentItem=0;
		d->pVerticalHeader->setCurrentRow(-1);
		return;
	}

//	int oldRow = d->curRow;
//	int oldCol = d->curCol;

	if(col>=0)
	{
		newcol = QMAX(0, col);
		newcol = QMIN(cols() - 1, newcol);
	}
	else {
		newcol = d->curCol; //no changes
	}
	newrow = QMAX( 0, row);
	newrow = QMIN( rows() - 1 + (isInsertingEnabled()?1:0), newrow);

//	d->pCurrentItem = itemAt(d->curRow);
//	kdDebug(44021) << "setCursor(): d->curRow=" << d->curRow << " oldRow=" << oldRow << " d->curCol=" << d->curCol << " oldCol=" << oldCol << endl;

	if ( d->curRow != newrow || d->curCol != newcol )
	{
		kdDebug(44021) << "setCursor(): " <<QString("old:%1,%2 new:%3,%4").arg(d->curCol).arg(d->curRow).arg(newcol).arg(newrow) << endl;
		
		if (d->curRow != newrow) {//update current row info
			d->navRowNumber->setText(QString::number(newrow+1));
			d->navBtnPrev->setEnabled(newrow>0);
			d->navBtnFirst->setEnabled(newrow>0);
			d->navBtnNext->setEnabled(newrow<(rows()-1+(isInsertingEnabled()?1:0)));
			d->navBtnLast->setEnabled(newrow!=(rows()-1));
		}
		
		// cursor moved: get rid of editor
		if (d->pEditor) {
			if (!d->contentsMousePressEvent_dblClick) {
				acceptEditor();
			}
		}

		// cursor moved to other row: end of row editing
		if (d->rowEditing && d->curRow != newrow) {
			acceptRowEdit();
		}

		//change position
		int oldRow = d->curRow;
		int oldCol = d->curCol;
		d->curRow = newrow;
		d->curCol = newcol;

//		int cw = columnWidth( d->curCol );
		int rh = rowHeight();
//		ensureVisible( columnPos( d->curCol ) + cw / 2, rowPos( d->curRow ) + rh / 2, cw / 2, rh / 2 );
//		center(columnPos(d->curCol) + cw / 2, rowPos(d->curRow) + rh / 2, cw / 2, rh / 2);
	kdDebug(44021) << " contentsY() = "<< contentsY() << endl;
		if (oldRow > d->curRow)
			ensureVisible(columnPos(d->curCol), rowPos(d->curRow) + rh, columnWidth(d->curCol), rh);
		else// if (oldRow <= d->curRow)
			ensureVisible(columnPos(d->curCol), rowPos(d->curRow), columnWidth(d->curCol), rh);

//		ensureVisible(columnPos(d->curCol), rowPos(d->curRow) - contentsY(), columnWidth(d->curCol), rh);
		d->pVerticalHeader->setCurrentRow(d->curRow);
		updateCell( oldRow, oldCol );
		updateCell( d->curRow, d->curCol );
		if (d->curCol != oldCol || d->curRow != oldRow ) //ensure this is also refreshed
			updateCell( oldRow, d->curCol );
		if (isInsertingEnabled() && d->curRow == rows()) {
			kdDebug(44021) << "NOW insert item is current" << endl;
			d->pCurrentItem = d->pInsertItem;
		}
		else {
			kdDebug(44021) << QString("NOW item at %1 (%2) is current").arg(d->curRow).arg((ulong)itemAt(d->curRow)) << endl;
			d->pCurrentItem = itemAt(d->curRow);
			emit itemSelected(d->pCurrentItem);
		}
	}
}

void KexiTableView::acceptEditor()
{
	if (!d->pEditor)
		return;
//	d->pCurrentItem->setText(d->curCol, d->pEditor->text());
	QVariant oldValue=d->pCurrentItem->at(d->curCol);
	(*d->pCurrentItem)[d->curCol] = d->pEditor->value();
	cancelEditor();
	emit itemChanged(d->pCurrentItem, d->curCol,oldValue);
	emit itemChanged(d->pCurrentItem, d->curCol);
}

void KexiTableView::cancelEditor()
{
	if (!d->pEditor)
		return;

	delete d->pEditor;
	d->pEditor = 0;
	viewport()->setFocus();
}

void KexiTableView::acceptRowEdit()
{
	if (!d->rowEditing)
		return;
	acceptEditor();
	d->rowEditing = false;
	//indicate on the vheader that we are not editing
	d->pVerticalHeader->setEditRow(-1);
//! \todo (js): accept changes for this row!
	kdDebug(44021) << "EDIT ROW ACCEPTED." << endl;
}

void KexiTableView::cancelRowEdit()
{
	if (!d->rowEditing)
		return;
	cancelEditor();
	d->rowEditing = false;
	//indicate on the vheader that we are not editing
	d->pVerticalHeader->setEditRow(-1);
	if (d->newRowEditing) {
		d->newRowEditing = false;
		//remove current edited row (it is @ the end of list)
		m_data->removeLast();
		//current item is now empty, last row
		d->pCurrentItem = d->pInsertItem;
		//update visibility
		d->pVerticalHeader->removeLabel(false); //-1 label
//		updateContents(columnPos(0), rowPos(rows()), 
//			viewport()->width(), d->rowHeight*3 + (d->navPanel ? d->navPanel->height() : 0)*3 );
		updateContents(); //js: above didnt work well so we do that dirty
//TODO: still dont repaint properly!!
		QSize s(tableSize());
		resizeContents(s.width(), s.height());
		d->pVerticalHeader->update();
		//--no cancel action is needed for datasource, 
		//  because the row was not yet stored.
	}
	
//! \todo (js): cancel changes for this row!
	kdDebug(44021) << "EDIT ROW CANCELLED." << endl;
}

void KexiTableView::setAdditionPolicy(AdditionPolicy policy)
{
	d->additionPolicy = policy;
//	updateContextMenu();
}

KexiTableView::AdditionPolicy KexiTableView::additionPolicy() const
{
	return d->additionPolicy;
}

void KexiTableView::setDeletionPolicy(DeletionPolicy policy)
{
	d->deletionPolicy = policy;
//	updateContextMenu();
}

KexiTableView::DeletionPolicy KexiTableView::deletionPolicy() const
{
	return d->deletionPolicy;
}

bool KexiTableView::updateContextMenu()
{
  // delete d->pContextMenu;
  //  d->pContextMenu = 0L;
//  d->pContextMenu->clear();
//	if(d->pCurrentItem && d->pCurrentItem->isInsertItem())
//    return;

//	if(d->additionPolicy != NoAdd || d->deletionPolicy != NoDelete)
//	{
//		d->pContextMenu = new QPopupMenu(this);
  d->pContextMenu->setItemVisible(d->menu_id_addRecord, d->additionPolicy != NoAdd);
#if 0 //todo(js)
  d->pContextMenu->setItemVisible(d->menu_id_removeRecord, d->deletionPolicy != NoDelete
    && d->pCurrentItem && !d->pCurrentItem->isInsertItem());
#else
  d->pContextMenu->setItemVisible(d->menu_id_removeRecord, d->deletionPolicy != NoDelete
    && d->pCurrentItem);
#endif
  for (int i=0; i<(int)d->pContextMenu->count(); i++) {
    if (d->pContextMenu->isItemVisible( d->pContextMenu->idAt(i) ))
      return true;
  }
  return false;
}

void KexiTableView::slotAutoScroll()
{
	kdDebug(44021) << "KexiTableView::slotAutoScroll()" <<endl;
	if(d->needAutoScroll)
	{
		switch(d->scrollDirection)
		{
			case ScrollDown:
				setCursor(d->curRow + 1, d->curCol);
				break;

			case ScrollUp:
				setCursor(d->curRow - 1, d->curCol);
				break;
			case ScrollLeft:
				setCursor(d->curRow, d->curCol - 1);
				break;

			case ScrollRight:
				setCursor(d->curRow, d->curCol + 1);
				break;
		}
	}
}

/*
void KexiTableView::inserted()
{
	d->pVerticalHeader->addLabel();
#if 0
	if(itemAt(d->numRows)->isInsertItem())
		d->pVerticalHeader->addLabel("*", d->rowHeight);
	else
		d->pVerticalHeader->addLabel("", d->rowHeight);
#endif
}*/

#ifndef KEXI_NO_PRINT
void
KexiTableView::print(KPrinter &/*printer*/)
{
//	printer.setFullPage(true);
#if 0
	int leftMargin = printer.margins().width() + 2 + d->rowHeight;
	int topMargin = printer.margins().height() + 2;
//	int bottomMargin = topMargin + ( printer.realPageSize()->height() * printer.resolution() + 36 ) / 72;
	int bottomMargin = 0;
	kdDebug(44021) << "KexiTableView::print: bottom = " << bottomMargin << endl;

	QPainter p(&printer);

	KexiTableItem *i;
	int width = leftMargin;
	for(int col=0; col < cols(); col++)
	{
		p.fillRect(width, topMargin - d->rowHeight, columnWidth(col), d->rowHeight, QBrush(gray));
		p.drawRect(width, topMargin - d->rowHeight, columnWidth(col), d->rowHeight);
		p.drawText(width, topMargin - d->rowHeight, columnWidth(col), d->rowHeight, AlignLeft | AlignVCenter, d->pTopHeader->label(col));
		width = width + columnWidth(col);
	}

	int yOffset = topMargin;
	int row = 0;
	int right = 0;
	for(i = m_data->first(); i; i = m_data->next())
	{
		if(!i->isInsertItem())
		{	kdDebug(44021) << "KexiTableView::print: row = " << row << " y = " << yOffset << endl;
			int xOffset = leftMargin;
			for(int col=0; col < cols(); col++)
			{
				kdDebug(44021) << "KexiTableView::print: col = " << col << " x = " << xOffset << endl;
				p.saveWorldMatrix();
				p.translate(xOffset, yOffset);
				paintCell(&p, i, col, QRect(0, 0, columnWidth(col) + 1, d->rowHeight), true);
				p.restoreWorldMatrix();
//			p.drawRect(xOffset, yOffset, columnWidth(col), d->rowHeight);
				xOffset = xOffset + columnWidth(col);
				right = xOffset;
			}

			row++;
			yOffset = topMargin  + row * d->rowHeight;
		}

		if(yOffset > 900)
		{
			p.drawLine(leftMargin, topMargin, leftMargin, yOffset);
			p.drawLine(leftMargin, topMargin, right - 1, topMargin);
			printer.newPage();
			yOffset = topMargin;
			row = 0;
		}
	}
	p.drawLine(leftMargin, topMargin, leftMargin, yOffset);
	p.drawLine(leftMargin, topMargin, right - 1, topMargin);

//	p.drawLine(60,60,120,150);
	p.end();
#endif
}
#endif

/*void
KexiTableView::takeInsertItem()
{
//	delete d->pInsertItem;
	d->pInsertItem = 0;
}
*
KexiTableRM* KexiTableView::recordMarker() const
{
	return d->pVerticalHeader;
}*/

KexiTableRM* KexiTableView::verticalHeader() const
{
	return d->pVerticalHeader; 
}

QString KexiTableView::columnCaption(int colNum) const
{
	return d->pTopHeader->label(colNum);
}

int KexiTableView::currentColumn() const
{ 
	return d->curCol;
}

int KexiTableView::currentRow() const
{
	return d->curRow;
}

KexiTableItem *KexiTableView::selectedItem() const
{
	return d->pCurrentItem;
}

void KexiTableView::setBackgroundAltering(bool altering) { d->bgAltering = altering; }
bool KexiTableView::backgroundAltering()  const { return d->bgAltering; }

//void KexiTableView::setRecordIndicator(bool indicator) { d->recordIndicator = indicator; }
//bool KexiTableView::recordIndicator() const { return d->recordIndicator; }

void KexiTableView::setEditableOnDoubleClick(bool set) { d->editOnDoubleClick = set; }
bool KexiTableView::editableOnDoubleClick() const { return d->editOnDoubleClick; }

void KexiTableView::setEmptyAreaColor(QColor c) { d->emptyAreaColor = c; }
QColor KexiTableView::emptyAreaColor() const { return d->emptyAreaColor; }

void KexiTableView::triggerUpdate()
{
	kdDebug(44021) << "KexiTableView::triggerUpdate()" << endl;
//	if (!d->pUpdateTimer->isActive())
		d->pUpdateTimer->start(200, true);
}

int KexiTableView::columnType(int col) const
{
	return m_data->columns[col].type;
}

bool KexiTableView::columnEditable(int col) const
{
	return !m_data->columns[col].readOnly;
}

QVariant KexiTableView::columnDefaultValue(int col) const
{
	return QVariant(0);
//TODO(js)	
//	return m_data->columns[col].defaultValue;
}

void KexiTableView::setReadOnly(bool set)
{
	if (m_data->isReadOnly() && !set)
		return; //not allowed!
	d->readOnly = (set ? 1 : 0);
}

bool KexiTableView::isReadOnly() const
{
	if (d->readOnly == 1 || d->readOnly == 0)
		return (bool)d->readOnly;
	return m_data->isReadOnly();
}

void KexiTableView::setInsertingEnabled(bool set)
{
	if (!m_data->isInsertingEnabled() && set)
		return; //not allowed!
	d->insertingEnabled = (set ? 1 : 0);
	d->navBtnNext->setEnabled(set);
}

bool KexiTableView::isInsertingEnabled() const
{
	if (d->insertingEnabled == 1 || d->insertingEnabled == 0)
		return (bool)d->insertingEnabled;
	return m_data ? m_data->isInsertingEnabled() : true;
}

bool KexiTableView::rowEditing() const
{
	return d->rowEditing;
}

bool KexiTableView::navigationPanelEnabled() const
{
	return d->navigationPanelEnabled;
}
	
void KexiTableView::setNavigationPanelEnabled(bool set)
{
	if (d->navigationPanelEnabled==set)
		return;
	d->navigationPanelEnabled = set;
}

void KexiTableView::setHBarGeometry( QScrollBar & hbar, int x, int y, int w, int h )
{
/*todo*/
	kdDebug(44021)<<"KexiTableView::setHBarGeometry"<<endl;
	if (d->navPanel && d->navigationPanelEnabled) {
    	hbar.setGeometry( x + d->navPanel->width(), y, w - d->navPanel->width(), h );
	}
}

int KexiTableView::validRowNumber(const QString& text)
{
	bool ok=true;
	int r = text.toInt(&ok);
	if (!ok || r<1)
		r = 1;
	else if (r > (rows()+(isInsertingEnabled()?1:0)))
		r = rows()+(isInsertingEnabled()?1:0);
	return r-1;
}

//! for navigator
void KexiTableView::navRowNumber_ReturnPressed(const QString& text)
{
	setFocus();
	selectRow( validRowNumber(text) );
	d->skipKeyPress=true;
}

void KexiTableView::navRowNumber_lostFocus()
{
	int r = validRowNumber(d->navRowNumber->text());
	d->navRowNumber->setText( QString::number( r+1 ) );
	selectRow( r );
}

void KexiTableView::updateRowCountInfo()
{
	d->navRowNumberValidator->setRange(1,rows()+(isInsertingEnabled()?1:0));
	d->navRowCount->setText(QString::number(rows()));
}

void KexiTableView::navBtnLastClicked()
{
	setFocus();
	selectRow(rows()>0 ? (rows()-1) : 0);
}

void KexiTableView::navBtnPrevClicked()
{
	setFocus();
	selectPrevRow();
}

void KexiTableView::navBtnNextClicked()
{
	setFocus();
	selectNextRow();
}

void KexiTableView::navBtnFirstClicked()
{
	setFocus();
	selectFirstRow();
}

void KexiTableView::navBtnNewClicked()
{
	if (!isInsertingEnabled())
		return;
	setFocus();
	selectRow(rows());
	startEditCurrentCell();
}

bool KexiTableView::eventFilter( QObject *o, QEvent *e )
{
	//UNUSED
	return QScrollView::eventFilter(o,e);
}

void KexiTableView::vScrollBarValueChanged(int v)
{
	kdDebug(44021) << "VCHANGED: " << v << " / " << horizontalScrollBar()->maxValue() <<  endl;
	
//	updateContents();
//	triggerUpdate();
	
	d->pVerticalHeader->update(); //<-- dirty but needed
	
	QRect r = verticalScrollBar()->sliderRect();
	kdDebug(44021) << r.x() << " " << r.y()  << endl;
	int row = rowAt(contentsY())+1;
	if (row<=0) {
		d->scrollBarTipTimer.stop();
		d->scrollBarTip->hide();
		return;
	}
	d->scrollBarTip->setText( i18n("Row: ") + QString::number(row) );
	d->scrollBarTip->adjustSize();
	d->scrollBarTip->move( 
	 mapToGlobal( r.topLeft() + verticalScrollBar()->pos() ) + QPoint( - d->scrollBarTip->width()-5, r.height()/2 - d->scrollBarTip->height()/2) );
	if (verticalScrollBar()->draggingSlider()) {
		kdDebug(44021) << "  draggingSlider()  " << endl;
		d->scrollBarTipTimer.stop();
		d->scrollBarTip->show();
		d->scrollBarTip->raise();
	}
	else {
		d->scrollBarTipTimerCnt++;
		if (d->scrollBarTipTimerCnt>4) {
			d->scrollBarTipTimerCnt=0;
			d->scrollBarTip->show();
			d->scrollBarTip->raise();
			d->scrollBarTipTimer.start(500, true);
		}
	}
}

void KexiTableView::vScrollBarSliderReleased()
{
	kdDebug(44021) << "vScrollBarSliderReleased()" << endl;
	d->scrollBarTip->hide();
}

void KexiTableView::scrollBarTipTimeout()
{
	if (d->scrollBarTip->isVisible()) {
		kdDebug(44021) << "TIMEOUT! - hide" << endl;
		if (d->scrollBarTipTimerCnt>0) {
			d->scrollBarTipTimerCnt=0;
			d->scrollBarTipTimer.start(500, true);
			return;
		}
		d->scrollBarTip->hide();
	}
	d->scrollBarTipTimerCnt=0;
}

#include "kexitableview.moc"
