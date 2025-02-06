#pragma once

#include <QMainWindow>

class QGraphicsScene;
class QGraphicsView;

class MainWindow : public QMainWindow
{
  Q_OBJECT
public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override = default;

private:
  QGraphicsScene *m_scene = nullptr;
  QGraphicsView *m_view = nullptr;

  void createToolbar();
  virtual QMenu *createPopupMenu() override;

  QString buildScript();

private slots:
  void applyConfig();
  void saveScript();
  void showInfo();
};
