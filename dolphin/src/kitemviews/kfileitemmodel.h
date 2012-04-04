/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef KFILEITEMMODEL_H
#define KFILEITEMMODEL_H

#include <libdolphin_export.h>
#include <KFileItemList>
#include <KUrl>
#include <kitemviews/kfileitemmodelfilter_p.h>
#include <kitemviews/kitemmodelbase.h>

#include <QHash>

class KDirLister;
class QTimer;

/**
 * @brief KItemModelBase implementation for KFileItems.
 *
 * KFileItemModel is connected with one KDirLister. Each time the KDirLister
 * emits new items, removes items or changes items the model gets synchronized.
 *
 * KFileItemModel supports sorting and grouping of items. Additional roles that
 * are not part of KFileItem can be added with KFileItemModel::setData().
 *
 * Also the recursive expansion of sub-directories is supported by
 * KFileItemModel::setExpanded().
 *
 * TODO: In the longterm instead of passing a KDirLister just an URL should
 * be passed and a KDirLister used internally. This solves the following issues:
 * - The user of the API does not need to decide whether he listens to KDirLister
 *   or KFileItemModel.
 * - It resolves minor conceptual differences between KDirLister and KFileItemModel.
 *   E.g. there is no way for KFileItemModel to check whether a completed() signal
 *   will be emitted after newItems() will be send by KDirLister or not (in the case
 *   of setShowingDotFiles() no completed() signal will get emitted).
 */
class LIBDOLPHINPRIVATE_EXPORT KFileItemModel : public KItemModelBase
{
    Q_OBJECT

public:
    explicit KFileItemModel(KDirLister* dirLister, QObject* parent = 0);
    virtual ~KFileItemModel();

    virtual int count() const;
    virtual QHash<QByteArray, QVariant> data(int index) const;
    virtual bool setData(int index, const QHash<QByteArray, QVariant>& values);

    /**
     * Sets a separate sorting with folders first (true) or a mixed sorting of files and folders (false).
     */
    void setSortFoldersFirst(bool foldersFirst);
    bool sortFoldersFirst() const;

    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const;

    /**
     * If set to true, only folders are shown as items of the model. Files
     * are ignored.
     */
    void setShowFoldersOnly(bool enabled);
    bool showFoldersOnly() const;

    /** @reimp */
    virtual QMimeData* createMimeData(const QSet<int>& indexes) const;

    /** @reimp */
    virtual int indexForKeyboardSearch(const QString& text, int startFromIndex = 0) const;

    /** @reimp */
    virtual bool supportsDropping(int index) const;

    /** @reimp */
    virtual QString roleDescription(const QByteArray& typeForRole) const;

    /** @reimp */
    virtual QList<QPair<int, QVariant> > groups() const;

    /**
     * @return The file-item for the index \a index. If the index is in a valid
     *         range it is assured that the file-item is not null. The runtime
     *         complexity of this call is O(1).
     */
    KFileItem fileItem(int index) const;

    /**
     * @return The file-item for the url \a url. If no file-item with the given
     *         URL is found KFileItem::isNull() will be true for the returned
     *         file-item. The runtime complexity of this call is O(1).
     */
    KFileItem fileItem(const KUrl& url) const;

    /**
     * @return The index for the file-item \a item. -1 is returned if no file-item
     *         is found or if the file-item is null. The runtime
     *         complexity of this call is O(1).
     */
    int index(const KFileItem& item) const;

    /**
     * @return The index for the URL \a url. -1 is returned if no file-item
     *         is found. The runtime complexity of this call is O(1).
     */
    int index(const KUrl& url) const;

    /**
     * @return Root item of all items.
     */
    KFileItem rootItem() const;

    /**
     * Clears all items of the model.
     */
    void clear();

    // TODO: "name" + "isDir" is default in ctor
    void setRoles(const QSet<QByteArray>& roles);
    QSet<QByteArray> roles() const;

    virtual bool setExpanded(int index, bool expanded);
    virtual bool isExpanded(int index) const;
    virtual bool isExpandable(int index) const;
    virtual int expandedParentsCount(int index) const;

    QSet<KUrl> expandedUrls() const;

