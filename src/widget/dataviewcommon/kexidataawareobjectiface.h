/* This file is part of the KDE project
   Copyright (C) 2005-2015 Jarosław Staniek <staniek@kde.org>

   Based on KexiTableView code.
   Copyright (C) 2002 Till Busch <till@bux.at>
   Copyright (C) 2003 Lucijan Busch <lucijan@gmx.at>
   Copyright (C) 2003 Daniel Molkentin <molkentin@kde.org>
   Copyright (C) 2003 Joseph Wenninger <jowenn@kde.org>

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

#ifndef KEXIDATAAWAREOBJECTINTERFACE_H
#define KEXIDATAAWAREOBJECTINTERFACE_H

#include "kexidataviewcommon_export.h"
#include <core/kexisearchandreplaceiface.h>
#include <core/kexidataiteminterface.h>
#include <kexiutils/KexiContextMessage.h>

#include <KDbTableViewData>

#include <QPointer>
#include <QList>
#include <QDebug>

class QHeaderView;
class QScrollBar;
class QMenu;
class KDbRecordData;
class KDbTableViewData;
class KexiRecordNavigatorIface;

//! default column width in pixels
#define KEXI_DEFAULT_DATA_COLUMN_WIDTH 120

//! \brief The KexiDataAwareObjectInterface is an interface for record-based data object.
/** This interface is implemented by KexiTableScrollArea and KexiFormView
 and used by KexiDataAwareView. If yu're implementing this interface,
 add KEXI_DATAAWAREOBJECTINTERFACE convenience macro just after Q_OBJECT.

 You should add following code to your destructor so data is deleted:
 \code
  if (m_owner)
    delete m_data;
  m_data = 0;
 \endcode
 This is not performed in KexiDataAwareObjectInterface because you may need
 to access m_data in your desctructor.
*/
class KEXIDATAVIEWCOMMON_EXPORT KexiDataAwareObjectInterface
{
public:
    KexiDataAwareObjectInterface();
    virtual ~KexiDataAwareObjectInterface();

    /*! Sets data for this object. if \a owner is true, the object will own
     \a data and therefore will be destroyed when needed, else: \a data is (possibly) shared and
     not owned by the widget.
     If widget already has _different_ data object assigned (and owns this data),
     old data is destroyed before new assignment.
     */
    void setData(KDbTableViewData *data, bool owner = true);

    /*! \return data structure displayed for this object */
    inline KDbTableViewData *data() const {
        return m_data;
    }

    /*! \return currently selected column number or -1. */
    inline int currentColumn() const {
        return m_curCol;
    }

    /*! \return currently selected row number or -1. */
    inline int currentRow() const {
        return m_curRow;
    }

    /*! \return last row visible on the screen (counting from 0).
     The returned value is guaranteed to be smaller or equal to currentRow() or -1
     if there are no rows. */
    virtual int lastVisibleRow() const = 0;

    /*! \return currently selected record data or null. */
    KDbRecordData *selectedItem() const {
        return m_currentItem;
    }

    /*! \return number of rows in this view. */
    int rowCount() const;

    /*! \return number of visible columns in this view.
     By default returns dataColumns(), what is proper table view.
     In case of form view, there can be a number of duplicated columns defined
     (data-aware widgets, see KexiFormScrollView::columnCount()),
     so columnCount() can return greater number than dataColumns(). */
    virtual int columnCount() const;

    /*! Helper function.
     \return number of columns of data. */
    int dataColumns() const;

    /*! \return true if data represented by this object
     is not editable (it can be editable with other ways although,
     outside of this object). */
    virtual bool isReadOnly() const;

    /*! Sets readOnly flag for this object.
     Unless the flag is set, the widget inherits readOnly flag from it's data
     structure assigned with setData(). The default value if false.

     This method is useful when you need to switch on the flag indepentently
     from the data structure.
     Note: it is not allowed to force readOnly off
     when internal data is readOnly - in that case the method does nothing.
     You can check internal data flag calling data()->isReadOnly().

     If \a set is true, insertingEnabled flag will be cleared automatically.
     \sa isInsertingEnabled()
    */
    void setReadOnly(bool set);

    /*! \return true if sorting is enabled. */
    inline bool isSortingEnabled() const {
        return m_isSortingEnabled;
    }

    /*! Sets sorting order on column @a column to @a order.
     This method do not work if sorting is disabled using setSortingEnabled(false).
     @a column may be -1, what means "no sorting". */
    virtual void setSorting(int column, Qt::SortOrder order = Qt::AscendingOrder);

    /*! Enables or disables sorting for this object
      This method is different that setSorting() because it prevents both user
      and programmer from sorting by clicking a column's header or calling setSorting().
      By default sorting is enabled.
    */
    virtual void setSortingEnabled(bool set);

    /*! \return sorted column number or -1 if no column is sorted within data or there
     is no data assigned at all.
     This does not mean that any sorting has been performed within GUI of this object,
     because the data could be changed in the meantime outside of this GUI object. */
    int dataSortColumn() const;

