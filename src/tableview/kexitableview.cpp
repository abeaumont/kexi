/* This file is part of the KDE project
   Copyright (C) 2002 Till Busch <till@bux.at>
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003 Daniel Molkentin <molkentin@kde.org>
   Copyright (C) 2003 Joseph Wenninger <jowenn@kde.org>
   Copyright (C) 2003-2004 Jaroslaw Staniek <js@iidea.pl>

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
#include <kapplication.h>
#include <kiconloader.h>
#include <kmessagebox.h>

#ifndef KEXI_NO_PRINT
# include <kprinter.h>
#endif

#include "kexitableview.h"
#include "kexitablerm.h"
#include "kexi_utils.h"
#include "kexivalidator.h"

#include "kexidatetableedit.h"
#include "kexitimetableedit.h"
#include "kexidatetimetableedit.h"
#include "kexicelleditorfactory.h"
#include "kexitableedit.h"
#include "kexiinputtableedit.h"
#include "kexicomboboxtableedit.h"
#include "kexiblobtableedit.h"
#include "kexibooltableedit.h"

#include "kexitableview_p.h"

KexiTableView::Appearance::Appearance(QWidget *widget)
 : alternateBackgroundColor( KGlobalSettings::alternateBackgroundColor() )
{
	//set defaults
	if (qApp) {
		QPalette p = widget ? widget->palette() : qApp->palette();
		baseColor = p.active().base();
		textColor = p.active().text();
		borderColor = QColor(200,200,200);
		emptyAreaColor = p.active().color(QColorGroup::Base);
		rowHighlightingColor = QColor(
			(alternateBackgroundColor.red()+baseColor.red())/2,
			(alternateBackgroundColor.green()+baseColor.green())/2,
			(alternateBackgroundColor.blue()+baseColor.blue())/2);
		rowHighlightingTextColor = textColor;
	}
	backgroundAltering = true;
	rowHighlightingEnabled = false;
	navigatorEnabled = true;
	fullRowSelection = false;
}


//-----------------------------------------

TableViewHeader::TableViewHeader(QWidget * parent, const char * name) 
	: QHeader(parent, name)
	, m_lastToolTipSection(-1)
{
	installEventFilter(this);
	connect(this, SIGNAL(sizeChange(int,int,int)), 
		this, SLOT(slotSizeChange(int,int,int)));
}

int TableViewHeader::addLabel ( const QString & s, int size )
{
	m_toolTips += "";
	slotSizeChange(0,0,0);//refresh
	return QHeader::addLabel(s, size);
}

int TableViewHeader::addLabel ( const QIconSet & iconset, const QString & s, int size )
{
	m_toolTips += "";
	slotSizeChange(0,0,0);//refresh
	return QHeader::addLabel(iconset, s, size);
}

void TableViewHeader::removeLabel( int section )
{
	if (section < 0 || section >= count())
		return;
	QStringList::Iterator it = m_toolTips.begin();
	it += section;
	m_toolTips.remove(it);
	slotSizeChange(0,0,0);//refresh
	QHeader::removeLabel(section);
}

void TableViewHeader::setToolTip( int section, const QString & toolTip )
{
	if (section < 0 || section >= (int)m_toolTips.count())
		return;
	m_toolTips[ section ] = toolTip;
}

bool TableViewHeader::eventFilter(QObject * watched, QEvent * e)
{
	if (e->type()==QEvent::MouseMove) {
		const int section = sectionAt( static_cast<QMouseEvent*>(e)->x() );
		if (section != m_lastToolTipSection && section >= 0 && section < (int)m_toolTips.count()) {
			QToolTip::remove(this, m_toolTipRect);
			QString tip = m_toolTips[ section ];
			if (tip.isEmpty()) { //try label
				QFontMetrics fm(font());
				int minWidth = fm.width( label( section ) ) + style().pixelMetric( QStyle::PM_HeaderMargin );
				QIconSet *iset = iconSet( section );
				if (iset)
					minWidth += (2+iset->pixmap( QIconSet::Small, QIconSet::Normal ).width()); //taken from QHeader::sectionSizeHint()
				if (minWidth > sectionSize( section ))
					tip = label( section );
			}
			if (tip.isEmpty()) {
				m_lastToolTipSection = -1;
			}
			else {
				QToolTip::add(this, m_toolTipRect = sectionRect(section), tip);
				m_lastToolTipSection = section;
			}
		}
	}
//			if (e->type()==QEvent::MouseButtonPress) {
//	todo
//			}
	return QHeader::eventFilter(watched, e);
}

void TableViewHeader::slotSizeChange(int /*section*/, int /*oldSize*/, int /*newSize*/ )
{
	if (m_lastToolTipSection>0)
		QToolTip::remove(this, m_toolTipRect);
	m_lastToolTipSection = -1; //tooltip's rect is now invalid
}


//-----------------------------------------

//sanity check
#define CHECK_DATA(r) \
	if (!m_data) { kdWarning() << "KexiTableView: No data assigned!" << endl; return r; }
#define CHECK_DATA_ \
	if (!m_data) { kdWarning() << "KexiTableView: No data assigned!" << endl; return; }

bool KexiTableView_cellEditorFactoriesInitialized = false;

// Initializes standard editor cell editor factories
void KexiTableView::initCellEditorFactories()
{
	if (KexiTableView_cellEditorFactoriesInitialized)
		return;
	KexiCellEditorFactoryItem* item;
	item = new KexiBlobEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::BLOB );

	item = new KexiDateEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::Date );

	item = new KexiTimeEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::Time );

	item = new KexiDateTimeEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::DateTime );

	item = new KexiComboBoxEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::Enum );

	item = new KexiBoolEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::Boolean );

	item = new KexiKIconTableEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::Text, "KIcon" );

	//default type
	item = new KexiInputEditorFactoryItem();
	KexiCellEditorFactory::registerItem( *item, KexiDB::Field::InvalidType );

	KexiTableView_cellEditorFactoriesInitialized = true;
}



KexiTableView::KexiTableView(KexiTableViewData* data, QWidget* parent, const char* name)
:QScrollView(parent, name, /*Qt::WRepaintNoErase | */Qt::WStaticContents /*| Qt::WResizeNoErase*/)
{
	KexiTableView::initCellEditorFactories();

	d = new KexiTableViewPrivate(this);

	m_data = new KexiTableViewData(); //to prevent crash because m_data==0
	m_owner = true;                   //-this will be deleted if needed

	setResizePolicy(Manual);
	viewport()->setBackgroundMode(NoBackground);
//	viewport()->setFocusPolicy(StrongFocus);
	viewport()->setFocusPolicy(WheelFocus);
	setFocusPolicy(WheelFocus); //<--- !!!!! important (was NoFocus), 
	//                             otherwise QApplication::setActiveWindow() won't activate 
	//                             this widget when needed!
//	setFocusProxy(viewport());
	viewport()->installEventFilter(this);

	//setup colors defaults
	setBackgroundMode(PaletteBackground);
//	setEmptyAreaColor(d->appearance.baseColor);//palette().active().color(QColorGroup::Base));

//	d->baseColor = colorGroup().base();
//	d->textColor = colorGroup().text();

//	d->altColor = KGlobalSettings::alternateBackgroundColor();
//	d->grayColor = QColor(200,200,200);
	d->diagonalGrayPattern = QBrush(d->appearance.borderColor, BDiagPattern);

	setLineWidth(1);
	horizontalScrollBar()->installEventFilter(this);
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
	d->pContextMenu = new KPopupMenu(this, "contextMenu");
#if 0 //moved to mainwindow's actions
	d->menu_id_addRecord = d->pContextMenu->insertItem(i18n("Add Record"), this, SLOT(addRecord()), CTRL+Key_Insert);
	d->menu_id_removeRecord = d->pContextMenu->insertItem(
		kapp->iconLoader()->loadIcon("button_cancel", KIcon::Small),
		i18n("Remove Record"), this, SLOT(removeRecord()), CTRL+Key_Delete);
#endif

#ifdef Q_WS_WIN
	d->rowHeight = fontMetrics().lineSpacing() + 4;
#else
	d->rowHeight = fontMetrics().lineSpacing() + 1;
#endif

	if(d->rowHeight < 17)
		d->rowHeight = 17;

	d->pUpdateTimer = new QTimer(this);

//	setMargins(14, fontMetrics().height() + 4, 0, 0);

	// Create headers
	d->pTopHeader = new TableViewHeader(this, "topHeader");
	d->pTopHeader->setOrientation(Horizontal);
	d->pTopHeader->setTracking(false);
	d->pTopHeader->setMovingEnabled(false);
	connect(d->pTopHeader, SIGNAL(sizeChange(int,int,int)), this, SLOT(slotTopHeaderSizeChange(int,int,int)));

	d->pVerticalHeader = new KexiTableRM(this);
	d->pVerticalHeader->setCellHeight(d->rowHeight);
//	d->pVerticalHeader->setFixedWidth(d->rowHeight);
	d->pVerticalHeader->setCurrentRow(-1);

	setMargins(
		QMIN(d->pTopHeader->sizeHint().height(), d->rowHeight),
		d->pTopHeader->sizeHint().height(), 0, 0);

	setupNavigator();

//	setMinimumHeight(horizontalScrollBar()->height() + d->rowHeight + topMargin());

//	navPanelLyr->addStretch(25);
//	enableClipper(true);

	if (data)
		setData( data );

#if 0//(js) doesn't work!
	d->scrollTimer = new QTimer(this);
	connect(d->scrollTimer, SIGNAL(timeout()), this, SLOT(slotAutoScroll()));
#endif

//	setBackgroundAltering(true);
//	setFullRowSelectionEnabled(false);

	setAcceptDrops(true);
	viewport()->setAcceptDrops(true);

	// Connect header, table and scrollbars
	connect(horizontalScrollBar(), SIGNAL(valueChanged(int)), d->pTopHeader, SLOT(setOffset(int)));
	connect(verticalScrollBar(), SIGNAL(valueChanged(int)),	d->pVerticalHeader, SLOT(setOffset(int)));
	connect(d->pTopHeader, SIGNAL(sizeChange(int, int, int)), this, SLOT(slotColumnWidthChanged(int, int, int)));
	connect(d->pTopHeader, SIGNAL(sectionHandleDoubleClicked(int)), this, SLOT(slotSectionHandleDoubleClicked(int)));
	connect(d->pTopHeader, SIGNAL(clicked(int)), this, SLOT(sortColumnInternal(int)));

	connect(d->pUpdateTimer, SIGNAL(timeout()), this, SLOT(slotUpdate()));
	
//	horizontalScrollBar()->show();
	updateScrollBars();
//	resize(sizeHint());
//	updateContents();
//	setMinimumHeight(horizontalScrollBar()->height() + d->rowHeight + topMargin());

//TMP
//setVerticalHeaderVisible(false);
//setHorizontalHeaderVisible(false);

//will be updated by setAppearance:	updateFonts();
	setAppearance(d->appearance); //refresh
}

KexiTableView::~KexiTableView()
{
	cancelRowEdit();

	if (m_owner)
		delete m_data;
	m_data = 0;
	delete d;
}

/*void KexiTableView::initActions(KActionCollection *ac)
{
	emit reloadActions(ac);
}*/

//! Setup navigator widget
void KexiTableView::setupNavigator()
{
	updateScrollBars();
	
	d->navPanel = new QFrame(this, "navPanel");
	d->navPanel->setFrameStyle(QFrame::NoFrame);//Panel|QFrame::Raised);
	d->navPanel->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Preferred);
	QHBoxLayout *navPanelLyr = new QHBoxLayout(d->navPanel,0,0,"nav_lyr");
//	navPanelLyr->setAutoAdd(true);

	navPanelLyr->addWidget( new QLabel(QString(" ")+i18n("Row:")+" ",d->navPanel) );
		
	int bw = 6+SmallIcon("navigator_first").width(); //QMIN( horizontalScrollBar()->height(), 20);
	QFont f = d->navPanel->font();
	f.setPixelSize((bw > 12) ? 12 : bw);
	QFontMetrics fm(f);
	d->nav1DigitWidth = fm.width("8");

	navPanelLyr->addWidget( d->navBtnFirst = new QToolButton(d->navPanel) );
	d->navBtnFirst->setFixedWidth(bw);
	d->navBtnFirst->setFocusPolicy(NoFocus);
	d->navBtnFirst->setIconSet( SmallIconSet("navigator_first") );
	QToolTip::add(d->navBtnFirst, i18n("First row"));
	
	navPanelLyr->addWidget( d->navBtnPrev = new QToolButton(d->navPanel) );
	d->navBtnPrev->setFixedWidth(bw);
	d->navBtnPrev->setFocusPolicy(NoFocus);
	d->navBtnPrev->setIconSet( SmallIconSet("navigator_prev") );
	QToolTip::add(d->navBtnPrev, i18n("Previous row"));
	
//	QWidget *spc = new QFrame(d->navPanel);
//	spc->setFixedWidth(6);
	navPanelLyr->addSpacing( 6 );
	
	navPanelLyr->addWidget( d->navRowNumber = new KLineEdit(d->navPanel) );
	d->navRowNumber->setAlignment(AlignRight | AlignVCenter);
	d->navRowNumber->setFocusPolicy(ClickFocus);
//	d->navRowNumber->setFixedWidth(fw);
	d->navRowNumberValidator = new QIntValidator(1, 1, this);
	d->navRowNumber->setValidator(d->navRowNumberValidator);
	d->navRowNumber->installEventFilter(this);
	QToolTip::add(d->navRowNumber, i18n("Current row number"));
	
	KLineEdit *lbl_of = new KLineEdit(i18n("of"), d->navPanel);
	lbl_of->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
	lbl_of->setMaximumWidth(fm.width(lbl_of->text())+8);
	lbl_of->setReadOnly(true);
	lbl_of->setLineWidth(0);
	lbl_of->setFocusPolicy(NoFocus);
	lbl_of->setAlignment(AlignCenter);
	navPanelLyr->addWidget( lbl_of );
	
	navPanelLyr->addWidget( d->navRowCount = new KLineEdit(d->navPanel) );
//	d->navRowCount->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
	d->navRowCount->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Preferred);
//	d->navRowCount->setMaximumWidth(fw);
	d->navRowCount->setReadOnly(true);
	d->navRowCount->setLineWidth(0);
	d->navRowCount->setFocusPolicy(NoFocus);
	d->navRowCount->setAlignment(AlignLeft | AlignVCenter);

	lbl_of->setFont(f);
	d->navRowNumber->setFont(f);
	d->navRowCount->setFont(f);
	d->navPanel->setFont(f);

	navPanelLyr->addWidget( d->navBtnNext = new QToolButton(d->navPanel) );
	d->navBtnNext->setFixedWidth(bw);
	d->navBtnNext->setFocusPolicy(NoFocus);
	d->navBtnNext->setIconSet( SmallIconSet("navigator_next") );
	QToolTip::add(d->navBtnNext, i18n("Next row"));
	
	navPanelLyr->addWidget( d->navBtnLast = new QToolButton(d->navPanel) );
	d->navBtnLast->setFixedWidth(bw);
	d->navBtnLast->setFocusPolicy(NoFocus);
	d->navBtnLast->setIconSet( SmallIconSet("navigator_last") );
	QToolTip::add(d->navBtnLast, i18n("Last row"));
	
