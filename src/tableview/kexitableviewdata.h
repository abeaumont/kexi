/* This file is part of the KDE project
   Copyright (C) 2002   Lucijan Busch <lucijan@gmx.at>
   Daniel Molkentin <molkentin@kde.org>
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

#ifndef KEXITABLEVIEWDATA_H
#define KEXITABLEVIEWDATA_H

#include <qptrlist.h>
#include <qvariant.h>
#include <qvaluevector.h>
#include <qstring.h>
#include <qobject.h>

#include "kexitableitem.h"

#include <kexidb/error.h>

namespace KexiDB {
class Field;
class QuerySchema;
class RowEditBuffer;
class Cursor;
}

class KexiValidator;


/*! Single column definition. */
class KEXIDATATABLE_EXPORT KexiTableViewColumn {
	public:
		typedef QPtrList<KexiTableViewColumn> List;
		typedef QPtrListIterator<KexiTableViewColumn> ListIterator;

		/*! Not db-aware ctor. if \a owner is true, the field \a will be owned by this column,
		 so you shouldn't care about destroying this field. */
		KexiTableViewColumn(KexiDB::Field& f, bool owner = false);

		/*! Convenience ctor, like above. The field is created using specifed parameters that are 
		 equal to these accepted nby KexiDB::Field ctor. The column will be the owner 
		 of this automatically generated field.
		 */
		KexiTableViewColumn(const QString& name, KexiDB::Field::Type ctype,
			uint cconst=KexiDB::Field::NoConstraints,
			uint options = KexiDB::Field::NoOptions,
			uint length=0, uint precision=0,
			QVariant defaultValue=QVariant(),
			const QString& caption = QString::null,
			const QString& description = QString::null,
			uint width = 0);

		//! Db-aware version.
		KexiTableViewColumn(const KexiDB::QuerySchema &query, KexiDB::Field& f);

		virtual ~KexiTableViewColumn();

		virtual bool acceptsFirstChar(const QChar& ch) const;

		/*! \return true is the column is read-only
		 For db-aware column this can depend on whether the column 
		 is in parent table of this query. \sa setReadOnly() */
		inline bool readOnly() const { return m_readOnly; }

		//! forces readOnly flag to be set to \a ro
		inline void setReadOnly(bool ro) { m_readOnly=ro; }

		//! returns whatever is available: field's caption or field's alias (from query) 
		//! or finally - field's name
		inline QString nameOrCaption() const { return m_nameOrCaption; }

		/*! Assigns validator \a v for this column. 
		 If the validator has no parent obejct, it will be owned by the column, 
		 so you shouldn't care about destroying it. */
		void setValidator( KexiValidator* v );

		//! \return validator assigned for this column of 0 if there is no validator assigned.
		KexiValidator* validator() const { return m_validator; }

		KexiDB::Field* field;

		bool isDBAware : 1; //!< true if data is stored in DB, not only in memeory

/*		QString caption;
		int type; //!< one of KexiDB::Field::Type
		uint width;
*/
//		bool isNull() const;
		
/*		virtual QString caption() const;
		virtual void setCaption(const QString& c);
	*/	
	protected:
		//! special ctor that do not allocate d member;
		KexiTableViewColumn(bool);

		QString m_nameOrCaption;

		KexiValidator* m_validator;

		bool m_readOnly : 1;
		bool m_fieldOwned : 1;
		
	friend class KexiTableViewData;
};


/*! List of column definitions. */
//typedef QValueVector<KexiTableViewColumn> KexiTableViewColumnList;

typedef QPtrList<KexiTableItem> KexiTableViewDataBase;

/*! Reimplements QPtrList to allow configurable sorting.
	Original author: Till Busch.
	Reimplemented by Jaroslaw Staniek.

	Notes:
	- use QPtrList::inSort ( const type * item ) to insert an item if you want 
		to maintain sorting (it is very slow!)
	- An alternative, especially if you have lots of items, is to simply QPtrList::append() 
		or QPtrList::insert() them and then use single sort().

	\sa QPtrList.
*/
class KEXIDATATABLE_EXPORT KexiTableViewData : public QObject, public KexiTableViewDataBase
{
	Q_OBJECT

public: 
	KexiTableViewData();

	KexiTableViewData(KexiDB::Cursor *c); //db-aware version

//	KexiTableViewData(KexiTableViewColumnList* cols);
	~KexiTableViewData();
//js	void setSorting(int key, bool order=true, short type=1);

