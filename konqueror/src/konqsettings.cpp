/* This file is part of the KDE project
   Copyright (C) 1999, 2007 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqsettings.h"
#include <kprotocolmanager.h>
#include "kglobalsettings.h"
#include <ksharedconfig.h>
#include <kglobal.h>
#include <kmimetype.h>
#include <kdesktopfile.h>
#include <kdebug.h>
#include <assert.h>
#include <kconfiggroup.h>

class KonqEmbedSettingsSingleton
{
public:
    KonqFMSettings self;
};
K_GLOBAL_STATIC(KonqEmbedSettingsSingleton, globalEmbedSettings)

KonqFMSettings* KonqFMSettings::settings()
{
    return &globalEmbedSettings->self;
}

//static
void KonqFMSettings::reparseConfiguration()
{
    if (globalEmbedSettings.exists()) {
        globalEmbedSettings->self.init(true);
    }
}
KonqFMSettings::KonqFMSettings()
{
    init(false);
}

KonqFMSettings::~KonqFMSettings()
{
}

void KonqFMSettings::init(bool reparse)
{
    if (reparse)
        fileTypesConfig()->reparseConfiguration();
    m_embedMap = fileTypesConfig()->entryMap("EmbedSettings");
}

KSharedConfig::Ptr KonqFMSettings::fileTypesConfig()
{
    if (!m_fileTypesConfig) {
        m_fileTypesConfig = KSharedConfig::openConfig("filetypesrc", KConfig::NoGlobals);
    }
    return m_fileTypesConfig;
}

static bool alwaysEmbedMimeTypeGroup(const QString& mimeType)
{
    if ( mimeType.startsWith("inode") || mimeType.startsWith("Browser") || mimeType.startsWith("Konqueror"))
        return true; //always embed mimetype inode/*, Browser/* and Konqueror/*
    return false;
}

bool KonqFMSettings::shouldEmbed(const QString & _mimeType) const
{
    KMimeType::Ptr mime = KMimeType::mimeType(_mimeType, KMimeType::ResolveAliases);
    if (!mime) {
        kWarning() << "Unknown mimetype" << _mimeType;
        return false; // unknown mimetype!
    }
    const QString mimeType = mime->name();

    // First check in user's settings whether to embed or not
    // 1 - in the filetypesrc config file (written by the configuration module)
    QMap<QString, QString>::const_iterator it = m_embedMap.find( QString::fromLatin1("embed-")+mimeType );
    if ( it != m_embedMap.end() ) {
        kDebug(1202) << mimeType << it.value();
        return it.value() == QLatin1String("true");
    }
    // 2 - in the configuration for the group if nothing was found in the mimetype
    if (alwaysEmbedMimeTypeGroup(mimeType))
        return true; //always embed mimetype inode/*, Browser/* and Konqueror/*
    const QString mimeTypeGroup = mimeType.left(mimeType.indexOf('/'));
    it = m_embedMap.find( QString::fromLatin1("embed-")+mimeTypeGroup );
    if ( it != m_embedMap.end() ) {
        kDebug(1202) << mimeType << "group setting:" << it.value();
        return it.value() == QLatin1String("true");
    }
    // 2 bis - configuration for group of parent mimetype, if different
    if (mimeType[0].isLower()) {
        QStringList parents;
        parents.append(mimeType);
        while (!parents.isEmpty()) {
            const QString parent = parents.takeFirst();
            if (alwaysEmbedMimeTypeGroup(parent)) {
                return true;
            }
            KMimeType::Ptr mime = KMimeType::mimeType(parent);
            Q_ASSERT(mime); // how could the -parent- be null?
            if (mime)
                parents += mime->parentMimeTypes();
        }
    }

    // 3 - if no config found, use default.
    // Note: if you change those defaults, also change keditfiletype/mimetypedata.cpp !
    // Embedding is false by default except for image/* and for zip, tar etc.
    const bool hasLocalProtocolRedirect = !KProtocolManager::protocolForArchiveMimetype(mimeType).isEmpty();
    if (mimeTypeGroup == "image" || mimeTypeGroup == "multipart" || hasLocalProtocolRedirect)
        return true;
    return false;
}
