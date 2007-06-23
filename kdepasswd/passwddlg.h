/* vi: ts=8 sts=4 sw=4
 *
 * $Id$
 *
 * This file is part of the KDE project, module kdesu.
 * Copyright (C) 2000 Geert Jansen <jansen@kde.org>
 */

#ifndef __PasswdDlg_h_Incluced__
#define __PasswdDlg_h_Incluced__

#include <kpassworddialog.h>
#include <knewpassworddialog.h>
#include <QtCore/QByteRef>

class KDEpasswd1Dialog
    : public KPasswordDialog
{
    Q_OBJECT

public:
    KDEpasswd1Dialog();
    ~KDEpasswd1Dialog();

    static int getPassword(QByteArray &password);

    void accept();
};


class KDEpasswd2Dialog
    : public KNewPasswordDialog
{
    Q_OBJECT

public:
    KDEpasswd2Dialog(const char *oldpass, const QByteArray &user);
    ~KDEpasswd2Dialog();

    void accept();


protected:
    bool checkPassword(const char *password);

private:
    const char *m_Pass;
    QByteArray m_User;
};



#endif // __PasswdDlg_h_Incluced__