//	spc = new QWidget(d->navPanel);
//	spc->setFixedWidth(6);
	navPanelLyr->addSpacing( 6 );
	navPanelLyr->addWidget( d->navBtnNew = new QToolButton(d->navPanel) );
	d->navBtnNew->setFixedWidth(bw);
	d->navBtnNew->setFocusPolicy(NoFocus);
	d->navBtnNew->setIconSet( SmallIconSet("navigator_new") );
	QToolTip::add(d->navBtnNew, i18n("New row"));
	d->navBtnNext->setEnabled(isInsertingEnabled());
	
//	spc = new QFrame(d->navPanel);
//	spc->setFixedWidth(6);
	navPanelLyr->addSpacing( 6 );

	navPanelLyr->addStretch(10);

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

	d->navPanel->updateGeometry();
}

void KexiTableView::setNavRowNumber(int newrow)
{
	QString n;
	if (newrow>=0)
		n = QString::number(newrow+1);
	else
		n = " ";
	if (d->navRowNumber->text().length() != n.length()) {//resize
		d->navRowNumber->setFixedWidth(
			d->nav1DigitWidth*QMAX( QMAX(n.length(),2)+1,d->navRowCount->text().length()+1)+6 
		);
	}
	d->navRowNumber->setText(n);
	d->navRowCount->deselect();
	d->navBtnPrev->setEnabled(newrow>0);
	d->navBtnFirst->setEnabled(newrow>0);
	d->navBtnNext->setEnabled(newrow<(rows()-1+(isInsertingEnabled()?1:0)));
	d->navBtnLast->setEnabled(newrow!=(rows()-1) && (isInsertingEnabled() || rows()>0));
}

void KexiTableView::setNavRowCount(int newrows)
{
	const QString & n = QString::number(newrows);
	if (d->navRowCount->text().length() != n.length()) {//resize
		d->navRowCount->setFixedWidth(d->nav1DigitWidth*n.length()+6);
		
		if (horizontalScrollBar()->isVisible()) {
			//+width of the delta
			d->navPanel->resize(
				d->navPanel->width()+(n.length()-d->navRowCount->text().length())*d->nav1DigitWidth,
				d->navPanel->height());
//			horizontalScrollBar()->move(d->navPanel->x()+d->navPanel->width()+20,horizontalScrollBar()->y());
		}
	}

	d->navRowCount->setText(n);
	d->navRowCount->deselect();

//	if (horizontalScrollBar()->isVisible()) {
//	}
//	updateNavPanelGeometry();
	updateScrollBars();
}

void KexiTableView::setData( KexiTableViewData *data, bool owner )
{
	const bool theSameData = m_data && m_data==data;
	if (m_owner && m_data && m_data!=data/*don't destroy if it's the same*/) {
		kdDebug(44021) << "KexiTableView::setData(): destroying old data (owned)" << endl;
		delete m_data; //destroy old data
		m_data = 0;
	}
	m_owner = owner;
	if(!data) {
		m_data = new KexiTableViewData();
		m_owner = true;
	}
	else {
		m_data = data;
		m_owner = owner;
		kdDebug(44021) << "KexiTableView::setData(): using shared data" << endl;
		//add columns
//		d->pTopHeader->setUpdatesEnabled(false);
		while(d->pTopHeader->count()>0)
			d->pTopHeader->removeLabel(0);

		{
			int i=0;
			for (KexiTableViewColumn::ListIterator it(m_data->columns);
				it.current(); ++it, i++) 
			{
				KexiDB::Field *f = it.current()->field();
//				if (!it.current()->fieldinfo || it.current()->fieldinfo->visible) {
				if (it.current()->visible()) {
					int wid = f->width();
					if (wid==0)
						wid=KEXITV_DEFAULT_COLUMN_WIDTH;//default col width in pixels
//js: TODO - add col width configuration and storage
//					d->pTopHeader->addLabel(f->captionOrName(), wid);
					d->pTopHeader->addLabel(it.current()->captionAliasOrName(), wid);
					if (!f->description().isEmpty())
						d->pTopHeader->setToolTip( i, f->description() );
				}
			}
		}

//		d->pTopHeader->setUpdatesEnabled(true);
		//add rows
//		triggerUpdate();
		d->pVerticalHeader->clear();
		d->pVerticalHeader->addLabels(m_data->count());
		if (m_data->count()==0)
			setNavRowNumber(0);
	}
	
	if (!theSameData) {
//todo: store sorting?
		setSorting(-1);
		connect(m_data, SIGNAL(refreshRequested()), this, SLOT(slotRefreshRequested()));
		connect(m_data, SIGNAL(destroying()), this, SLOT(slotDataDestroying()));
		connect(m_data, SIGNAL(rowsDeleted( const QValueList<int> & )), 
			this, SLOT(slotRowsDeleted( const QValueList<int> & )));
		connect(m_data, SIGNAL(aboutToDeleteRow(KexiTableItem&,KexiDB::ResultInfo*,bool)),
			this, SLOT(slotAboutToDeleteRow(KexiTableItem&,KexiDB::ResultInfo*,bool)));
		connect(m_data, SIGNAL(rowDeleted()), this, SLOT(slotRowDeleted()));
		connect(m_data, SIGNAL(rowInserted(KexiTableItem*,bool)), 
			this, SLOT(slotRowInserted(KexiTableItem*,bool)));
		connect(m_data, SIGNAL(rowInserted(KexiTableItem*,uint,bool)), 
			this, SLOT(slotRowInserted(KexiTableItem*,uint,bool))); //not db-aware
		connect(m_data, SIGNAL(rowRepaintRequested(KexiTableItem&)), 
			this, SLOT(slotRowRepaintRequested(KexiTableItem&)));
	}

	if (!data) {
//		clearData();
		cancelRowEdit();
		m_data->clearInternal();
	}

	if (!d->pInsertItem) {//first setData() call - add 'insert' item
		d->pInsertItem = new KexiTableItem(m_data->columns.count());
	}
	else {//just reinit
		d->pInsertItem->init(m_data->columns.count());
	}

	//update gui mode
	d->navBtnNew->setEnabled(isInsertingEnabled());
	d->pVerticalHeader->showInsertRow(isInsertingEnabled());

	initDataContents();

	emit dataSet( m_data );

//	QSize s(tableSize());
//	resizeContents(s.width(),s.height());
}

void KexiTableView::initDataContents()
{
	QSize s(tableSize());
	resizeContents(s.width(),s.height());

	if (!d->cursorPositionSetExplicityBeforeShow) {
		//set current row:
		d->pCurrentItem = 0;
		int curRow = -1, curCol = -1;
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

		setCursor(curRow, curCol);
	}
	ensureVisible(d->curRow,d->curCol);
	updateRowCountInfo();
	updateContents();

	d->cursorPositionSetExplicityBeforeShow = false;

	emit dataRefreshed();
}

void KexiTableView::slotDataDestroying()
{
	m_data = 0;
}

void KexiTableView::slotRowsDeleted( const QValueList<int> &rows )
{
	viewport()->repaint();
	QSize s(tableSize());
	resizeContents(s.width(),s.height());
	setCursor(QMAX(0, (int)d->curRow - (int)rows.count()), -1, true);
}


/*void KexiTableView::addDropFilter(const QString &filter)
{
	d->dropFilters.append(filter);
	viewport()->setAcceptDrops(true);
}*/

void KexiTableView::setFont( const QFont &font )
{
	QScrollView::setFont(font);
	updateFonts(true);
}

void KexiTableView::updateFonts(bool repaint)
{
#ifdef Q_WS_WIN
	d->rowHeight = fontMetrics().lineSpacing() + 4;
#else
	d->rowHeight = fontMetrics().lineSpacing() + 1;
#endif
	if (d->appearance.fullRowSelection) {
		d->rowHeight -= 1;
	}
	if(d->rowHeight < 17)
		d->rowHeight = 17;
//	if(d->rowHeight < 22)
//		d->rowHeight = 22;
	setMargins(
		QMIN(d->pTopHeader->sizeHint().height(), d->rowHeight),
		d->pTopHeader->sizeHint().height(), 0, 0);
//	setMargins(14, d->rowHeight, 0, 0);
	d->pVerticalHeader->setCellHeight(d->rowHeight);

	QFont f = font();
	f.setItalic(true);
	d->autonumberFont = f;

	QFontMetrics fm(d->autonumberFont);
	d->autonumberTextWidth = fm.width(i18n("(autonumber)"));

	if (repaint)
		updateContents();
}

bool KexiTableView::beforeDeleteItem(KexiTableItem *)
{
	//always return
	return true;
}

bool KexiTableView::deleteItem(KexiTableItem *item)/*, bool moveCursor)*/
{
	if (!item || !beforeDeleteItem(item))
		return false;

	QString msg, desc;
//	bool current = (item == d->pCurrentItem);
	if (!m_data->deleteRow(*item, true /*repaint*/)) {
		//error
		if (m_data->result()->desc.isEmpty())
			KMessageBox::sorry(this, m_data->result()->msg);
		else
			KMessageBox::detailedSorry(this, m_data->result()->msg, m_data->result()->desc);
		return false;
	}
	else {
//setCursor() wil lset this!		if (current)
			//d->pCurrentItem = m_data->current();
	}

//	repaintAfterDelete();
	if (d->spreadSheetMode) { //append empty row for spreadsheet mode
			m_data->append(new KexiTableItem(m_data->columns.count()));
			d->pVerticalHeader->addLabels(1);
	}

	return true;
}
/*
void KexiTableView::repaintAfterDelete()
{
	int row = d->curRow;
	if (!isInsertingEnabled() && row>=rows())
		row--; //move up

	QSize s(tableSize());
	resizeContents(s.width(),s.height());

	setCursor(row, d->curCol, true);

	d->pVerticalHeader->removeLabel();
//		if(moveCursor)
//		selectPrev();
//		d->pUpdateTimer->start(1,true);
	//get last visible row
	int r = rowAt(clipper()->height());
	if (r==-1) {
		r = rows()+1+(isInsertingEnabled()?1:0);
	}
	//update all visible rows below 
	updateContents( contentsX(), rowPos(d->curRow), clipper()->width(), d->rowHeight*(r-d->curRow));

	//update navigator's data
	setNavRowCount(rows());
}*/

void KexiTableView::deleteCurrentRow()
{
	if (d->newRowEditing) {//we're editing fresh new row: just cancel this!
		cancelRowEdit();
		return;
	}

	if (!acceptRowEdit())
		return;

	if (!isDeleteEnabled() || !d->pCurrentItem || d->pCurrentItem == d->pInsertItem)
		return;
	switch (d->deletionPolicy) {
	case NoDelete:
		return;
	case ImmediateDelete:
		break;
	case AskDelete:
		if (KMessageBox::No == KMessageBox::questionYesNo(this, i18n("Do you want to delete selected row?"), 0, 
			KGuiItem(i18n("&Delete row")), KStdGuiItem::no(), "dontAskBeforeDeleteRow"/*config entry*/))
			return;
		break;
	case SignalDelete:
		emit itemDeleteRequest(d->pCurrentItem, d->curRow, d->curCol);
		emit currentItemDeleteRequest();
		return;
	default:
		return;
	}

	if (!deleteItem(d->pCurrentItem)) {//nothing
	}
}

void KexiTableView::slotAboutToDeleteRow(KexiTableItem& item, KexiDB::ResultInfo* /*result*/, bool repaint)
{
	if (repaint) {
		d->rowWillBeDeleted = m_data->findRef(&item);
	}
}

void KexiTableView::slotRowDeleted()
{
	if (d->rowWillBeDeleted >= 0) {
//		if (!isInsertingEnabled() && d->rowWillBeDeleted>=rows())
		if (d->rowWillBeDeleted > 0 && d->rowWillBeDeleted >= rows())//+(isInsertingEnabled()?1:0) ))
			d->rowWillBeDeleted--; //move up
		QSize s(tableSize());
		resizeContents(s.width(),s.height());

		setCursor(d->rowWillBeDeleted, d->curCol, true/*forceSet*/);

		d->pVerticalHeader->removeLabel();
		//get last visible row
		int r = rowAt(clipper()->height());
		if (r==-1) {
			r = rows()+1+(isInsertingEnabled()?1:0);
		}
		//update all visible rows below 
		updateContents( contentsX(), rowPos(d->curRow), clipper()->width(), d->rowHeight*(r - d->curRow));

		//update navigator's data
		setNavRowCount(rows());

		d->rowWillBeDeleted = -1;
	}
}

void KexiTableView::slotRowInserted(KexiTableItem *item, bool repaint)
{
	int row = m_data->findRef(item);
	slotRowInserted( item, row, repaint );
}

void KexiTableView::slotRowInserted(KexiTableItem * /*item*/, uint row, bool repaint)
{
	if (repaint && (int)row<rows()) {
		QSize s(tableSize());
		resizeContents(s.width(),s.height());

		//redraw only this row and below:
		int leftcol = d->pTopHeader->sectionAt( d->pTopHeader->offset() );
	//	updateContents( columnPos( leftcol ), rowPos(d->curRow), 
	//		clipper()->width(), clipper()->height() - (rowPos(d->curRow) - contentsY()) );
		updateContents( columnPos( leftcol ), rowPos(row), 
			clipper()->width(), clipper()->height() - (rowPos(row) - contentsY()) );

		if (!d->pVerticalHeaderAlreadyAdded)
			d->pVerticalHeader->addLabel();
		else //it was added because this inserting was interactive
			d->pVerticalHeaderAlreadyAdded = false;

		//update navigator's data
		setNavRowCount(rows());

		if (d->curRow >= (int)row) {
			//update
			editorShowFocus( d->curRow, d->curCol );
		}
	}
}

KexiTableItem *KexiTableView::insertEmptyRow(int row)
{
	if ( !acceptRowEdit() || !isEmptyRowInsertingEnabled() 
		|| (row!=-1 && row >= (rows()+isInsertingEnabled()?1:0) ) )
		return 0;

	KexiTableItem *newItem = new KexiTableItem(m_data->columns.count());
	insertItem(newItem, row);
	return newItem;
}

void KexiTableView::insertItem(KexiTableItem *newItem, int row)
{
	bool changeCurrent = (row==-1 || row==d->curRow);
	if (changeCurrent) {
		row = (d->curRow >= 0 ? d->curRow : 0);
		d->pCurrentItem = newItem;
		d->curRow = row;
	}
	else if (d->curRow >= row) {
		d->curRow++;
	}
	m_data->insertRow(*newItem, row, true /*repaint*/);

/*
	QSize s(tableSize());
	resizeContents(s.width(),s.height());

	//redraw only this row and below:
	int leftcol = d->pTopHeader->sectionAt( d->pTopHeader->offset() );
//	updateContents( columnPos( leftcol ), rowPos(d->curRow), 
//		clipper()->width(), clipper()->height() - (rowPos(d->curRow) - contentsY()) );
	updateContents( columnPos( leftcol ), rowPos(row), 
		clipper()->width(), clipper()->height() - (rowPos(row) - contentsY()) );

	d->pVerticalHeader->addLabel();

	//update navigator's data
	setNavRowCount(rows());

	if (d->curRow >= row) {
		//update
		editorShowFocus( d->curRow, d->curCol );
	}
	*/
}

