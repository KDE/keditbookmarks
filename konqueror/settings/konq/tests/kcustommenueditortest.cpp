#include "kcustommenueditor.h"
#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kcmdlineargs.h>

int main(int argc, char** argv)
{
  KCmdLineArgs::init(argc, argv, "kcustommenueditortest", "kdelibs4", ki18n("kcustommenueditortest"),"0",ki18n("test app"));
  KApplication app;
  app.setQuitOnLastWindowClosed(false);
  KCustomMenuEditor editor(0);
  KConfig *cfg = new KConfig("kdesktop_custom_menu2");
  editor.load(cfg);
  if (editor.exec())
  {
     editor.save(cfg);
     cfg->sync();
  }
  delete cfg;
}

