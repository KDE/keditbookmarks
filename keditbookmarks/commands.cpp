/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "commands.h"
#include "toplevel.h"
#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>
#include <kdebug.h>
#include <klistview.h>
#include <klocale.h>

void MoveCommand::execute()
{
    kdDebug() << "MoveCommand::execute moving from=" << m_from << " to=" << m_to << endl;

    // Look for m_from in the QDom tree
    KBookmark bk = KBookmarkManager::self()->findByAddress( m_from );
    ASSERT( !bk.isNull() );
    //kdDebug() << "BEFORE:" << KBookmarkManager::self()->internalDocument().toCString() << endl;
    int posInOldParent = KBookmark::positionInParent( m_from );
    KBookmark oldParent = KBookmarkManager::self()->findByAddress( KBookmark::parentAddress( m_from ) );
    KBookmark oldPreviousSibling = posInOldParent == 0 ? KBookmark(QDomElement())
                                   : KBookmarkManager::self()->findByAddress( KBookmark::previousAddress( m_from ) );

    // Look for m_to in the QDom tree (as parent address and position in parent)
    int posInNewParent = KBookmark::positionInParent( m_to );
    QString parentAddress = KBookmark::parentAddress( m_to );
    kdDebug() << "MoveCommand::execute parentAddress=" << parentAddress << " posInNewParent=" << posInNewParent << endl;
    KBookmark newParentBk = KBookmarkManager::self()->findByAddress( parentAddress );
    ASSERT( !newParentBk.isNull() );

    if ( posInNewParent == 0 ) // First child
    {
        newParentBk.toGroup().moveItem( bk, QDomElement() );
    }
    else
    {
        QString afterAddress = KBookmark::previousAddress( m_to );
        kdDebug() << "MoveCommand::execute afterAddress=" << afterAddress << endl;
        KBookmark afterNow = KBookmarkManager::self()->findByAddress( afterAddress );
        ASSERT(!afterNow.isNull());
        QDomNode result = newParentBk.internalElement().insertAfter( bk.internalElement(), afterNow.internalElement() );
        ASSERT(!result.isNull());
        kdDebug() << "MoveCommand::execute after moving in the dom tree : item=" << bk.address() << endl;
    }

    // Ok, now this is the most tricky bit.
    // Because we moved stuff around, the addresses from/to can have changed
    // So we look into the dom tree to get the new positions, using a reference
    // The reference is :
    if ( posInOldParent == 0 ) // the old parent, if we were the first child
        m_from = oldParent.address() + "/0";
    else // otherwise the previous sibling
        m_from = KBookmark::nextAddress( oldPreviousSibling.address() );
    m_to = bk.address();
    kdDebug() << "MoveCommand::execute : new addresses from=" << m_from << " to=" << m_to << endl;
}

void MoveCommand::unexecute()
{
    // Let's not duplicate code.
    MoveCommand undoCmd( "dummy", m_to, m_from );
    undoCmd.execute();
    // Get the addresses back from that command, in case they changed
    m_from = undoCmd.m_to;
    m_to = undoCmd.m_from;
}

void CreateCommand::execute()
{
    // Gather some info
    QString parentAddress = KBookmark::parentAddress( m_to );
    KBookmarkGroup parentGroup = KBookmarkManager::self()->findByAddress( parentAddress ).toGroup();
    QString previousSibling = KBookmark::previousAddress( m_to );
    //kdDebug() << "CreateCommand::execute previousSibling=" << previousSibling << endl;
    KBookmark prev = previousSibling.isEmpty() ? KBookmark(QDomElement())
                     : KBookmarkManager::self()->findByAddress( previousSibling );

    // Create
    KBookmark bk = KBookmark(QDomElement());
    if (m_separator)
        bk = parentGroup.createNewSeparator();
    else
        if (m_group)
        {
            bk = parentGroup.createNewFolder( m_text );
            m_text = bk.fullText(); // remember it, we won't have to ask it again
        }
        else
            bk = parentGroup.addBookmark( m_text, m_url );

    // Move to right position
    parentGroup.moveItem( bk, prev );
    // Open the parent (useful if it was empty)
    parentGroup.internalElement().setAttribute( "OPEN", 1 );
    ASSERT( bk.address() == m_to );
}