    /*! \return sort order for data. This information does not mean that any sorting
     has been performed within GUI of this object, because the data could be changed
     in the meantime outside of this GUI object.
     dataSortColumn() should be checked first to see if sorting is enabled (and if there's data). */
    Qt::SortOrder dataSortOrder() const;

    /*! Sorts all rows by column selected with setSorting().
     If there is currently row edited, it is accepted.
     If acception failed, sort() will return false.
     \return true on success. */
    virtual bool sort();

    /*! Sorts currently selected column in ascending order.
     This slot is used typically for "data_sort_az" action. */
    void sortAscending();

    /*! Sorts currently selected column in descending order.
     This slot is used typically for "data_sort_za" action. */
    void sortDescending();

    /*! \return true if data inserting is enabled (the default). */
    virtual bool isInsertingEnabled() const;

    /*! Sets insertingEnabled flag. If true, empty row is available
     at the end of this widget for new entering new data.
     Unless the flag is set, the widget inherits insertingEnabled flag from it's data
     structure assigned with setData(). The default value if false.

     Note: it is not allowed to force insertingEnabled on when internal data
     has insertingEnabled set off - in that case the method does nothing.
     You can check internal data flag calling data()->insertingEnabled().

     Setting this flag to true will have no effect if read-only flag is true.
     \sa setReadOnly()
    */
    void setInsertingEnabled(bool set);

    /*! \return true if row deleting is enabled.
     Equal to deletionPolicy() != NoDelete && !isReadOnly()). */
    bool isDeleteEnabled() const;

    /*! \return true if inserting empty rows are enabled (false by default).
     Mostly usable for not db-aware objects (e.g. used in Kexi Alter Table).
     Note, that if inserting is disabled, or the data set is read-only,
     this flag will be ignored. */
    bool isEmptyRowInsertingEnabled() const {
        return m_emptyRowInsertingEnabled;
    }

    /*! Sets emptyRowInserting flag.
     Note, that if inserting is disabled, this flag is ignored. */
    void setEmptyRowInsertingEnabled(bool set);

    /*! Enables or disables filtering. Filtering is enabled by default. */
    virtual void setFilteringEnabled(bool set);

    /*! \return true if filtering is enabled. */
    inline bool isFilteringEnabled() const {
        return m_isFilteringEnabled;
    }

    /*! Added for convenience: if @a set is true, this object
     will behave more like a spreadsheet (it's used for things like table designer view):
     - hides navigator
     - disables sorting, inserting and filtering
     - enables accepting row after cell accepting; see setAcceptsRowEditAfterCellAccepting()
     - enables inserting empty row; see setEmptyRowInsertingEnabled() */
    virtual void setSpreadSheetMode(bool set);

    /*! \return true id "spreadSheetMode" is enabled. It's false by default. */
    bool spreadSheetMode() const {
        return m_spreadSheetMode;
    }

    /*! \return number of currently edited row or -1. */
    inline int rowEditing() const {
        return m_rowEditing;
    }

    enum DeletionPolicy {
        NoDelete = 0,
        AskDelete = 1,
        ImmediateDelete = 2,
        SignalDelete = 3
    };

    /*! \return deletion policy for this object.
     The default (after allocating) is AskDelete. */
    DeletionPolicy deletionPolicy() const {
        return m_deletionPolicy;
    }

    virtual void setDeletionPolicy(DeletionPolicy policy);

    /*! Deletes currently selected record; does nothing if no record
     is currently selected. If record is in edit mode, editing
     is cancelled before deleting.  */
    virtual void deleteCurrentRow();

    /*! Inserts one empty record above \a pos. If \a pos is -1 (the default),
     new record is inserted above the current record (or above 1st record if there is no current).
     A new record becomes current if \a pos is -1 or if \a pos is equal currentRow().
     This method does nothing if:
     -inserting flag is disabled (see isInsertingEnabled())
     -read-only flag is set (see isReadOnly())
     \return inserted record's data
    */
    virtual KDbRecordData *insertEmptyRow(int pos = -1);

    /*! For reimplementation: called by deleteItem(). If returns false, deleting is aborted.
     Default implementation just returns true. */
    virtual bool beforeDeleteItem(KDbRecordData *record);

    /*! Deletes \a record. Used by deleteCurrentRow(). Calls beforeDeleteItem() before deleting,
     to double-check if deleting is allowed.
     \return true on success. */
    bool deleteItem(KDbRecordData *record);

    /*! Inserts newRecord at position \a pos. -1 means current record. Used by insertEmptyRow(). */
    void insertItem(KDbRecordData *newRecord, int pos = -1);

    /*! Clears entire table data, its visible representation
     and deletes data at database backend (if this is db-aware object).
     Does not clear columns information.
     Does not destroy KDbTableViewData object (if present) but only clears its contents.
     Displays confirmation dialog if \a ask is true (the default is false).
     Repaints widget if \a repaint is true (the default).
     For empty tables, true is returned immediately.
     If isDeleteEnabled() is false, false is returned.
     For spreadsheet mode all current rows are just replaced by empty rows.
     \return true on success, false on failure, and cancelled if user cancelled deletion
     (only possible if \a ask is true).
     */
    tristate deleteAllRows(bool ask = false, bool repaint = true);

