#include "mainwindow.h"
#include <QtCore>
#include <QtWidgets>
#include "monitoritem.h"
#include "xinputbackend.h"
#include "xrandrbackend.h"
#include "version.h"

MainWindow::MainWindow(QWidget *parent)
:QMainWindow(parent)
{
  setWindowIcon(QIcon(":/assets/app_icon.svg"));
  bool adjustViewSize = true;
  const QStringList monitors = XRandrBackend::instance().connectedMonitorNames();

  if(monitors.isEmpty())
  {
    adjustViewSize = false;
    this->resize(640, 480);
    QMessageBox::critical(this, tr("Error"),
                          tr("No monitor information detected via xrandr.\nPlease ensure that xrandr is correctly installed and available in your PATH."));
  }

  m_scene = new QGraphicsScene(this);
  m_view = new QGraphicsView(m_scene, this);
  m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
  m_view->setFrameStyle(QFrame::NoFrame);
  setCentralWidget(m_view);
  createToolbar();

  for(const QString &monName : monitors)
  {
    MonitorItem *itm = new MonitorItem(monName);
    m_scene->addItem(itm);
  }

  if(adjustViewSize)
  {
    QTimer::singleShot(0, this, [this]()
    {
      this->adjustSize();
      QRectF bounding = m_scene->itemsBoundingRect();
      QSize desiredViewSize(int(bounding.width() * 2), int(bounding.height() * 2));
      QSize extra = size() - m_view->size();
      QSize newSize = desiredViewSize + extra;
      resize(newSize);

      QScreen *primaryScreen = QGuiApplication::primaryScreen();
      if (primaryScreen)
      {
        QRect primaryGeom = primaryScreen->availableGeometry();
        int x = primaryGeom.x() + (primaryGeom.width() - width()) / 2;
        int y = primaryGeom.y() + (primaryGeom.height() - height()) / 2;
        move(x, y);
      }
    });
  }
}

void MainWindow::createToolbar()
{
  QToolBar *toolbar = addToolBar("Main Toolbar");
  toolbar->setMovable(false);
  toolbar->setFloatable(false);
  toolbar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

  QAction *applyAction = new QAction(QIcon(":/assets/data_check.svg"), tr("Apply"), this);
  QAction *scriptAction = new QAction(QIcon(":/assets/data_object.svg"), tr("Script"), this);
  QAction *infoAction = new QAction(QIcon(":/assets/info.svg"), tr("Info"), this);

  connect(applyAction, &QAction::triggered, this, &MainWindow::applyConfig);
  connect(scriptAction, &QAction::triggered, this, &MainWindow::saveScript);
  connect(infoAction, &QAction::triggered, this, &MainWindow::showInfo);

  toolbar->addAction(applyAction);
  toolbar->addAction(scriptAction);
  toolbar->addAction(infoAction);
}

#include "version.h"  // Zorg dat je dit include, zodat de defines beschikbaar zijn

void MainWindow::showInfo()
{
  QString infoText = tr(
                         "This application allows you to arrange your monitor layout visually.\n\n"
                         "Right-click on a monitor to access its context menu. From there, you can:\n"
                         "  - Use 'Identify' to display the physical monitor name on the corresponding screen.\n"
                         "  - Mark a monitor as primary.\n"
                         "  - Change the resolution and orientation.\n\n"
                         "You can drag and drop the monitor items to reposition them.\n\n"
                         "Note that the xrandr configuration applied is not persistent – it will be lost after a reboot.\n\n"
                         "Use the Apply button to immediately apply the current configuration.\n"
                         "Use the Script button to create a startup script that you must run after each boot."
      );

  infoText += tr("\n\nApplication version: %1\nCommit: %2")
                  .arg(APP_VERSION)
                  .arg(GIT_COMMIT_SHORT);

  QMessageBox::information(this, tr("Application Info"), infoText);
}

QMenu *MainWindow::createPopupMenu()
{
  return nullptr;
}

QString MainWindow::buildScript()
{
  QList<XRandrMonitorConfig> xrandrConfigs;
  QList<XInputDeviceConfig> xinputConfigs;

  for(QGraphicsItem *item : m_scene->items())
  {
    MonitorItem *monItem = dynamic_cast<MonitorItem *>(item);
    if (!monItem)
      continue;

    XRandrMonitorConfig xrandrCfg;
    xrandrCfg.screenName = monItem->screenName();
    xrandrCfg.resolution = monItem->currentResolution();
    QPointF pos = monItem->pos();
    xrandrCfg.position = QPoint(qRound(pos.x() / monItem->scaleFactor()),
                                qRound(pos.y() / monItem->scaleFactor()));
    xrandrCfg.orientation = orientationToString(monItem->orientation());
    xrandrCfg.isPrimary = monItem->isPrimary();
    xrandrConfigs.append(xrandrCfg);

    XInputDeviceConfig xinputCfg;
    QRect monitorRect = monItem->sceneBoundingRect().toRect();
    if (monItem->orientation() == Orientation::Left
    ||  monItem->orientation() == Orientation::Right)
    {
      monitorRect.setSize(monitorRect.size().transposed());
    }
    QSize totalSize(m_scene->itemsBoundingRect().width(), m_scene->itemsBoundingRect().height());
    xinputCfg.idPath = monItem->touchDeviceIdPath();
    xinputCfg.deviceName = monItem->touchDeviceName();
    xinputCfg.outputName = monItem->screenName();
    xinputCfg.orientation = monItem->orientation();
    xinputCfg.totalSize = totalSize;
    xinputCfg.monitorRect = monitorRect;
    xinputConfigs.append(xinputCfg);
  }

  QString script = "#!/bin/bash\n\n";
  script += XRandrBackend::instance().buildScript(xrandrConfigs);
  script += "\n\n";
  script += XInputBackend::instance().buildScript(xinputConfigs);
  return script;
}

void MainWindow::applyConfig()
{
  QString script = buildScript();
  if(script.isEmpty())
  {
    qWarning() << "No valid config found.";
  }
  else
  {
    QProcess::startDetached("/bin/sh", QStringList() << "-c" << script);
  }
}

void MainWindow::saveScript()
{
  QString script = buildScript();
  QString defaultFileName = QDir::homePath() + "/monitor_setup.sh";
  QString filename = QFileDialog::getSaveFileName(this,
                                                  tr("Save Script"),
                                                  defaultFileName,
                                                  tr("Shell Script (*.sh);;All Files (*)"));
  if (filename.isEmpty())
    return;

  QFile file(filename);
  if(!file.open(QIODevice::WriteOnly | QIODevice::Text))
  {
    qWarning() << "Cannot open file for writing:" << filename;
    return;
  }
  QTextStream ts(&file);
  ts << script;
  file.close();

  QFile::Permissions perms = file.permissions();
  perms |= QFile::ExeOwner;
  file.setPermissions(perms);

  qDebug() << "Script saved at" << filename << "with +x";
}
