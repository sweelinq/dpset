#pragma once

#include <QtCore>
#include <QGraphicsRectItem>
#include "orientation.h"

class MonitorItem : public QObject, public QGraphicsRectItem
{
  Q_OBJECT
public:
  explicit MonitorItem(const QString &screenName);
  QString screenName() const { return m_screenName; }
  QSize currentResolution() const { return m_currentResolution; }
  bool isPrimary() const { return m_isPrimary; }
  Orientation orientation() const { return m_orientation; }
  QString touchDeviceIdPath() const { return m_touchDeviceIdPath; }
  void setTouchDeviceIdPath(const QString &path);
  QString touchDeviceName() const { return m_touchDeviceName; }
  void setTouchDeviceName(const QString &name);
  void setPrimary(bool primary);
  double scaleFactor();
protected:
  QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
  QRectF boundingRect() const override;
  QPainterPath shape() const override;
  void contextMenuEvent(QGraphicsSceneContextMenuEvent *event) override;
private:
  static constexpr double kScaleFactor = 0.1;
  QString m_screenName;
  QSize m_currentResolution;
  QString m_touchDeviceIdPath;
  QString m_touchDeviceName;
  bool m_isPrimary;
  Orientation m_orientation;
  QList<QSize> m_possibleResolutions;
  QGraphicsTextItem *m_nameItem = nullptr;
  QSettings m_settings;

  void setResolution(const QSize &res);
  void setOrientation(Orientation orient);
  void updateRectFromResolutionAndAngle();
  void updateTextAngle();
  double snapX(double proposedX);
  double snapY(double proposedY);
  void trySnapHorizontal(double myEdge, double otherEdge, double myWidth, bool edgeIsRight, double &bestX, double &minDelta);
  void trySnapVertical(double myEdge, double otherEdge, double myHeight, bool edgeIsBottom, double &bestY, double &minDelta);
};