    /*! \return maximum number of rows that can be displayed per one "page"
     for current view's size. */
    virtual int rowsPerPage() const = 0;

    virtual void selectRow(int row);
    virtual void selectNextRow();
    virtual void selectPrevRow();
    virtual void selectNextPage(); //!< page down action
    virtual void selectPrevPage(); //!< page up action
    virtual void selectFirstRow();
    virtual void selectLastRow();
    virtual void addNewRecordRequested();


    /*! Clears current selection. Current row and column will be now unspecified:
     currentRow(), currentColumn() will return -1, and selectedItem() will return null. */
    virtual void clearSelection();

    //! Flags for setCursorPosition()
    enum CursorPositionFlag {
        NoCursorPositionFlags = 0,  //!< Default flag
        ForceSetCursorPosition = 1, //!< Update cursor position even if row and col doesn't
                                    //!< differ from actual position.
        DontEnsureCursorVisibleIfPositionUnchanged = 2 //!< Don't call ensureCellVisible()
                                                       //!< when position is unchanged and
                                                       //!< ForceSetCursorPosition is off.
    };
    Q_DECLARE_FLAGS(CursorPositionFlags, CursorPositionFlag)

    /*! Moves cursor to \a row and \a col. If \a col is -1, current column number is used.
     If forceSet is true, cursor position is updated even if \a row and \a col doesn't
     differ from actual position. */
    virtual void setCursorPosition(int row, int col = -1,
                                   CursorPositionFlags flags = NoCursorPositionFlags);

    /*! Ensures that cell at \a row and \a col is visible.
     If \a col is -1, current column number is used. \a row and \a col, if not -1, must
     be between 0 and rowCount()-1 (or columnCount()-1 accordingly). */
    virtual void ensureCellVisible(int row, int col) = 0;

    /*! Ensures that column \a col is visible.
     If \a col is -1, current column number is used. \a col, if not -1, must be between
     0 and columnCount()-1. */
    virtual void ensureColumnVisible(int col) = 0;

    /*! Specifies, if this object automatically accepts
     row editing (using acceptRowEdit()) on accepting any cell's edit
     (i.e. after acceptEditor()). \sa acceptsRowEditAfterCellAccepting() */
    virtual void setAcceptsRowEditAfterCellAccepting(bool set);

    /*! \return true, if this object automatically accepts
     row editing (using acceptRowEdit()) on accepting any cell's edit
     (i.e. after acceptEditor()).
     By default this flag is set to false.
     Not that if the query for this table has given constraints defined,
     like NOT NULL / NOT EMPTY for more than one field - editing a record would
     be impossible for the flag set to true, because of constraints violation.
     However, setting this flag to true can be useful especially for not-db-aware
     data set (it's used e.g. in Kexi Alter Table's field editor). */
    bool acceptsRowEditAfterCellAccepting() const {
        return m_acceptsRowEditAfterCellAccepting;
    }

    /*! \return true, if this table accepts dropping data on the rows. */
    bool dropsAtRowEnabled() const {
        return m_dropsAtRowEnabled;
    }

    /*! Specifies, if this table accepts dropping data on the rows.
     If enabled:
     - dragging over row is indicated by drawing a line at bottom side of this row
     - dragOverRow() signal will be emitted on dragging,
      -droppedAtRow() will be emitted on dropping
     By default this flag is set to false. */
    virtual void setDropsAtRowEnabled(bool set);

    /*! \return currently used data (field/cell) editor or 0 if there is no data editing. */
    inline KexiDataItemInterface *editor() const {
        return m_editor;
    }

    /*! Cancels row editing All changes made to the editing
     row during this current session will be undone.
     \return true on success or false on failure (e.g. when editor does not exist) */
    virtual bool cancelRowEdit();

    /*! Accepts row editing. All changes made to the editing
     row during this current session will be accepted (saved).
     \return true if accepting was successful, false otherwise
     (e.g. when current row contain data that does not meet given constraints). */
    virtual bool acceptRowEdit();

    virtual void removeEditor();

    /*! Cancels changes made to the currently active editor.
     Reverts the editor's value to old one.
     \return true on success or false on failure (e.g. when editor does not exist) */
    virtual bool cancelEditor();

    //! Accepst changes made to the currently active editor.
    //! \return true on success or false on failure (e.g. when editor does not exist or there is data validation error)
    virtual bool acceptEditor();

    //! Flags for use in createEditor()
    enum CreateEditorFlag {
        ReplaceOldValue = 1,      //!< Remove old value replacing it with a new one
        EnsureCellVisible = 2,    //!< Ensure the cell behind the editor is visible
        DefaultCreateEditorFlags = EnsureCellVisible //!< Default flags.
    };
    Q_DECLARE_FLAGS(CreateEditorFlags, CreateEditorFlag)

