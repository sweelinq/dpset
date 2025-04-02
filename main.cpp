#include <QtCore>
#include <QtWidgets>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  app.setWindowIcon(QIcon(":/assets/app_icon.svg"));

  const QString localeName = QLocale::system().name();
  const QString shortLang = localeName.section('_', 0, 0).toLower();
  const QString qmFilePath = ":/i18n/dpset_" + shortLang + ".qm";

  QTranslator translator;
  if(translator.load(qmFilePath))
  {
    app.installTranslator(&translator);
  }

  MainWindow w;
  w.show();

  return app.exec();
}