    /**
     * Marks the URLs in \a urls as subfolders which were expanded previously.
     * They are re-expanded one by one each time the KDirLister's completed() signal is received.
     * Note that a manual triggering of the KDirLister is required.
     */
    void restoreExpandedUrls(const QSet<KUrl>& urls);

    /**
     * Expands all parent-items of \a url.
     */
    void expandParentItems(const KUrl& url);

    void setNameFilter(const QString& nameFilter);
    QString nameFilter() const;

    struct RoleInfo
    {   QByteArray role;
        QString translation;
        QString group;
        bool requiresNepomuk;
        bool requiresIndexer;
    };

    /**
     * @return Provides static information for all available roles that
     *         are supported by KFileItemModel. Some roles can only be
     *         determined if Nepomuk is enabled and/or the Nepomuk
     *         indexing is enabled.
     */
    static QList<RoleInfo> rolesInformation();

signals:
    /**
     * Is emitted after the loading of a directory has been completed or new
     * items have been inserted to an already loaded directory. Usually
     * one or more itemsInserted() signals are emitted before loadingCompleted()
     * (the only exception is loading an empty directory, where only a
     * loadingCompleted() signal gets emitted).
     */
    void loadingCompleted();

protected:
    virtual void onGroupedSortingChanged(bool current);
    virtual void onSortRoleChanged(const QByteArray& current, const QByteArray& previous);
    virtual void onSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous);

private slots:
    /**
     * Resorts all items dependent on the set sortRole(), sortOrder()
     * and foldersFirst() settings.
     */
    void resortAllItems();

    void slotCompleted();
    void slotCanceled();
    void slotNewItems(const KFileItemList& items);
    void slotItemsDeleted(const KFileItemList& items);
    void slotRefreshItems(const QList<QPair<KFileItem, KFileItem> >& items);
    void slotClear();
    void slotClear(const KUrl& url);
    void slotNaturalSortingChanged();

    void dispatchPendingItemsToInsert();

private:
    enum RoleType {
        // User visible roles:
        NoRole, NameRole, SizeRole, DateRole, PermissionsRole, OwnerRole,
        GroupRole, TypeRole, DestinationRole, PathRole,
        // User visible roles available with Nepomuk:
        CommentRole, TagsRole, RatingRole, ImageSizeRole, OrientationRole,
        WordCountRole, LineCountRole, ArtistRole, AlbumRole, DurationRole, TrackRole,
        CopiedFromRole,
        // Non-visible roles:
        IsDirRole, IsExpandedRole, IsExpandableRole, ExpandedParentsCountRole,
        // Mandatory last entry:
        RolesCount
    };

    struct ItemData
    {
        KFileItem item;
        QHash<QByteArray, QVariant> values;
        ItemData* parent;
    };

    void insertItems(const KFileItemList& items);
    void removeItems(const KFileItemList& items);

    /**
     * Helper method for insertItems() and removeItems(): Creates
     * a list of ItemData elements based on the given items.
     * Note that the ItemData instances are created dynamically and
     * must be deleted by the caller.
     */
    QList<ItemData*> createItemDataList(const KFileItemList& items) const;

    void removeExpandedItems();

    /**
     * Resets all values from m_requestRole to false.
     */
    void resetRoles();

    /**
     * @return Role-type for the given role.
     *         Runtime complexity is O(1).
     */
    RoleType typeForRole(const QByteArray& role) const;

    /**
     * @return Role-byte-array for the given role-type.
     *         Runtime complexity is O(1).
     */
    QByteArray roleForType(RoleType roleType) const;

    QHash<QByteArray, QVariant> retrieveData(const KFileItem& item) const;

    /**
     * @return True if the item-data \a a should be ordered before the item-data
     *         \b. The item-data may have different parent-items.
     */
    bool lessThan(const ItemData* a, const ItemData* b) const;

    /**
     * Helper method for lessThan() and expandedParentsCountCompare(): Compares
     * the passed item-data using m_sortRole as criteria. Both items must
     * have the same parent item, otherwise the comparison will be wrong.
     */
    int sortRoleCompare(const ItemData* a, const ItemData* b) const;

    int stringCompare(const QString& a, const QString& b) const;

    /**
     * Compares the expansion level of both items. The "expansion level" is defined
     * by the number of parent directories. However simply comparing just the numbers
     * is not sufficient, it is also important to check the hierarchy for having
     * a correct order like shown in a tree.
     */
    int expandedParentsCountCompare(const ItemData* a, const ItemData* b) const;

    /**
     * Helper method for expandedParentsCountCompare().
     */
    QString subPath(const KFileItem& item,
                    const QString& itemPath,
                    int start,
                    bool* isDir) const;

    bool useMaximumUpdateInterval() const;

    QList<QPair<int, QVariant> > nameRoleGroups() const;
    QList<QPair<int, QVariant> > sizeRoleGroups() const;
    QList<QPair<int, QVariant> > dateRoleGroups() const;
    QList<QPair<int, QVariant> > permissionRoleGroups() const;
    QList<QPair<int, QVariant> > ratingRoleGroups() const;
    QList<QPair<int, QVariant> > genericStringRoleGroups(const QByteArray& typeForRole) const;

    /**
     * Helper method for all xxxRoleGroups() methods to check whether the
     * item with the given index is a child-item. A child-item is defined
     * as item having an expansion-level > 0. All xxxRoleGroups() methods
     * should skip the grouping if the item is a child-item (although
     * KItemListView would be capable to show sub-groups in groups this
     * results in visual clutter for most usecases).
     */
    bool isChildItem(int index) const;

    /**
     * @return Recursive list of child items that have \a item as upper most parent.
     */
    KFileItemList childItems(const KFileItem& item) const;

    /**
     * Maps the QByteArray-roles to RoleTypes and provides translation- and
     * group-contexts.
     */
    struct RoleInfoMap
    {
        const char* const role;
        const RoleType roleType;
        const char* const roleTranslationContext;
        const char* const roleTranslation;
        const char* const groupTranslationContext;
        const char* const groupTranslation;
        const bool requiresNepomuk;
        const bool requiresIndexer;
    };

    /**
     * @return Map of user visible roles that are accessible by KFileItemModel::rolesInformation().
     */
    static const RoleInfoMap* rolesInfoMap(int& count);