    //! Creates editors and shows it, what usually means the beginning of a cell editing
    virtual void createEditor(int row, int col, const QString& addText = QString(),
                              CreateEditorFlags flags = DefaultCreateEditorFlags) = 0;

    /*! Used when Return key is pressed on cell, the cell has been double clicked
     or "+" navigator's button is clicked.
     Also used when we want to continue editing a cell after "invalid value" message
     was displayed (in this case, \a setText is usually not empty, what means
     that text will be set in the cell replacing previous value).
    */
    virtual void startEditCurrentCell(const QString& setText = QString(),
                                      CreateEditorFlags flags = DefaultCreateEditorFlags);

    /*! Deletes currently selected cell's contents, if allowed.
     In most cases delete is not accepted immediately but "record editing" mode is just started. */
    virtual void deleteAndStartEditCurrentCell();

    inline KDbRecordData *itemAt(int pos) const;

    /*! \return column information for column number \a col.
     Default implementation just returns column # col,
     but for Kexi Forms column data
     corresponding to widget number is used here
     (see KexiFormScrollView::fieldNumberForColumn()). */
    virtual KDbTableViewColumn* column(int col);

    /*! \return field number within data model connected to a data-aware
     widget at column \a col. Can return -1 if there's no such column. */
    virtual int fieldNumberForColumn(int col) {
        return col;
    }

    bool hasDefaultValueAt(const KDbTableViewColumn& tvcol);

    const QVariant* bufferedValueAt(int row, int col, bool useDefaultValueIfPossible = true);

    //! \return a type of column \a col - one of KDbField::Type
    int columnType(int col);

    //! \return default value for column \a col
    QVariant columnDefaultValue(int col) const;

    /*! \return true is column \a col is editable.
     Default implementation takes information about 'readOnly' flag from data member.
     Within forms, this is reimplemented for checking 'readOnly' flag from a widget
     ('readOnly' flag from data member is still checked though).
    */
    virtual bool columnEditable(int col);

    /*! Redraws the current cell. To be implemented. */
    virtual void updateCurrentCell() = 0;

    //! @return height of the horizontal header, 0 by default.
    virtual int horizontalHeaderHeight() const;

    //! signals
    virtual void itemChanged(KDbRecordData*, int row, int col) = 0;
    virtual void itemChanged(KDbRecordData*, int row, int col, QVariant oldValue) = 0;
    virtual void itemDeleteRequest(KDbRecordData*, int row, int col) = 0;
    virtual void currentItemDeleteRequest() = 0;
    //! Emitted for spreadsheet mode when an item was deleted and a new item has been appended
    virtual void newItemAppendedForAfterDeletingInSpreadSheetMode() = 0;

    /*! Data has been refreshed on-screen - emitted from initDataContents(). */
    virtual void dataRefreshed() = 0;
    virtual void dataSet(KDbTableViewData *data) = 0;

    /*! \return a pointer to context menu. This can be used to plug some actions there. */
    QMenu* contextMenu() const {
        return m_contextMenu;
    }

    /*! \return true if the context menu is enabled (visible) for the view.
      True by default. */
    bool contextMenuEnabled() const {
        return m_contextMenuEnabled;
    }

    /*! Enables or disables the context menu for the view. */
    void setContextMenuEnabled(bool set) {
        m_contextMenuEnabled = set;
    }

    /*! Sets a title with icon for the context menu.
     Set empty icon and text to remove the title item.
     This method should be called before customizing the menu
     because it will be recreated by the method. */
    void setContextMenuTitle(const QIcon &icon, const QString &text);

    /*! \return title text of the context menu. */
    QString contextMenuTitleText() const {
        return m_contextMenuTitleText;
    }

    /*! \return title icon of the context menu. */
    QIcon contextMenuTitleIcon() const {
        return m_contextMenuTitleIcon;
    }

    /*! \return true if vertical scrollbar's tooltips are enabled (true by default). */
    bool scrollbarToolTipsEnabled() const;

    /*! Enables or disables vertical scrollbar's tooltip. */
    void setScrollbarToolTipsEnabled(bool set);

    /*! Typically handles pressing Enter or F2 key:
     if current cell has boolean type, toggles it's value,
     otherwise starts editing (startEditCurrentCell()). */
    void startEditOrToggleValue();

    /*! \return true if new row is edited; implies: rowEditing==true. */
    inline bool newRowEditing() const {
        return m_newRowEditing;
    }

    /*! Reaction on toggling a boolean value of a cell:
     we're starting to edit the cell and inverting it's state. */
    virtual void boolToggled();

    virtual void connectCellSelectedSignal(const QObject* receiver,
                                           const char* intIntMember) = 0;

    virtual void connectRowEditStartedSignal(const QObject* receiver,
            const char* intMember) = 0;

    virtual void connectRowEditTerminatedSignal(const QObject* receiver,
            const char* voidMember) = 0;

    virtual void connectUpdateSaveCancelActionsSignal(const QObject* receiver,
            const char* voidMember) = 0;

    virtual void connectReloadActionsSignal(const QObject* receiver,
                                            const char* voidMember) = 0;

