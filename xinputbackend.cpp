#include "xinputbackend.h"
#include <QtCore>
#include <QtGui>

XInputBackend &XInputBackend::instance()
{
  static XInputBackend inst;
  return inst;
}

QList<XInputDevice> XInputBackend::devices()
{
  if (!m_parsed)
    parseXInput();

  return m_devices;
}

QString XInputBackend::buildScript(const QList<XInputDeviceConfig> &configs)
{
  bool touchDeviceFound = false;
  for(const XInputDeviceConfig &config : configs)
  {
    if(!config.idPath.isEmpty() && !config.deviceName.isEmpty())
    {
      touchDeviceFound = true;
    }
  }
  if (!touchDeviceFound)
    return QString();

  QString script;
  script += "find_xinput_device() {\n";
  script += "    local target_id_path=\"$(echo \"$1\" | xargs)\"\n";
  script += "    local target_name=\"$(echo \"$2\" | xargs)\"\n";
  script += "    for id in $(xinput list --id-only); do\n";
  script += "        local node_line\n";
  script += "        node_line=$(xinput list-props \"$id\" 2>/dev/null | grep \"Device Node\")\n";
  script += "        if [ -z \"$node_line\" ]; then\n";
  script += "            continue\n";
  script += "        fi\n";
  script += "        local device_node\n";
  script += "        device_node=$(echo \"$node_line\" | sed -n 's/.*\"\\(\\/dev\\/input\\/[^\\\"]*\\)\".*/\\1/p')\n";
  script += "        if [ -z \"$device_node\" ]; then\n";
  script += "            continue\n";
  script += "        fi\n";
  script += "        local udev_line\n";
  script += "        udev_line=$(udevadm info \"$device_node\" 2>/dev/null | grep \"E: ID_PATH=\")\n";
  script += "        if [ -z \"$udev_line\" ]; then\n";
  script += "            continue\n";
  script += "        fi\n";
  script += "        local id_path_value\n";
  script += "        id_path_value=$(echo \"$udev_line\" | cut -d'=' -f2 | xargs)\n";
  script += "        if [ \"$id_path_value\" = \"$target_id_path\" ]; then\n";
  script += "            local dev_name\n";
  script += "            dev_name=$(xinput list --name-only \"$id\" | xargs)\n";
  script += "            if [ \"$dev_name\" = \"$target_name\" ]; then\n";
  script += "                echo \"$id\"\n";
  script += "                return 0\n";
  script += "            fi\n";
  script += "        fi\n";
  script += "    done\n";
  script += "    echo \"-1\"\n";
  script += "    return 1\n";
  script += "}\n\n";

  for (const XInputDeviceConfig &config : configs)
  {
    if (!config.idPath.isEmpty() && !config.deviceName.isEmpty())
    {
      const QStringList transform = transformToStringList(
          createScreenTransform(config.totalSize, config.monitorRect, config.orientation)
          );
      QString command = "#Touch mapping for " + config.outputName + "\n";
      command += "DEVICE_ID=$(find_xinput_device \"" + config.idPath + "\" \"" + config.deviceName + "\")\n";
      command += "if [ \"$DEVICE_ID\" -eq \"-1\" ]; then\n";
      command += "    echo \"DEBUG: Could not find touch device for " + config.outputName +
                 " with id_path " + config.idPath + " and name " + config.deviceName + "\" >&2\n";
      command += "else\n";
      command += "    xinput set-prop $DEVICE_ID 'Coordinate Transformation Matrix' " + transform.join(' ') + "\n";
      command += "fi\n\n";
      script += command;
    }
  }
  return script;
}

