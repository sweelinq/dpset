#include "monitoritem.h"
#include "xinputbackend.h"
#include "xrandrbackend.h"
#include <QtWidgets>
#include <QRegularExpression>

namespace
{
  constexpr double SNAP_DISTANCE = 15.0;

  bool touchDeviceExists(const QString &mapping)
  {
    QStringList parts = mapping.split("||");
    if (parts.size() < 2)
      return false;
    QString idPath = parts.at(0).trimmed();
    QString name = parts.at(1).trimmed();
    const QList<XInputDevice> devices = XInputBackend::instance().devices();
    for (const XInputDevice &dev : devices) {
      if (dev.idPath == idPath && dev.name == name)
        return true;
    }
    return false;
  }

  class ClickableOverlay : public QWidget
  {
  public:
    explicit ClickableOverlay(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags())
        : QWidget(parent, flags)
    {
      setAttribute(Qt::WA_DeleteOnClose, true);
    }

  protected:
    void mousePressEvent(QMouseEvent *event) override
    {
      Q_UNUSED(event);
      close();
    }
  };
}

MonitorItem::MonitorItem(const QString &screenName)
:m_screenName(screenName),
m_orientation(Orientation::Normal),
m_isPrimary(false),
m_settings("Orgelmakerij Noorlander B.V.", "dpset")
{
  const auto &map = XRandrBackend::instance().monitors();
  if (map.contains(m_screenName))
  {
    const XRandrMonitorInfo &info = map.value(m_screenName);
    m_currentResolution = info.currentResolution;
    m_isPrimary = info.isPrimary;
    m_orientation = info.orientation;
    m_possibleResolutions = info.allResolutions;
  }
  else
  {
    m_currentResolution = QSize(1024, 768);
    m_isPrimary = false;
    m_orientation = Orientation::Normal;
  }
  updateRectFromResolutionAndAngle();
  if(map.contains(m_screenName))
  {
    auto info = map.value(m_screenName);
    setPos(info.position.x() * kScaleFactor, info.position.y() * kScaleFactor);
  }
  setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable
           | QGraphicsItem::ItemSendsGeometryChanges);
  setOpacity(0.5);
  m_nameItem = new QGraphicsTextItem(m_screenName, this);
  updateTextAngle();

  m_settings.beginGroup("TouchDeviceMappings");
  QString savedMapping = m_settings.value(m_screenName).toString();
  if (!savedMapping.isEmpty() && touchDeviceExists(savedMapping))
  {
    QStringList parts = savedMapping.split("||");
    if (parts.size() >= 2)
    {
      m_touchDeviceIdPath = parts.at(0).trimmed();
      m_touchDeviceName = parts.at(1).trimmed();
    }
  }
  m_settings.endGroup();
}

void MonitorItem::setTouchDeviceIdPath(const QString &path)
{
  m_touchDeviceIdPath = path;
  m_settings.beginGroup("TouchDeviceMappings");
  m_settings.setValue(m_screenName, m_touchDeviceIdPath + "||" + m_touchDeviceName);
  m_settings.endGroup();
}

void MonitorItem::setTouchDeviceName(const QString &name)
{
  m_touchDeviceName = name;
  m_settings.beginGroup("TouchDeviceMappings");
  m_settings.setValue(m_screenName, m_touchDeviceIdPath + "||" + m_touchDeviceName);
  m_settings.endGroup();
}

void MonitorItem::setPrimary(bool primary)
{
  if (!scene())
    return;
  if (primary)
  {
    for (QGraphicsItem *item : scene()->items())
    {
      if (MonitorItem *otherMon = dynamic_cast<MonitorItem *>(item))
        otherMon->m_isPrimary = false;
    }
    m_isPrimary = true;
  }
  else
  {
    m_isPrimary = false;
  }
}

double MonitorItem::scaleFactor()
{
  return kScaleFactor;
}

QVariant MonitorItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
  if (change == ItemPositionChange && scene())
  {
    QPointF proposedPos = value.toPointF();
    double snappedX = snapX(proposedPos.x());
    double snappedY = snapY(proposedPos.y());
    return QPointF(snappedX, snappedY);
  }
  return QGraphicsRectItem::itemChange(change, value);
}

void MonitorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  Q_UNUSED(widget);
  painter->setBrush(Qt::lightGray);
  bool isSelected = (option->state & QStyle::State_Selected);
  if (isSelected)
    painter->setPen(QPen(Qt::black, 2, Qt::DotLine));
  else
    painter->setPen(QPen(Qt::black, 2, Qt::SolidLine));
  painter->drawRect(rect());
}

QRectF MonitorItem::boundingRect() const
{
  return QGraphicsRectItem::rect();
}

QPainterPath MonitorItem::shape() const
{
  QPainterPath path;
  path.addRect(rect());
  return path;
}

void MonitorItem::contextMenuEvent(QGraphicsSceneContextMenuEvent *event)
{
  QMenu menu;
  QAction *identifyAction = menu.addAction(tr("Identify"));
  menu.addSeparator();
  QAction *primaryAction = menu.addAction(tr("Primary"));
  primaryAction->setCheckable(true);
  primaryAction->setChecked(m_isPrimary);

  QMenu *resMenu = menu.addMenu(tr("Resolution"));
  for (const QSize &res : m_possibleResolutions)
  {
    QString resText = QString("%1x%2").arg(res.width()).arg(res.height());
    QAction *act = resMenu->addAction(resText);
    act->setCheckable(true);
    if(res == m_currentResolution)
    {
      act->setChecked(true);
    }
    act->setData(QVariant::fromValue(res));
  }
  if(!m_possibleResolutions.contains(m_currentResolution))
  {
    resMenu->addSeparator();
    QString customResText = tr("Custom: ") + QString("%1x%2")
                                                 .arg(m_currentResolution.width())
                                                 .arg(m_currentResolution.height());
    QAction *customResItem = resMenu->addAction(customResText);
    customResItem->setCheckable(true);
    customResItem->setChecked(true);
    customResItem->setData(QVariant::fromValue(m_currentResolution));
  }
  resMenu->addSeparator();
  QAction *setCustomResAction = resMenu->addAction(tr("Set Custom Resolution..."));

  QMenu *orientMenu = menu.addMenu(tr("Orientation"));
  struct { Orientation orient; QString label; } orients[] = {
      { Orientation::Normal, tr("Normal") },
      { Orientation::Left, tr("Left") },
      { Orientation::Inverted, tr("Inverted") },
      { Orientation::Right, tr("Right") }
  };
  for (auto &o : orients)
  {
    QAction *oa = orientMenu->addAction(o.label);
    oa->setCheckable(true);
    if(m_orientation == o.orient)
    {
      oa->setChecked(true);
    }
    oa->setData(static_cast<int>(o.orient));
  }

  QMenu *touchMenu = menu.addMenu(tr("Touch device"));
  QAction *noneTouchAction = touchMenu->addAction(tr("(none)"));
  noneTouchAction->setCheckable(true);
  bool currentlyNone = m_touchDeviceIdPath.isEmpty();
  noneTouchAction->setChecked(currentlyNone);
  noneTouchAction->setData(QString());
  const auto &allDevices = XInputBackend::instance().devices();
  for (const XInputDevice &dev : allDevices)
  {
    QString text = QString("%1 (%2)").arg(dev.name).arg(dev.idPath);
    QAction *act = touchMenu->addAction(text);
    act->setCheckable(true);
    if(dev.idPath == m_touchDeviceIdPath && dev.name == m_touchDeviceName)
    {
      act->setChecked(true);
    }
    act->setData(dev.idPath + "||" + dev.name);
  }

  QAction *chosen = menu.exec(event->screenPos());
  if(!chosen)
  {
    return;
  }

  if(chosen == identifyAction)
  {
    QScreen *targetScreen = nullptr;
    const QList<QScreen *> screens = QGuiApplication::screens();
    for (QScreen *screen : screens)
    {
      if(screen->name() == m_screenName)
      {
        targetScreen = screen;
        break;
      }
    }
    if(!targetScreen)
    {
      return;
    }
    QRect screenGeometry = targetScreen->geometry();
    ClickableOverlay *overlayWidget = new ClickableOverlay(nullptr, Qt::FramelessWindowHint | Qt::Tool);
    overlayWidget->setAttribute(Qt::WA_TranslucentBackground);
    overlayWidget->setAttribute(Qt::WA_ShowWithoutActivating);
    overlayWidget->setWindowModality(Qt::NonModal);
    overlayWidget->setGeometry(screenGeometry);
    QLabel *label = new QLabel(overlayWidget);
    label->setText(m_screenName);
    QFont font = label->font();
    font.setPointSize(64);
    font.setBold(true);
    label->setFont(font);
    label->setStyleSheet("QLabel { color : red; background: transparent; }");
    label->setAlignment(Qt::AlignCenter);
    label->setGeometry(overlayWidget->rect());
    overlayWidget->show();
    QTimer::singleShot(3000, overlayWidget, [overlayWidget]() { overlayWidget->close(); });
    return;
  }
  if(chosen == primaryAction)
  {
    setPrimary(!m_isPrimary);
    return;
  }
  if(resMenu->actions().contains(chosen))
  {
    if(chosen == setCustomResAction)
    {
      bool ok;
      QString input = QInputDialog::getText(nullptr, tr("Custom Resolution"),
                                            tr("Enter resolution as WIDTHxHEIGHT (e.g. 1280x1024):"),
                                            QLineEdit::Normal, "", &ok);
      if(ok && !input.isEmpty())
      {
        QStringList parts = input.split(QRegularExpression("[xX]"), Qt::SkipEmptyParts);
        if(parts.size() == 2)
        {
          bool okWidth, okHeight;
          int width = parts.at(0).toInt(&okWidth);
          int height = parts.at(1).toInt(&okHeight);
          if(okWidth && okHeight && width > 0 && height > 0)
          {
            setResolution(QSize(width, height));
          }
          else
          {
            QMessageBox::warning(nullptr, tr("Invalid Input"), tr("Invalid resolution values."));
          }
        }
        else
        {
          QMessageBox::warning(nullptr, tr("Invalid Input"), tr("Resolution must be in the format WIDTHxHEIGHT."));
        }
      }
      return;
    }
    else
    {
      QSize newRes = chosen->data().value<QSize>();
      setResolution(newRes);
      return;
    }
  }
  if(orientMenu->actions().contains(chosen))
  {
    int val = chosen->data().toInt();
    setOrientation(static_cast<Orientation>(val));
    return;
  }
  if(touchMenu->actions().contains(chosen))
  {
    QString data = chosen->data().toString();
    if(data.isEmpty())
    {
      setTouchDeviceIdPath("");
      setTouchDeviceName("");
    }
    else
    {
      QStringList parts = data.split("||");
      if(parts.size() >= 2)
      {
        setTouchDeviceIdPath(parts.at(0));
        setTouchDeviceName(parts.at(1));
      }
      else
      {
        setTouchDeviceIdPath(data);
        setTouchDeviceName("");
      }
    }
    return;
  }
}