	/*! Sets sorting for \a column. If \a column is -1, sorting is disabled. */
	void setSorting(int column, bool ascending=true);

	/*! \return the column number by which the data is sorted, 
	 or -1 if sorting is disabled. */
	int sortedColumn() const { return m_key; }

	/*! \return true if ascending sort order is set, or false if sorting is descending.
	 This is independant of whether data is sorted now.
	*/
	bool sortingAscending() const { return m_order == 1; }

	/*! Adds column \a col. 
	 Warning: \a col will be owned by this object, and deleted on its destruction. */
	void addColumn( KexiTableViewColumn* col );

	virtual bool isDBAware();

	inline KexiDB::Cursor* cursor() const { return m_cursor; }

	uint columnsCount() const { return columns.count(); }

	inline KexiTableViewColumn* column(uint c) { return columns.at(c); }

	/*! Columns information */
	KexiTableViewColumn::List columns;

	virtual bool isReadOnly() const { return m_readOnly; }
	virtual void setReadOnly(bool set) { m_readOnly = set; }

	virtual bool isInsertingEnabled() const { return m_insertingEnabled; }
	virtual void setInsertingEnabled(bool set) { m_insertingEnabled = set; }

	/*! Clears and initializes internal row edit buffer for incoming editing. 
	 Creates buffer using KexiDB::RowEditBuffer(false) (false means not db-aware type) id our data is not db-aware,
	 or db-aware buffer if data is db-aware (isDBAware()==true).
	 \sa KexiDB::RowEditBuffer
	*/
	void clearRowEditBuffer();

	/*! Updates internal row edit buffer: currently edited column (number \colnum) 
	 has now assigned new value of \a newval.
	 Uses column's caption to address the column in buffer 
	 if the buffer is of simple type, or db-aware buffer if (isDBAware()==true).
	 (then fields are addressed with KexiDB::Field, instead of caption strings).
	 \sa KexiDB::RowEditBuffer */
	bool updateRowEditBuffer(KexiTableItem *item, int colnum, QVariant newval);

	inline KexiDB::RowEditBuffer* rowEditBuffer() const { return m_pRowEditBuffer; }

	/*! \return last operation's result information (always not null). */
	inline KexiDB::ResultInfo* result() { return &m_result; }

	bool saveRowChanges(KexiTableItem& item);

	bool saveNewRow(KexiTableItem& item);

	bool deleteRow(KexiTableItem& item);

signals:
	/*! Emitted before change of the single, currently edited cell.
	 Connect this signal to your slot and set \a allow value to false 
	 to disallow the change. */
	void aboutToChangeCell(KexiTableItem *, int colnum, QVariant newValue,
		KexiDB::ResultInfo* result);

	/*! Emited before inserting of a new, current row.
	 Connect this signal to your slot and set \a result->success to false 
	 to disallow this inserting. */
	void aboutToInsertRow(KexiTableItem *, KexiDB::ResultInfo* result);

	/*! Emited before changing of an edited, current row.
	 Connect this signal to your slot and set \a result->success to false 
	 to disallow this change. */
	void aboutToUpdateRow(KexiTableItem *, KexiDB::RowEditBuffer* buffer,
		KexiDB::ResultInfo* result);

	/*! Emited before deleting of a current row.
	 Connect this signal to your slot and set \a result->success to false 
	 to disallow this deleting. */
	void aboutToDeleteRow(KexiTableItem& item, KexiDB::ResultInfo* result);

	void rowUpdated(KexiTableItem*); //!< Current row has been updated
	void rowInserted(KexiTableItem*); //!< A row has been inserted
	void rowDeleted(); //!< Current row has been deleted

protected:
	virtual int compareItems(Item item1, Item item2);
	int cmpStr(Item item1, Item item2);
	int cmpInt(Item item1, Item item2);

	//! internal: for saveRowChanges() and saveNewRow()
	bool saveRow(KexiTableItem& item, bool insert);

	int			m_key;
	short		m_order;
	short		m_type;
	static unsigned short charTable[];
	KexiDB::RowEditBuffer *m_pRowEditBuffer;
	KexiDB::Cursor *m_cursor;

	//! used to faster lookup columns of simple type (not dbaware)
//	QDict<KexiTableViewColumn> *m_simpleColumnsByName;

	KexiDB::ResultInfo m_result;

	bool m_readOnly : 1;
	bool m_insertingEnabled : 1;

	int (KexiTableViewData::*cmpFunc)(void *, void *);
};

#endif
