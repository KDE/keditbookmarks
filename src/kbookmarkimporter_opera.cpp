/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2002-2003 Alexander Kellett <lypanov@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "kbookmarkimporter_opera.h"
#include "kbookmarkimporter_opera_p.h"

#include <QApplication>
#include <QDebug>
#include <QFileDialog>

#include <qplatformdefs.h>

void KOperaBookmarkImporter::parseOperaBookmarks()
{
    QFile file(m_fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QString url;
    QString name;
    QString type;
    int lineno = 0;
    QTextStream stream(&file);
    while (!stream.atEnd()) {
        lineno++;
        QString line = stream.readLine().trimmed();

        // first two headers lines contain details about the format
        if (lineno <= 2) {
            if (line.startsWith(QLatin1String("options:"), Qt::CaseInsensitive)) {
                const auto lst = line.mid(8).split(QLatin1Char(','));
                for (const QString &ba : lst) {
                    const int pos = ba.indexOf(QLatin1Char('='));
                    if (pos < 1) {
                        continue;
                    }
                }
            }
            continue;
        }

        // at least up till version<=3 the following is valid
        if (line.isEmpty()) {
            // end of data block
            if (type.isNull()) {
                continue;
            } else if (type == QLatin1String("URL")) {
                Q_EMIT newBookmark(name, url, QLatin1String(""));
            } else if (type == QLatin1String("FOLDER")) {
                Q_EMIT newFolder(name, false, QLatin1String(""));
            }

            type.clear();
            name.clear();
            url.clear();
        } else if (line == QLatin1String("-")) {
            // end of folder
            Q_EMIT endFolder();
        } else {
            // data block line
            QString tag;
            if (tag = QStringLiteral("#"), line.startsWith(tag)) {
                type = line.remove(0, tag.length());
            } else if (tag = QStringLiteral("NAME="), line.startsWith(tag)) {
                name = line.remove(0, tag.length());
            } else if (tag = QStringLiteral("URL="), line.startsWith(tag)) {
                url = line.remove(0, tag.length());
            }
        }
    }
}

QString KOperaBookmarkImporter::operaBookmarksFile()
{
    static OperaBookmarkImporterImpl *p = nullptr;
    if (!p) {
        p = new OperaBookmarkImporterImpl;
    }
    return p->findDefaultLocation();
}

void OperaBookmarkImporterImpl::parse()
{
    KOperaBookmarkImporter importer(m_fileName);
    setupSignalForwards(&importer, this);
    importer.parseOperaBookmarks();
}

QString OperaBookmarkImporterImpl::findDefaultLocation(bool saving) const
{
    const QString operaHomePath = QDir::homePath() + QLatin1String("/.opera");
    return saving ? QFileDialog::getSaveFileName(QApplication::activeWindow(), QString(), operaHomePath, tr("Opera Bookmark Files (*.adr)"))
                  : QFileDialog::getOpenFileName(QApplication::activeWindow(), QString(), operaHomePath, tr("Opera Bookmark Files (*.adr);;All Files (*)"));
}

/////////////////////////////////////////////////

class OperaExporter : private KBookmarkGroupTraverser
{
public:
    OperaExporter();
    QString generate(const KBookmarkGroup &grp)
    {
        traverse(grp);
        return m_string;
    }

private:
    void visit(const KBookmark &) override;
    void visitEnter(const KBookmarkGroup &) override;
    void visitLeave(const KBookmarkGroup &) override;

private:
    QString m_string;
    QTextStream m_out;
};

OperaExporter::OperaExporter()
    : m_out(&m_string, QIODevice::WriteOnly)
{
    m_out << "Opera Hotlist version 2.0\n";
    m_out << "Options: encoding = utf8, version=3\n";
    m_out.flush();
}

void OperaExporter::visit(const KBookmark &bk)
{
    // qCDebug(KBOOKMARKS_LOG) << "visit(" << bk.text() << ")";
    m_out << "#URL\n";
    m_out << "\tNAME=" << bk.fullText() << '\n';
    m_out << "\tURL=" << bk.url().toString().toUtf8() << '\n';
    m_out << '\n';
    m_out.flush();
}

void OperaExporter::visitEnter(const KBookmarkGroup &grp)
{
    // qCDebug(KBOOKMARKS_LOG) << "visitEnter(" << grp.text() << ")";
    m_out << "#FOLDER\n";
    m_out << "\tNAME=" << grp.fullText() << '\n';
    m_out << '\n';
    m_out.flush();
}

void OperaExporter::visitLeave(const KBookmarkGroup &)
{
    // qCDebug(KBOOKMARKS_LOG) << "visitLeave()";
    m_out << "-\n";
    m_out << '\n';
    m_out.flush();
}

void KOperaBookmarkExporterImpl::write(const KBookmarkGroup &parent)
{
    OperaExporter exporter;
    QString content = exporter.generate(parent);
    QFile file(m_fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        qCritical() << "Can't write to file " << m_fileName;
        return;
    }
    QTextStream fstream(&file);
    fstream << content;
}

#include "moc_kbookmarkimporter_opera.cpp"
#include "moc_kbookmarkimporter_opera_p.cpp"