void MonitorItem::setResolution(const QSize &res)
{
  m_currentResolution = res;
  updateRectFromResolutionAndAngle();
  updateTextAngle();
}

void MonitorItem::setOrientation(Orientation orient)
{
  m_orientation = orient;
  updateRectFromResolutionAndAngle();
  updateTextAngle();
}

void MonitorItem::updateRectFromResolutionAndAngle()
{
  double w = m_currentResolution.width();
  double h = m_currentResolution.height();
  if (m_orientation == Orientation::Left || m_orientation == Orientation::Right)
  {
    double tmp = w;
    w = h;
    h = tmp;
  }
  double sw = w * kScaleFactor;
  double sh = h * kScaleFactor;
  setRect(0, 0, sw, sh);
}

void MonitorItem::updateTextAngle()
{
  if (!m_nameItem)
    return;
  qreal angle = 0;
  switch (m_orientation)
  {
    case Orientation::Normal:
      angle = 0;
      break;
    case Orientation::Left:
      angle = 90;
      break;
    case Orientation::Inverted:
      angle = 180;
      break;
    case Orientation::Right:
      angle = 270;
      break;
  }
  QRectF tb = m_nameItem->boundingRect();
  m_nameItem->setTransformOriginPoint(tb.center());
  m_nameItem->setRotation(angle);
  double bw = rect().width();
  double bh = rect().height();
  double tw = tb.width();
  double th = tb.height();
  double cx = bw * 0.5 - tw * 0.5;
  double cy = bh * 0.5 - th * 0.5;
  m_nameItem->setPos(cx, cy);
}