tristate KexiTableView::deleteAllRows(bool ask, bool repaint)
{
	CHECK_DATA(true);
	if (m_data->count()<1)
		return true;

	if (ask) {
		QString tableName = m_data->dbTableName();
		if (!tableName.isEmpty()) {
			tableName.prepend(" \"");
			tableName.append("\"");
		}
		if (KMessageBox::No == KMessageBox::questionYesNo(this, 
				i18n("Do you want to clear the contents of table %1?").arg(tableName),
				0, KGuiItem(i18n("&Clear contents")), KStdGuiItem::no()))
			return cancelled;
	}

	cancelRowEdit();
//	acceptRowEdit();
//	d->pVerticalHeader->clear();
	const bool repaintLater = repaint && d->spreadSheetMode;
	const int oldRows = rows();

	bool res = m_data->deleteAllRows(repaint && !repaintLater);

	if (res) {
		if (d->spreadSheetMode) {
			const uint columns = m_data->columns.count();
			for (int i=0; i<oldRows; i++) {
				m_data->append(new KexiTableItem(columns));
			}
		}
	}
	if (repaintLater)
		m_data->refresh();

//	d->clearVariables();
//	d->pVerticalHeader->setCurrentRow(-1);

//	d->pUpdateTimer->start(1,true);
//	if (repaint)
//		viewport()->repaint();
	return res;
}

