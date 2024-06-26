/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2000 David Faure <faure@kde.org>
   SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#include "importers.h"
#include "globalbookmarkmanager.h"

#include "kbookmarkmodel/model.h"
#include "toplevel.h" // for KEBApp

#include "keditbookmarks_debug.h"

#include <QFileDialog>
#include <QStandardPaths>

#include <KBookmark>
#include <KBookmarkManager>
#include <KMessageBox>

#include "kbookmarkimporter.h"
#include "kbookmarkimporter_ie.h"
#include "kbookmarkimporter_ns.h"
#include "kbookmarkimporter_opera.h"
#include <kbookmarkdombuilder.h>

ImportCommand::ImportCommand(KBookmarkModel *model)
    : QUndoCommand()
    , m_model(model)
    , m_utf8(false)
    , m_folder(false)
    , m_cleanUpCmd(nullptr)
{
}

void ImportCommand::setVisibleName(const QString &visibleName)
{
    m_visibleName = visibleName;
    setText(i18nc("(qtundo-format)", "Import %1 Bookmarks", visibleName));
}

QString ImportCommand::folder() const
{
    return m_folder ? i18n("%1 Bookmarks", visibleName()) : QString();
}

ImportCommand *ImportCommand::importerFactory(KBookmarkModel *model, const QString &type)
{
    if (type == QLatin1String("Galeon"))
        return new GaleonImportCommand(model);
    else if (type == QLatin1String("IE"))
        return new IEImportCommand(model);
    else if (type == QLatin1String("KDE2"))
        return new KDE2ImportCommand(model);
    else if (type == QLatin1String("Opera"))
        return new OperaImportCommand(model);
    // else if (type == "Crashes") return new CrashesImportCommand();
    else if (type == QLatin1String("Moz"))
        return new MozImportCommand(model);
    else if (type == QLatin1String("NS"))
        return new NSImportCommand(model);
    else {
        qCCritical(KEDITBOOKMARKS_LOG) << "ImportCommand::importerFactory() - invalid type (" << type << ")!";
        return nullptr;
    }
}

ImportCommand *ImportCommand::performImport(KBookmarkModel *model, const QString &type, QWidget *top)
{
    ImportCommand *importer = ImportCommand::importerFactory(model, type);

    Q_ASSERT(importer);

    QString mydirname = importer->requestFilename();
    if (mydirname.isEmpty()) {
        delete importer;
        return nullptr;
    }

    int answer = KMessageBox::questionTwoActionsCancel(top,
                                                       i18n("Import as a new subfolder or replace all the current bookmarks?"),
                                                       i18nc("@title:window", "%1 Import", importer->visibleName()),
                                                       KGuiItem(i18n("As New Folder")),
                                                       KGuiItem(i18n("Replace")));

    if (answer == KMessageBox::Cancel) {
        delete importer;
        return nullptr;
    }

    importer->import(mydirname, answer == KMessageBox::ButtonCode::PrimaryAction);
    return importer;
}

void ImportCommand::doCreateHoldingFolder(KBookmarkGroup &bkGroup)
{
    bkGroup = GlobalBookmarkManager::self()->mgr()->root().createNewFolder(folder());
    bkGroup.setIcon(m_icon);
    m_group = bkGroup.address();
}

void ImportCommand::redo()
{
    KBookmarkGroup bkGroup;

    if (!folder().isNull()) {
        doCreateHoldingFolder(bkGroup);

    } else {
        // import into the root, after cleaning it up
        bkGroup = GlobalBookmarkManager::self()->root();
        delete m_cleanUpCmd;
        m_cleanUpCmd = DeleteCommand::deleteAll(m_model, bkGroup);

        new DeleteCommand(m_model, bkGroup.address(), true /* contentOnly */, m_cleanUpCmd);
        m_cleanUpCmd->redo();

        // import at the root
        m_group = QLatin1String("");
    }

    doExecute(bkGroup);

    // notify the model that the data has changed
    //
    // FIXME Resetting the model completely has the unwanted
    // side-effect of collapsing all items in tree views
    // (and possibly other side effects)
    m_model->resetModel();
}

void ImportCommand::undo()
{
    if (!folder().isEmpty()) {
        // we created a group -> just delete it
        DeleteCommand cmd(m_model, m_group);
        cmd.redo();

    } else {
        // we imported at the root -> delete everything
        KBookmarkGroup root = GlobalBookmarkManager::self()->root();
        QUndoCommand *cmd = DeleteCommand::deleteAll(m_model, root);

        cmd->redo();
        delete cmd;

        // and recreate what was there before
        m_cleanUpCmd->undo();
    }
}