double MonitorItem::snapX(double proposedX)
{
  QRectF futureRect(QPointF(proposedX, pos().y()), rect().size());
  double bestX = proposedX;
  double minDelta = std::numeric_limits<double>::max();
  for (QGraphicsItem *other : scene()->items())
  {
    if (other == this)
      continue;
    auto otherRectItem = dynamic_cast<QGraphicsRectItem *>(other);
    if (!otherRectItem)
      continue;
    QRectF oRect = otherRectItem->sceneBoundingRect();
    trySnapHorizontal(futureRect.left(), oRect.left(), futureRect.width(), false, bestX, minDelta);
    trySnapHorizontal(futureRect.left(), oRect.right(), futureRect.width(), false, bestX, minDelta);
    trySnapHorizontal(futureRect.right(), oRect.left(), futureRect.width(), true, bestX, minDelta);
    trySnapHorizontal(futureRect.right(), oRect.right(), futureRect.width(), true, bestX, minDelta);
  }
  QRectF sRect = scene()->sceneRect();
  double deltaLeft = futureRect.left() - sRect.left();
  if (qAbs(deltaLeft) < SNAP_DISTANCE && qAbs(deltaLeft) < minDelta)
  {
    minDelta = qAbs(deltaLeft);
    bestX = proposedX - deltaLeft;
  }
  double deltaRight = futureRect.right() - sRect.right();
  if (qAbs(deltaRight) < SNAP_DISTANCE && qAbs(deltaRight) < minDelta)
  {
    minDelta = qAbs(deltaRight);
    bestX = proposedX - deltaRight;
  }
  return bestX;
}

double MonitorItem::snapY(double proposedY)
{
  QRectF futureRect(QPointF(pos().x(), proposedY), rect().size());
  double bestY = proposedY;
  double minDelta = std::numeric_limits<double>::max();
  for (QGraphicsItem *other : scene()->items())
  {
    if (other == this)
      continue;
    auto otherRectItem = dynamic_cast<QGraphicsRectItem *>(other);
    if (!otherRectItem)
      continue;
    QRectF oRect = otherRectItem->sceneBoundingRect();
    trySnapVertical(futureRect.top(), oRect.top(), futureRect.height(), false, bestY, minDelta);
    trySnapVertical(futureRect.top(), oRect.bottom(), futureRect.height(), false, bestY, minDelta);
    trySnapVertical(futureRect.bottom(), oRect.top(), futureRect.height(), true, bestY, minDelta);
    trySnapVertical(futureRect.bottom(), oRect.bottom(), futureRect.height(), true, bestY, minDelta);
  }
  QRectF sRect = scene()->sceneRect();
  double deltaTop = futureRect.top() - sRect.top();
  if (qAbs(deltaTop) < SNAP_DISTANCE && qAbs(deltaTop) < minDelta)
  {
    minDelta = qAbs(deltaTop);
    bestY = proposedY - deltaTop;
  }
  double deltaBottom = futureRect.bottom() - sRect.bottom();
  if (qAbs(deltaBottom) < SNAP_DISTANCE && qAbs(deltaBottom) < minDelta)
  {
    minDelta = qAbs(deltaBottom);
    bestY = proposedY - deltaBottom;
  }
  return bestY;
}

void MonitorItem::trySnapHorizontal(double myEdge,
				    double otherEdge,
				    double myWidth,
				    bool edgeIsRight,
				    double &bestX,
				    double &minDelta)
{
  double delta = qAbs(myEdge - otherEdge);
  if (delta < SNAP_DISTANCE && delta < minDelta)
  {
    minDelta = delta;
    double shift = otherEdge - myEdge;
    bestX += shift;
  }
}

void MonitorItem::trySnapVertical(double myEdge,
				  double otherEdge,
				  double myHeight,
				  bool edgeIsBottom,
				  double &bestY,
				  double &minDelta)
{
  double delta = qAbs(myEdge - otherEdge);
  if (delta < SNAP_DISTANCE && delta < minDelta)
  {
    minDelta = delta;
    double shift = otherEdge - myEdge;
    bestY += shift;
  }
}