    virtual void connectDataSetSignal(const QObject* receiver,
                                      const char* kexiTableViewDataMember) = 0;

    virtual void connectToReloadDataSlot(const QObject* sender,
                                         const char* voidSignal) = 0;

    virtual void slotDataDestroying();

    //! Copy current selection to a clipboard (e.g. cell)
    virtual void copySelection() = 0;

    //! Cut current selection to a clipboard (e.g. cell)
    virtual void cutSelection() = 0;

    //! Paste current clipboard contents (e.g. to a cell)
    virtual void paste() = 0;

    /*! Finds \a valueToFind within the data items
     \a options are used to control the process. Selection is moved to found value.
     If \a next is true, "find next" is performed, else "find previous" is performed.

     Searching behaviour also depends on status of the previous search: for every search,
     position of the cells containing the found value is stored internally
     by the data-aware interface (not in options).
     Moreover, position (start, end) of the found value is also stored.
     Thus, the subsequent search will reuse this information to be able to start
     searching exactly after the previously found value (or before for "find previous" option).
     The flags can be zeroed, what will lead to seaching from the first character
     of the current item (cell).

     \return true if value has been found, false if value has not been found,
     and cancelled if there is nothing to find or there is no data to search in. */
    virtual tristate find(const QVariant& valueToFind,
                          const KexiSearchAndReplaceViewInterface::Options& options, bool next);

    /*! Finds \a valueToFind within the data items and replaces with \a replacement
     \a options are used to control the process.
     \return true if value has been found and replaced, false if value
     has not been found and replaced, and cancelled if there is nothing
     to find or there is no data to search in or the data is read only.
     If \a replaceAll is true, all found values are replaced. */
    virtual tristate findNextAndReplace(const QVariant& valueToFind,
                                        const QVariant& replacement,
                                        const KexiSearchAndReplaceViewInterface::Options& options, bool replaceAll);

    /*! \return vertical scrollbar */
    virtual QScrollBar* verticalScrollBar() const = 0;

    /*! Used in KexiTableView::keyPressEvent() (and in continuous forms).
     \return true when the key press event \e was consumed.
     You should also check e->isAccepted(), if it's true, nothing should be done;
     if it is false, you should call setCursorPosition() for the altered \a curCol
     and \c curRow variables.

     If \a moveToFirstField is not 0, *moveToFirstField will be set to true
     when the cursor should be moved to the first field (in tab order) and to false otherwise.
     If \a moveToLastField is not 0, *moveToLastField will be set to true
     when the cursor should be moved to the last field (in tab order) and to false otherwise.
     Note for forms: if moveToFirstField and moveToLastField are not 0,
     \a curCol is altered after calling this method, so setCursorPosition() will set to
     the index of an appropriate column (field). This is needed because field widgets can be
     inserted and ordered in custom tab order, so the very first field in the data source
     can be other than the very first field in the form.

     Used by KexiTableView::keyPressEvent() and KexiTableView::keyPressEvent(). */
    virtual bool handleKeyPress(QKeyEvent *e, int &curRow, int &curCol, bool fullRecordSelection,
                                bool *moveToFirstField = 0, bool *moveToLastField = 0);

protected:
    /*! Reimplementation for KexiDataAwareObjectInterface.
     Initializes data contents (resizes it, sets cursor at 1st row).
     Sets record count for record navigator.
     Sets cursor positin (using setCursorPosition()) to first row or sets
     (-1, -1) position if no rows are available.
     Called on setData(). Also called once on show event after
     refreshRequested() signal was received from TableViewData object. */
    virtual void initDataContents();

    /*! Clears columns information and thus all internal table data
     and its visible representation. Repaints widget if \a repaint is true. */
    virtual void clearColumns(bool repaint = true);

    /*! Called by clearColumns() to clear internals of the object.
     For example, KexiTableView removes contents of it's horizontal header. */
    virtual void clearColumnsInternal(bool repaint) = 0;

    /*! @internal for implementation
     \return sorting order (within GUI).
     currentLocalSortColumn() should be also checked, and if it returns -1, no particular
     sorting is set up.
     Even this does not mean that any sorting has been performed within GUI of this object,
     because the data could be changed in the meantime outside of this GUI object.
     @see dataSortOrder() currentLocalSortColumn() */
    virtual Qt::SortOrder currentLocalSortOrder() const = 0;

    /*! @internal for implementation
     \return sorted column number for this widget or -1 if no column
     is sorted witin GUI.
     This does not mean that the same sorting is performed within data member
     which is used by this widget, because the data could be changed in the meantime
     outside of this GUI widget.
     @see dataSortColumn() currentLocalSortOrder() */
    virtual int currentLocalSortColumn() const = 0;

    /*! @internal for implementation
     Shows sorting indicator order in the GUI for column @a column.
     This should not perform any sorting in data assigned to this object.
     @a column may be -1, what means "no sorting". */
    virtual void setLocalSortOrder(int column, Qt::SortOrder order) = 0;

    /*! @internal Sets order for \a column: -1: descending, 1: ascending,
     0: invert order */
    virtual void sortColumnInternal(int col, int order = 0);

