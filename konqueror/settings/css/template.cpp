
// Own
#include "template.h"

// Qt
#include <QtCore/QFile>
#include <QtCore/QTextStream>

bool CSSTemplate::expand(const QString &destname, const QMap<QString,QString> &dict)
{
  QFile inf(_filename);
  if (!inf.open(QIODevice::ReadOnly))
    return false;
  QTextStream is(&inf);
  
  QFile outf(destname);
  if (!outf.open(QIODevice::WriteOnly))
    return false;
  QTextStream os(&outf);

  QString line;
  while (!is.atEnd())
    {
      line = is.readLine();

      int start = line.indexOf('$');
      if (start >= 0)
	{
	  int end = line.indexOf('$', start+1);
	  if (end >= 0)
            {
	      QString expr = line.mid(start+1, end-start-1);
	      QString res = dict[expr];

	      line.replace(start, end-start+1, res);
	    }
	}
      os << line << endl;
    }  

  inf.close();
  outf.close();

  return true;
}