QString ImportCommand::affectedBookmarks() const
{
    QString rootAdr = GlobalBookmarkManager::self()->root().address();
    if (m_group == rootAdr)
        return m_group;
    else
        return KBookmark::parentAddress(m_group);
}

/* -------------------------------------- */

QString MozImportCommand::requestFilename() const
{
    static KMozillaBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

QString NSImportCommand::requestFilename() const
{
    static NSBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

QString OperaImportCommand::requestFilename() const
{
    static OperaBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

QString IEImportCommand::requestFilename() const
{
    static IEBookmarkImporterImpl importer;
    return importer.findDefaultLocation();
}

// following two are really just xbel

QString GaleonImportCommand::requestFilename() const
{
    return QFileDialog::getOpenFileName(KEBApp::self(),
                                        QString(),
                                        QString(QDir::homePath() + QStringLiteral("/.galeon")),
                                        i18n("Galeon Bookmark Files (*.xbel)"));
}

QString KDE2ImportCommand::requestFilename() const
{
    return QFileDialog::getOpenFileName(KEBApp::self(),
                                        QString(),
                                        QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + QLatin1String("/konqueror"),
                                        i18n("KDE Bookmark Files (*.xml)"));
}

/* -------------------------------------- */

static void parseInto(const KBookmarkGroup &bkGroup, BookmarkImporterBase *importer)
{
    BookmarkDomBuilder builder(bkGroup);
    builder.connectImporter(importer);
    importer->parse();
}

void OperaImportCommand::doExecute(const KBookmarkGroup &bkGroup)
{
    OperaBookmarkImporterImpl importer;
    importer.setFilename(m_fileName);
    parseInto(bkGroup, &importer);
}

void IEImportCommand::doExecute(const KBookmarkGroup &bkGroup)
{
    IEBookmarkImporterImpl importer;
    importer.setFilename(m_fileName);
    parseInto(bkGroup, &importer);
}

void HTMLImportCommand::doExecute(const KBookmarkGroup &bkGroup)
{
    NSBookmarkImporterImpl importer;
    importer.setFilename(m_fileName);
    importer.setUtf8(m_utf8);
    parseInto(bkGroup, &importer);
}

/* -------------------------------------- */

void XBELImportCommand::doCreateHoldingFolder(KBookmarkGroup &)
{
    // rather than reuse the old group node we transform the
    // root xbel node into the group when doing an xbel import
}

void XBELImportCommand::doExecute(const KBookmarkGroup & /*bkGroup*/)
{
    // check if already open first???
    KBookmarkManager pManager(m_fileName);

    QDomDocument doc = GlobalBookmarkManager::self()->mgr()->internalDocument();

    // get the xbel
    QDomNode subDoc = pManager.internalDocument().namedItem(QStringLiteral("xbel")).cloneNode();
    if (subDoc.isProcessingInstruction())
        subDoc = subDoc.nextSibling();
    if (subDoc.isDocumentType())
        subDoc = subDoc.nextSibling();
    if (subDoc.nodeName() != QLatin1String("xbel"))
        return;

    if (!folder().isEmpty()) {
        // transform into folder
        subDoc.toElement().setTagName(QStringLiteral("folder"));

        // clear attributes
        QStringList tags;
        for (int i = 0; i < subDoc.attributes().count(); i++)
            tags << subDoc.attributes().item(i).toAttr().name();
        for (QStringList::const_iterator it = tags.constBegin(); it != tags.constEnd(); ++it)
            subDoc.attributes().removeNamedItem((*it));

        subDoc.toElement().setAttribute(QStringLiteral("icon"), m_icon);

        // give the folder a name
        QDomElement textElem = doc.createElement(QStringLiteral("title"));
        subDoc.insertBefore(textElem, subDoc.firstChild());
        textElem.appendChild(doc.createTextNode(folder()));
    }

    // import and add it
    QDomNode node = doc.importNode(subDoc, true);

    if (!folder().isEmpty()) {
        GlobalBookmarkManager::self()->root().internalElement().appendChild(node);
        m_group = KBookmarkGroup(node.toElement()).address();

    } else {
        QDomElement root = GlobalBookmarkManager::self()->root().internalElement();

        QList<QDomElement> childList;

        QDomNode n = subDoc.firstChild().toElement();
        while (!n.isNull()) {
            QDomElement e = n.toElement();
            if (!e.isNull())
                childList.append(e);
            n = n.nextSibling();
        }

        QList<QDomElement>::Iterator it = childList.begin();
        QList<QDomElement>::Iterator end = childList.end();
        for (; it != end; ++it)
            root.appendChild((*it));
    }
}

#include "moc_importers.cpp"