    /*! @internal for implementation
     Updates GUI after sorting.
     After sorting you need to ensure current row and column
     is visible to avoid user confusion. For exaple, in KexiTableView
     implementation, current cell is centered (if possible)
     and updateContents() is called. */
    virtual void updateGUIAfterSorting(int previousRow) = 0;

    /*! Emitted in initActions() to force reload actions
     You should remove existing actions and add them again.
     Define and emit reloadActions() signal here. */
    virtual void reloadActions() = 0;

    /*! Reloads data for this object. */
    virtual void reloadData();

    /*! for implementation as a signal */
    virtual void itemSelected(KDbRecordData *) = 0;

    /*! for implementation as a signal */
    virtual void cellSelected(int row, int col) = 0;

    /*! for implementation as a signal */
    virtual void sortedColumnChanged(int col) = 0;

    /*! for implementation as a signal */
    virtual void rowEditTerminated(int row) = 0;

    /*! for implementation as a signal */
    virtual void updateSaveCancelActions() = 0;
    
    /*! Prototype for signal rowEditStarted(int), implemented by KexiFormScrollView. */
    virtual void rowEditStarted(int row) = 0;

    /*! Clear temporary members like the pointer to current editor.
     If you reimplement this method, don't forget to call this one. */
    virtual void clearVariables();

    /*! @internal
     Creates editor structure without filling it with data.
     Used in createEditor() and few places to be able to display cell contents
     dependending on its type. If \a ignoreMissingEditor is false (the default),
     and editor cannot be instantiated, current row editing (if present) is cancelled.
     */
    virtual KexiDataItemInterface *editor(int col, bool ignoreMissingEditor = false) = 0;

    /*! Updates editor's position, size and shows its focus (not the editor!)
     for \a row and \a col, using editor(). Does nothing if editor not found. */
    virtual void editorShowFocus(int row, int col) = 0;

    /*! Redraws specified cell. */
    virtual void updateCell(int row, int col) = 0;

    /*! Redraws all cells of specified row. */
    virtual void updateRow(int row) = 0;

    /*! Updates contents of the widget. Just call update() here on your widget. */
    virtual void updateWidgetContents() = 0;

    /*! Updates widget's contents size e.g. using QScrollView::resizeContents(). */
    virtual void updateWidgetContentsSize() = 0;

    /*! @internal
     Updates row appearance after canceling row edit.
     Used by cancelRowEdit(). By default just calls updateRow(m_curRow).
     Reimplemented by KexiFormScrollView. */
    virtual void updateAfterCancelRowEdit();

    /*! @internal
     Updates row appearance after accepting row edit.
     Used by acceptRowEdit(). By default just calls updateRow(m_curRow).
     Reimplemented by KexiFormScrollView. */
    virtual void updateAfterAcceptRowEdit();

    //! Handles TableViewData::rowRepaintRequested() signal
    virtual void slotRowRepaintRequested(KDbRecordData& record) {
        Q_UNUSED(record);
    }

    //! Handles TableViewData::aboutToDeleteRow() signal. Prepares info for slotRowDeleted().
    virtual void slotAboutToDeleteRow(KDbRecordData& record, KDbResultInfo* result,
                                      bool repaint);

    //! Handles TableViewData::rowDeleted() signal to repaint when needed.
    virtual void slotRowDeleted();

    //! Handles TableViewData::rowInserted() signal to repaint when needed.
    virtual void slotRowInserted(KDbRecordData *record, bool repaint);

    virtual void beginInsertItem(KDbRecordData *newRecord, int pos);

    virtual void endInsertItem(KDbRecordData *newRecord, int pos);

    virtual void beginRemoveItem(KDbRecordData *record, int pos);

    virtual void endRemoveItem(int pos);

    //! Like above, not db-aware version
    virtual void slotRowInserted(KDbRecordData *record, uint row, bool repaint);

    virtual void slotRowsDeleted(const QList<int> &) {}

    //! for sanity checks (return true if m_data is present; else: outputs warning)
    inline bool hasData() const;

    /*! Used by setCursorPosition() if cursor's position changed. */
    virtual void selectCellInternal(int previousRow, int previousColumn);

    /*! Used in KexiDataAwareObjectInterface::slotRowDeleted()
     to repaint tow \a row and all visible below.
     Implemented if there is more than one row displayed, i.e. currently for KexiTableView. */
    virtual void updateAllVisibleRowsBelow(int row) {
        Q_UNUSED(row);
    }

    /*! @return geometry of the viewport, i.e. the scrollable area, minus any scrollbars, etc. */
    virtual QRect viewportGeometry() const = 0;

    //! Call this from the subclass. */
    virtual void focusOutEvent(QFocusEvent* e);

    /*! Handles verticalScrollBar()'s valueChanged(int) signal.
     Called when vscrollbar's value has been changed.
     Call this method from the subclass. */
    virtual void vScrollBarValueChanged(int v);

    /*! Changes 'row editing' >=0 there's currently edited row, else -1.
     * Can be reimplemented with calling superclass setRowEditing()
     * Sends rowEditStarted(int) signal.
     * @see rowEditing() rowEditStarted().
     */
    void setRowEditing(int row);

