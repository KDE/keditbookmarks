/* vi: ts=8 sts=4 sw=4
 *
 * $Id$
 *
 * This file is part of the KDE project, module kdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 */

#include <klocale.h>
#include <kmessagebox.h>

#include "passwd.h"
#include "passwddlg.h"


KDEpasswd1Dialog::KDEpasswd1Dialog(QCString user)
    : KPasswordDialog(Password, "")
{
    m_User = user;

    setCaption(i18n("Change Password"));
    setPrompt(i18n("Please enter your current password."));
}


KDEpasswd1Dialog::~KDEpasswd1Dialog()
{
}


bool KDEpasswd1Dialog::checkPassword(const char *password)
{
    PasswdProcess proc(m_User);

    int ret = proc.checkCurrent(password);
    switch (ret)
    {
    case -1:
	KMessageBox::error(this, i18n("Conversation with `passwd' failed."));
	done(Rejected);
	return true;

    case 0:
	return true;

    case PasswdProcess::PasswdNotFound:
	KMessageBox::error(this, i18n("Could not find the program `passwd'."));
	return true;

    case PasswdProcess::PasswordIncorrect:
        KMessageBox::sorry(this, i18n("Incorrect password! Please try again."));
	return false;

    default:
	KMessageBox::error(this, i18n("Internal error: illegal return value "
		"from PasswdProcess::checkCurrent."));
	return true;
    }
}


// static
int KDEpasswd1Dialog::getPassword(QCString &password, QCString user)
{
    KDEpasswd1Dialog *dlg = new KDEpasswd1Dialog(user);
    int res = dlg->exec();
    if (res == Accepted)
	password = dlg->password();
    delete dlg;
    return res;
}



KDEpasswd2Dialog::KDEpasswd2Dialog(const char *oldpass, QCString user)
    : KPasswordDialog(NewPassword, false, 0)
{
    m_Pass = oldpass;
    m_User = user;

    setCaption(i18n("Change Password"));
    setPrompt(i18n("Please enter your new password."));
}


KDEpasswd2Dialog::~KDEpasswd2Dialog()
{
}


bool KDEpasswd2Dialog::checkPassword(const char *password)
{
    PasswdProcess proc(m_User);

    if (strlen(password) > 8)
    {
	switch(KMessageBox::questionYesNoCancel(this,
		i18n("Your password is longer than 8 characters. On some "
			"systems, this can cause problems. You can truncate "
			"the password to 8 characters, or leave it as it is."),
		i18n("Password too long"),
		i18n("Truncate"),
		i18n("Use as is"),
		"truncatePassword"))
	{
	case KMessageBox::Yes :
		const_cast<char *>(password)[8] = '\000';
		break;
	case KMessageBox::No :
		break;
	default : return false;
	}
    }

    int ret = proc.exec(m_Pass, password);
    switch (ret)
    {
    case 0:
	if (!proc.error().isEmpty())
	{
	    // The pw change succeeded but there is a warning.
	    KMessageBox::information(this, proc.error());
	}
	return true;

    case PasswdProcess::PasswordNotGood:
	// The pw change did not succeed. Print the error.
	KMessageBox::sorry(this, proc.error());
	return false;

    default:
	KMessageBox::sorry(this, i18n("Conversation with `passwd' failed."));
	done(Rejected);
	return true;
    }

    return true;
}


#include "passwddlg.moc"