private:
    QWeakPointer<KDirLister> m_dirLister;

    bool m_naturalSorting;
    bool m_sortFoldersFirst;

    RoleType m_sortRole;
    QSet<QByteArray> m_roles;
    Qt::CaseSensitivity m_caseSensitivity;

    QList<ItemData*> m_itemData;
    QHash<KUrl, int> m_items; // Allows O(1) access for KFileItemModel::index(const KFileItem& item)

    KFileItemModelFilter m_filter;
    QSet<KFileItem> m_filteredItems; // Items that got hidden by KFileItemModel::setNameFilter()

    bool m_requestRole[RolesCount];

    QTimer* m_maximumUpdateIntervalTimer;
    QTimer* m_resortAllItemsTimer;
    KFileItemList m_pendingItemsToInsert;

    // Cache for KFileItemModel::groups()
    mutable QList<QPair<int, QVariant> > m_groups;

    // Stores the smallest expansion level of the root-URL. Is required to calculate
    // the "expandedParentsCount" role in an efficient way. A value < 0 indicates a
    // special meaning:
    enum ExpandedParentsCountRootTypes
    {
        // m_expandedParentsCountRoot is uninitialized and must be determined by checking
        // the root URL from the KDirLister.
        UninitializedExpandedParentsCountRoot = -1,
        // All items should be forced to get an expanded parents count of 0 even if they
        // represent child items. This is useful for slaves that provide no parent items
        // for child items like e.g. the search IO slaves.
        ForceExpandedParentsCountRoot = -2
    };
    mutable int m_expandedParentsCountRoot;

    // Stores the URLs of the expanded folders.
    QSet<KUrl> m_expandedUrls;

    // URLs that must be expanded. The expanding is initially triggered in setExpanded()
    // and done step after step in slotCompleted().
    QSet<KUrl> m_urlsToExpand;

    friend class KFileItemModelSortAlgorithm;  // Accesses lessThan() method
    friend class KFileItemModelTest;           // For unit testing
};

inline bool KFileItemModel::isChildItem(int index) const
{
    return m_requestRole[ExpandedParentsCountRole] && m_itemData.at(index)->values.value("expandedParentsCount").toInt() > 0;
}

#endif


