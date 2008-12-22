/*  This file is part of the KDE project

    Copyright (C) 2000 Alexander Neundorf <neundorf@kde.org>

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

// Own
#include "smbrodlg.h"

// Qt
#include <QtCore/QTextCodec>
#include <QtGui/QLabel>
#include <QtGui/QLayout>

// KDE
#include <kcharsets.h>
#include <kcombobox.h>
#include <kconfig.h>
#include <kdialog.h>
#include <kgenericfactory.h>
#include <kglobal.h>
#include <klocale.h>

#include <config-apps.h>

K_PLUGIN_FACTORY_DECLARATION(KioConfigFactory)

SMBRoOptions::SMBRoOptions(QWidget *parent, const QVariantList &, const KComponentData &componentData)
  : KCModule(componentData.isValid() ? componentData : KioConfigFactory::componentData(), parent)
{
   QGridLayout *layout = new QGridLayout(this );
   layout->setMargin( KDialog::marginHint() );
   layout->setSpacing( KDialog::spacingHint() );
   QLabel *label=new QLabel(i18n("These settings apply to network browsing only."),this);
   layout->addWidget(label,0,0, 1, 2 );

   m_userLe=new QLineEdit(this);
   label=new QLabel(i18n("Default user name:"),this);
   label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
   label->setBuddy( m_userLe );
   layout->addWidget(label,1,0);
   layout->addWidget(m_userLe,1,1);

   m_passwordLe=new QLineEdit(this);
   m_passwordLe->setEchoMode(QLineEdit::Password);
   label=new QLabel(i18n("Default password:"),this);
   label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
   label->setBuddy( m_passwordLe );
   layout->addWidget(label,2,0);
   layout->addWidget(m_passwordLe,2,1);

/*   m_workgroupLe=new QLineEdit(this);
   label=new QLabel(m_workgroupLe,i18n("Workgroup:"),this);
   layout->addWidget(label,3,0);
   layout->addWidget(m_workgroupLe,3,1);

   m_showHiddenShares=new QCheckBox(i18n("Show hidden shares"),this);
   layout->addWidget(m_showHiddenShares,4,0, 1, 2 );

   m_encodingList = new KComboBox( false, this );
   QStringList _strList = KGlobal::charsets()->availableEncodingNames();
   m_encodingList->addItems( _strList );

   label = new QLabel( i18n( "MS Windows encoding:" ), this );
   label->setBuddy( m_encodingList );
   layout->addWidget( label, 3, 0 );
   layout->addWidget( m_encodingList, 3, 1 );
   */

   layout->addWidget(new QWidget(this),4,0);

//   connect(m_showHiddenShares, SIGNAL(toggled(bool)), this, SLOT(changed()));
   connect(m_userLe, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
   connect(m_passwordLe, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
//   connect(m_workgroupLe, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
//   connect( m_encodingList, SIGNAL( activated( const QString & ) ), this , SLOT( changed() ) );

   layout->setRowStretch(4, 1);

}

SMBRoOptions::~SMBRoOptions()
{
}

void SMBRoOptions::load()
{
   KConfig *cfg = new KConfig("kioslaverc");

   QString tmp;
   KConfigGroup group = cfg->group("Browser Settings/SMBro" );
   m_userLe->setText(group.readEntry("User"));
//   m_workgroupLe->setText(group.readEntry("Workgroup"));
//   m_showHiddenShares->setChecked(group.readEntry("ShowHiddenShares", QVariant(false)).toBool());

   //QStringList _strList = KGlobal::charsets()->availableEncodingNames();
   //QString m_encoding = QTextCodec::codecForLocale()->name();
   //m_encodingList->setCurrentIndex( _strList.indexOf( group.readEntry( "Encoding", m_encoding.toLower() ) ) );

   // unscramble
   QString scrambled = group.readEntry( "Password" );
   QString password = "";
   for (int i=0; i<scrambled.length()/3; i++)
   {
      QChar qc1 = scrambled[i*3];
      QChar qc2 = scrambled[i*3+1];
      QChar qc3 = scrambled[i*3+2];
      unsigned int a1 = qc1.toLatin1() - '0';
      unsigned int a2 = qc2.toLatin1() - 'A';
      unsigned int a3 = qc3.toLatin1() - '0';
      unsigned int num = ((a1 & 0x3F) << 10) | ((a2& 0x1F) << 5) | (a3 & 0x1F);
      password[i] = QChar((uchar)((num - 17) ^ 173)); // restore
   }
   m_passwordLe->setText(password);

   delete cfg;
}

void SMBRoOptions::save()
{
   KConfig *cfg = new KConfig("kioslaverc");

   KConfigGroup group = cfg->group("Browser Settings/SMBro" );
   group.writeEntry( "User", m_userLe->text());
//   group.writeEntry( "Workgroup", m_workgroupLe->text());
//   group.writeEntry( "ShowHiddenShares", m_showHiddenShares->isChecked());
//   group.writeEntry( "Encoding", m_encodingList->currentText() );

   //taken from Nicola Brodu's smb ioslave
   //it's not really secure, but at
   //least better than storing the plain password
   QString password(m_passwordLe->text());
   QString scrambled;
   for (int i=0; i<password.length(); i++)
   {
      QChar c = password[i];
      unsigned int num = (c.unicode() ^ 173) + 17;
      unsigned int a1 = (num & 0xFC00) >> 10;
      unsigned int a2 = (num & 0x3E0) >> 5;
      unsigned int a3 = (num & 0x1F);
      scrambled += (char)(a1+'0');
      scrambled += (char)(a2+'A');
      scrambled += (char)(a3+'0');
   }
   group.writeEntry( "Password", scrambled);

   delete cfg;
}

void SMBRoOptions::defaults()
{
   m_userLe->setText("");
   m_passwordLe->setText("");
//   m_workgroupLe->setText("");
//   m_showHiddenShares->setChecked(false);
}

void SMBRoOptions::changed()
{
   emit KCModule::changed(true);
}

QString SMBRoOptions::quickHelp() const
{
   return i18n("<p><h1>Windows Shares</h1>Konqueror is able to access shared "
        "Microsoft Windows file systems, if properly configured. If there is a "
        "specific computer from which you want to browse, fill in "
        "the <em>Browse server</em> field. This is mandatory if you "
        "are not running Samba locally. The <em>Broadcast address</em> "
        "and <em>WINS address</em> fields will also be available, if you "
        "use the native code, or the location of the 'smb.conf' file "
        "from which the options are read, when using Samba. In any case, the "
        "broadcast address (interfaces in smb.conf) must be set up if it "
        "is guessed incorrectly or you have multiple cards. A WINS server "
        "usually improves performance, and reduces the network load a lot.</p><p>"
        "The bindings are used to assign a default user for a given server, "
        "possibly with the corresponding password, or for accessing specific "
        "shares. If you choose to, new bindings will be created for logins and "
        "shares accessed during browsing. You can edit all of them from here. "
        "Passwords will be stored locally, and scrambled so as to render them "
        "unreadable to the human eye. For security reasons, you may not want to "
        "do that, as entries with passwords are clearly indicated as such.</p>");
}

#include "smbrodlg.moc"