void KexiTableView::clearColumns(bool repaint)
{
	cancelRowEdit();
	m_data->clearInternal();

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

void KexiTableView::slotRefreshRequested()
{
//	cancelRowEdit();
	acceptRowEdit();
	d->pVerticalHeader->clear();

	if (d->curCol>=0 && d->curCol<columns()) {
		//find the editor for this column
		KexiTableEdit *edit = editor( d->curCol );
		if (edit) {
			edit->hideFocus();
		}
	}
//	setCursor(-1, -1, true);
	d->clearVariables();
	d->pVerticalHeader->setCurrentRow(-1);
	if (isVisible())
		initDataContents();
	else
		d->initDataContentsOnShow = true;
	d->pVerticalHeader->addLabels(m_data->count());

	updateScrollBars();
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
//	kdDebug(44021) << " KexiTableView::slotUpdate() -- " << endl;
//	QSize s(tableSize());
//	viewport()->setUpdatesEnabled(false);
///	resizeContents(s.width(), s.height());
//	viewport()->setUpdatesEnabled(true);

	updateContents();
	updateScrollBars();
	updateNavPanelGeometry();

	QSize s(tableSize());
	resizeContents(s.width(),s.height());
//	updateContents(0, contentsY()+clipper()->height()-2*d->rowHeight, clipper()->width(), d->rowHeight*3);
	
	//updateGeometries();
//	updateContents(0, 0, viewport()->width(), contentsHeight());
//	updateGeometries();
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
	emit reloadActions();
}

int KexiTableView::sortedColumn()
{
	if (m_data && d->isSortingEnabled)
		return m_data->sortedColumn();
	return -1;
}

bool KexiTableView::sortingAscending() const
{ 
	return m_data && m_data->sortingAscending();
}

void KexiTableView::setSorting(int col, bool ascending/*=true*/)
{
	if (!m_data || !d->isSortingEnabled)
		return;
	d->pTopHeader->setSortIndicator(col, ascending ? Ascending : Descending);
	m_data->setSorting(col, ascending);
}

bool KexiTableView::sort()
{
	if (!m_data || !d->isSortingEnabled)
		return false;

	if (rows() < 2)
		return true;

	if (!acceptRowEdit())
		return false;
			
	if (m_data->sortedColumn()!=-1)
		m_data->sort();

	//locate current record
	if (!d->pCurrentItem) {
		d->pCurrentItem = m_data->first();
		d->curRow = 0;
		if (!d->pCurrentItem)
			return true;
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
	return true;
}

void KexiTableView::sortAscending()
{
	if (currentColumn()<0)
		return;
	sortColumnInternal( currentColumn(), 1 );
}

void KexiTableView::sortDescending()
{
	if (currentColumn()<0)
		return;
	sortColumnInternal( currentColumn(), -1 );
}

void KexiTableView::sortColumnInternal(int col, int order)
{
	//-select sorting 
	bool asc;
	if (order == 0) {// invert
		if (col==sortedColumn())
			asc = !sortingAscending(); //inverse sorting for this column
		else
			asc = true;
	}
	else
		asc = (order==1);
	
	const QHeader::SortOrder prevSortOrder = d->pTopHeader->sortIndicatorOrder();
	const int prevSortColumn = d->pTopHeader->sortIndicatorSection();
	setSorting( col, asc );
	//-perform sorting 
	if (!sort())
		d->pTopHeader->setSortIndicator(prevSortColumn, prevSortOrder);
	
	if (col != prevSortColumn)
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
	int w = QMAX( ts.width() + leftMargin()+ verticalScrollBar()->sizeHint().width() + 2*2, 
		(d->navPanel->isVisible() ? d->navPanel->width() : 0) );
	int h = QMAX( ts.height()+topMargin()+horizontalScrollBar()->sizeHint().height(), 
		minimumSizeHint().height() );
	w = QMIN( w, qApp->desktop()->width()*3/4 ); //stretch
	h = QMIN( h, qApp->desktop()->height()*3/4 ); //stretch

//	kdDebug() << "KexiTableView::sizeHint()= " <<w <<", " <<h << endl;

	return QSize(w, h);
		/*QSize(
		QMAX( ts.width() + leftMargin() + 2*2, (d->navPanel ? d->navPanel->width() : 0) ),
		//+ QMIN(d->pVerticalHeader->width(),d->rowHeight) + margin()*2,
		QMAX( ts.height()+topMargin()+horizontalScrollBar()->sizeHint().height(), 
			minimumSizeHint().height() )
	);*/
//		QMAX(ts.height() + topMargin(), minimumSizeHint().height()) );
}

QSize KexiTableView::minimumSizeHint() const
{
	return QSize(
		leftMargin() + ((columns()>0)?columnWidth(0):KEXITV_DEFAULT_COLUMN_WIDTH) + 2*2, 
		d->rowHeight*5/2 + topMargin() + (d->navPanel && d->navPanel->isVisible() ? d->navPanel->height() : 0)
	);
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
	if (!item)
		return;
	// Go through the columns in the row r
	// if we know from where to where, go through [colfirst, collast],
	// else go through all of them
	if (colfirst==-1)
		colfirst=0;
	if (collast==-1)
		collast=columns()-1;

	int transly = rowp-cy;

	if (d->appearance.rowHighlightingEnabled && r == d->highlightedRow)
		pb->fillRect(0, transly, maxwc, d->rowHeight, d->appearance.rowHighlightingColor);
	else if(d->appearance.backgroundAltering && (r%2 != 0))
		pb->fillRect(0, transly, maxwc, d->rowHeight, d->appearance.alternateBackgroundColor);
	else
		pb->fillRect(0, transly, maxwc, d->rowHeight, d->appearance.baseColor);

	for(int c = colfirst; c <= collast; c++)
	{
		// get position and width of column c
		int colp = columnPos(c);
		if (colp==-1)
			continue; //invisible column?
		int colw = columnWidth(c);
		int translx = colp-cx;

		// Translate painter and draw the cell
		pb->saveWorldMatrix();
		pb->translate(translx, transly);
			paintCell( pb, item, c, r, QRect(colp, rowp, colw, d->rowHeight));
		pb->restoreWorldMatrix();
	}

	if (d->dragIndicatorLine>=0) {
		int y_line = -1;
		if (r==(rows()-1) && d->dragIndicatorLine==rows()) {
			y_line = transly+d->rowHeight-3; //draw at last line
		}
		if (d->dragIndicatorLine==r) {
			y_line = transly+1;
		}
		if (y_line>=0) {
			RasterOp op = pb->rasterOp();
			pb->setRasterOp(XorROP);
			pb->setPen( QPen(white, 3) );
			pb->drawLine(0, y_line, maxwc, y_line);
			pb->setRasterOp(op);
		}
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

/*	kdDebug(44021) << QString(" KexiTableView::drawContents(cx:%1 cy:%2 cw:%3 ch:%4)")
			.arg(cx).arg(cy).arg(cw).arg(ch) << endl;*/

	if (rowlast == -1) {
		rowlast = rows() - 1;
		plus1row = inserting;
		if (rowfirst == -1) {
			if (rowAt(cy - d->rowHeight) != -1) {
				paintOnlyInsertRow = true;
//				kdDebug(44021) << "-- paintOnlyInsertRow --" << endl;
			}
		}
	}
//	kdDebug(44021) << "rowfirst="<<rowfirst<<" rowlast="<<rowlast<<" rows()="<<rows()<<endl;
//	kdDebug(44021)<<" plus1row=" << plus1row<<endl;
	
	if ( collast == -1 )
		collast = columns() - 1;

	if (colfirst>collast) {
		int tmp = colfirst;
		colfirst = collast;
		collast = tmp;
	}
	if (rowfirst>rowlast) {
		int tmp = rowfirst;
		rowfirst = rowlast;
		rowlast = tmp;
	}

// 	qDebug("cx:%3d cy:%3d w:%3d h:%3d col:%2d..%2d row:%2d..%2d tsize:%4d,%4d", 
//	cx, cy, cw, ch, colfirst, collast, rowfirst, rowlast, tableSize().width(), tableSize().height());
//	triggerUpdate();

	if (rowfirst == -1 || colfirst == -1) {
		if (!paintOnlyInsertRow && !plus1row) {
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
	int maxwc = columnPos(columns() - 1) + columnWidth(columns() - 1);
//	kdDebug(44021) << "KexiTableView::drawContents(): maxwc: " << maxwc << endl;

	pb->fillRect(cx, cy, cw, ch, d->appearance.baseColor);

	int rowp;
	int r;
	if (paintOnlyInsertRow) {
		r = rows();
		rowp = rowPos(r); // 'insert' row's position
	}
	else {
		QPtrListIterator<KexiTableItem> it = m_data->iterator();
		it += rowfirst;//move to 1st row
		rowp = rowPos(rowfirst); // row position 
		for (r = rowfirst;r <= rowlast; r++, ++it, rowp+=d->rowHeight) {
			paintRow(it.current(), pb, r, rowp, cx, cy, colfirst, collast, maxwc);
		}
	}

	if (plus1row) { //additional - 'insert' row
		paintRow(d->pInsertItem, pb, r, rowp, cx, cy, colfirst, collast, maxwc);
	}

	delete pb;

	p->drawPixmap(cx,cy,*d->pBufferPm, 0,0,cw,ch);

  //(js)
	paintEmptyArea(p, cx, cy, cw, ch);
}

void KexiTableView::paintCell(QPainter* p, KexiTableItem *item, int col, int row, const QRect &cr, bool print)
{
	p->save();
//	kdDebug() <<"KexiTableView::paintCell(col=" << col <<"row="<<row<<")"<<endl;
	Q_UNUSED(print);
	int w = cr.width();
	int h = cr.height();
	int x2 = w - 1;
	int y2 = h - 1;

	//	Draw our lines
	QPen pen(p->pen());

	if (!d->appearance.fullRowSelection) {
		p->setPen(d->appearance.borderColor);
		p->drawLine( x2, 0, x2, y2 );	// right
		p->drawLine( 0, y2, x2, y2 );	// bottom
	}
	p->setPen(pen);

	if (d->pEditor && row == d->curRow && col == d->curCol //don't paint contents of edited cell
		&& d->pEditor->hasFocusableWidget() //..if it's visible
	   ) {
		p->restore();
		return;
	}

	KexiTableEdit *edit = editor( col, /*ignoreMissingEditor=*/true );
//	if (!edit)
//		return;

/*
#ifdef Q_WS_WIN
	int x = 1;
	int y_offset = -1;
#else
	int x = 1;
	int y_offset = 0;
#endif

//	const int ctype = columnType(col);*/
//	int x=1;
	int x = edit ? edit->leftMargin() : 0;
	int y_offset=0;

	int align = SingleLine | AlignVCenter;
	QString txt; //text to draw

	QVariant cell_value;
	if ((uint)col < item->count()) {
		if (d->pCurrentItem == item) {
			if (d->pEditor && row == d->curRow && col == d->curCol 
				&& !d->pEditor->hasFocusableWidget())
			{
				//we're over editing cell and the editor has no widget
				// - we're displaying internal values, not buffered
				bool ok;
				cell_value = d->pEditor->value(ok);
			}
			else {
				//we're displaying values from edit buffer, if available
				cell_value = *bufferedValueAt(col);
			}
		}
		else {
			cell_value = item->at(col);
		}
	}

	if (edit)
		edit->setupContents( p, d->pCurrentItem == item && col == d->curCol, 
			cell_value, txt, align, x, y_offset, w, h );

	if (d->appearance.fullRowSelection)
		y_offset++; //correction because we're not drawing cell borders

/*
	if (KexiDB::Field::isFPNumericType( ctype )) {
#ifdef Q_WS_WIN
#else
			x = 0;
#endif
//js TODO: ADD OPTION to desplaying NULL VALUES as e.g. "(null)"
		if (!cell_value.isNull())
			txt = KGlobal::locale()->formatNumber(cell_value.toDouble());
		w -= 6;
		align |= AlignRight;
	}
	else if (ctype == KexiDB::Field::Enum)
	{
		txt = m_data->column(col)->field->enumHints().at(cell_value.toInt());
		align |= AlignLeft;
	}
	else if (KexiDB::Field::isIntegerType( ctype )) {
		int num = cell_value.toInt();
#ifdef Q_WS_WIN
		x = 1;
#else
		x = 0;
#endif
		w -= 6;
		align |= AlignRight;
		if (!cell_value.isNull())
			txt = QString::number(num);
	}
	else if (ctype == KexiDB::Field::Boolean) {
		int s = QMAX(h - 5, 12);
		QRect r(w/2 - s/2 + x, h/2 - s/2 - 1, s, s);
		p->setPen(QPen(colorGroup().text(), 1));
		p->drawRect(r);
		if (cell_value.asBool())
		{
			p->drawLine(r.x() + 2, r.y() + 2, r.right() - 1, r.bottom() - 1);
			p->drawLine(r.x() + 2, r.bottom() - 2, r.right() - 1, r.y() + 1);
		}
	}
	else if (ctype == KexiDB::Field::Date) { //todo: datetime & time
#ifdef Q_WS_WIN
		x = 5;
#else
		x = 5;
#endif
		if(cell_value.toDate().isValid())
		{
#ifdef USE_KDE
			txt = KGlobal::locale()->formatDate(cell_value.toDate(), true);
#else
			if (!cell_value.isNull())
				txt = cell_value.toDate().toString(Qt::LocalDate);
#endif
		}
		align |= AlignLeft;
	}
	else {//default:
#ifdef Q_WS_WIN
		x = 5;
//		y_offset = -1;
#else
		x = 5;
//		y_offset = 0;
#endif
		if (!cell_value.isNull())
			txt = cell_value.toString();
		align |= AlignLeft;
	}*/
	
	// draw selection background
//	const bool has_focus = hasFocus() || viewport()->hasFocus() || d->pContextMenu->hasFocus();

	const bool columnReadOnly = m_data->column(col)->readOnly();

	if (d->pCurrentItem == item && col == d->curCol) {
/*		edit->paintSelectionBackground( p, isEnabled(), txt, align, x, y_offset, w, h,
			has_focus ? colorGroup().highlight() : gray,
			columnReadOnly, d->fullRowSelectionEnabled );*/
		if (edit)
			edit->paintSelectionBackground( p, isEnabled(), txt, align, x, y_offset, w, h,
				isEnabled() ? colorGroup().highlight() : QColor(200,200,200),//d->grayColor,
				columnReadOnly, d->appearance.fullRowSelection );
	}

/*
	if (!txt.isEmpty() && d->pCurrentItem == item 
		&& col == d->curCol && !columnReadOnly) //js: && !d->recordIndicator)
	{
		QRect bound=fontMetrics().boundingRect(x, y_offset, w - (x+x), h, align, txt);
		bound.setX(bound.x()-1);
		bound.setY(0);
		bound.setWidth( QMIN( bound.width()+2, w - (x+x)+1 ) );
		bound.setHeight(d->rowHeight-1);
		if (has_focus)
			p->fillRect(bound, colorGroup().highlight());
		else
			p->fillRect(bound, gray);
	}
*/	
	if (!edit){
		p->fillRect(0, 0, x2, y2, d->diagonalGrayPattern);
	}

//	If we are in the focus cell, draw indication
	if(d->pCurrentItem == item && col == d->curCol //js: && !d->recordIndicator)
		&& !d->appearance.fullRowSelection) 
	{
//		kdDebug() << ">>> CURRENT CELL ("<<d->curCol<<"," << d->curRow<<") focus="<<has_focus<<endl;
//		if (has_focus) {
		if (isEnabled()) {
			p->setPen(d->appearance.textColor);
		}
		else {
			QPen gray_pen(p->pen());
			gray_pen.setColor(d->appearance.borderColor);
			p->setPen(gray_pen);
		}
		if (edit)
			edit->paintFocusBorders( p, cell_value, 0, 0, x2, y2 );
		else
			p->drawRect(0, 0, x2, y2);
	}

	bool autonumber = false;
	if ((!d->newRowEditing &&item == d->pInsertItem) 
		|| (d->newRowEditing && item == d->pCurrentItem && cell_value.isNull())) {
		//we're in "insert row"
		if (m_data->column(col)->field()->isAutoIncrement()) {
			//"autonumber" column
			txt = i18n("(autonumber)");
			autonumber = true;
		}
	}

	// draw text
	if (!txt.isEmpty()) {
		if (autonumber) {
			p->setPen(blue);
			p->setFont(d->autonumberFont);
			p->drawPixmap( w - x - x - 9 - d->autonumberTextWidth - d->autonumberIcon.width(),
				(h-d->autonumberIcon.height())/2, d->autonumberIcon );
		}
		else if (d->pCurrentItem == item && col == d->curCol && !columnReadOnly)
			p->setPen(colorGroup().highlightedText());
		else if (d->appearance.rowHighlightingEnabled && row == d->highlightedRow)
			p->setPen(d->appearance.rowHighlightingTextColor);
		else
			p->setPen(d->appearance.textColor);
		p->drawText(x, y_offset, w - (x + x)- ((align & AlignLeft)?2:0)/*right space*/, h, align, txt);
	}
	p->restore();
}

QPoint KexiTableView::contentsToViewport2( const QPoint &p )
{
	return QPoint( p.x() - contentsX(), p.y() - contentsY() );
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
//	ts.setWidth( QMIN( ts.width(), visibleWidth() ) );
//	ts.setHeight( QMIN( ts.height() - (d->navPanel ? d->navPanel->height() : 0), visibleHeight()) );
/*	kdDebug(44021) << QString(" (cx:%1 cy:%2 cw:%3 ch:%4)")
			.arg(cx).arg(cy).arg(cw).arg(ch) << endl;
	kdDebug(44021) << QString(" (w:%3 h:%4)")
			.arg(ts.width()).arg(ts.height()) << endl;*/
	
	// Region of the rect we should draw, calculated in viewport
	// coordinates, as a region can't handle bigger coordinates
	contentsToViewport2( cx, cy, cx, cy );
	QRegion reg( QRect( cx, cy, cw, ch ) );

//kdDebug() << "---cy-- " << contentsY() << endl;

	// Subtract the table from it
//	reg = reg.subtract( QRect( QPoint( 0, 0 ), ts-QSize(0,d->navPanel->isVisible() ? d->navPanel->height() : 0) ) );
	reg = reg.subtract( QRect( QPoint( 0, 0 ), ts
		-QSize(0,QMAX((d->navPanel ? d->navPanel->height() : 0), horizontalScrollBar()->sizeHint().height())
			- (horizontalScrollBar()->isVisible() ? horizontalScrollBar()->sizeHint().height()/2 : 0)
			+ (horizontalScrollBar()->isVisible() ? 0 : 
				d->internal_bottomMargin
//	horizontalScrollBar()->sizeHint().height()/2
		)
//- /*d->bottomMargin */ horizontalScrollBar()->sizeHint().height()*3/2
			+ contentsY()
//			- (verticalScrollBar()->isVisible() ? horizontalScrollBar()->sizeHint().height()/2 : 0)
			)
		) );
//	reg = reg.subtract( QRect( QPoint( 0, 0 ), ts ) );

	// And draw the rectangles (transformed inc contents coordinates as needed)
	QMemArray<QRect> r = reg.rects();
	for ( int i = 0; i < (int)r.count(); i++ ) {
		QRect rect( viewportToContents2(r[i].topLeft()), r[i].size() );
/*		kdDebug(44021) << QString("- pEA: p->fillRect(x:%1 y:%2 w:%3 h:%4)")
			.arg(rect.x()).arg(rect.y())
			.arg(rect.width()).arg(rect.height()) << endl;*/
//		p->fillRect( QRect(viewportToContents2(r[i].topLeft()),r[i].size()), d->emptyAreaColor );
		p->fillRect( rect, d->appearance.emptyAreaColor );
//		p->fillRect( QRect(viewportToContents2(r[i].topLeft()),r[i].size()), viewport()->backgroundBrush() );
	}
}

void KexiTableView::contentsMouseDoubleClickEvent(QMouseEvent *e)
{
//	kdDebug(44021) << "KexiTableView::contentsMouseDoubleClickEvent()" << endl;
	d->contentsMousePressEvent_dblClick = true;
	contentsMousePressEvent(e);
	d->contentsMousePressEvent_dblClick = false;

	if(d->pCurrentItem)
	{
		if(d->editOnDoubleClick && columnEditable(d->curCol) 
			&& columnType(d->curCol) != KexiDB::Field::Boolean)
		{
			startEditCurrentCell();
//			createEditor(d->curRow, d->curCol, QString::null);
		}

		emit itemDblClicked(d->pCurrentItem, d->curRow, d->curCol);
	}
}

void KexiTableView::contentsMousePressEvent( QMouseEvent* e )
{
//	kdDebug(44021) << "KexiTableView::contentsMousePressEvent() ??" << endl;
	setFocus();
	if(m_data->count()==0 && !isInsertingEnabled()) {
		QScrollView::contentsMousePressEvent( e );
		return;
	}

	if (columnAt(e->pos().x())==-1) { //outside a colums
		QScrollView::contentsMousePressEvent( e );
		return;
	}
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
			if (newrow==-1 && m_data->count()>0) {
				QScrollView::contentsMousePressEvent( e );
				return;
			}
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
		if (rowAt(e->pos().y())==-1 || columnAt(e->pos().x())==-1) {
			QScrollView::contentsMousePressEvent( e );
			return; //clicked outside a grid
		}
		// get new focus cell
		newrow = rowAt(e->pos().y());
	}
	newcol = columnAt(e->pos().x());

	if(e->button() != NoButton) {
		setCursor(newrow,newcol);
	}

//	kdDebug(44021)<<"void KexiTableView::contentsMousePressEvent( QMouseEvent* e ) by now the current items should be set, if not -> error + crash"<<endl;
	if(e->button() == RightButton)
	{
		showContextMenu(e->globalPos());
	}
	else if(e->button() == LeftButton)
	{
		if(columnType(d->curCol) == KexiDB::Field::Boolean && columnEditable(d->curCol))
		{
			boolToggled();
		}
#if 0 //js: TODO
		else if(columnType(d->curCol) == QVariant::StringList && columnEditable(d->curCol))
		{
			createEditor(d->curRow, d->curCol);
		}
#endif
	}
//ScrollView::contentsMousePressEvent( e );
}

void KexiTableView::contentsMouseReleaseEvent( QMouseEvent* e )
{
//	kdDebug(44021) << "KexiTableView::contentsMousePressEvent() ??" << endl;
	if(m_data->count()==0 && !isInsertingEnabled())
		return;

	int col = columnAt(e->pos().x());
	int row = rowAt(e->pos().y());
	if (!d->pCurrentItem || col==-1 || row==-1 || col!=d->curCol || row!=d->curRow)//outside a current cell
		return;

	QScrollView::contentsMouseReleaseEvent( e );

	emit itemMouseReleased(d->pCurrentItem, d->curRow, d->curCol);
}

KPopupMenu* KexiTableView::popup() const
{
	return d->pContextMenu;
}

void KexiTableView::showContextMenu(QPoint pos)
{
	if (!d->contextMenuEnabled || d->pContextMenu->count()<1)
		return;
	if (pos==QPoint(-1,-1)) {
		pos = viewport()->mapToGlobal( QPoint( columnPos(d->curCol), rowPos(d->curRow) + d->rowHeight ) );
	}
	//show own context menu if configured
//	if (updateContextMenu()) {
		selectRow(d->curRow);
		d->pContextMenu->exec(pos);
/*	}
	else {
		//request other context menu
		emit contextMenuRequested(d->pCurrentItem, d->curCol, pos);
	}*/
}

void KexiTableView::contentsMouseMoveEvent( QMouseEvent *e )
{
	if (d->appearance.rowHighlightingEnabled) {
		int row;
		if (columnAt(e->x())<0)
			row = -1;
		else
			row = rowAt( e->y() );

//	const col = columnAt(e->x());
//	columnPos(col) + columnWidth(col)
//	columnPos(d->numCols - 1) + columnWidth(d->numCols - 1)));

		if (row != d->highlightedRow) {
			updateRow(d->highlightedRow);
				d->highlightedRow = row;
			updateRow(d->highlightedRow);
		}
	}

#if 0//(js) doesn't work!

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
#endif
	QScrollView::contentsMouseMoveEvent(e);
}

void KexiTableView::startEditCurrentCell(const QString &setText)
{
//	if (columnType(d->curCol) == KexiDB::Field::Boolean)
//		return;
	if (isReadOnly() || !columnEditable(d->curCol))
		return;
	if (d->pEditor) {
		if (d->pEditor->hasFocusableWidget()) {
			d->pEditor->show();
			d->pEditor->setFocus();
		}
	}
	ensureVisible(columnPos(d->curCol), rowPos(d->curRow)+rowHeight(), columnWidth(d->curCol), rowHeight());
	createEditor(d->curRow, d->curCol, setText, !setText.isEmpty());
}

void KexiTableView::deleteAndStartEditCurrentCell()
{
	if (isReadOnly() || !columnEditable(d->curCol))
		return;
	if (d->pEditor) {//if we've editor - just clear it
		d->pEditor->clear();
		return;
	}
	if (columnType(d->curCol) == KexiDB::Field::Boolean)
		return;
	ensureVisible(columnPos(d->curCol), rowPos(d->curRow)+rowHeight(), columnWidth(d->curCol), rowHeight());
	createEditor(d->curRow, d->curCol, QString::null, false/*removeOld*/);
	if (!d->pEditor)
		return;
	d->pEditor->clear();
	if (d->pEditor->acceptEditorAfterDeleteContents())
		acceptEditor();
}

#if 0//(js) doesn't work!
void KexiTableView::contentsMouseReleaseEvent(QMouseEvent *)
{
	if(d->needAutoScroll)
	{
		d->scrollTimer->stop();
	}
}
#endif

void KexiTableView::plugSharedAction(KAction* a)
{
	if (!a)
		return;
	d->sharedActions.insert(a->name(), a);
}

bool KexiTableView::shortCutPressed( QKeyEvent *e, const QCString &action_name )
{
	KAction *action = d->sharedActions[action_name];
	if (action) {
		if (!action->isEnabled())//this action is disabled - don't process it!
			return false; 
		if (action->shortcut() == KShortcut( KKey(e) ))
			return false;//this shortcut is owned by shared action - don't process it!
	}

	//check default shortcut
	if (action_name=="data_save_row")
		return (e->key() == Key_Return || e->key() == Key_Enter) && e->state()==ShiftButton;
	if (action_name=="edit_delete_row")
		return e->key() == Key_Delete && e->state()==ShiftButton;
	if (action_name=="edit_delete")
		return e->key() == Key_Delete && e->state()==NoButton;
	if (action_name=="edit_edititem")
		return e->key() == Key_F2 && e->state()==NoButton;
	if (action_name=="edit_insert_empty_row")
		return e->key() == Key_Insert && e->state()==(ShiftButton | ControlButton);

	return false;
}

void KexiTableView::keyPressEvent(QKeyEvent* e)
{
	CHECK_DATA_;
//	kdDebug() << "KexiTableView::keyPressEvent: key=" <<e->key() << " txt=" <<e->text()<<endl;

	const bool ro = isReadOnly();
	QWidget *w = focusWidget();
//	if (!w || w!=viewport() && w!=this && (!d->pEditor || w!=d->pEditor->view() && w!=d->pEditor)) {
//	if (!w || w!=viewport() && w!=this && (!d->pEditor || w!=d->pEditor->view())) {
	if (!w || w!=viewport() && w!=this && (!d->pEditor || !Kexi::hasParent(d->pEditor, w))) {
		//don't process stranger's events
		e->ignore();
		return;
	}
	if (d->skipKeyPress) {
		d->skipKeyPress=false;
		e->ignore();
		return;
	}
	
	if(d->pCurrentItem == 0 && (m_data->count() > 0 || isInsertingEnabled()))
	{
		setCursor(0,0);
	}
	else if(m_data->count() == 0 && !isInsertingEnabled())
	{
		e->accept();
		return;
	}

	if(d->pEditor) {// if a cell is edited, do some special stuff
		if (e->key() == Key_Escape) {
			cancelEditor();
			e->accept();
			return;
		} else if (e->key() == Key_Return || e->key() == Key_Enter) {
			if (columnType(d->curCol) == KexiDB::Field::Boolean) {
				boolToggled();
			}
			else {
				acceptEditor();
			}
			e->accept();
			return;
		}
	}
	else if (d->rowEditing) {// if a row is in edit mode, do some special stuff
		if (shortCutPressed( e, "data_save_row")) {
			kdDebug() << "shortCutPressed!!!" <<endl;
			acceptRowEdit();
			return;
		}
	}

	if(e->key() == Key_Return || e->key() == Key_Enter)
	{
		emit itemReturnPressed(d->pCurrentItem, d->curRow, d->curCol);
	}

	int curRow = d->curRow;
	int curCol = d->curCol;

	const bool nobtn = e->state()==NoButton;
	bool printable = false;

	//check shared shortcuts
	if (!ro) {
		if (shortCutPressed(e, "edit_delete_row")) {
			deleteCurrentRow();
			e->accept();
			return;
		} else if (shortCutPressed(e, "edit_delete")) {
			deleteAndStartEditCurrentCell();
			e->accept();
			return;
		}
		else if (shortCutPressed(e, "edit_insert_empty_row")) {
			insertEmptyRow();
			e->accept();
			return;
		}
	}

	switch (e->key())
	{
/*	case Key_Delete:
		if (e->state()==Qt::ControlButton) {//remove current row
			deleteCurrentRow();
		}
		else if (nobtn) {//remove contents of the current cell
			deleteAndStartEditCurrentCell();
		}
		break;*/

	case Key_Shift:
	case Key_Alt:
	case Key_Control:
	case Key_Meta:
		e->ignore();
		break;
	case Key_Up:
		if (nobtn) {
			selectPrevRow();
			e->accept();
			return;
		}
		break;
	case Key_Down:
		if (nobtn) {
//			curRow = QMIN(rows() - 1 + (isInsertingEnabled()?1:0), curRow + 1);
			selectNextRow();
			e->accept();
			return;
		}
		break;
	case Key_PageUp:
		if (nobtn) {
//			curRow -= visibleHeight() / d->rowHeight;
//			curRow = QMAX(0, curRow);
			selectPrevPage();
			e->accept();
			return;
		}
		break;
	case Key_PageDown:
		if (nobtn) {
//			curRow += visibleHeight() / d->rowHeight;
//			curRow = QMIN(rows() - 1 + (isInsertingEnabled()?1:0), curRow);
			selectNextPage();
			e->accept();
			return;
		}
		break;
	case Key_Home:
		if (d->appearance.fullRowSelection) {
			//we're in row-selection mode: home key always moves to 1st row
			curRow = 0;//to 1st row
		}
		else {//cell selection mode: different actions depending on ctrl and shift keys state
			if (nobtn) {
				curCol = 0;//to 1st col
			}
			else if (e->state()==ControlButton) {
				curRow = 0;//to 1st row
			}
			else if (e->state()==(ControlButton|ShiftButton)) {
				curRow = 0;//to 1st row and col
				curCol = 0;
			}
		}
		break;
	case Key_End:
		if (d->appearance.fullRowSelection) {
			//we're in row-selection mode: home key always moves to last row
			curRow = m_data->count()-1+(isInsertingEnabled()?1:0);//to last row
		}
		else {//cell selection mode: different actions depending on ctrl and shift keys state
			if (nobtn) {
				curCol = columns()-1;//to last col
			}
			else if (e->state()==ControlButton) {
				curRow = m_data->count()-1+(isInsertingEnabled()?1:0);//to last row
			}
			else if (e->state()==(ControlButton|ShiftButton)) {
				curRow = m_data->count()-1+(isInsertingEnabled()?1:0);//to last row and col
				curCol = columns()-1;//to last col
			}
		}
		break;
	case Key_Backspace:
		if (nobtn && !ro && columnType(curCol) != KexiDB::Field::Boolean && columnEditable(curCol))
			createEditor(curRow, curCol, QString::null, true);
		break;
	case Key_Space:
		if (nobtn && !ro && columnEditable(curCol)) {
			if (columnType(curCol) == KexiDB::Field::Boolean) {
				boolToggled();
				break;
			}
			else
				printable = true; //just space key
		}
	case Key_Escape:
		if (nobtn && d->rowEditing) {
			cancelRowEdit();
			return;
		}
	default:
		//others:
		if (nobtn && (e->key()==Key_Tab || e->key()==Key_Right)) {
//! \todo add option for stopping at 1st column for Key_left
			//tab
			if (acceptEditor()) {
				if (curCol == (columns() - 1)) {
					if (curRow < (rows()-1+(isInsertingEnabled()?1:0))) {//skip to next row
						curRow++;
						curCol = 0;
					}
				}
				else
					curCol++;
			}
		}
		else if ((e->state()==ShiftButton && e->key()==Key_Tab)
		 || (nobtn && e->key()==Key_Backtab)
		 || (e->state()==ShiftButton && e->key()==Key_Backtab)
		 || (nobtn && e->key()==Key_Left)
			) {
//! \todo add option for stopping at last column
			//backward tab
			if (acceptEditor()) {
				if (curCol == 0) {
					if (curRow>0) {//skip to previous row
						curRow--;
						curCol = columns() - 1;
					}
				}
				else
					curCol--;
			}
		}
		else if ( nobtn && (e->key()==Key_Enter || e->key()==Key_Return || shortCutPressed(e, "edit_edititem")) ) {
			startEditOrToggleValue();
		}
		else if (nobtn && e->key()==KGlobalSettings::contextMenuKey()) { //Key_Menu:
			showContextMenu();
		}
		else {
			KexiTableEdit *edit = editor( d->curCol );
			if (edit && edit->handleKeyPress(e, d->pEditor==edit)) {
				//try to handle the event @ editor's level
				e->accept();
				return;
			}

			qDebug("KexiTableView::KeyPressEvent(): default");
			if (e->text().isEmpty() || !e->text().isEmpty() && !e->text()[0].isPrint() ) {
				kdDebug(44021) << "NOT PRINTABLE: 0x0" << QString("%1").arg(e->key(),0,16) <<endl;
//				e->ignore();
				QScrollView::keyPressEvent(e);
				return;
			}

			printable = true;
		}
	}
	//finally: we've printable char:
	if (printable && !ro) {
		KexiTableViewColumn *colinfo = m_data->column(curCol);
		if (colinfo->acceptsFirstChar(e->text()[0])) {
			kdDebug(44021) << "KexiTableView::KeyPressEvent(): ev pressed: acceptsFirstChar()==true";
	//			if (e->text()[0].isPrint())
			createEditor(curRow, curCol, e->text(), true);
		}
		else {
//TODO show message "key not allowed eg. on a statusbar"
			kdDebug(44021) << "KexiTableView::KeyPressEvent(): ev pressed: acceptsFirstChar()==false";
		}
	}

	d->vScrollBarValueChanged_enabled=false;

	// if focus cell changes, repaint
	setCursor(curRow, curCol);

	d->vScrollBarValueChanged_enabled=true;

	e->accept();
}

void KexiTableView::emitSelected()
{
	if(d->pCurrentItem)
		emit itemSelected(d->pCurrentItem);
}

void KexiTableView::startEditOrToggleValue()
{
	if ( !isReadOnly() && columnEditable(d->curCol) ) {
		if (columnType(d->curCol) == KexiDB::Field::Boolean) {
			boolToggled();
		}
		else {
			startEditCurrentCell();
			return;
		}
	}
}

void KexiTableView::boolToggled()
{
	startEditCurrentCell();
	if (d->pEditor) {
		d->pEditor->clickedOnContents();
	}
	acceptEditor();
	updateCell(d->curRow, d->curCol);

/*	int s = d->pCurrentItem->at(d->curCol).toInt();
	QVariant oldValue=d->pCurrentItem->at(d->curCol);
	(*d->pCurrentItem)[d->curCol] = QVariant(s ? 0 : 1);
	updateCell(d->curRow, d->curCol);
//	emit itemChanged(d->pCurrentItem, d->curRow, d->curCol, oldValue);
//	emit itemChanged(d->pCurrentItem, d->curRow, d->curCol);*/
}

void KexiTableView::clearSelection()
{
//	selectRow( -1 );
	int oldRow = d->curRow;
//	int oldCol = d->curCol;
	d->curRow = -1;
	d->curCol = -1;
	d->pCurrentItem = 0;
	updateRow( oldRow );
	setNavRowNumber(-1);
}

void KexiTableView::selectNextRow()
{
	selectRow( QMIN( rows() - 1 +(isInsertingEnabled()?1:0), d->curRow + 1 ) );
}

int KexiTableView::rowsPerPage() const
{
	return visibleHeight() / d->rowHeight;
}

void KexiTableView::selectPrevPage()
{
	selectRow( 
		QMAX( 0, d->curRow - rowsPerPage() )
	);
}

void KexiTableView::selectNextPage()
{
	selectRow( 
		QMIN( 
			rows() - 1 + (isInsertingEnabled()?1:0),
			d->curRow + rowsPerPage()
		)
	);
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
}

void KexiTableView::selectPrevRow()
{
	selectRow( QMAX( 0, d->curRow - 1 ) );
}

KexiTableEdit *KexiTableView::editor( int col, bool ignoreMissingEditor )
{
	if (!m_data || col<0 || col>=columns())
		return 0;
	KexiTableViewColumn *tvcol = m_data->column(col);
//	int t = tvcol->field->type();

	//find the editor for this column
	KexiTableEdit *editor = d->editors[ tvcol ];
	if (editor)
		return editor;

	//not found: create
//	editor = KexiCellEditorFactory::createEditor(*m_data->column(col)->field, this);
	editor = KexiCellEditorFactory::createEditor(*m_data->column(col), this);
	if (!editor) {//create error!
		if (!ignoreMissingEditor) {
			//js TODO: show error???
			cancelRowEdit();
		}
		return 0;
	}
	editor->hide();
	connect(editor,SIGNAL(editRequested()),this,SLOT(slotEditRequested()));
	connect(editor,SIGNAL(cancelRequested()),this,SLOT(cancelEditor()));
	connect(editor,SIGNAL(acceptRequested()),this,SLOT(acceptEditor()));

	editor->resize(columnWidth(col)-1, rowHeight()-1);
	editor->installEventFilter(this);
	if (editor->view())
		editor->view()->installEventFilter(this);
	//store
	d->editors.insert( tvcol, editor );
	return editor;
}

void KexiTableView::editorShowFocus( int /*row*/, int col )
{
	KexiTableEdit *edit = editor( col );
	/*nt p = rowPos(row);
	 (!edit || (p < contentsY()) || (p > (contentsY()+clipper()->height()))) {
		kdDebug()<< "KexiTableView::editorShowFocus() : OUT" << endl;
		return;
	}*/
	if (edit) {
		kdDebug()<< "KexiTableView::editorShowFocus() : IN" << endl;
		QRect rect = cellGeometry( d->curRow, d->curCol );
//		rect.moveBy( -contentsX(), -contentsY() );
		edit->showFocus( rect );
	}
}

void KexiTableView::slotEditRequested()
{
//	KexiTableEdit *edit = editor( d->curCol );
//	if (edit) {

	createEditor(d->curRow, d->curCol);
}

void KexiTableView::createEditor(int row, int col, const QString& addText, bool removeOld)
{
	kdDebug(44021) << "KexiTableView::createEditor('"<<addText<<"',"<<removeOld<<")"<<endl;
	if (isReadOnly()) {
		kdDebug(44021) << "KexiTableView::createEditor(): DATA IS READ ONLY!"<<endl;
		return;
	}

	if (m_data->column(col)->readOnly()) {//d->pColumnModes.at(d->numCols-1) & ColumnReadOnly)
		kdDebug(44021) << "KexiTableView::createEditor(): COL IS READ ONLY!"<<endl;
		return;
	}
	
/*	QVariant val;
	if (!removeOld) {
		val = *bufferedValueAt(col);
//		val = d->pCurrentItem->at(col);
//		val = d->pCurrentItem->at(d->curCol);
	}*/
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

	const bool startRowEdit = !d->rowEditing; //remember if we're starting row edit

	if (!d->rowEditing) {
		//we're starting row editing session
		m_data->clearRowEditBuffer();
		
		d->rowEditing = true;
		//indicate on the vheader that we are editing:
		d->pVerticalHeader->setEditRow(d->curRow);
		if (isInsertingEnabled() && d->pCurrentItem==d->pInsertItem) {
			//we should know that we are in state "new row editing"
			d->newRowEditing = true;
			//'insert' row editing: show another row after that:
			m_data->append( d->pInsertItem );
			//new empty insert item
			d->pInsertItem = new KexiTableItem(columns());
//			updateContents();
			d->pVerticalHeader->addLabel();
			d->pVerticalHeaderAlreadyAdded = true;
			QSize s(tableSize());
			resizeContents(s.width(), s.height());
			//refr. current and next row
			updateContents(columnPos(0), rowPos(row), viewport()->width(), d->rowHeight*2);
//			updateContents(columnPos(0), rowPos(row+1), viewport()->width(), d->rowHeight);
			qApp->processEvents(500);
			ensureVisible(columnPos(d->curCol), rowPos(row+1)+d->rowHeight-1, columnWidth(d->curCol), d->rowHeight);
		}
	}	
//	else {//just reinit
//		d->pAfterInsertItem->init(columns());
//			d->paintAfterInsertRow = true;
//	}

	d->pEditor = editor( col );
	if (!d->pEditor)
		return;

	d->pEditor->init(*bufferedValueAt(col), addText, removeOld);
	if (d->pEditor->hasFocusableWidget()) {
		moveChild(d->pEditor, columnPos(d->curCol), rowPos(d->curRow));

		d->pEditor->resize(columnWidth(d->curCol)-1, rowHeight()-1);
		d->pEditor->show();

		d->pEditor->setFocus();
	}

	if (startRowEdit)
		emit rowEditStarted(d->curRow);
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

bool KexiTableView::focusNextPrevChild(bool /*next*/)
{
	return false; //special Tab/BackTab meaning
/*	if (d->pEditor)
		return true;
	return QScrollView::focusNextPrevChild(next);*/
}

void KexiTableView::resizeEvent(QResizeEvent *e)
{
	QScrollView::resizeEvent(e);
	//updateGeometries();
	
	updateNavPanelGeometry();

	if ((contentsHeight() - e->size().height()) <= d->rowHeight) {
		slotUpdate();
		triggerUpdate();
	}
//	d->pTopHeader->repaint();


/*		d->navPanel->setGeometry(
			frameWidth(),
			viewport()->height() +d->pTopHeader->height() 
			-(horizontalScrollBar()->isVisible() ? 0 : horizontalScrollBar()->sizeHint().height())
			+frameWidth(),
			d->navPanel->sizeHint().width(), // - verticalScrollBar()->sizeHint().width() - horizontalScrollBar()->sizeHint().width(),
			horizontalScrollBar()->sizeHint().height()
		);*/
//		updateContents();
//		d->navPanel->setGeometry(1,horizontalScrollBar()->pos().y(),
	//		d->navPanel->width(), horizontalScrollBar()->height());
//	updateContents(0,0,2000,2000);//js
//	erase(); repaint();
}

void KexiTableView::updateNavPanelGeometry()
{
	if (!d->navPanel)
		return;
	d->navPanel->updateGeometry();
//	QRect g = d->navPanel->geometry();
//		kdDebug(44021) << "**********"<< g.top() << " " << g.left() <<endl;
	int navWidth;
	if (horizontalScrollBar()->isVisible()) {
		navWidth = d->navPanel->sizeHint().width();
	}
	else {
		navWidth = leftMargin() + clipper()->width();
	}
	
	d->navPanel->setGeometry(
		frameWidth(),
		height() - horizontalScrollBar()->sizeHint().height()-frameWidth(),
		navWidth,
		horizontalScrollBar()->sizeHint().height()
	);

//	horizontalScrollBar()->move(d->navPanel->x()+d->navPanel->width()+20,horizontalScrollBar()->y());

//	horizontalScrollBar()->hide();
//	updateGeometry();
	updateScrollBars();
//	horizontalScrollBar()->move(d->navPanel->x()+d->navPanel->width()+20,horizontalScrollBar()->y());
#if 0 //prev impl.
	QRect g = d->navPanel->geometry();
//		kdDebug(44021) << "**********"<< g.top() << " " << g.left() <<endl;
	d->navPanel->setGeometry(
		frameWidth(),
		height() - horizontalScrollBar()->sizeHint().height()-frameWidth(),
		d->navPanel->sizeHint().width(), // - verticalScrollBar()->sizeHint().width() - horizontalScrollBar()->sizeHint().width(),
		horizontalScrollBar()->sizeHint().height()
	);
#endif
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
	if (!d->maximizeColumnsWidthOnShow.isEmpty()) {
		maximizeColumnsWidth(d->maximizeColumnsWidthOnShow);
		d->maximizeColumnsWidthOnShow.clear();
	}

	if (d->initDataContentsOnShow) {
		//full init
		d->initDataContentsOnShow = false;
		initDataContents();
	}
	else {
		//just update size
		QSize s(tableSize());
//	QRect r(cellGeometry(rows() - 1 + (isInsertingEnabled()?1:0), columns() - 1 ));
//	resizeContents(r.right() + 1, r.bottom() + 1);
		resizeContents(s.width(),s.height());
	}
	updateGeometries();

	//now we can ensure cell's visibility ( if there was such a call before show() )
	if (d->ensureCellVisibleOnShow!=QPoint(-1,-1)) {
		ensureCellVisible( d->ensureCellVisibleOnShow.x(), d->ensureCellVisibleOnShow.y() );
		d->ensureCellVisibleOnShow = QPoint(-1,-1); //reset the flag
	}
	updateNavPanelGeometry();
}

bool KexiTableView::dropsAtRowEnabled() const
{
	return d->dropsAtRowEnabled;
}

void KexiTableView::setDropsAtRowEnabled(bool set)
{
//	const bool old = d->dropsAtRowEnabled;
	if (!set)
		d->dragIndicatorLine = -1;
	if (d->dropsAtRowEnabled && !set) {
		d->dropsAtRowEnabled = false;
		update();
	}
	else {
		d->dropsAtRowEnabled = set;
	}
}

void KexiTableView::contentsDragMoveEvent(QDragMoveEvent *e)
{
	CHECK_DATA_;
	if (d->dropsAtRowEnabled) {
		QPoint p = e->pos();
		int row = rowAt(p.y());
		KexiTableItem *item = 0;
//		if (row==(rows()-1) && (p.y() % d->rowHeight) > (d->rowHeight*2/3) ) {
		if ((p.y() % d->rowHeight) > (d->rowHeight*2/3) ) {
			row++;
		}
		item = m_data->at(row);
		emit dragOverRow(item, row, e);
		if (e->isAccepted()) {
			if (d->dragIndicatorLine>=0 && d->dragIndicatorLine != row) {
				//erase old indicator
				updateRow(d->dragIndicatorLine);
			}
			if (d->dragIndicatorLine != row) {
				d->dragIndicatorLine = row;
				updateRow(d->dragIndicatorLine);
			}
		}
		else {
			if (d->dragIndicatorLine>=0) {
				//erase old indicator
				updateRow(d->dragIndicatorLine);
			}
			d->dragIndicatorLine = -1;
		}
	}
	else
		e->acceptAction(false);
/*	for(QStringList::Iterator it = d->dropFilters.begin(); it != d->dropFilters.end(); it++)
	{
		if(e->provides((*it).latin1()))
		{
			e->acceptAction(true);
			return;
		}
	}*/
//	e->acceptAction(false);
}

void KexiTableView::contentsDropEvent(QDropEvent *ev)
{
	CHECK_DATA_;
	if (d->dropsAtRowEnabled) {
		//we're no longer dragging over the table
		if (d->dragIndicatorLine>=0) {
			int row2update = d->dragIndicatorLine;
			d->dragIndicatorLine = -1;
			updateRow(row2update);
		}
		QPoint p = ev->pos();
		int row = rowAt(p.y());
		if ((p.y() % d->rowHeight) > (d->rowHeight*2/3) ) {
			row++;
		}
		KexiTableItem *item = m_data->at(row);
		KexiTableItem *newItem = 0;
		emit droppedAtRow(item, row, ev, newItem);
		if (newItem) {
			const int realRow = (row==d->curRow ? -1 : row);
			insertItem(newItem, realRow);
			setCursor(row, 0);
//			d->pCurrentItem = newItem;
		}
	}
}

void KexiTableView::viewportDragLeaveEvent( QDragLeaveEvent * )
{
	CHECK_DATA_;
	if (d->dropsAtRowEnabled) {
		//we're no longer dragging over the table
		if (d->dragIndicatorLine>=0) {
			int row2update = d->dragIndicatorLine;
			d->dragIndicatorLine = -1;
			updateRow(row2update);
		}
	}
}

void KexiTableView::updateCell(int row, int col)
{
//	kdDebug(44021) << "updateCell("<<row<<", "<<col<<")"<<endl;
	updateContents(cellGeometry(row, col));
/*	QRect r = cellGeometry(row, col);
	r.setHeight(r.height()+6);
	r.setTop(r.top()-3);
	updateContents();*/
}

void KexiTableView::updateRow(int row)
{
//	kdDebug(44021) << "updateRow("<<row<<")"<<endl;
	if (row < 0 || row >= rows())
		return;
	int leftcol = d->pTopHeader->sectionAt( d->pTopHeader->offset() );
//	int rightcol = d->pTopHeader->sectionAt( clipper()->width() );
	updateContents( QRect( columnPos( leftcol ), rowPos(row), clipper()->width(), rowHeight() ) ); //columnPos(rightcol)+columnWidth(rightcol), rowHeight() ) );
}

void KexiTableView::slotColumnWidthChanged( int, int, int )
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
	updateScrollBars();
	updateNavPanelGeometry();
}

void KexiTableView::slotSectionHandleDoubleClicked( int section )
{
	adjustColumnWidthToContents(section);
	slotColumnWidthChanged(0,0,0); //to update contents and redraw
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
	CHECK_DATA(0);
	int vcID = m_data->visibleColumnID( col );
	return vcID==-1 ? 0 : d->pTopHeader->sectionSize( vcID );
}

int KexiTableView::rowHeight() const
{
	return d->rowHeight;
}

int KexiTableView::columnPos(int col) const
{
	CHECK_DATA(0);
	//if this column is hidden, find first column before that is visible
	int c = QMIN(col, (int)m_data->columnsCount()-1), vcID = 0;
	while (c>=0 && (vcID=m_data->visibleColumnID( c ))==-1)
		c--;
	if (c<0)
		return 0;
	if (c==col)
		return d->pTopHeader->sectionPos(vcID);
	return d->pTopHeader->sectionPos(vcID)+d->pTopHeader->sectionSize(vcID);
}

int KexiTableView::rowPos(int row) const
{
	return d->rowHeight*row;
}

int KexiTableView::columnAt(int pos) const
{
	CHECK_DATA(-1);
	int r = d->pTopHeader->sectionAt(pos);
	if (r<0)
		return r;
	return m_data->globalColumnID( r );

//	if (r==-1)
//		kdDebug() << "columnAt("<<pos<<")==-1 !!!" << endl;
//	return r;
}

int KexiTableView::rowAt(int pos, bool ignoreEnd) const
{
	CHECK_DATA(-1);
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
	if ((rows()+ (isInsertingEnabled()?1:0) ) > 0 && columns() > 0) {
/*		kdDebug() << "tableSize()= " << columnPos( columns() - 1 ) + columnWidth( columns() - 1 ) 
			<< ", " << rowPos( rows()-1+(isInsertingEnabled()?1:0)) + d->rowHeight
//			+ QMAX(d->navPanel ? d->navPanel->height() : 0, horizontalScrollBar()->sizeHint().height())
			+ (d->navPanel->isVisible() ? QMAX( d->navPanel->height(), horizontalScrollBar()->sizeHint().height() ) :0 )
			+ margin() << endl;
*/
//		kdDebug()<< d->navPanel->isVisible() <<" "<<d->navPanel->height()<<" "
//		<<horizontalScrollBar()->sizeHint().height()<<" "<<rowPos( rows()-1+(isInsertingEnabled()?1:0))<<endl;

		int xx = horizontalScrollBar()->sizeHint().height()/2;

		QSize s( 
			columnPos( columns() - 1 ) + columnWidth( columns() - 1 ),
//			+ verticalScrollBar()->sizeHint().width(),
			rowPos( rows()-1+(isInsertingEnabled()?1:0) ) + d->rowHeight
			+ (horizontalScrollBar()->isVisible() ? 0 : horizontalScrollBar()->sizeHint().height())
			+ d->internal_bottomMargin
//				horizontalScrollBar()->sizeHint().height()/2
//			- /*d->bottomMargin */ horizontalScrollBar()->sizeHint().height()*3/2

//			+ ( (d->navPanel && d->navPanel->isVisible() && verticalScrollBar()->isVisible()
	//			&& !horizontalScrollBar()->isVisible()) 
		//		? horizontalScrollBar()->sizeHint().height() : 0)

//			+ QMAX( (d->navPanel && d->navPanel->isVisible()) ? d->navPanel->height() : 0, 
//				horizontalScrollBar()->isVisible() ? horizontalScrollBar()->sizeHint().height() : 0)

//			+ (d->navPanel->isVisible() 
//				? QMAX( d->navPanel->height(), horizontalScrollBar()->sizeHint().height() ) :0 )

//			- (horizontalScrollBar()->isVisible() ? horizontalScrollBar()->sizeHint().height() :0 )
			+ margin() 
//-2*d->rowHeight
		);

//		kdDebug() << rows()-1 <<" "<< (isInsertingEnabled()?1:0) <<" "<< (d->rowEditing?1:0) << " " <<  s << endl;
		return s;
//			+horizontalScrollBar()->sizeHint().height() + margin() );
	}
	return QSize(0,0);
}

int KexiTableView::rows() const
{
	CHECK_DATA(0);
	return m_data->count();
}

int KexiTableView::columns() const
{
	CHECK_DATA(0);
	return m_data->columns.count();
}

void KexiTableView::ensureCellVisible(int row, int col/*=-1*/)
{
	if (!isVisible()) {
		//the table is invisible: we can't ensure visibility now
		d->ensureCellVisibleOnShow = QPoint(row,col);
		return;
	}

	//quite clever: ensure the cell is visible:
	QRect r( columnPos(col==-1 ? d->curCol : col), rowPos(row) +(d->appearance.fullRowSelection?1:0), 
		columnWidth(col==-1 ? d->curCol : col), rowHeight());

/*	if (d->navPanel && horizontalScrollBar()->isHidden() && row == rows()-1) {
		//when cursor is moved down and navigator covers the cursor's area,
		//area is scrolled up
		if ((viewport()->height() - d->navPanel->height()) < r.bottom()) {
			scrollBy(0,r.bottom() - (viewport()->height() - d->navPanel->height()));
		}
	}*/

	if (d->navPanel && d->navPanel->isVisible() && horizontalScrollBar()->isHidden()) {
		//a hack: for visible navigator: increase height of the visible rect 'r'
		r.setBottom(r.bottom()+d->navPanel->height());
	}

	QPoint pcenter = r.center();
	ensureVisible(pcenter.x(), pcenter.y(), r.width()/2, r.height()/2);
//	updateContents();
//	updateNavPanelGeometry();
//	slotUpdate();
}

void KexiTableView::setCursor(int row, int col/*=-1*/, bool forceSet)
{
	int newrow = row;
	int newcol = col;

	if(rows() <= 0) {
		d->pVerticalHeader->setCurrentRow(-1);
		if (isInsertingEnabled()) {
			d->pCurrentItem=d->pInsertItem;
			newrow=0;
			if (col>=0)
				newcol=col;
			else
				newcol=0;
		}
		else {
			d->pCurrentItem=0;
			d->curRow=-1;
			d->curCol=-1;
			return;
		}
	}

	if(col>=0)
	{
		newcol = QMAX(0, col);
		newcol = QMIN(columns() - 1, newcol);
	}
	else {
		newcol = d->curCol; //no changes
		newcol = QMAX(0, newcol); //may not be < 0 !
	}
	newrow = QMAX( 0, row);
	newrow = QMIN( rows() - 1 + (isInsertingEnabled()?1:0), newrow);

//	d->pCurrentItem = itemAt(d->curRow);
//	kdDebug(44021) << "setCursor(): d->curRow=" << d->curRow << " oldRow=" << oldRow << " d->curCol=" << d->curCol << " oldCol=" << oldCol << endl;

	if ( forceSet || d->curRow != newrow || d->curCol != newcol )
	{
		kdDebug(44021) << "setCursor(): " <<QString("old:%1,%2 new:%3,%4").arg(d->curCol).arg(d->curRow).arg(newcol).arg(newrow) << endl;
		
		// cursor moved: get rid of editor
		if (d->pEditor) {
			if (!d->contentsMousePressEvent_dblClick) {
				if (!acceptEditor()) {
					return;
				}
				//update row num. again
				newrow = QMIN( rows() - 1 + (isInsertingEnabled()?1:0), newrow);
			}
		}

		if (d->curRow != newrow) {//update current row info
			setNavRowNumber(newrow);
//			d->navBtnPrev->setEnabled(newrow>0);
//			d->navBtnFirst->setEnabled(newrow>0);
//			d->navBtnNext->setEnabled(newrow<(rows()-1+(isInsertingEnabled()?1:0)));
//			d->navBtnLast->setEnabled(newrow!=(rows()-1));
		}

		// cursor moved to other row: end of row editing
		if (d->rowEditing && d->curRow != newrow) {
			if (!acceptRowEdit()) {
				//accepting failed: cancel setting the cursor
				return;
			}
			//update row number, because number of rows changed
			newrow = QMIN( rows() - 1 + (isInsertingEnabled()?1:0), newrow);
		}

		//change position
		int oldRow = d->curRow;
		int oldCol = d->curCol;
		d->curRow = newrow;
		d->curCol = newcol;

//		int cw = columnWidth( d->curCol );
//		int rh = rowHeight();
//		ensureVisible( columnPos( d->curCol ) + cw / 2, rowPos( d->curRow ) + rh / 2, cw / 2, rh / 2 );
//		center(columnPos(d->curCol) + cw / 2, rowPos(d->curRow) + rh / 2, cw / 2, rh / 2);
//	kdDebug(44021) << " contentsY() = "<< contentsY() << endl;

//js		if (oldRow > d->curRow)
//js			ensureVisible(columnPos(d->curCol), rowPos(d->curRow) + rh, columnWidth(d->curCol), rh);
//js		else// if (oldRow <= d->curRow)
//js		ensureVisible(columnPos(d->curCol), rowPos(d->curRow), columnWidth(d->curCol), rh);


		//show editor-dependent focus, if we're changing the current column
		if (oldCol>=0 && oldCol<columns() && d->curCol!=oldCol) {
			//find the editor for this column
			KexiTableEdit *edit = editor( oldCol );
			if (edit) {
				edit->hideFocus();
			}
		}

		//show editor-dependent focus, if needed
		//find the editor for this column
		editorShowFocus( d->curRow, d->curCol );

		updateCell( oldRow, oldCol );

		//quite clever: ensure the cell is visible:
		ensureCellVisible(d->curRow, d->curCol);

//		QPoint pcenter = QRect( columnPos(d->curCol), rowPos(d->curRow), columnWidth(d->curCol), rh).center();
//		ensureVisible(pcenter.x(), pcenter.y(), columnWidth(d->curCol)/2, rh/2);

//		ensureVisible(columnPos(d->curCol), rowPos(d->curRow) - contentsY(), columnWidth(d->curCol), rh);
		d->pVerticalHeader->setCurrentRow(d->curRow);
		updateCell( d->curRow, d->curCol );
		if (d->curCol != oldCol || d->curRow != oldRow ) //ensure this is also refreshed
			updateCell( oldRow, d->curCol );
		if (isInsertingEnabled() && d->curRow == rows()) {
			kdDebug(44021) << "NOW insert item is current" << endl;
			d->pCurrentItem = d->pInsertItem;
		}
		else {
			kdDebug(44021) << QString("NOW item at %1 (%2) is current").arg(d->curRow).arg((ulong)itemAt(d->curRow)) << endl;
//NOT EFFECTIVE!!!!!!!!!!!
			d->pCurrentItem = itemAt(d->curRow);
		}

		emit itemSelected(d->pCurrentItem);
		emit cellSelected(d->curCol, d->curRow);
	}

	if(d->initDataContentsOnShow) {
		d->cursorPositionSetExplicityBeforeShow = true;
	}
}

void KexiTableView::removeEditor()
{
	if (!d->pEditor)
		return;

//	d->pEditor->blockSignals(true);
	d->pEditor->hide();
//	delete d->pEditor;
	d->pEditor = 0;
	viewport()->setFocus();
}

bool KexiTableView::acceptEditor()
{
	CHECK_DATA(true);
	if (!d->pEditor || d->inside_acceptEditor)
		return true;

	d->inside_acceptEditor = true;//avoid recursion

	QVariant newval;
	KexiValidator::Result res = KexiValidator::Ok;
	QString msg, desc;
	bool setNull = false;
//	bool allow = true;
//	static const QString msg_NOT_NULL = i18n("\"%1\" column requires a value to be entered.");

	//autoincremented field can be omitted (left as null or empty) if we're inserting a new row
	const bool autoIncColumnCanBeOmitted = d->newRowEditing && d->pEditor->field()->isAutoIncrement();

	bool valueChanged = d->pEditor->valueChanged();
	bool editCurrentCellAgain = false;

	if (valueChanged) {
		if (d->pEditor->valueIsNull()) {//null value entered
			if (d->pEditor->field()->isNotNull() && !autoIncColumnCanBeOmitted) {
				kdDebug() << "KexiTableView::acceptEditor(): NULL NOT ALLOWED!" << endl;
				res = KexiValidator::Error;
				msg = KexiValidator::msgColumnNotEmpty().arg(d->pEditor->field()->captionOrName())
					+ "\n\n" + KexiValidator::msgYouCanImproveData();
				desc = i18n("The column's constraint is declared as NOT NULL.");
				editCurrentCellAgain = true;
	//			allow = false;
	//			removeEditor();
	//			return true;
			}
			else {
				kdDebug() << "KexiTableView::acceptEditor(): NULL VALUE WILL BE SET" << endl;
				//ok, just leave newval as NULL
				setNull = true;
			}
		}
		else if (d->pEditor->valueIsEmpty()) {//empty value entered
			if (d->pEditor->field()->hasEmptyProperty()) {
				if (d->pEditor->field()->isNotEmpty() && !autoIncColumnCanBeOmitted) {
					kdDebug() << "KexiTableView::acceptEditor(): EMPTY NOT ALLOWED!" << endl;
					res = KexiValidator::Error;
					msg = KexiValidator::msgColumnNotEmpty().arg(d->pEditor->field()->captionOrName())
						+ "\n\n" + KexiValidator::msgYouCanImproveData();
					desc = i18n("The column's constraint is declared as NOT EMPTY.");
					editCurrentCellAgain = true;
	//				allow = false;
	//				removeEditor();
	//				return true;
				}
				else {
					kdDebug() << "KexiTableView::acceptEditor(): EMPTY VALUE WILL BE SET" << endl;
				}
			}
			else {
				if (d->pEditor->field()->isNotNull() && !autoIncColumnCanBeOmitted) {
					kdDebug() << "KexiTableView::acceptEditor(): NEITHER NULL NOR EMPTY VALUE CAN BE SET!" << endl;
					res = KexiValidator::Error;
					msg = KexiValidator::msgColumnNotEmpty().arg(d->pEditor->field()->captionOrName())
						+ "\n\n" + KexiValidator::msgYouCanImproveData();
					desc = i18n("The column's constraint is declared as NOT EMPTY and NOT NULL.");
					editCurrentCellAgain = true;
//				allow = false;
	//				removeEditor();
	//				return true;
				}
				else {
					kdDebug() << "KexiTableView::acceptEditor(): NULL VALUE WILL BE SET BECAUSE EMPTY IS NOT ALLOWED" << endl;
					//ok, just leave newval as NULL
					setNull = true;
				}
			}
		}
	}//changed

	//try to get the value entered:
	if (res == KexiValidator::Ok) {
		if (!setNull && !valueChanged
			|| setNull && d->pCurrentItem->at(d->curCol).isNull()) {
			kdDebug() << "KexiTableView::acceptEditor(): VALUE NOT CHANGED." << endl;
			removeEditor();
			d->inside_acceptEditor = false;
			if (d->acceptsRowEditAfterCellAccepting || d->internal_acceptsRowEditAfterCellAccepting)
				acceptRowEdit();
			return true;
		}
		if (!setNull) {//get the new value 
			bool ok;
			newval = d->pEditor->value(ok);
			if (!ok) {
				kdDebug() << "KexiTableView::acceptEditor(): INVALID VALUE - NOT CHANGED." << endl;
				res = KexiValidator::Error;
//js: TODO get detailed info on why d->pEditor->value() failed
				msg = i18n("Entered value is invalid.")
					+ "\n\n" + KexiValidator::msgYouCanImproveData();
				editCurrentCellAgain = true;
//				removeEditor();
//				return true;
			}
		}

		//Check other validation rules:
		//1. check using validator
		KexiValidator *validator = m_data->column(d->curCol)->validator();
		if (validator) {
			res = validator->check(m_data->column(d->curCol)->field()->captionOrName(), 
				newval, msg, desc);
		}
	}

	//show the validation result if not OK:
	if (res == KexiValidator::Error) {
		if (desc.isEmpty())
			KMessageBox::sorry(this, msg);
		else
			KMessageBox::detailedSorry(this, msg, desc);
		editCurrentCellAgain = true;
//		allow = false;
	}
	else if (res == KexiValidator::Warning) {
		//js: todo: message!!!
		KMessageBox::messageBox(this, KMessageBox::Sorry, msg + "\n" + desc);
		editCurrentCellAgain = true;
	}

	if (res == KexiValidator::Ok) {
		//2. check using signal
		//bool allow = true;
//		emit aboutToChangeCell(d->pCurrentItem, newval, allow);
//		if (allow) {
		//send changes to the backend
		if (m_data->updateRowEditBuffer(d->pCurrentItem,d->curCol,newval)) {
			kdDebug() << "KexiTableView::acceptEditor(): ------ EDIT BUFFER CHANGED TO:" << endl;
			m_data->rowEditBuffer()->debug();
		} else {
			kdDebug() << "KexiTableView::acceptEditor(): ------ CHANGE FAILED in KexiTableViewData::updateRowEditBuffer()" << endl;
			res = KexiValidator::Error;

			//now: there might be called cancelEditor() in updateRowEditBuffer() handler,
			//if this is true, d->pEditor is NULL.

			if (d->pEditor && m_data->result()->column>=0 && m_data->result()->column<columns()) {
				//move to faulty column (if d->pEditor is not cleared)
				setCursor(d->curRow, m_data->result()->column);
			}
			if (!m_data->result()->msg.isEmpty()) {
				if (m_data->result()->desc.isEmpty())
					KMessageBox::sorry(this, m_data->result()->msg);
				else
					KMessageBox::detailedSorry(this, m_data->result()->msg, m_data->result()->desc);
			}
		}
	}

	if (res == KexiValidator::Ok) {
		removeEditor();
		emit itemChanged(d->pCurrentItem, d->curRow, d->curCol, d->pCurrentItem->at(d->curCol));
		emit itemChanged(d->pCurrentItem, d->curRow, d->curCol);
	}
	d->inside_acceptEditor = false;
	if (res == KexiValidator::Ok) {
		if (d->acceptsRowEditAfterCellAccepting || d->internal_acceptsRowEditAfterCellAccepting)
			acceptRowEdit();
		return true;
	}
	if (d->pEditor) {
		//allow to edit the cell again, (if d->pEditor is not cleared)
		startEditCurrentCell(newval.type()==QVariant::String ? newval.toString() : QString::null);
		d->pEditor->setFocus();
	}
	return false;
}

void KexiTableView::cancelEditor()
{
	if (!d->pEditor)
		return;

	removeEditor();
}

bool KexiTableView::acceptRowEdit()
{
	if (!d->rowEditing)
		return true;
	if (d->inside_acceptEditor) {
		d->internal_acceptsRowEditAfterCellAccepting = true;
		return true;
	}
	d->internal_acceptsRowEditAfterCellAccepting = false;
	if (!acceptEditor())
		return false;
	kdDebug() << "EDIT ROW ACCEPTING..." << endl;

	bool success = true;
//	bool allow = true;
//	int faultyColumn = -1; // will be !=-1 if cursor has to be moved to that column
	const bool inserting = d->newRowEditing;
//	QString msg, desc;
//	bool inserting = d->pInsertItem && d->pInsertItem==d->pCurrentItem;

	if (m_data->rowEditBuffer()->isEmpty() && !d->newRowEditing) {
/*		if (d->newRowEditing) {
			cancelRowEdit();
			kdDebug() << "-- NOTHING TO INSERT!!!" << endl;
			return true;
		}
		else {*/
			kdDebug() << "-- NOTHING TO ACCEPT!!!" << endl;
//		}
	}
	else {//not empty edit buffer or new row to insert:
		if (d->newRowEditing) {
//			emit aboutToInsertRow(d->pCurrentItem, m_data->rowEditBuffer(), success, &faultyColumn);
//			if (success) {
			kdDebug() << "-- INSERTING: " << endl;
			m_data->rowEditBuffer()->debug();
			success = m_data->saveNewRow(*d->pCurrentItem);
//				if (!success) {
//				}
//			}
		}
		else {
//			emit aboutToUpdateRow(d->pCurrentItem, m_data->rowEditBuffer(), success, &faultyColumn);
			if (success) {
				//accept changes for this row:
				kdDebug() << "-- UPDATING: " << endl;
				m_data->rowEditBuffer()->debug();
				success = m_data->saveRowChanges(*d->pCurrentItem);//, &msg, &desc, &faultyColumn);
//				if (!success) {
//				}
			}
		}
	}

	if (success) {
		//editing is finished:
		d->rowEditing = false;
		d->newRowEditing = false;
		//indicate on the vheader that we are not editing
		d->pVerticalHeader->setEditRow(-1);
		//redraw
		updateRow(d->curRow);

		kdDebug() << "EDIT ROW ACCEPTED:" << endl;
		/*debug*/itemAt(d->curRow);

		if (inserting) {
//			emit rowInserted(d->pCurrentItem);
			//update navigator's data
			setNavRowCount(rows());
		}
		else {
//			emit rowUpdated(d->pCurrentItem);
		}

		emit rowEditTerminated(d->curRow);
	}
	else {
//		if (!allow) {
//			kdDebug() << "INSERT/EDIT ROW - DISALLOWED by signal!" << endl;
//		}
//		else {
//			kdDebug() << "EDIT ROW - ERROR!" << endl;
//		}
		if (m_data->result()->column>=0 && m_data->result()->column<columns()) {
			//move to faulty column
			setCursor(d->curRow, m_data->result()->column);
		}
		if (m_data->result()->desc.isEmpty())
			KMessageBox::sorry(this, m_data->result()->msg);
		else
			KMessageBox::detailedSorry(this, m_data->result()->msg, m_data->result()->desc);

		//edit this cell
		startEditCurrentCell();
	}

	return success;
}

void KexiTableView::slotRowRepaintRequested(KexiTableItem& item)
{
	updateRow( m_data->findRef(&item) );
}

void KexiTableView::cancelRowEdit()
{
	CHECK_DATA_;
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
//TODO: still doesn't repaint properly!!
		QSize s(tableSize());
		resizeContents(s.width(), s.height());
		d->pVerticalHeader->update();
		//--no cancel action is needed for datasource, 
		//  because the row was not yet stored.
	}

	m_data->clearRowEditBuffer();
	updateRow(d->curRow);
	
//! \todo (js): cancel changes for this row!
	kdDebug(44021) << "EDIT ROW CANCELLED." << endl;

	emit rowEditTerminated(d->curRow);
}

bool KexiTableView::acceptsRowEditAfterCellAccepting() const
{
	return d->acceptsRowEditAfterCellAccepting;
}

void KexiTableView::setAcceptsRowEditAfterCellAccepting(bool set)
{
	d->acceptsRowEditAfterCellAccepting = set;
}

/*void KexiTableView::setInsertionPolicy(InsertionPolicy policy)
{
	d->insertionPolicy = policy;
//	updateContextMenu();
}

KexiTableView::InsertionPolicy KexiTableView::insertionPolicy() const
{
	return d->insertionPolicy;
}*/

void KexiTableView::setDeletionPolicy(DeletionPolicy policy)
{
	d->deletionPolicy = policy;
//	updateContextMenu();
}

KexiTableView::DeletionPolicy KexiTableView::deletionPolicy() const
{
	return d->deletionPolicy;
}

#if 0
bool KexiTableView::updateContextMenu()
{
  // delete d->pContextMenu;
  //  d->pContextMenu = 0L;
//  d->pContextMenu->clear();
//	if(d->pCurrentItem && d->pCurrentItem->isInsertItem())
//	return;

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
#endif

//(js) unused
void KexiTableView::slotAutoScroll()
{
	kdDebug(44021) << "KexiTableView::slotAutoScroll()" <<endl;
	if (!d->needAutoScroll)
		return;

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
	for(int col=0; col < columns(); col++)
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
			for(int col=0; col < columns(); col++)
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

KexiTableRM* KexiTableView::verticalHeader() const
{
	return d->pVerticalHeader; 
}

QString KexiTableView::columnCaption(int colNum) const
{
	return d->pTopHeader->label(colNum);
}

KexiDB::Field* KexiTableView::field(int colNum) const
{
	if (!m_data || !m_data->column(colNum))
		return 0;
	return m_data->column(colNum)->field();
}

void KexiTableView::adjustColumnWidthToContents(int colNum)
{
	CHECK_DATA_;
	if (columns()<=colNum || colNum < -1)
		return;

	if (colNum==-1) {
//		const int cols = columns();
		for (int i=0; i<columns(); i++)
			adjustColumnWidthToContents(i);
		return;
	}

	KexiCellEditorFactoryItem *item = KexiCellEditorFactory::item( columnType(colNum) );
	if (!item)
		return;
	QFontMetrics fm(font());
	int maxw = fm.width( d->pTopHeader->label( colNum ) );
//	int start = rowAt(contentsY());
//	int end = QMAX( start, rowAt( contentsY() + viewport()->height() - 1 ) );
//	for (int i=start; i<=end; i++) {

//! \todo js: this is NOT EFFECTIVE for big data sets!!!!

	KexiTableEdit *ed = editor( colNum );
//	KexiDB::Field *f = m_data->column( colNum )->field;
	if (ed) {
//		KexiDB::Field *f = m_data->column(colNum)->field;
		for (QPtrListIterator<KexiTableItem> it = m_data->iterator(); it.current(); ++it) {
			maxw = QMAX( maxw, ed->widthForValue( it.current()->at( colNum ), fm ) );
//			maxw = QMAX( maxw, item->widthForValue( *f, it.current()->at( colNum ), fm ) );
		}
		maxw += (fm.width("  ") + ed->leftMargin() + ed->rightMargin());
	}
	if (maxw < KEXITV_MINIMUM_COLUMN_WIDTH )
		maxw = KEXITV_MINIMUM_COLUMN_WIDTH; //not too small
	setColumnWidth( colNum, maxw );
}

void KexiTableView::setColumnWidth(int colNum, int width)
{
	if (columns()<=colNum || colNum < 0)
		return;
	const int oldWidth = d->pTopHeader->sectionSize( colNum );
	d->pTopHeader->resizeSection( colNum, width );
	slotTopHeaderSizeChange( colNum, oldWidth, d->pTopHeader->sectionSize( colNum ) );
}

void KexiTableView::maximizeColumnsWidth( const QValueList<int> &columnList )
{
	if (!isVisible()) {
		d->maximizeColumnsWidthOnShow += columnList;
		return;
	}
	if (width() <= d->pTopHeader->headerWidth())
		return;
	//sort the list and make it unique
	QValueList<int>::const_iterator it;
	QValueList<int> cl, sortedList = columnList;
	qHeapSort(sortedList);
	int i=-999;

	for (it=sortedList.constBegin(); it!=sortedList.end(); ++it) {
		if (i!=(*it)) {
			cl += (*it);
			i = (*it);
		}
	}
	//resize
	int sizeToAdd = (width() - d->pTopHeader->headerWidth()) / cl.count() - verticalHeader()->width();
	if (sizeToAdd<=0)
		return;
	for (it=cl.constBegin(); it!=cl.end(); ++it) {
		int w = d->pTopHeader->sectionSize(*it);
		if (w>0) {
			d->pTopHeader->resizeSection(*it, w+sizeToAdd);
		}
	}
	updateContents();
	editorShowFocus( d->curRow, d->curCol );
}

void KexiTableView::adjustHorizontalHeaderSize()
{
	d->pTopHeader->adjustHeaderSize();
}

void KexiTableView::setColumnStretchEnabled( bool set, int colNum )
{
	d->pTopHeader->setStretchEnabled( set, colNum );
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

//void KexiTableView::setBackgroundAltering(bool altering) { d->bgAltering = altering; }
//bool KexiTableView::backgroundAltering()  const { return d->bgAltering; }

void KexiTableView::setEditableOnDoubleClick(bool set) { d->editOnDoubleClick = set; }
bool KexiTableView::editableOnDoubleClick() const { return d->editOnDoubleClick; }

/*void KexiTableView::setEmptyAreaColor(const QColor& c)
{
	d->emptyAreaColor = c;
}

QColor KexiTableView::emptyAreaColor() const
{
	return d->emptyAreaColor;
}

bool KexiTableView::fullRowSelectionEnabled() const
{
	return d->fullRowSelectionEnabled;
}*/
/*
void KexiTableView::setFullRowSelectionEnabled(bool set)
{
	if (d->fullRowSelectionEnabled == set)
		return;
	if (set) {
		d->rowHeight -= 1;
	}
	else {
		d->rowHeight += 1;
	}
	d->fullRowSelectionEnabled = set;
	if (d->pVerticalHeader)
		d->pVerticalHeader->setCellHeight(d->rowHeight);
	if (d->pTopHeader) {
		setMargins(
			QMIN(d->pTopHeader->sizeHint().height(), d->rowHeight),
			d->pTopHeader->sizeHint().height(), 0, 0);
	}
	setFont(font());//update
}
*/
bool KexiTableView::verticalHeaderVisible() const
{
	return d->pVerticalHeader->isVisible();
}

void KexiTableView::setVerticalHeaderVisible(bool set)
{
	int left_width;
	if (set) {
		d->pVerticalHeader->show();
		left_width = QMIN(d->pTopHeader->sizeHint().height(), d->rowHeight);
	}
	else {
		d->pVerticalHeader->hide();
		left_width = 0;
	}
	setMargins( left_width, horizontalHeaderVisible() ? d->pTopHeader->sizeHint().height() : 0, 0, 0);
}

bool KexiTableView::horizontalHeaderVisible() const
{
	return d->pTopHeader->isVisible();
}

void KexiTableView::setHorizontalHeaderVisible(bool set)
{
	int top_height;
	if (set) {
		d->pTopHeader->show();
		top_height = d->pTopHeader->sizeHint().height();
	}
	else {
		d->pTopHeader->hide();
		top_height = 0;
	}
	setMargins( verticalHeaderVisible() ? d->pVerticalHeader->width() : 0, top_height, 0, 0);
}

void KexiTableView::triggerUpdate()
{
//	kdDebug(44021) << "KexiTableView::triggerUpdate()" << endl;
//	if (!d->pUpdateTimer->isActive())
		d->pUpdateTimer->start(20, true);
//		d->pUpdateTimer->start(200, true);
}

int KexiTableView::columnType(int col) const
{
	return (m_data && col>=0 && col<columns()) ? m_data->column(col)->field()->type() : KexiDB::Field::InvalidType;
}

bool KexiTableView::columnEditable(int col) const
{
	return (m_data && col>=0 && col<columns()) ? !m_data->column(col)->readOnly() : false;
}

QVariant KexiTableView::columnDefaultValue(int /*col*/) const
{
	return QVariant(0);
//TODO(js)	
//	return m_data->columns[col].defaultValue;
}

void KexiTableView::setReadOnly(bool set)
{
	if (isReadOnly() == set || (m_data && m_data->isReadOnly() && !set))
		return; //not allowed!
	d->readOnly = (set ? 1 : 0);
	if (set)
		setInsertingEnabled(false);
	update();
	emit reloadActions();
}

bool KexiTableView::isReadOnly() const
{
	if (d->readOnly == 1 || d->readOnly == 0)
		return (bool)d->readOnly;
	CHECK_DATA(true);
	return m_data->isReadOnly();
}

void KexiTableView::setInsertingEnabled(bool set)
{
	if (isInsertingEnabled() == set || (m_data && !m_data->isInsertingEnabled() && set))
		return; //not allowed!
	d->insertingEnabled = (set ? 1 : 0);
	d->navBtnNew->setEnabled(set);
	d->pVerticalHeader->showInsertRow(set);
	if (set)
		setReadOnly(false);
	update();
	emit reloadActions();
}

bool KexiTableView::isInsertingEnabled() const
{
	if (d->insertingEnabled == 1 || d->insertingEnabled == 0)
		return (bool)d->insertingEnabled;
	CHECK_DATA(true);
	return m_data->isInsertingEnabled();
}

bool KexiTableView::isEmptyRowInsertingEnabled() const
{
	return d->emptyRowInsertingEnabled;//js && isInsertingEnabled();
}

void KexiTableView::setEmptyRowInsertingEnabled(bool set)
{
	d->emptyRowInsertingEnabled = set;
	emit reloadActions();
}

bool KexiTableView::isDeleteEnabled() const
{
	return d->deletionPolicy != NoDelete && !isReadOnly();
}

bool KexiTableView::rowEditing() const
{
	return d->rowEditing;
}

/*bool KexiTableView::navigatorEnabled() const
{
	return d->navigatorEnabled;
}
	
void KexiTableView::setNavigatorEnabled(bool set)
{
	if (d->navigatorEnabled==set)
		return;
	d->navigatorEnabled = set;
	if(!set)
		d->navPanel->hide();
	else
		d->navPanel->show();
}*/

bool KexiTableView::contextMenuEnabled() const
{
	return d->contextMenuEnabled;
}

void KexiTableView::setContextMenuEnabled(bool set)
{
	d->contextMenuEnabled = set;
}

void KexiTableView::setHBarGeometry( QScrollBar & hbar, int x, int y, int w, int h )
{
/*todo*/
	kdDebug(44021)<<"KexiTableView::setHBarGeometry"<<endl;
	if (d->appearance.navigatorEnabled) {
		hbar.setGeometry( x + d->navPanel->width(), y, w - d->navPanel->width(), h );
	}
	else
	{
		hbar.setGeometry( x , y, w, h );
	}
}

void KexiTableView::setFilteringEnabled(bool set)
{
	d->filteringEnabled = set;
}

bool KexiTableView::filteringEnabled() const
{
	return d->filteringEnabled;
}

void KexiTableView::setSpreadSheetMode()
{
	d->spreadSheetMode = true;
	Appearance a = d->appearance;
//	setNavigatorEnabled( false );
	setSortingEnabled( false );
	setInsertingEnabled( false );
	setAcceptsRowEditAfterCellAccepting( true );
	setFilteringEnabled( false );
	setEmptyRowInsertingEnabled( true );
	a.navigatorEnabled = false;
	setAppearance( a );
}

bool KexiTableView::spreadSheetMode() const
{
	return d->spreadSheetMode;
}

bool KexiTableView::scrollbarToolTipsEnabled() const
{
	return d->scrollbarToolTipsEnabled;
}

void KexiTableView::setScrollbarToolTipsEnabled(bool set)
{
	d->scrollbarToolTipsEnabled=set;
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
	setNavRowNumber(r);
//	d->navRowNumber->setText( QString::number( r+1 ) );
	selectRow( r );
}

void KexiTableView::updateRowCountInfo()
{
	d->navRowNumberValidator->setRange(1,rows()+(isInsertingEnabled()?1:0));
	kdDebug() << QString("updateRowCountInfo(): d->navRowNumberValidator: bottom=%1 top=%2").arg(d->navRowNumberValidator->bottom()).arg(d->navRowNumberValidator->top()) << endl;
	setNavRowCount(rows());
//	d->navRowCount->setText(QString::number(rows()));
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
	if (d->rowEditing) {
		if (!acceptRowEdit())
			return;
	}
/*	if (d->pEditor) {
		//already editing!
		d->pEditor->setFocus();
		return;
	}*/
	setFocus();
	selectRow(rows());
	startEditCurrentCell();
}

bool KexiTableView::eventFilter( QObject *o, QEvent *e )
{
	//don't allow to stole key my events by others:
//	kdDebug() << "spontaneous " << e->spontaneous() << " type=" << e->type() << endl;

	if (e->type()==QEvent::KeyPress) {
		if (e->spontaneous() /*|| e->type()==QEvent::AccelOverride*/) {
			QKeyEvent *ke = static_cast<QKeyEvent*>(e);
			int k = ke->key();
			//cell editor's events:
			//try to handle the event @ editor's level
			KexiTableEdit *edit = editor( d->curCol );
			if (edit && edit->handleKeyPress(ke, d->pEditor==edit)) {
				ke->accept();
				return true;
			}
			else if (d->pEditor && (o==d->pEditor || o==d->pEditor->view())) {
				if (   (k==Key_Tab && (k==NoButton || k==ShiftButton))
					|| (k==Key_Enter || k==Key_Return || k==Key_Up || k==Key_Down) 
					|| (k==Key_Left && d->pEditor->cursorAtStart())
					|| (k==Key_Right && d->pEditor->cursorAtEnd())
					) {
					keyPressEvent(ke);
					if (ke->isAccepted())
						return true;
				}
			}
			else if (e->type()==QEvent::KeyPress && (o==this /*|| o==viewport()*/)) {
				keyPressEvent(ke);
				if (ke->isAccepted())
					return true;
			}
		}
	}
	else if (o==horizontalScrollBar()) {
		if ((e->type()==QEvent::Show && !horizontalScrollBar()->isVisible()) 
			|| (e->type()==QEvent::Hide && horizontalScrollBar()->isVisible())) {
			QSize s(tableSize());
			resizeContents(s.width(), s.height());
		}
	}
	else if (e->type()==QEvent::Leave) {
		if (o==viewport() && d->appearance.rowHighlightingEnabled) {
			if (d->highlightedRow>=0)
				updateRow(d->highlightedRow);
			d->highlightedRow = -1;
		}
	}
/*	else if (e->type()==QEvent::FocusOut && o->inherits("QWidget")) {
		//hp==true if currently focused widget is a child of this table view
		const bool hp = Kexi::hasParent( static_cast<QWidget*>(o), focusWidget());
		if (!hp && Kexi::hasParent( this, static_cast<QWidget*>(o))) {
			//accept row editing if focus is moved to foreign widget 
			//(not a child, like eg. editor) from one of our table view's children
			//or from table view itself
			if (!acceptRowEdit()) {
				static_cast<QWidget*>(o)->setFocus();
				return true;
			}
		}
	}*/
	return QScrollView::eventFilter(o,e);
}

void KexiTableView::vScrollBarValueChanged(int v)
{
	if (!d->vScrollBarValueChanged_enabled)
		return;
	kdDebug(44021) << "VCHANGED: " << v << " / " << horizontalScrollBar()->maxValue() <<  endl;
	
//	updateContents();
	d->pVerticalHeader->update(); //<-- dirty but needed

	if (d->scrollbarToolTipsEnabled) {
		QRect r = verticalScrollBar()->sliderRect();
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
	//update bottom view region
	if (d->navPanel && (contentsHeight() - contentsY() - clipper()->height()) <= QMAX(d->rowHeight,d->navPanel->height())) {
		slotUpdate();
		triggerUpdate();
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

void KexiTableView::slotTopHeaderSizeChange( 
	int /*section*/, int /*oldSize*/, int /*newSize*/ )
{
	editorShowFocus( d->curRow, d->curCol );
}

const QVariant* KexiTableView::bufferedValueAt(int col)
{
	if (d->rowEditing && m_data->rowEditBuffer())
	{
		KexiTableViewColumn* tvcol = m_data->column(col);
		if (tvcol->isDBAware) {
//			QVariant *cv = m_data->rowEditBuffer()->at( *static_cast<KexiDBTableViewColumn*>(tvcol)->field );
			const QVariant *cv = m_data->rowEditBuffer()->at( *tvcol->fieldinfo );
			if (cv)
				return cv;

			return &d->pCurrentItem->at(col);
		}
		const QVariant *cv = m_data->rowEditBuffer()->at( tvcol->field()->name() );
		if (cv)
			return cv;
	}
	return &d->pCurrentItem->at(col);
}

void KexiTableView::setBottomMarginInternal(int pixels)
{
	d->internal_bottomMargin = pixels;
}

void KexiTableView::paletteChange( const QPalette & )
{
}

KexiTableView::Appearance KexiTableView::appearance() const
{
	return d->appearance;
}

void KexiTableView::setAppearance(const Appearance& a)
{
//	if (d->appearance.fullRowSelection != a.fullRowSelection) {
	if (a.fullRowSelection) {
		d->rowHeight -= 1;
	}
	else {
		d->rowHeight += 1;
	}
	if (d->pVerticalHeader)
		d->pVerticalHeader->setCellHeight(d->rowHeight);
	if (d->pTopHeader) {
		setMargins(
			QMIN(d->pTopHeader->sizeHint().height(), d->rowHeight),
			d->pTopHeader->sizeHint().height(), 0, 0);
	}
//	}

//	if (d->appearance.navigatorEnabled != a.navigatorEnabled) {
	if(!a.navigatorEnabled)
		d->navPanel->hide();
	else
		d->navPanel->show();
//	}

	d->highlightedRow = -1;
//TODO is setMouseTracking useful for other purposes?
	viewport()->setMouseTracking(a.rowHighlightingEnabled);

	d->appearance = a;

	setFont(font()); //this also updates contents
}

int KexiTableView::highlightedRow() const
{
	return d->highlightedRow;
}

void KexiTableView::setHighlightedRow(int row)
{
	if (row!=-1) {
		row = QMAX( 0, QMIN(rows()-1, row) );
		ensureCellVisible(row, -1);
	}
	updateRow(d->highlightedRow);
	if (d->highlightedRow == row)
		return;
	d->highlightedRow = row;
	if (d->highlightedRow!=-1)
		updateRow(d->highlightedRow);
}

KexiTableItem *KexiTableView::highlightedItem() const
{
	return m_data->at(d->highlightedRow);
}

#include "kexitableview.moc"