    /*! Shows error message box suitable for \a resultInfo. This can be "sorry" or "detailedSorry"
     message box or "queryYesNo" if resultInfo->allowToDiscardChanges is true.
     \return code of button clicked: KMessageBox::Ok in case of "sorry" or "detailedSorry" messages
     and KMessageBox::Yes or KMessageBox::No in case of "queryYesNo" message. */
    int showErrorMessageForResult(const KDbResultInfo& resultInfo);

    /*! Shows context message @a message for editor @a item. */
    void showEditorContextMessage(
            KexiDataItemInterface *item,
            const QString &message,
            KMessageWidget::MessageType type,
            KMessageWidget::CalloutPointerDirection direction);

    /*! Shows context message about exceeded length for editor @a item.
     If @a exceeded is true, a new message is created, else the message will be removed. */
    void showLengthExceededMessage(KexiDataItemInterface *item, bool exceeded);

    /*! Updates message about exceeded length for editor @a item.
     Useful only where message created with showLengthExceededMessage() is displayed. */
    void showUpdateForLengthExceededMessage(KexiDataItemInterface *item);

    /*! Prepares array of indices of visible values to search within.
     This is per-interface global cache.
     Needed for faster lookup because there could be lookup values.
     Called whenever columns definition changes, i.e. in setData() and clearColumns().
     @see find() */
    void updateIndicesForVisibleValues();

    //! @return horizontal header, 0 by default.
    virtual QHeaderView* horizontalHeader() const;

    //! @return vertical header, 0 by default.
    virtual QHeaderView* verticalHeader() const;

    //! Update section of vertical header
    virtual void updateVerticalHeaderSection(int section) = 0;

    //! data structure displayed for this object
    KDbTableViewData *m_data;

    //! current row (cursor)
    int m_curRow;

    //! current column (cursor)
    int m_curCol;

    //! current record's data
    KDbRecordData *m_currentItem;

    //! data iterator
    KDbTableViewData::Iterator m_itemIterator;

    //! record's data for inserting
    KDbRecordData *m_insertItem;

    //! true if m_data member is owned by this object
    bool m_owner;

    /*! true if new row is edited; implies: rowEditing==true. */
    bool m_newRowEditing;

    /*! 'sorting by column' availability flag for widget */
    bool m_isSortingEnabled;

    /*! true if filtering is enabled for the view. */
    bool m_isFilteringEnabled;

    /*! Public version of 'acceptsRowEditAfterCellAcceptin' flag (available for a user).
     It's OR'es together with above flag.
    */
    bool m_acceptsRowEditAfterCellAccepting;

    /*! Used in acceptEditor() to avoid infinite recursion,
     eg. when we're calling acceptRowEdit() during cell accepting phase. */
    bool m_inside_acceptEditor; // no bit field allowed

    /*! Used in acceptRowEdit() to avoid infinite recursion,
     eg. when we're calling acceptRowEdit() during cell accepting phase. */
    bool m_inside_acceptRowEdit; // no bit field allowed

    /*! @internal if true, this object automatically accepts
     row editing (using acceptRowEdit()) on accepting any cell's edit
     (i.e. after acceptEditor()). */
    bool m_internal_acceptsRowEditAfterCellAccepting;

    /*! true, if inserting empty rows are enabled (false by default) */
    bool m_emptyRowInsertingEnabled;

    /*! Contains 1 if the object is readOnly, 0 if not;
     otherwise (-1 means "do not know") the 'readOnly' flag from object's
     internal data structure (KDbTableViewData *KexiTableView::m_data) is reused.
     */
    int m_readOnly;

//! @todo really keep this here and not in KexiTableView?
    /*! true if currently double click action was is performed
    (so accept/cancel editor shoudn't be executed) */
    bool m_contentsMousePressEvent_dblClick;

    /*! like for readOnly: 1 if inserting is enabled */
    int m_insertingEnabled;

    /*! true, if initDataContents() should be called on show event. */
    bool m_initDataContentsOnShow;

    /*! Set to true in setCursorPosition() to indicate that cursor position was set
     before show() and it shouldn't be changed on show().
     Only used if initDataContentsOnShow is true. */
    bool m_cursorPositionSetExplicityBeforeShow;

    /*! true if spreadSheetMode is enabled. False by default.
     @see KexiTableView::setSpreadSheetMode() */
    bool m_spreadSheetMode;

    /*! true, if this table accepts dropping data on the rows (false by default). */
    bool m_dropsAtRowEnabled;

    /*! true, if this entire (visible) row should be updated when boving to other row.
     False by default. For table view with 'row highlighting' flag enabled, it is true. */
    bool m_updateEntireRowWhenMovingToOtherRow;

    DeletionPolicy m_deletionPolicy;

    KexiDataItemInterface *m_editor;

    /*! Navigation panel, used if navigationPanelEnabled is true. */
    KexiRecordNavigatorIface *m_navPanel; //!< main navigation widget