void XInputBackend::parseXInput()
{
  if (m_parsed)
    return;
  m_parsed = true;
  m_devices.clear();

  QProcess proc;
  proc.start("xinput", QStringList() << "list" << "--short");
  if (!proc.waitForFinished(10000)) {
    qWarning() << "xinput timed out or failed.";
    return;
  }
  QByteArray output = proc.readAllStandardOutput();
  QList<QByteArray> lines = output.split('\n');
  QRegularExpression rxId(R"(id=(\d+))");

  for (const QByteArray &lineBA : lines)
  {
    QString line = QString::fromLocal8Bit(lineBA).trimmed();
    if (line.isEmpty())
      continue;
    if (!line.contains(QRegularExpression("slave\\s+pointer")))
      continue;
    line.remove(QRegularExpression("[⎡⎜⎣]"));
    line.remove("↳");
    QRegularExpressionMatch match = rxId.match(line);
    if (!match.hasMatch())
      continue;
    int devId = match.captured(1).toInt();
    QStringList parts = line.split("id=");
    if (parts.size() < 2)
      continue;
    QString devName = parts.at(0).trimmed();

    XInputDevice dev;
    dev.name = devName;

    //get device id_path
    QProcess propProc;
    propProc.start("xinput", QStringList() << "--list-props" << QString::number(devId));
    if (propProc.waitForFinished(10000))
    {
      QString propOutput = QString::fromLocal8Bit(propProc.readAllStandardOutput());
      QRegularExpression rxNode(R"(Device Node\s*\(.*\):\s*\"(.*)\"\s*)");
      QRegularExpressionMatch matchNode = rxNode.match(propOutput);
      if(matchNode.hasMatch())
      {
        QString devNode = matchNode.captured(1).trimmed();
        QProcess udevProc;
        udevProc.start("udevadm", QStringList() << "info" << devNode);
        if(udevProc.waitForFinished(10000))
        {
          QString udevOutput = QString::fromLocal8Bit(udevProc.readAllStandardOutput());
          QRegularExpression rxPath(R"(ID_PATH=(.*))");
          QRegularExpressionMatch matchPath = rxPath.match(udevOutput);
          if (matchPath.hasMatch())
          {
            dev.idPath = matchPath.captured(1).trimmed();
          }
        }
      }
    }

    if(dev.idPath.isEmpty())
      continue;

    m_devices.append(dev);
  }
}

QTransform XInputBackend::createScreenTransform(const QSize &totalSize, const QRect &screenRect, Orientation orientation)
{
  int boundingW = screenRect.width();
  int boundingH = screenRect.height();
  if (orientation == Orientation::Right || orientation == Orientation::Left)
  {
    boundingW = screenRect.height();
    boundingH = screenRect.width();
  }

  double scaleX = double(boundingW) / double(totalSize.width());
  double scaleY = double(boundingH) / double(totalSize.height());
  double transX = double(screenRect.x()) / double(totalSize.width());
  double transY = double(screenRect.y()) / double(totalSize.height());

  QTransform S;
  S.setMatrix(scaleX, 0, transX, 0, scaleY, transY, 0, 0, 1);

  QTransform R;
  switch (orientation)
  {
    case Orientation::Normal:
      R.setMatrix(1, 0, 0, 0, 1, 0, 0, 0, 1);
      break;
    case Orientation::Right:
      R.setMatrix(0, 1, 0, -1, 0, 1, 0, 0, 1);
      break;
    case Orientation::Inverted:
      R.setMatrix(-1, 0, 1, 0, -1, 1, 0, 0, 1);
      break;
    case Orientation::Left:
      R.setMatrix(0, -1, 1, 1, 0, 0, 0, 0, 1);
      break;
  }

  QTransform M = S;
  M *= R;

  return M;
}

QStringList XInputBackend::transformToStringList(const QTransform &T)
{
  QStringList list;
  list << QString("%1 %2 %3").arg(T.m11(), 0, 'g', 8).arg(T.m12(), 0, 'g', 8).arg(T.m13(), 0, 'g', 8);
  list << QString("%1 %2 %3").arg(T.m21(), 0, 'g', 8).arg(T.m22(), 0, 'g', 8).arg(T.m23(), 0, 'g', 8);
  list << QString("%1 %2 %3").arg(T.m31(), 0, 'g', 8).arg(T.m32(), 0, 'g', 8).arg(T.m33(), 0, 'g', 8);
  return list;
}
