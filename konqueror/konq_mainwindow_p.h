/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __konq_mainwindow_p_h__
#define __konq_mainwindow_p_h__

class KonqExtendedBookmarkOwner : public KExtendedBookmarkOwner
{
  Q_OBJECT
public:
  KonqExtendedBookmarkOwner(KonqMainWindow *);
  // for KBookmarkOwner
  virtual void openBookmarkURL( const QString & _url );
  virtual QString currentTitle() const;
  virtual QString currentURL() const;
public slots:
  // for KExtendedBookmarkOwner
  void slotFillBookmarksList( KExtendedBookmarkOwner::QStringPairList & list );
private:
  KonqMainWindow *m_pKonqMainWindow;
};

#endif
