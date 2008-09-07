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
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

// Own
#include "domainlistview.h"

// Qt
#include <QtGui/QHBoxLayout>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QTreeWidget>

// KDE
#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>

// Local
#include "policies.h"
#include "policydlg.h"

DomainListView::DomainListView(KSharedConfig::Ptr config,const QString &title,
		QWidget *parent) :
	QGroupBox(title, parent), config(config) {
  QHBoxLayout* thisLayout = new QHBoxLayout(this);
  thisLayout->setSpacing(KDialog::spacingHint());
  thisLayout->setMargin(KDialog::marginHint());

  domainSpecificLV = new QTreeWidget(this);
  domainSpecificLV->setRootIsDecorated(false);
  domainSpecificLV->setHeaderLabels(QStringList() << i18n("Host/Domain") << i18n("Policy"));
  domainSpecificLV->setColumnWidth(0, 100);
  connect(domainSpecificLV,SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), SLOT(changePressed()));
  connect(domainSpecificLV, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), SLOT(updateButton()));
  thisLayout->addWidget(domainSpecificLV);

  QVBoxLayout* btnsLayout = new QVBoxLayout;
  thisLayout->addLayout(btnsLayout);
  addDomainPB = new QPushButton(i18n("&New..."), this);
  btnsLayout->addWidget(addDomainPB);
  connect(addDomainPB, SIGNAL(clicked()), SLOT(addPressed()));

  changeDomainPB = new QPushButton( i18n("Chan&ge..."), this);
  btnsLayout->addWidget(changeDomainPB);
  connect(changeDomainPB, SIGNAL(clicked()), this, SLOT(changePressed()));

  deleteDomainPB = new QPushButton(i18n("De&lete"), this);
  btnsLayout->addWidget(deleteDomainPB);
  connect(deleteDomainPB, SIGNAL(clicked()), this, SLOT(deletePressed()));

  importDomainPB = new QPushButton(i18n("&Import..."), this);
  btnsLayout->addWidget(importDomainPB);
  connect(importDomainPB, SIGNAL(clicked()), this, SLOT(importPressed()));
  importDomainPB->setEnabled(false);
  importDomainPB->hide();

  exportDomainPB = new QPushButton(i18n("&Export..."), this);
  btnsLayout->addWidget(exportDomainPB);
  connect(exportDomainPB, SIGNAL(clicked()), this, SLOT(exportPressed()));
  exportDomainPB->setEnabled(false);
  exportDomainPB->hide();

  btnsLayout->addStretch();

  addDomainPB->setWhatsThis( i18n("Click on this button to manually add a host or domain "
                                       "specific policy.") );
  changeDomainPB->setWhatsThis( i18n("Click on this button to change the policy for the "
                                          "host or domain selected in the list box.") );
  deleteDomainPB->setWhatsThis( i18n("Click on this button to delete the policy for the "
                                          "host or domain selected in the list box.") );
  updateButton();
}

DomainListView::~DomainListView() {
  // free all policies
  DomainPolicyMap::Iterator it = domainPolicies.begin();
  for (; it != domainPolicies.end(); ++it) {
    delete it.value();
  }/*next it*/
}

void DomainListView::updateButton()
{
    QTreeWidgetItem *index = domainSpecificLV->currentItem();
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
        QTreeWidgetItem* index = new QTreeWidgetItem( domainSpecificLV, QStringList() << pDlg.domain() <<
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
    QTreeWidgetItem *index = domainSpecificLV->currentItem();
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
    QTreeWidgetItem *index = domainSpecificLV->currentItem();
    if ( index == 0 )
    {
        KMessageBox::information( 0, i18n("You must first select a policy to delete." ) );
        return;
    }

    DomainPolicyMap::Iterator it = domainPolicies.find(index);
    if (it != domainPolicies.end()) {
      delete it.value();
      domainPolicies.erase(it);
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
    domainPolicies.clear();
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
      QTreeWidgetItem *index =
        new QTreeWidgetItem( domainSpecificLV, QStringList() << domain << policy );

      domainPolicies[index] = pol;
    }
}

void DomainListView::save(const QString &group, const QString &domainListKey) {
    QStringList domainList;
    DomainPolicyMap::Iterator it = domainPolicies.begin();
    for (; it != domainPolicies.end(); ++it) {
    	QTreeWidgetItem *current = it.key();
	Policies *pol = it.value();
	pol->save();
	domainList.append(current->text(0));
    }
    config->group(group).writeEntry(domainListKey, domainList);
}

void DomainListView::setupPolicyDlg(PushButton /*trigger*/,
		PolicyDialog &/*pDlg*/,Policies * /*copy*/) {
  // do nothing
}

#include "domainlistview.moc"