void CreateCommand::unexecute()
{
    kdDebug() << "CreateCommand::unexecute deleting " << m_to << endl;
    KBookmark bk = KBookmarkManager::self()->findByAddress( m_to );
    ASSERT( !bk.isNull() );
    // Update GUI
    // but first we need to select a valid item
    KListView * lv = KEBTopLevel::self()->listView();
    KEBListViewItem * item = static_cast<KEBListViewItem*>(lv->selectedItem());
    if (item && item->bookmark().address() == m_to)
    {
        lv->setSelected(item,false);
        // can't use itemBelow here, in case we're deleting a folder
        if ( item->nextSibling() )
            lv->setSelected(item->nextSibling(),true);
    }

    bk.parentGroup().deleteBookmark( bk );
}

void DeleteCommand::execute()
{
    kdDebug() << "DeleteCommand::execute " << m_from << endl;
    KBookmark bk = KBookmarkManager::self()->findByAddress( m_from );
    if ( !m_cmd )
        if ( bk.isGroup() )
            m_cmd = new CreateCommand("dummy", m_from, bk.fullText());
        else
            if ( bk.isSeparator() )
                m_cmd = new CreateCommand("dummy", m_from );
            else
                m_cmd = new CreateCommand("dummy", m_from, bk.fullText(), bk.url());

    m_cmd->unexecute();
}

void DeleteCommand::unexecute()
{
    m_cmd->execute();
}

void EditCommand::execute()
{
    KBookmark bk = KBookmarkManager::self()->findByAddress( m_address );
    ASSERT( !bk.isNull() );
    m_reverseEditions.clear();
    QValueList<Edition>::Iterator it = m_editions.begin();
    for ( ; it != m_editions.end() ; ++it )
    {
        // backup current value
        m_reverseEditions.append( Edition((*it).attr, bk.internalElement().attribute((*it).attr)) );
        // set new value
        bk.internalElement().setAttribute( (*it).attr, (*it).value );
    }
}

void EditCommand::unexecute()
{
    // Let's not duplicate code.
    EditCommand cmd( "dummy", m_address, m_reverseEditions );
    cmd.execute();
    // Get the editions back from it, in case they changed (hmm, shouldn't happen)
    m_editions = cmd.m_reverseEditions;
}

void RenameCommand::execute()
{
    KBookmark bk = KBookmarkManager::self()->findByAddress( m_address );
    ASSERT( !bk.isNull() );

    QDomText domtext;
    if (bk.isGroup())
        domtext = bk.internalElement().elementsByTagName("TEXT").item(0).firstChild().toText();
    else
        domtext = bk.internalElement().firstChild().toText();

    m_oldText = domtext.data();
    domtext.setData( m_newText );
}

void RenameCommand::unexecute()
{
    // Let's not duplicate code.
    RenameCommand cmd( "dummy", m_address, m_oldText );
    cmd.execute();
    // Get the old text back from it, in case they changed (hmm, shouldn't happen)
    m_newText = cmd.m_oldText;
}

void ImportCommand::execute()
{
    // Find or create "Netscape Bookmarks" toplevel item
    // Hmm, let's just create it. The user will clean up if he imports twice.

    KBookmarkGroup netscapeGroup = KBookmarkManager::self()->root().createNewFolder(m_folder);
    netscapeGroup.internalElement().setAttribute("ICON", m_icon);
    m_group = netscapeGroup.address();

    mstack.push( &netscapeGroup );
    KNSBookmarkImporter importer(m_fileName);
    connect( &importer, SIGNAL( newBookmark( const QString &, const QCString & ) ),
             SLOT( newBookmark( const QString &, const QCString & ) ) );
    connect( &importer, SIGNAL( newFolder( const QString & ) ),
             SLOT( newFolder( const QString & ) ) );
    connect( &importer, SIGNAL( newSeparator() ), SLOT( newSeparator() ) );
    connect( &importer, SIGNAL( endMenu() ), SLOT( endMenu() ) );
    importer.parseNSBookmarks();
    // Save memory
    mlist.clear();
    mstack.clear();
}

void ImportCommand::unexecute()
{
    // Just delete the whole imported group
    CreateCommand cmd("dummy", m_group, "dummy");
    cmd.unexecute();
}

void ImportCommand::newBookmark( const QString & text, const QCString & url )
{
    mstack.top()->addBookmark( text, QString::fromUtf8(url) );
}

void ImportCommand::newFolder( const QString & text )
{
    // We use a qvaluelist so that we keep pointers to valid objects in the stack
    mlist.append( mstack.top()->createNewFolder( text ) );
    mstack.push( &(mlist.last()) );
}

void ImportCommand::newSeparator()
{
    mstack.top()->createNewSeparator();
}

void ImportCommand::endMenu()
{
    mstack.pop();
}

#include "commands.moc"
