#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
  QApplication app(argc, argv);
  app.setWindowIcon(QIcon(":/assets/app_icon.svg"));

  MainWindow w;
  w.setWindowTitle("dpset");
  w.show();

  return app.exec();
}
