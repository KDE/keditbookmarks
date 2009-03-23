/* This file is part of the KDE project
   Copyright (C) 1998-2009 David Faure <faure@kde.org>
                 2003      Sven Leiber <s.leiber@web.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KNEWMENU_H
#define KNEWMENU_H

#include <kactionmenu.h>
#include <kurl.h>
#include <libkonq_export.h>

class KJob;
namespace KIO { class Job; }

class KActionCollection;

/**
 * The 'New' submenu, both for the File menu and the RMB popup menu.
 * (The same instance can be used by both).
 * It fills the menu with 'Folder' and one item per installed template.
 *
 * To use this class, you need to connect aboutToShow() of the File menu
 * with slotCheckUpToDate() and to call slotCheckUpToDate() before showing
 * the RMB popupmenu.
 *
 * KNewMenu automatically updates the list of templates shown if installed templates
 * are added/updated/deleted.
 *
 * @author David Faure <faure@kde.org>
 * Ideas and code for the new template handling mechanism ('link' desktop files)
 * from Christoph Pickart <pickart@iam.uni-bonn.de>
 */
class LIBKONQ_EXPORT KNewMenu : public KActionMenu
{
  Q_OBJECT
public:

    /**
     * Constructor
     * @param parent the parent KActionCollection this KAction should be
     * added to.
     * @param parentWidget the parent widget that will be the owner of
     * this KNewMenu and that will take care of destroying this instance
     * once the parentWidget itself got destroyed.
     * @param name action name, when adding the action to the collection
     */
    KNewMenu( KActionCollection * parent, QWidget* parentWidget, const QString& name );
    virtual ~KNewMenu();

    /**
     * Set the files the popup is shown for
     * Call this before showing up the menu
     */
    void setPopupFiles(const KUrl::List& files);

public Q_SLOTS:
    /**
     * Checks if updating the list is necessary
     * IMPORTANT : Call this in the slot for aboutToShow.
     */
    void slotCheckUpToDate();

    /**
     * Call this to create a new directory as if the user had done it using
     * a popupmenu. This is useful to make sure that creating a directory with
     * a key shortcut (e.g. F10) triggers the exact same code as when using
     * the New menu.
     * Requirements: call setPopupFiles first, and keep this KNewMenu instance
     * alive (the mkdir is async).
     * @since 4.3
     */
    void createDirectory();

Q_SIGNALS:
    /**
     * Emitted once @p url has been successfully created
     */
    void itemCreated(const KUrl& url);

protected Q_SLOTS:
    /**
     * Called when the job that copied the template has finished.
     * This method is virtual so that error handling can be reimplemented.
     * Make sure to call the base class slotResult when !job->error() though.
     */
    virtual void slotResult( KJob* job );

private Q_SLOTS:
    /**
     * Called when New->* is clicked
     */
    void slotActionTriggered(QAction*);

    /**
     * Fills the templates list.
     */
    void slotFillTemplates();

private:
    void newDir();

    /**
     * Fills the menu from the templates list.
     */
    void fillMenu();

    /**
     * Opens the desktop files and completes the Entry list
     * Input: the entry list. Output: the entry list ;-)
     */
    void parseFiles();

    /**
     * Make the main menus on the startup.
     */
    void makeMenus();

    /**
     * For entryType
     * LINKTOTEMPLATE: a desktop file that points to a file or dir to copy
     * TEMPLATE: a real file to copy as is (the KDE-1.x solution)
     * SEPARATOR: to put a separator in the menu
     * 0 means: not parsed, i.e. we don't know
     */
    enum { LINKTOTEMPLATE = 1, TEMPLATE, SEPARATOR };

    class KNewMenuPrivate;
    KNewMenuPrivate* d;
};

#endif
