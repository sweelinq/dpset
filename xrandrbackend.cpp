#include "xrandrbackend.h"

XRandrBackend &XRandrBackend::instance()
{
  static XRandrBackend inst;
  return inst;
}

const QHash<QString, XRandrMonitorInfo> &XRandrBackend::monitors()
{
  if (!m_parsed)
    parseXRandr();
  return m_monitorMap;
}

QStringList XRandrBackend::connectedMonitorNames()
{
  if (!m_parsed)
    parseXRandr();
  QStringList result;
  for (auto it = m_monitorMap.constBegin(); it != m_monitorMap.constEnd(); ++it)
    if (it.value().connected)
      result << it.key();
  return result;
}

QString XRandrBackend::buildScript(const QList<XRandrMonitorConfig> &configs) const
{
  if (configs.isEmpty())
    return QString();

  QString script;
  QStringList standardArguments;
  standardArguments << "xrandr";

  for (const XRandrMonitorConfig &config : configs)
  {
    if (config.screenName.isEmpty())
      continue;

    bool isCustom = true;
    if (m_monitorMap.contains(config.screenName))
    {
      const XRandrMonitorInfo &info = m_monitorMap.value(config.screenName);
      if (info.allResolutions.contains(config.resolution))
        isCustom = false;
    }

    if (isCustom)
    {
      script += QString("# Custom resolution for %1\n").arg(config.screenName);
      script += QString("MODELINE=$(cvt %1 %2 | sed -n '2p' | sed 's/^Modeline //')\n")
                    .arg(config.resolution.width())
                    .arg(config.resolution.height());
      script += "MODE_NAME=$(echo $MODELINE | cut -d ' ' -f 1 | tr -d '\"')\n";
      script += "MODE_PARAMS=$(echo $MODELINE | cut -d ' ' -f 2-)\n";
      script += "if ! xrandr --query | grep -w \"$MODE_NAME\" > /dev/null; then\n";
      script += "    xrandr --newmode $MODE_NAME $MODE_PARAMS\n";
      script += "fi\n";
      script += QString("xrandr --addmode %1 $MODE_NAME\n").arg(config.screenName);
      script += QString("xrandr --output %1 --mode $MODE_NAME --pos %2x%3 --rotate %4")
                    .arg(config.screenName)
                    .arg(config.position.x())
                    .arg(config.position.y())
                    .arg(config.orientation);
      if (config.isPrimary)
        script += " --primary";
      script += "\n\n";
    }
    else
    {
      standardArguments << "--output" << config.screenName
                        << "--mode" << QString("%1x%2")
                                           .arg(config.resolution.width())
                                           .arg(config.resolution.height())
                        << "--pos" << QString("%1x%2")
                                          .arg(config.position.x())
                                          .arg(config.position.y())
                        << "--rotate" << config.orientation;
      if (config.isPrimary)
        standardArguments << "--primary";
    }
  }
  if (standardArguments.size() > 1)
    script += standardArguments.join(" ");
  return script;
}

void XRandrBackend::parseXRandr()
{
  if (m_parsed)
    return;
  m_parsed = true;
  m_monitorMap.clear();
  QProcess proc;
  proc.start("xrandr", {"--query"});
  if (!proc.waitForFinished(10000)) {
    qWarning() << "xrandr query timed out or failed.";
    return;
  }
  QByteArray output = proc.readAllStandardOutput();
  QList<QByteArray> lines = output.split('\n');
  QRegularExpression reMon(
      R"(^(?<name>\S+)\s+(?<status>connected|disconnected)(?:\s+(?<primary>primary))?\s*(?<restLine>.*)$)");
  QRegularExpression reLineConnected(
      R"(.*?(?<width>\d+)x(?<height>\d+)\+(?<x>\d+)\+(?<y>\d+)(?:\s+(?<orient>normal|left|inverted|right))?\s*\((?<dummy>[^)]+)\).*)");
  QRegularExpression reAnyRes(R"(^\s*(?<w>\d+)x(?<h>\d+)\s+\S+\s*(?<flags>.*))");
  QString currentMonitor;
  bool inConnectedSection = false;
  for(const QByteArray &lineBA : lines)
  {
    QString line = QString::fromLocal8Bit(lineBA).trimmed();
    if (line.isEmpty())
      continue;
    QRegularExpressionMatch mm = reMon.match(line);
    if(mm.hasMatch())
    {
      currentMonitor = mm.captured("name");
      QString status = mm.captured("status");
      QString maybePrimary = mm.captured("primary");
      QString restLine = mm.captured("restLine");
      inConnectedSection = (status == "connected");
      XRandrMonitorInfo info;
      info.connected = inConnectedSection;
      info.isPrimary = !maybePrimary.isEmpty();
      if(inConnectedSection)
      {
        QRegularExpressionMatch mm2 = reLineConnected.match(restLine);
        if (mm2.hasMatch()) {
          int w = mm2.captured("width").toInt();
          int h = mm2.captured("height").toInt();
          int px = mm2.captured("x").toInt();
          int py = mm2.captured("y").toInt();
          QString realOrient = mm2.captured("orient");
          if (realOrient.isEmpty())
            realOrient = "normal";
          Orientation orient = stringToOrientation(realOrient);
          if (orient == Orientation::Left || orient == Orientation::Right)
            qSwap(w, h);
          info.position = QPoint(px, py);
          info.currentResolution = QSize(w, h);
          info.orientation = orient;
        }
      }
      m_monitorMap.insert(currentMonitor, info);
      continue;
    }
    if(inConnectedSection && !currentMonitor.isEmpty())
    {
      if (!m_monitorMap.contains(currentMonitor))
        continue;
      XRandrMonitorInfo info = m_monitorMap.value(currentMonitor);
      QRegularExpressionMatch rm = reAnyRes.match(line);
      if(rm.hasMatch())
      {
        int w = rm.captured("w").toInt();
        int h = rm.captured("h").toInt();
        info.allResolutions << QSize(w, h);
        QString flags = rm.captured("flags");
        if (flags.contains('*'))
          info.currentResolution = QSize(w, h);
        m_monitorMap.insert(currentMonitor, info);
      }
    }
  }
}
