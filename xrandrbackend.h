#pragma once

#include <QtCore>
#include "orientation.h"

struct XRandrMonitorInfo
{
  bool connected = false;
  bool isPrimary = false;
  QPoint position;
  QSize currentResolution;
  Orientation orientation;
  QList<QSize> allResolutions;
};

struct XRandrMonitorConfig
{
  QString screenName;
  QSize resolution;
  QPoint position;
  QString orientation;
  bool isPrimary;
};

class XRandrBackend
{
public:
  static XRandrBackend& instance();

  const QHash<QString, XRandrMonitorInfo>& monitors();
  QStringList connectedMonitorNames();

  QString buildScript(const QList<XRandrMonitorConfig>& configs) const;

private:
  bool m_parsed = false;
  QHash<QString, XRandrMonitorInfo> m_monitorMap;

  void parseXRandr();
};
