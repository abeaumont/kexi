/* This file is part of the KDE project
   Copyright (C) 2003 Lucijan Busch <lucijan@kde.org>
   Copyright (C) 2003-2018 Jarosław Staniek <staniek@kde.org>

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

#ifndef KEXIMAINWINDOW_H
#define KEXIMAINWINDOW_H

//#define KEXI_IMPL_WARNINGS

#include "keximain_export.h"

#include <KMainWindow>
#include <core/KexiMainWindowIface.h>
#include <core/kexiguimsghandler.h>

#include <QCommandLineOption>
#include <QTabWidget>

class QPaintEvent;
class KDbObject;
class KDbConnectionData;
class KexiProjectData;
class KexiMainWidget;
namespace KexiPart
{
class Info;
class Part;
}

#define KexiMainWindowSuper QWidget //KMainWindow

/**
 * @short Kexi's main window implementation
 */
class KEXIMAIN_EXPORT KexiMainWindow
            : public QWidget /*KMainWindow*/, public KexiMainWindowIface, public KexiGUIMessageHandler
{
    Q_OBJECT

public:
    /*! Creates an empty mainwindow. */
    explicit KexiMainWindow(QWidget *parent = 0);
    virtual ~KexiMainWindow();

    virtual KActionCollection* actionCollection() const;

//! @todo virtual QWidget* focusWidget() const;
    virtual QWidget* focusWidget() const {
        return KexiMainWindowSuper::focusWidget();
    }

    /*! Used by the main Kexi's routine. Creates a new Kexi main window.
     For Kexi applications @a arguments are equal to CoreApplication::arguments() but test
     applications alter the list.
     If @a componentName is provided, it is assigned to application's KAboutData::componentName.
     It's not used by Kexi itself but is useful for test application that are based
     on KexiMainWindow.
     @a extraOptions can be supplied to extend the list of supported options.
     @note Extra options must not override Kexi's built-in options.
     @return 0 on success (the result can be used as a result of main()) and other value on error */
    static int create(const QStringList &arguments, const QString &componentName = QString(),
                      const QList<QCommandLineOption> &extraOptions = QList<QCommandLineOption>());

    //! Project data of currently opened project or NULL if no project here yet.
    virtual KexiProject *project();

    /*! Registers window \a window for watching and adds it to the main window's stack. */
    virtual void registerChild(KexiWindow *window);

    /*! Activates a window by it's document identifier.
     \return false if doc couldn't be raised or isn't opened. */
    bool activateWindow(int id);

    /*! Like above, using \a window passed explicitly. Above method just calls this one. */
    bool activateWindow(KexiWindow& window);

    /*! Performs startup actions. \return false if application should exit immediately
     with an error status. */
    tristate startup();

    /*! \return true if the application window is in the User Mode. */
    virtual bool userMode() const;

    /*! \return true if opening of item \a item in \a viewMode mode is allowed.
     userMode() is taken into account as well
     as KexiPart::PartInfo::supportedUserViewModes() for \a  item. */
    bool openingAllowed(KexiPart::Item* item, Kexi::ViewMode viewMode, QString* errorMessage = 0);

    /*! Implemented for KexiMainWindow. */
    virtual QList<QAction*> allActions() const;

    /*! \return currently active window or 0 if there is no active window.
     Implemented for KexiWindow. */
    virtual KexiWindow* currentWindow() const;

    /*! @return window for tab @a tabIndex or 0 if there is no such tab. */
    KexiWindow* windowForTab(int tabIndex) const;

//! @todo temporary solution before the tabbed toolbar framework emerges
    // see KexiMainWindowIface
    virtual void appendWidgetToToolbar(const QString& name, QWidget* widget);

//! @todo temporary solution before the tabbed toolbar framework emerges
    // see KexiMainWindowIface
    virtual void setWidgetVisibleInToolbar(QWidget* widget, bool visible);

//! @todo replace with the final Actions API
    // see KexiMainWindowIface
    virtual void addToolBarAction(const QString& toolBarName, QAction *action);

    // see KexiMainWindowIface
    virtual KToolBar *toolBar(const QString& name) const;

    //! Shows design tab @a tabName again and activates it as current if it was hidden
    //! before for the same object.
    void restoreDesignTabAndActivateIfNeeded(const QString &tabName);

    //! Shows design tab again when switching between objects or views.
    void restoreDesignTabIfNeeded(const QString &pluginId, Kexi::ViewMode viewMode, int previousItemId);

    //! Sets currently visible design tab when switching to design view, according to object type opened.
    virtual void activateDesignTabIfNeeded(const QString &pluginId, Kexi::ViewMode viewMode);

    //! Hides design tabs when they are closed (depending on ID @a pluginId).
    //! If @a pluginId is empty, all tabs get hidden.
    virtual void hideDesignTab(int itemId, const QString &pluginId = QString());

    /*! Implemented for KexiMainWindow */
    virtual KexiUserFeedbackAgent* userFeedbackAgent() const;

    /*! Implemented for KexiMainWindow */
    virtual KexiMigrateManagerInterface* migrateManager();

public Q_SLOTS:
    /*! Implemented for KexiMainWindow */
    virtual tristate closeWindow(KexiWindow *window);

    /*! Closes the current window. */
    tristate closeCurrentWindow();

    /*! Closes window inside tab @a tabIndex. */
    tristate closeWindowForTab(int tabIndex);

    /*! Internal implementation. If \a doNotSaveChanges is true,
     messages asking for saving the will be skipped and the changes will be dropped.
     This should not be usually used, maybe except for test suites
     (see kexi/tests/altertable/ directory). */
    tristate closeWindow(KexiWindow *window, bool layoutTaskBar, bool doNotSaveChanges = false);

    /**
     * Activates next window
     *
     * If any assistant window is active and visible, moves to previous page of the assistant.
     * If assistant window is not active, this action triggers activateNextTab() if tabs are present.
     */
    void activateNextWindow();

    /**
     * Activates previous window
     *
     * If any assistant window is active and visible, moves to previous page of the assistant.
     * If assistant window is not active, this action triggers activatePreviousTab() if tabs are present.
     */
    void activatePreviousWindow();

    /*! Activates next tab if tabs are present. */
    void activateNextTab();

    /*! Activates next tab if tabs are present. */
    void activatePreviousTab();

//! @todo move part of this to KexiProject, because currently KexiProject::openObject() allows multiple opens!
    /*! Opens object pointed by \a item in a view \a viewMode.
     \a staticObjectArgs can be passed for static object
     (only works when part for this item is of type KexiPart::StaticPart).
     \a openingCancelled is set to true if opening has been cancelled.
     \a errorMessage, if not 0, points to a string that can be set to error message
     if one encountered.
     @c nullptr can be returned if the KexiWindow object for @a item is not yet fully constructed
     but openObject() has been quickly called again for the same @a item. This can happen if user
     clicked multiple times on the same Project navigator's item. In this case @a openingCancelled
     is not set; the caller should not display error message but the opening should be silently abandoned. */
    virtual KexiWindow* openObject(KexiPart::Item *item, Kexi::ViewMode viewMode,
                                   bool *openingCancelled, QMap<QString, QVariant>* staticObjectArgs = 0,
                                   QString* errorMessage = 0);

    //! For convenience
    virtual KexiWindow* openObject(const QString& pluginId, const QString& name,
                                   Kexi::ViewMode viewMode, bool *openingCancelled,
                                   QMap<QString, QVariant>* staticObjectArgs = 0);

    /*! Closes the object for \a item.
     \return true on success (closing can be dealyed though), false on failure and cancelled
     if the object has "opening" job assigned. */
    virtual tristate closeObject(KexiPart::Item* item);

    /*! Implemented for KexiMainWindow */
    virtual tristate saveObject(KexiWindow *window,
                                const QString& messageWhenAskingForName = QString(),
                                SaveObjectOptions options = 0);

    /*! Implemented for KexiMainWindowIface. */
    virtual KexiWindow *openedWindowFor(int identifier);
    virtual KexiWindow *openedWindowFor(const KexiPart::Item *item);

    /*! Implemented for KexiMainWindowIface */
    virtual QList<QVariant> currentParametersForQuery(int queryId) const;

    /*! Implemented for KexiMainWindowIface. */
    virtual KDbQuerySchema *unsavedQuery(int queryId);

    /*! Implemented for KexiMainWindow */
    virtual tristate getNewObjectInfo(KexiPart::Item *partItem,
                                      const QString &originalName,
                                      KexiPart::Part *part,
                                      bool allowOverwriting, bool *overwriteNeeded,
                                      const QString& messageWhenAskingForName = QString());

    /*! Implemented for KexiMainWindow */
    virtual void highlightObject(const QString& pluginId, const QString& name);

    /*! Opens project pointed by \a projectData.
     Application state (e.g. actions) is updated.
     \a projectData is copied into a project structures.
     \return true on success */
    tristate openProject(const KexiProjectData& projectData);

    /*! Helper. Opens project pointed by \a aFileName.
     If \a aFileName is empty, a connection shortcut (.kexic file name) is obtained from
     global connection set using \a cdata (if present).
     In this case:
     * If connection shortcut has been found and \a dbName (a server database name) is provided
      'kexi --skip-dialog --connection file.kexic dbName' is executed (or the project
      is opened directly if there's no project opened in the current Kexi main window.
     * If connection shortcut has been found and \a dbName is not provided,
      'kexi --skip-dialog file.kexic' is executed (or the connection is opened
      directly if there's no porject opened in the current Kexi main window. */
    tristate openProject(const QString& aFileName, KDbConnectionData *cdata,
                         const QString& dbName = QString(),
                         const KexiProjectData::AutoOpenObjects& autoopenObjects = KexiProjectData::AutoOpenObjects());

    /*! Helper. Opens project pointed by \a aFileName.
     Like above but \a fileNameForConnectionData can be passed instead of
     a pointer to connection data itself.
     \return false if \a fileNameForConnectionData is not empty but there is no such
     connection in Kexi::connset() for this filename.
     \a fileNameForConnectionData can be empty. */
    tristate openProject(const QString& aFileName,
                         const QString& fileNameForConnectionData, const QString& dbName = QString());

    /*! Helper. Opens project pointed by \a aFileName. */
    tristate openProject(const QString& aFileName);

    /*! Opens project referenced by @a data.
     If @a shortcutPath is a empty .kexis filename and there is another project opened,
     a new instance of Kexi is started with the .kexis file as argument.
     Value pointed by @a opened is set to true if the database has been opened successfully.
     @return true on successful opening, cancelled if the operation was cancelled
     and false on failure.*/
    tristate openProject(const KexiProjectData& data, const QString& shortcutPath, bool *opened);

    /*! Creates a new project using template pointed by \a projectData.
     Application state (e.g. actions) is updated.
     New project data is copied into a project structures.
     \return true on success */
    tristate createProjectFromTemplate(const KexiProjectData& projectData);

    /*! Closes current project, \return true on success.
     Application state (e.g. actions) is updated.
     \return true on success.
     If closing was cancelled by user, cancelled is returned. */
    tristate closeProject();

    //! Shows "print" dialog for \a item.
    //! \return true on success.
    virtual tristate printItem(KexiPart::Item* item);

    //! Shows "print preview" window.
    //! \return true on success.
    virtual tristate printPreviewForItem(KexiPart::Item* item);

    //! Shows "page setup" window for \a item.
    //! \return true on success and cancelled when the action was cancelled.
    virtual tristate showPageSetupForItem(KexiPart::Item* item);

    /*! Executes custom action for the main window, usually provided by a plugin.
     Also used by KexiFormEventAction. */
    virtual tristate executeCustomActionForObject(KexiPart::Item* item, const QString& actionName);

    /*! Add searchable model to the main window. This extends search to a new area.
     One example is Project Navigator. @see KexiMainWindowIface */
    void addSearchableModel(KexiSearchableModel *model) override;

    /*! Removes searchable model from the main window. @a model is not deleted.
     @see KexiMainWindowIface */
    void removeSearchableModel(KexiSearchableModel *model) override;

    //! Shows design tab when switching between objects or views. Depends on current window and view mode.
    void showDesignTabIfNeeded(int previousItemId);

    void toggleFullScreen(bool isFullScreen);

    /*! Implemented for KexiMainWindowIface.
     Sets reasonable dialog size based on main window size, that is 80% of its size. */
    virtual void setReasonableDialogSize(QDialog *dialog);

Q_SIGNALS:
    //! Emitted to make sure the project can be close.
    //! Connect a slot here and set \a cancel to true to cancel the closing.
    void acceptProjectClosingRequested(bool *cancel);

    //! Emitted before closing the project (and destroying all it's data members).
    //! You can do you cleanup of your structures here.
    void beforeProjectClosing();

    //! Emitted after closing the project.
    void projectClosed();

    //! Emitted after opening a project, even after slotAutoOpenObjectsLater().
    void projectOpened();

protected:
    /*! Setups main widget */
    void setupMainWidget();

    /*! Creates the Project Navigator (if it's not yet created),
     lookups items for current project and fills the nav. with not-opened items */
    void setupProjectNavigator();

    void setupContextHelp();

    void setupPropertyEditor();

    void setupMainMenuActionShortcut(QAction * action);

    /*! Creates standard actions like new, open, save ... */
    void setupActions();

    /*! Creates user project-wide actions */
    void setupUserActions();

    /*! Sets up the window from user settings. */
    void restoreSettings();

    /*! Writes user settings back. */
    void storeSettings();

    /*! Invalidates availability of all actions for current application state. */
    void invalidateActions();

    /*! Invalidates action availability for current application state.
     These actions are dependent on active window. */
    virtual void invalidateSharedActions(QObject *o);

    /*! Invalidates action availability for current application state.
     These actions only depend on project availability, not on curently active window. */
    void invalidateProjectWideActions();

    /*! Shows dialog for creating new project, and creates one.
     The dialog is not shown if option for automatic creation
     is checked or KexiStartupHandler::global()->projectData() was provided from command line.
     \a cancelled is set to true if creation has been cancelled (e.g. user answered
     no when asked for database overwriting, etc.
     \return true if database was created, false on error or when cancel was pressed */
    void createNewProject();

    /*! Shows dialog for creating new blank project,
     and return a data describing it. If the dialog was cancelled,
     \a cancelled will be set to true (false otherwise).
     \a shortcutFileName, if not 0, will be set to a shortcut filename
     (in case when server database project was selected). */
    KexiProjectData* createBlankProjectData(bool *cancelled, bool confirmOverwrites = true, QString *shortcutFileName = 0);

    /*! Reimplemented from KexiSharedActionHost:
     accepts only KexiDockBase and KexiWindow subclasses.  */
    virtual bool acceptsSharedActions(QObject *w);

    /*! Performs lookup like in KexiSharedActionHost::focusWindow()
     but starting from \a w instead of a widget returned by QWidget::focusWidget().
     \return NULL if no widget matches acceptsSharedActions() or if \a w is NULL. */
    virtual QWidget* findWindow(QWidget *w);

    /*! Updates application's caption - also shows project's name. */
    void updateAppCaption();

    virtual void closeEvent(QCloseEvent *ev);

    //! Called by KexiMainWidget::queryClose()
    bool queryClose();

    /*! Implemented for KexiMainWindowIface.
     Switches \a window to view \a mode. Activates the window if it is not the current window. */
    virtual tristate switchToViewMode(KexiWindow& window, Kexi::ViewMode viewMode);

    /*! Helper. Updates setup of property panel's tabs. Used when switching
     from \a prevWindow window to a current window. */
    void updateCustomPropertyPanelTabs(KexiWindow *prevWindow, Kexi::ViewMode prevViewMode);

    /*! @overload void updateCustomPropertyPanelTabs(KexiWindow *prevWindow, Kexi::ViewMode prevViewMode) */
    void updateCustomPropertyPanelTabs(KexiPart::Part *prevWindowPart,
                                       Kexi::ViewMode prevViewMode, KexiPart::Part *curWindowPart, Kexi::ViewMode curViewMode);

    /*! Used in openProject when running another Kexi process is required. */
    tristate openProjectInExternalKexiInstance(const QString& aFileName, KDbConnectionData *cdata, const QString& dbName);

    /*! Used in openProject when running another Kexi process is required. */
    tristate openProjectInExternalKexiInstance(const QString& aFileName, const QString& fileNameForConnectionData, const QString& dbName);

    /*! Updates info label of the property editor by reusing properties provided
     by the current property set.
     Read documentation of KexiPropertyEditorView class for information about accepted properties.
     If the current property is 0 and @a textToDisplayForNullSet string is not empty, this string is displayed
     (without icon or any other additional part).
     If the current property is 0 and @a textToDisplayForNullSet string is empty, the info label widget becomes
     hidden.
     Implemented for KexiMainWindow.
     @see KexiPropertyPaneViewBase::updateInfoLabelForPropertySet() */
    virtual void updatePropertyEditorInfoLabel(const QString& textToDisplayForNullSet);

    //! Activates design tab when switching to design view, according to \a pluginId.
    void activateDesignTab(const QString &pluginId);

protected Q_SLOTS:
    tristate createNewProject(const KexiProjectData &projectData);

    /*! Called once after timeout (after ctors are executed). */
    void slotAutoOpenObjectsLater();

    /*! Called if a window (tab) changes from \a prevWindow to \a window. Both parameters can be 0. */
    void activeWindowChanged(KexiWindow *window, KexiWindow *prevWindow);

    void slotPartLoaded(KexiPart::Part* p);

    //! Internal - creates and initializes Kexi project object based on @a data.
    KexiProject* createKexiProjectObject(const KexiProjectData &data);

    /*! Handles event when user double clicked (or single -depending on settings)
     or pressed Return key on the part item in the navigator.
     This differs from openObject() signal in that if the object is already opened
     in view mode other than \a viewMode, the mode is not changed.
     \sa KexiProjectNavigator::openOrActivateItem() */
    KexiWindow* openObjectFromNavigator(KexiPart::Item* item,
                                        Kexi::ViewMode viewMode, bool *openingCancelled);

    //! For convenience
    KexiWindow* openObjectFromNavigator(KexiPart::Item* item, Kexi::ViewMode viewMode);

    /*! Creates new object of type defined by \a info part info.
     \a openingCancelled is set to true if opening has been cancelled.
     \return true on success. */
    virtual bool newObject(KexiPart::Info *info, bool *openingCancelled);

    //! For convenience
    bool newObject(KexiPart::Info *info) {
        bool openingCancelled;
        return newObject(info, &openingCancelled);
    }

    //! For convenience
    KexiWindow* openObject(KexiPart::Item *item, Kexi::ViewMode viewMode,
                           QMap<QString, QVariant>* staticObjectArgs = 0) {
        bool openingCancelled;
        return openObject(item, viewMode, &openingCancelled, staticObjectArgs);
    }

    /*! Removes object pointed by \a item from current project.
     Asks for confirmation. \return true on success
     or cancelled if removing was cancelled (only possible if \a dontAsk is false). */
    tristate removeObject(KexiPart::Item *item, bool dontAsk = false);

    /*! Renames object pointed by \a item to a new name \a _newName.
     Sets \a success to false on failure. Used as a slot connected
     to KexiProjectNavigator::renameItem() signal. */
    void renameObject(KexiPart::Item *item, const QString& _newName, bool *succes);

    /*! Changes caption of object pointed by \a item to \a _newCaption.
     Sets \a success to false on failure. Used as a slot connected
     to KexiProjectNavigator::changeItemCaption() signal. */
    void setObjectCaption(KexiPart::Item *item, const QString& _newCaption, bool *succes);

    /*! Reaction for object rename (signalled by KexiProject).
     If this item has opened window, it's caption is updated,
     and also optionally application's caption. */
    virtual void slotObjectRenamed(const KexiPart::Item &item, const QString& oldName);

//! @todo virtual void fillWindowMenu();

    void invalidateSharedActions();
    void invalidateSharedActionsLater();

    //! Updates the statusbar, navigator and "Insert->....." actions, dependent on read-only state.
    //! Only called on project opening and closing.
    void updateReadOnlyState();

    void slotProjectWelcome();
    void slotProjectNew();
    void slotProjectOpen();
    void slotProjectSave();
    void slotProjectSaveAs();
    void slotProjectPrint();
    void slotProjectPrintPreview();
    void slotProjectPageSetup();
    void slotProjectProperties();
    void slotProjectImportExportOrSend();
    void slotProjectClose();
    void slotProjectRelations();
    void slotProjectImportDataTable();
    void slotProjectExportDataTable();
    void slotProjectQuit();
    void slotEditPasteSpecialDataTable();
    void slotEditCopySpecialDataTable();
    void slotEditFind();
    void slotEditFind(bool next); //!< helper
    void slotEditFindNext();
    void slotEditFindPrevious();
    void slotEditReplace(bool all); //!< helper
    void slotEditReplace();
    void slotEditReplaceNext();
    void slotEditReplaceAll();
    void slotActivateNavigator();
    void slotActivateMainArea();
    void slotActivatePropertyEditor();
    void slotShowNavigator();
    void slotShowPropertyEditor();
    void slotViewDataMode();
    void slotViewDesignMode();
    void slotViewTextMode(); //!< sometimes called "SQL View"
    void slotSettings();
    void slotConfigureKeys();
    void slotConfigureToolbars();
    void slotToolsImportProject();
    void slotToolsImportTables();
    void slotToolsCompactDatabase();

    void slotReportBug();
    void slotTipOfTheDay();
    void slotImportFile();
    void slotImportServer();

    //! There are performed all actions that need to be done immediately after  ctro (using timer)
    void slotLastActions();

    virtual void acceptPropertySetEditing();

    virtual void propertySetSwitched(KexiWindow *window, bool force = false,
                                     bool preservePrevSelection = true,
                                     bool sortedProperties = false,
                                     const QByteArray& propertyToSelect = QByteArray());

    /*! Handles changes in 'dirty' flag for windows. */
    void slotDirtyFlagChanged(KexiWindow* window);

    /*! Shows Project Migration Wizard. \return true on successful migration,
     cancelled on cancellation, and false on failure.
     If \a mimeType and \a databaseName are not empty, the wizard will only ask about
     parameters of destination project and skip pages related to source project. */
    tristate showProjectMigrationWizard(const QString& mimeType, const QString& databaseName);

    /*! @overload tristate showProjectMigrationWizard(const QString& mimeType, const QString& databaseName)
     @a cdata is used preselect a server-based connection. */
    tristate showProjectMigrationWizard(const QString& mimeType, const QString& databaseName,
                                        const KDbConnectionData &cdata);

    //! Receives "selectionChanged()" signal from navigator to update some actions.
    void slotPartItemSelectedInNavigator(KexiPart::Item* item);

    /*! Receives the "executeItem" signal from navigator to perform "execute" action
     on \a item. \return true on success */
    tristate executeItem(KexiPart::Item* item);

    //! Shows "export as data table" dialog for \a item.
    tristate exportItemAsDataTable(KexiPart::Item* item);

    //! Shows "copy special as data table" dialog for \a item.
    tristate copyItemToClipboardAsDataTable(KexiPart::Item* item);

    bool checkForDirtyFlagOnExport(KexiPart::Item *item, QMap<QString, QString> *args);

    /*! Shows a question message
     * "Design of query %1 that you want to export data from is changed and has not yet been saved.
     * Do you want to use data from the changed query for exporting or from its original (saved) version?"
    \return true if the user picked the first option,
     * false if the user picked the second option and cancelled value if user cancelled the export.
     */
    tristate askOnExportingChangedQuery(KexiPart::Item* item) const;

    //! Shows "print" dialog for \a item.
    //! \return true on success.
    bool printItem(KexiPart::Item* item, const QString& titleText);

    //! Shows "print" dialog for \a item and \a settings.
    //! \return true on success.
    //! @todo reenable when ported  bool printItem(KexiPart::Item* item, const KexiSimplePrintingSettings& settings, const QString& titleText = QString());

    /*! Shows "print preview" window for \a item.
     The preview windoe is cached, so \a reload == true is sometimes needed
     if data or print settings have changed in the meantime.
     \return true on success. */
    bool printPreviewForItem(KexiPart::Item* item, const QString& titleText, bool reload);

    //! Shows "print preview" window.
    //! \return true on success.
    //! @todo reenable when ported  bool printPreviewForItem(KexiPart::Item* item, const KexiSimplePrintingSettings& settings, const QString& titleText = QString(), bool reload = false);

    /*! Implemented for KexiMainWindow. Helper for printItem() and printPreviewForItem().
     Also used by KexiFormEventAction.
     \return true on success and cancelled when the action was cancelled. */
    //! @todo reenable when ported  tristate printActionForItem(KexiPart::Item* item, PrintActionType action);

    void slotSetProjectNavigatorVisible(bool set);
    void slotSetPropertyEditorVisible(bool set);
    void slotProjectNavigatorVisibilityChanged(bool visible);
    void slotPropertyEditorVisibilityChanged(bool visible);
    void slotMultiTabBarTabClicked(int id);

private:
    //! Adds action @a name with text @a text and optional shortcut @a shortcut.
    //! This is helper method containing workaround for Kexi
    //! until QAction::setShortcut() works again.
    //! @return created action
    QAction * addAction(const char *name, const QString &text, const char *shortcut = 0);

    //! Like @ref addAction(const char *, const QString&, const char *) but also adds
    //! icon @a icon.
    //! @return created action
    QAction * addAction(const char *name, const QIcon &icon, const QString& text,
                       const char *shortcut = 0);

    class MessageHandler;
    class Private;
    Private * const d;

    friend class KexiWindow;
    friend class KexiMainWidget;
};

#endif
