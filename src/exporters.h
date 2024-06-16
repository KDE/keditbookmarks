/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2003 Alexander Kellett <lypanov@kde.org>

   SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only
*/

#ifndef __exporters_h
#define __exporters_h

#include <KBookmark>
// Added by qt3to4:
#include <QTextStream>

class HTMLExporter : private KBookmarkGroupTraverser
{
public:
    HTMLExporter();
    ~HTMLExporter() override
    {
    }
    QString toString(const KBookmarkGroup &, bool showAddress = false);
    void write(const KBookmarkGroup &, const QString &, bool showAddress = false);

private:
    void visit(const KBookmark &) override;
    void visitEnter(const KBookmarkGroup &) override;
    void visitLeave(const KBookmarkGroup &) override;

private:
    QString m_string;
    QTextStream m_out;
    bool m_showAddress;
};

#endif
