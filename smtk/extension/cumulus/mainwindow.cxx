#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtGui/QDesktopWidget>
#include <QtGui/QStatusBar>

namespace cumulus {

MainWindow::MainWindow()
  : m_ui(new Ui::MainWindow)
{
  m_ui->setupUi(this);

  QRect screenGeometry = QApplication::desktop()->screenGeometry();
  int x = (screenGeometry.width()-this->width()) / 2;
  int y = (screenGeometry.height()-this->height()) / 2;
  this->move(x, y);

  this->createMainMenu();

  connect(this->m_ui->cumulusWidget, SIGNAL(info(QString)),
      this, SLOT(displayInfo(QString)));
}

MainWindow::~MainWindow()
{
  delete m_ui;
}

void MainWindow::girderUrl(const QString &url)
{
  this->m_ui->cumulusWidget->girderUrl(url);
}

void MainWindow::createMainMenu()
{
  connect(m_ui->actionQuit, SIGNAL(triggered()),
          qApp, SLOT(quit()));
}

void MainWindow::closeEvent(QCloseEvent *theEvent)
{
  qApp->quit();
}

void MainWindow::displayInfo(const QString &msg)
{
  this->statusBar()->showMessage(msg);
}


} // end namespace
