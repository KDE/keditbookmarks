/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/
#ifndef RENAMEDIALOG_H
#define RENAMEDIALOG_H

#include "libdolphin_export.h"

#include <kdialog.h>
#include <kurl.h>

class KFileItem;
class KFileItemList;
class KLineEdit;

#include <QString>

/**
 * @brief Dialog for renaming a variable number of files.
 *
 * The renaming is not done by the dialog, the invoker
 * must do this itself:
 * \code
 * RenameDialog dialog(...);
 * if (dialog.exec() == QDialog::Accepted) {
 *     const QString& newName = dialog.newName();
 *     if (newName.isEmpty()) {
 *         // an invalid name has been chosen, use
 *         // dialog.errorString() to tell the user about this
 *     }
 *     else {
 *         // rename items corresponding to the new name
 *     }
 * }
 * \endcode
 */
class LIBDOLPHINPRIVATE_EXPORT RenameDialog : public KDialog
{
    Q_OBJECT

public:
    explicit RenameDialog(QWidget *parent, const KFileItemList& items);
    virtual ~RenameDialog();

    /**
     * Returns the new name of the items. If more than one
     * item should be renamed, then it is assured that the # character
     * is part of the returned string. If the returned string is empty,
     * then RenameDialog::errorString() should be used to show the reason
     * of having an empty string (e. g. if the # character has
     * been deleted by the user, although more than one item should be
     * renamed).
     */
    QString newName() const;

    /**
     * Returns the error string, if Dialog::newName() returned an empty string.
     */
    QString errorString() const;

protected slots:
    virtual void slotButtonClicked(int button);

private:
    bool m_renameOneItem;
    KLineEdit* m_lineEdit;
    QString m_newName;
    QString m_errorString;

    friend class RenameDialogTest; // allow access for unit testing
};

inline QString RenameDialog::newName() const
{
    return m_newName;
}

inline QString RenameDialog::errorString() const
{
    return m_errorString;
}

#endif
