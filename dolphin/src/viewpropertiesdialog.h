/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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

#ifndef VIEWPROPERTIESDIALOG_H
#define VIEWPROPERTIESDIALOG_H

#include <kdialog.h>

class QCheckBox;
class QComboBox;
class QRadioButton;
class ViewProperties;
class DolphinView;

/**
 * @brief Dialog for changing the current view properties of a directory.
 *
 * It is possible to specify the view mode, the sorting order, whether hidden files
 * and previews should be shown. The properties can be assigned to the current folder,
 * or recursively to all sub folders.
 */
class ViewPropertiesDialog : public KDialog
{
    Q_OBJECT

public:
    explicit ViewPropertiesDialog(DolphinView* dolphinView);
    virtual ~ViewPropertiesDialog();

private slots:
    void slotOk();
    void slotApply();
    void slotViewModeChanged(int index);
    void slotSortingChanged(int index);
    void slotSortOrderChanged(int index);
    void slotCategorizedSortingChanged();
    void slotAdditionalInfoChanged(int index);
    void slotShowPreviewChanged();
    void slotShowHiddenFilesChanged();
    void markAsDirty();

private:
    void loadSettings();

private:
    bool m_isDirty;
    DolphinView* m_dolphinView;
    ViewProperties* m_viewProps;

    QComboBox* m_viewMode;
    QComboBox* m_sortOrder;
    QComboBox* m_sorting;
    QComboBox* m_additionalInfo;
    QCheckBox* m_showPreview;
    QCheckBox* m_showInGroups;
    QCheckBox* m_showHiddenFiles;
    QRadioButton* m_applyToCurrentFolder;
    QRadioButton* m_applyToSubFolders;
    QRadioButton* m_applyToAllFolders;
    QCheckBox* m_useAsDefault;

    void applyViewProperties();
};

#endif
