/*****************************************************************************
 * Copyright (C) 2008 by Sebastian Trueg <trueg@kde.org>                     *
 * Copyright (C) 2009-2010 by Peter Penz <peter.penz@gmx.at>                 *
 *                                                                           *
 * This library is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Library General Public               *
 * License version 2 as published by the Free Software Foundation.           *
 *                                                                           *
 * This library is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Library General Public License for more details.                          *
 *                                                                           *
 * You should have received a copy of the GNU Library General Public License *
 * along with this library; see the file COPYING.LIB.  If not, write to      *
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 * Boston, MA 02110-1301, USA.                                               *
 *****************************************************************************/

#ifndef KMETADATAWIDGET_H
#define KMETADATAWIDGET_H

#include <kfileitem.h>

#include <QList>
#include <QWidget>

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
    #define DISABLE_NEPOMUK_LEGACY
    #include <nepomuk/variant.h>
#endif

class KMetaDataModel;
class KUrl;

/**
 * @brief Shows the meta data of one or more file items.
 *
 * Meta data like name, size, rating, comment, ... are
 * shown as several rows containing a description and
 * the meta data value. It is possible for the user
 * to change specific meta data like rating, tags and
 * comment. The changes are stored automatically by the
 * meta data widget.
 *
 * To show more than basic meta data, the meta data widget
 * must be provided with a meta data model
 * (see KMetaDataWidget::setModel()).
 *
 * Per default most meta data values are shown as labels.
 * However it is possible to adjust KMetaDataWidget to use
 * custom widgets for showing and modifying meta data (e. g.
 * like done already for the rating, tags or comments). The
 * following steps are necessary:
 * - Derive a custom widget from KMetaDataWidget.
 * - Create the custom widgets in the constructor and
 *   make them invisible per default.
 * - Implement the methods valueWidget(), setValue() and
 *   (optionally) label() by accessing the custom widgets.
 */
class KMetaDataWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Allows to specify which general data types should be shown
     * by the meta data widget.
     * @see KMetaDataWidget::setVisibleDataTypes()
     * @see KMetaDataWidget::visibleDataTypes()
     */
    enum MetaDataType
    {
        None = 0,
        TypeData = 1,
        SizeData = 2,
        ModifiedData = 4,
        OwnerData =  8,
        PermissionsData = 16,
        RatingData = 32,
        TagsData = 64,
        CommentData = 128
    };
    Q_DECLARE_FLAGS(MetaDataTypes, MetaDataType)

    explicit KMetaDataWidget(QWidget* parent = 0);
    virtual ~KMetaDataWidget();

    /**
     * Triggers the asynchronous loading of the meta data
     * for the file item \p item. Connect to the signal
     * loadingFinished() to be able to read the meta
     * data.
     */
    void setItem(const KFileItem& item);

    /**
     * Triggers the asynchronous loading of the meta data
     * for the file items \p items. Connect to the signal
     * loadingFinished() to be able to read the meta
     * data.
     */
    void setItems(const KFileItemList& items);

    /**
     * Convenience method for KMetaDataWidget::setItem(const KFileItem&),
     * if the application has only an URL and no file item.
     * For performance reason it is recommended to use this convenience
     * method only if the application does not have a file item already.
     */
    void setItem(const KUrl& url);

    /**
     * Convenience method for KMetaDataWidget::setItems(const KFileItemList&),
     * if the application has only URLs and no file items.
     * For performance reason it is recommended to use this convenience
     * method only if the application does not have a file items already.
     */
    void setItems(const QList<KUrl>& urls);

    KFileItemList items() const;

    /**
     * Sets the used model which provides the data for the widget.
     * One model can be shared by several meta data widgets.
     */
    void setModel(KMetaDataModel* model);
    KMetaDataModel* model() const;

    /**
     * If set to true, data like comment, tag or rating cannot be changed by the user.
     * Per default read-only is disabled.
     */
    void setReadOnly(bool readOnly);
    bool isReadOnly() const;

    /**
     * Specifies which kind of data types should be shown (@see KMetaDataWidget::Data).
     * Example: metaDataWidget->setVisibleDataTypes(KMetaDataWidget::TypeData | KMetaDataWidget::ModifiedData);
     * Per default all data types are shown.
     */
    void setVisibleDataTypes(MetaDataTypes data);

    /**
     * Returns which kind of data is shown (@see KMetaDataWidget::Data).
     * Example: if (metaDataWidget->shownData() & KMetaDataWidget::TypeData) ...
     */
    MetaDataTypes visibleDataTypes() const;

    /** @see QWidget::sizeHint() */
    virtual QSize sizeHint() const;

Q_SIGNALS:
    void urlActivated(const KUrl& url);

protected:
#ifdef HAVE_NEPOMUK
    /**
     * @return Translated string for the label of the meta data represented
     *         by \p metaDataUri. If no custom translation is provided, the
     *         base implementation must be invoked.
     */
    virtual QString label(const KUrl& metaDataUri) const;

    /**
     * @return Pointer to the custom value-widget that should be used
     *         to show the meta data represented by \p metaDataUri. If 0
     *         is returned, the meta data will be shown inside a label
     *         as fallback. If no custom value widget is used for the
     *         given URI, the base implementation must be invoked.
     */
    virtual QWidget* valueWidget(const KUrl& metaDataUri) const;

    /**
     * Sets the value of a custom value-widget to \p value. If the meta data
     * represented by \p metaDataUri is not shown by a custom value-widget (see
     * KMetaDataWidget::valueWidget()), then the base implementation must be
     * invoked.
     * @return True, if a custom value-widget is available, where the value got applied.
     */
    virtual bool setValue(const KUrl& metaDataUri, const Nepomuk::Variant& value);
#endif

    virtual bool event(QEvent* event);

private:
    class Private;
    Private* d;

    Q_PRIVATE_SLOT(d, void slotLoadingFinished())
    Q_PRIVATE_SLOT(d, void slotRatingChanged(unsigned int rating))
    Q_PRIVATE_SLOT(d, void slotTagsChanged(const QList<Nepomuk::Tag>& tags))
    Q_PRIVATE_SLOT(d, void slotCommentChanged(const QString& comment))
    Q_PRIVATE_SLOT(d, void slotMetaDataUpdateDone())
    Q_PRIVATE_SLOT(d, void slotLinkActivated(const QString& link))
    Q_PRIVATE_SLOT(d, void slotTagActivated(const Nepomuk::Tag& tag))
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KMetaDataWidget::MetaDataTypes)

#endif
