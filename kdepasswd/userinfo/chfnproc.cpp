/***************************************************************************
 *   Copyright 2003 Braden MacDonald <bradenm_k@shaw.ca>                   *
 *   Copyright 2003 Ravikiran Rajagopal <ravi@ee.eng.ohio-state.edu>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License (version 2) as   *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/

/**
 * @file Change a user's 'finger' information, specifically their full name.
 * derived from kdepasswd.
 */

#include <unistd.h>
#include <qcstring.h>

#include <kdesu/process.h>
#include "chfnproc.h"


int ChfnProcess::exec(const char *pass, const char *name)
{
  // Try to set the default locale to make the parsing of the output
  // of `chfn' easier.
  putenv((char*)"LANG=C");

  QCStringList args;
      args += "-f";
      args += name;
  int ret = PtyProcess::exec("chfn", args);
  if (ret < 0)
      return ChfnNotFound;

  ret = ConverseChfn(pass);

  waitForChild();
  return ret;
}


/*
 * The actual work.
 * Return values: -1 = unknown error, 0 = ok, >0 = error code.
 */
int ChfnProcess::ConverseChfn(const char *pass)
{
  int status=-1;

  QCString line;
  while(1)
  {
    line = readLine();

    if ( line.isEmpty() )
      continue;// discard line

    if ( line.contains( "Password: " )/*isPrompt( line, "password" )*/ )
    {
      WaitSlave();
      write(m_Fd, pass, strlen(pass));
      write(m_Fd, "\n", 1);
    }
    else if ( line.contains( "Changing finger info" ) )
    {
      // do nothing
    }
    else if ( line.contains( "information changed" ) )
    {
      status=0;
      break;
    }
    else if ( line.contains( "Password error" ) )
    {
      status=PasswordError;
      break;
    }
    else
    {
      status=MiscError;
      m_Error=line;
      break;
    }
  }
  return status;
}
