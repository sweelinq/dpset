#pragma once
#include <QtCore>
#include "orientation.h"

struct XInputDevice
{
  QString name;
  QString idPath;
};

struct XInputDeviceConfig
{
  QString idPath;
  QString deviceName; // nieuw veld
  QString outputName;
  Orientation orientation;
  QSize totalSize;
  QRect monitorRect;
};

class XInputBackend : public QObject
{
  Q_OBJECT
public:
  static XInputBackend& instance();
  QList<XInputDevice> devices();
  QString buildScript(const QList<XInputDeviceConfig>& configs);
private:
  QList<XInputDevice> m_devices;
  bool m_parsed = false;
  void parseXInput();
  QTransform createScreenTransform(const QSize& totalSize,
                                   const QRect& screenRect,
                                   Orientation orientation);
  QStringList transformToStringList(const QTransform& T);
};
