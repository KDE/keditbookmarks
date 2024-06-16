/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#include "exporters.h"

#include "keditbookmarks_debug.h"
#include <KLocalizedString>

#include <QFile>

HTMLExporter::HTMLExporter()
    : m_out(&m_string, QIODevice::WriteOnly)
{
}

void HTMLExporter::write(const KBookmarkGroup &grp, const QString &filename, bool showAddress)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) {
        qCCritical(KEDITBOOKMARKS_LOG) << "Can't write to file " << filename;
        return;
    }
    QTextStream tstream(&file);
    tstream << toString(grp, showAddress);
}

QString HTMLExporter::toString(const KBookmarkGroup &grp, bool showAddress)
{
    m_showAddress = showAddress;
    traverse(grp);
    return QStringLiteral("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n")
        + QStringLiteral("<html><head><title>") + i18n("My Bookmarks") + QStringLiteral("</title>\n")
        + QStringLiteral("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">") + QStringLiteral("</head>\n") + QStringLiteral("<body>\n")
        + QStringLiteral("<div>") + m_string + QStringLiteral("</div>\n") + QStringLiteral("</body>\n</html>\n");
}

void HTMLExporter::visit(const KBookmark &bk)
{
    // //qCDebug(KEDITBOOKMARKS_LOG) << "visit(" << bk.text() << ")";
    if (bk.isSeparator()) {
        m_out << bk.fullText() << "<br>" << QLatin1Char('\n');
    } else {
        if (m_showAddress) {
            m_out << bk.fullText() << "<br>" << QLatin1Char('\n');
            m_out << "<i><div style =\"margin-left: 1em\">" << bk.url().url().toUtf8() << "</div></i>";
        } else {
            m_out << "<a href=\"" << bk.url().url().toUtf8() << "\">";
            m_out << bk.fullText() << "</a><br>" << QLatin1Char('\n');
        }
    }
    m_out.flush();
}

void HTMLExporter::visitEnter(const KBookmarkGroup &grp)
{
    // //qCDebug(KEDITBOOKMARKS_LOG) << "visitEnter(" << grp.text() << ")";
    m_out << "<b>" << grp.fullText() << "</b><br>" << QLatin1Char('\n');
    m_out << "<div style=\"margin-left: 2em\">" << QLatin1Char('\n');
    m_out.flush();
}

void HTMLExporter::visitLeave(const KBookmarkGroup &)
{
    // //qCDebug(KEDITBOOKMARKS_LOG) << "visitLeave()";
    m_out << "</div>" << QLatin1Char('\n');
    m_out.flush();
}