    bool m_navPanelEnabled;

    /*! Row number that over which user drags a mouse pointer.
     Used to indicate dropping possibility for that row.
     Equal -1 if no indication is needed. */
    int m_dragIndicatorLine;

    /*! Context menu widget. */
    QMenu *m_contextMenu;

    /*! Text of context menu title. */
    QString m_contextMenuTitleText;

    /*! Icon of context menu title. */
    QIcon m_contextMenuTitleIcon;

    /*! True if context menu is enabled. */
    bool m_contextMenuEnabled;

    //! Used by updateAfterCancelRowEdit()
    bool m_alsoUpdateNextRow;

    /*! Row number (>=0 or -1 == no row) that will be deleted in deleteRow().
     It is set in slotAboutToDeleteRow(KDbRecordData&,KDbResultInfo*,bool)) slot
     received from KDbTableViewData member.
     This value will be used in slotRowDeleted() after rowDeleted() signal
     is received from KDbTableViewData member and then cleared (set to -1). */
    int m_rowWillBeDeleted;

    /*! Displays passive error popup label used when invalid data has been entered. */
    QPointer<KexiContextMessageWidget> m_errorMessagePopup;

    /*! Used to enable/disable execution of vScrollBarValueChanged()
     when users navigate through rows using keyboard, so vscrollbar tooltips are not visible. */
    bool m_vScrollBarValueChanged_enabled;

    /*! True, if vscrollbar tooltips are enabled (true by default). */
    bool m_scrollbarToolTipsEnabled;

    //! Used to mark recently found value
    class PositionOfValue
    {
    public:
        PositionOfValue() : firstCharacter(0), lastCharacter(0), exists(false) {}
        uint firstCharacter;
        uint lastCharacter;
        bool exists;
    };

    /*! Used to mark recently found value. Updated on successful execution of find().
     If the current cursor's position changes, or data in the current cell changes,
     positionOfRecentlyFoundValue.exists is set to false. */
    PositionOfValue m_positionOfRecentlyFoundValue;

    /*! Used to compare whether we're looking for new value. */
    QVariant m_recentlySearchedValue;

    /*! Used to compare whether the search direction has changed. */
    KexiSearchAndReplaceViewInterface::Options::SearchDirection m_recentSearchDirection;

    //! Setup by updateIndicesForVisibleValues() and used by find()
    QVector<uint> m_indicesForVisibleValues;

private:
    /*! >= 0 if a row is edited */
    int m_rowEditing;

    bool m_lengthExceededMessageVisible;

    //! true if acceptRowEdit() should be called in setCursorPosition() (true by default)
    bool m_acceptRowEdit_in_setCursorPosition_enabled;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KexiDataAwareObjectInterface::CreateEditorFlags)
Q_DECLARE_OPERATORS_FOR_FLAGS(KexiDataAwareObjectInterface::CursorPositionFlags)

inline bool KexiDataAwareObjectInterface::hasData() const
{
    if (!m_data)
        qDebug() << "KexiDataAwareObjectInterface: No data assigned!";
    return m_data != 0;
}

inline KDbRecordData *KexiDataAwareObjectInterface::itemAt(int pos) const
{
    KDbRecordData *record = m_data->at(pos);
    if (!record)
        qDebug() << "pos:" << pos << "- NO ITEM!!";
    else {
        /*  qDebug() << "record:" << row;
            int i=1;
            for (KexiTableItem::Iterator it = item->begin();it!=item->end();++it,i++)
              qDebug() << i<<": " << (*it).toString();*/
    }
    return record;
}

//! Convenience macro used for KexiDataAwareObjectInterface implementations.
#define KEXI_DATAAWAREOBJECTINTERFACE \
    public: \
    void connectCellSelectedSignal(const QObject* receiver, const char* intIntMember) { \
        connect(this, SIGNAL(cellSelected(int,int)), receiver, intIntMember); \
    } \
    void connectRowEditStartedSignal(const QObject* receiver, const char* intMember) { \
        connect(this, SIGNAL(rowEditStarted(int)), receiver, intMember); \
    } \
    void connectRowEditTerminatedSignal(const QObject* receiver, const char* voidMember) { \
        connect(this, SIGNAL(rowEditTerminated(int)), receiver, voidMember); \
    } \
    void connectUpdateSaveCancelActionsSignal(const QObject* receiver, \
                                              const char* voidMember) { \
        connect(this, SIGNAL(updateSaveCancelActions()), receiver, voidMember); \
    } \
    void connectReloadActionsSignal(const QObject* receiver, const char* voidMember) { \
        connect(this, SIGNAL(reloadActions()), receiver, voidMember); \
    } \
    void connectDataSetSignal(const QObject* receiver, \
                              const char* kexiTableViewDataMember) { \
        connect(this, SIGNAL(dataSet(KDbTableViewData*)), receiver, kexiTableViewDataMember); \
    } \
    void connectToReloadDataSlot(const QObject* sender, const char* voidSignal) { \
        connect(sender, voidSignal, this, SLOT(reloadData())); \
    }

#endif
