/*
  Copyright (c) 2002 Leo Savernik <l.savernik@aon.at>
  Derived from jsopts.cpp and javaopts.cpp, code copied from there is
  copyrighted to its respective owners.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#include <qlayout.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>

#include <kconfig.h>
#include <klistview.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "domainlistview.h"
#include "policies.h"
#include "policydlg.h"

DomainListView::DomainListView(KConfig *config,const QString &title,
		QWidget *parent,const char *name) :
	QGroupBox(title, parent, name), config(config) {
  this->setColumnLayout(0, Qt::Vertical);
  this->layout()->setSpacing(0);
  this->layout()->setMargin(0);
  QGridLayout* thisLayout = new QGridLayout(this->layout());
  thisLayout->setAlignment(Qt::AlignTop);
  thisLayout->setSpacing(KDialog::spacingHint());
  thisLayout->setMargin(KDialog::marginHint());

  domainSpecificLV = new KListView(this);
  domainSpecificLV->addColumn(i18n("Host/Domain"));
  domainSpecificLV->addColumn(i18n("Policy"), 100);
  connect(domainSpecificLV,SIGNAL(doubleClicked(QListViewItem *)), SLOT(changePressed()));
  connect(domainSpecificLV,SIGNAL(returnPressed(QListViewItem *)), SLOT(changePressed()));
  connect(domainSpecificLV, SIGNAL( executed( QListViewItem *)), SLOT( updateButton()));
  connect(domainSpecificLV, SIGNAL(selectionChanged()), SLOT(updateButton()));
  thisLayout->addMultiCellWidget(domainSpecificLV, 0, 5, 0, 0);

  addDomainPB = new QPushButton(i18n("&New..."), this);
  thisLayout->addWidget(addDomainPB, 0, 1);
  connect(addDomainPB, SIGNAL(clicked()), SLOT(addPressed()));

  changeDomainPB = new QPushButton( i18n("Chan&ge..."), this);
  thisLayout->addWidget(changeDomainPB, 1, 1);
  connect(changeDomainPB, SIGNAL(clicked()), this, SLOT(changePressed()));

  deleteDomainPB = new QPushButton(i18n("De&lete"), this);
  thisLayout->addWidget(deleteDomainPB, 2, 1);
  connect(deleteDomainPB, SIGNAL(clicked()), this, SLOT(deletePressed()));

  importDomainPB = new QPushButton(i18n("&Import..."), this);
  thisLayout->addWidget(importDomainPB, 3, 1);
  connect(importDomainPB, SIGNAL(clicked()), this, SLOT(importPressed()));
  importDomainPB->setEnabled(false);
  importDomainPB->hide();

  exportDomainPB = new QPushButton(i18n("&Export..."), this);
  thisLayout->addWidget(exportDomainPB, 4, 1);
  connect(exportDomainPB, SIGNAL(clicked()), this, SLOT(exportPressed()));
  exportDomainPB->setEnabled(false);
  exportDomainPB->hide();

  QSpacerItem* spacer = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
  thisLayout->addItem(spacer, 5, 1);

  QWhatsThis::add( addDomainPB, i18n("Click on this button to manually add a host or domain "
                                       "specific policy.") );
  QWhatsThis::add( changeDomainPB, i18n("Click on this button to change the policy for the "
                                          "host or domain selected in the list box.") );
  QWhatsThis::add( deleteDomainPB, i18n("Click on this button to delete the policy for the "
                                          "host or domain selected in the list box.") );
  updateButton();
}

DomainListView::~DomainListView() {
  // free all policies
  DomainPolicyMap::Iterator it = domainPolicies.begin();
  for (; it != domainPolicies.end(); ++it) {
    delete it.data();
  }/*next it*/
}

void DomainListView::updateButton()
{
    QListViewItem *index = domainSpecificLV->currentItem();
    bool enable = ( index != 0 );
    changeDomainPB->setEnabled( enable );
    deleteDomainPB->setEnabled( enable );

}

void DomainListView::addPressed()
{
//    JavaPolicies pol_copy(m_pConfig,m_groupname,false);
    Policies *pol = createPolicies();
    pol->defaults();
    PolicyDialog pDlg(pol, this);
    setupPolicyDlg(AddButton,pDlg,pol);
    if( pDlg.exec() ) {
        QListViewItem* index = new QListViewItem( domainSpecificLV, pDlg.domain(),
                                                  pDlg.featureEnabledPolicyText() );
	pol->setDomain(pDlg.domain());
        domainPolicies.insert(index, pol);
        domainSpecificLV->setCurrentItem( index );
        emit changed(true);
    } else {
        delete pol;
    }
    updateButton();
}

void DomainListView::changePressed()
{
    QListViewItem *index = domainSpecificLV->currentItem();
    if ( index == 0 )
    {
        KMessageBox::information( 0, i18n("You must first select a policy to be changed." ) );
        return;
    }

    Policies *pol = domainPolicies[index];
    // This must be copied because the policy dialog is allowed to change
    // the data even if the changes are rejected in the end.
    Policies *pol_copy = copyPolicies(pol);

    PolicyDialog pDlg( pol_copy, this );
    pDlg.setDisableEdit( true, index->text(0) );
    setupPolicyDlg(ChangeButton,pDlg,pol_copy);
    if( pDlg.exec() )
    {
        pol_copy->setDomain(pDlg.domain());
        domainPolicies[index] = pol_copy;
	pol_copy = pol;
        index->setText(0, pDlg.domain() );
        index->setText(1, pDlg.featureEnabledPolicyText());
        emit changed(true);
    }
    delete pol_copy;
}

void DomainListView::deletePressed()
{
    QListViewItem *index = domainSpecificLV->currentItem();
    if ( index == 0 )
    {
        KMessageBox::information( 0, i18n("You must first select a policy to delete." ) );
        return;
    }

    DomainPolicyMap::Iterator it = domainPolicies.find(index);
    if (it != domainPolicies.end()) {
      delete it.data();
      domainPolicies.remove(it);
      delete index;
      emit changed(true);
    }
    updateButton();
}

void DomainListView::importPressed()
{
  // PENDING(kalle) Implement this.
}

void DomainListView::exportPressed()
{
  // PENDING(kalle) Implement this.
}

void DomainListView::initialize(const QStringList &domainList)
{
    domainSpecificLV->clear();
//    JavaPolicies pol(m_pConfig,m_groupname,false);
    for (QStringList::ConstIterator it = domainList.begin();
         it != domainList.end(); ++it) {
      QString domain = *it;
      Policies *pol = createPolicies();
      pol->setDomain(domain);
      pol->load();

      QString policy;
      if (pol->isFeatureEnabledPolicyInherited())
        policy = i18n("Use Global");
      else if (pol->isFeatureEnabled())
        policy = i18n("Accept");
      else
        policy = i18n("Reject");
      QListViewItem *index =
        new QListViewItem( domainSpecificLV, domain, policy );

      domainPolicies[index] = pol;
    }
}

void DomainListView::save(const QString &group, const QString &domainListKey) {
    QStringList domainList;
    DomainPolicyMap::Iterator it = domainPolicies.begin();
    for (; it != domainPolicies.end(); ++it) {
    	QListViewItem *current = it.key();
	Policies *pol = it.data();
	pol->save();
	domainList.append(current->text(0));
    }
    config->setGroup(group);
    config->writeEntry(domainListKey, domainList);
}

void DomainListView::setupPolicyDlg(PushButton /*trigger*/,
		PolicyDialog &/*pDlg*/,Policies */*copy*/) {
  // do nothing
}

#include "domainlistview.moc"
