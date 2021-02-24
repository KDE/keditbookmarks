// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) version 3.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#ifndef __importers_h
#define __importers_h

#include "kbookmarkmodel/commands.h"
#include <KLocalizedString>

#include <QObject>

class KBookmark;

// TODO: remove QObject base class? doesn't seem to be used.
class ImportCommand : public QObject, public QUndoCommand, public IKEBCommand
{
    Q_OBJECT
public:
    explicit ImportCommand(KBookmarkModel *model);

    virtual void import(const QString &fileName, bool folder) = 0;

    void setVisibleName(const QString &visibleName);
    QString visibleName() const
    {
        return m_visibleName;
    }
    virtual QString requestFilename() const = 0;

    static ImportCommand *performImport(KBookmarkModel *model, const QString &, QWidget *);
    static ImportCommand *importerFactory(KBookmarkModel *model, const QString &);

    virtual ~ImportCommand()
    {
    }

    void redo() override;
    void undo() override;
    QString affectedBookmarks() const override;

    QString groupAddress() const
    {
        return m_group;
    }
    QString folder() const;

protected:
    /**
     * @param fileName HTML file to import
     * @param folder name of the folder to create. Empty for no creation (root()).
     * @param icon icon for the new folder, if @p folder isn't empty
     * @param utf8 true if the HTML is in utf-8 encoding
     */
    void init(const QString &fileName, bool folder, const QString &icon, bool utf8)
    {
        m_fileName = fileName;
        m_folder = folder;
        m_icon = icon;
        m_utf8 = utf8;
    }

    virtual void doCreateHoldingFolder(KBookmarkGroup &bkGroup);
    virtual void doExecute(const KBookmarkGroup &) = 0;

protected:
    KBookmarkModel *m_model;
    QString m_visibleName;
    QString m_fileName;
    QString m_icon;
    QString m_group;
    bool m_utf8;

private:
    bool m_folder;
    QUndoCommand *m_cleanUpCmd;
};

// part pure
class XBELImportCommand : public ImportCommand
{
public:
    explicit XBELImportCommand(KBookmarkModel *model)
        : ImportCommand(model)
    {
    }
    void import(const QString &fileName, bool folder) override = 0;
    QString requestFilename() const override = 0;

private:
    void doCreateHoldingFolder(KBookmarkGroup &bkGroup) override;
    void doExecute(const KBookmarkGroup &) override;
};

class GaleonImportCommand : public XBELImportCommand
{
public:
    explicit GaleonImportCommand(KBookmarkModel *model)
        : XBELImportCommand(model)
    {
        setVisibleName(i18n("Galeon"));
    }
    void import(const QString &fileName, bool folder) override
    {
        init(fileName, folder, QLatin1String(""), false);
    }
    QString requestFilename() const override;
};

class KDE2ImportCommand : public XBELImportCommand
{
public:
    explicit KDE2ImportCommand(KBookmarkModel *model)
        : XBELImportCommand(model)
    {
        setVisibleName(i18n("KDE"));
    }
    void import(const QString &fileName, bool folder) override
    {
        init(fileName, folder, QLatin1String(""), false);
    }
    QString requestFilename() const override;
};

// part pure
class HTMLImportCommand : public ImportCommand
{
public:
    explicit HTMLImportCommand(KBookmarkModel *model)
        : ImportCommand(model)
    {
    }
    void import(const QString &fileName, bool folder) override = 0;
    QString requestFilename() const override = 0;

private:
    void doExecute(const KBookmarkGroup &) override;
};

class NSImportCommand : public HTMLImportCommand
{
public:
    explicit NSImportCommand(KBookmarkModel *model)
        : HTMLImportCommand(model)
    {
        setVisibleName(i18n("Netscape"));
    }
    void import(const QString &fileName, bool folder) override
    {
        init(fileName, folder, QStringLiteral("netscape"), false);
    }
    QString requestFilename() const override;
};

class MozImportCommand : public HTMLImportCommand
{
public:
    explicit MozImportCommand(KBookmarkModel *model)
        : HTMLImportCommand(model)
    {
        setVisibleName(i18n("Mozilla"));
    }
    void import(const QString &fileName, bool folder) override
    {
        init(fileName, folder, QStringLiteral("mozilla"), true);
    }
    QString requestFilename() const override;
};

class IEImportCommand : public ImportCommand
{
public:
    explicit IEImportCommand(KBookmarkModel *model)
        : ImportCommand(model)
    {
        setVisibleName(i18n("IE"));
    }
    void import(const QString &fileName, bool folder) override
    {
        init(fileName, folder, QLatin1String(""), false);
    }
    QString requestFilename() const override;

private:
    void doExecute(const KBookmarkGroup &) override;
};

class OperaImportCommand : public ImportCommand
{
public:
    explicit OperaImportCommand(KBookmarkModel *model)
        : ImportCommand(model)
    {
        setVisibleName(i18n("Opera"));
    }
    void import(const QString &fileName, bool folder) override
    {
        init(fileName, folder, QStringLiteral("opera"), false);
    }
    QString requestFilename() const override;

private:
    void doExecute(const KBookmarkGroup &) override;
};

#endif
