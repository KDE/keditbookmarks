// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    tstream.setCodec("UTF-8");
#endif
    tstream << toString(grp, showAddress);
}

QString HTMLExporter::toString(const KBookmarkGroup &grp, bool showAddress)
{
    m_showAddress = showAddress;
    traverse(grp);
    return "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
           "<html><head><title>"+i18n("My Bookmarks")+"</title>\n"
           "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">"
           "</head>\n"
           "<body>\n"
           "<div>"
         + m_string +
           "</div>\n"
           "</body>\n</html>\n";
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
