#include "scanwidget.h"
#include "ui_scanwidget.h"

#include <QLineEdit>
#include <QProgressBar>
#include <assert.h>
#include <iostream>

void ScanWidget::connectLineEdit(const char *lineEditName, Counter *counter)
{
  QLineEdit *ui_lineEdit = findChild<QLineEdit*>(lineEditName);

  assert(ui_lineEdit != NULL);

  QObject::connect(counter, SIGNAL(changed(const QString &)), ui_lineEdit, SLOT(setText(const QString &)));
  ui_lineEdit->setText(QString::number(counter->value()));
}

void ScanWidget::doConnect(ScanModel *model)
{
  connectLineEdit("totalCount", model->totalCount());
  connectLineEdit("scannedCount", model->scannedCount());
  connectLineEdit("malwareCount", model->malwareCount());
  connectLineEdit("suspiciousCount", model->suspiciousCount());
  connectLineEdit("unhandledCount", model->unhandledCount());
  connectLineEdit("cleanCount", model->cleanCount());

  QProgressBar *ui_progressBar = findChild<QProgressBar*>("progressBar");
  assert(ui_progressBar != NULL);
  ui_progressBar->setMaximum(model->totalCount()->value());
  QObject::connect(model->totalCount(), SIGNAL(changed(int)), ui_progressBar, SLOT(setMaximum(int)));
  ui_progressBar->setValue(model->scannedCount()->value());
  QObject::connect(model->scannedCount(), SIGNAL(changed(int)), ui_progressBar, SLOT(setValue(int)));

  QLineEdit *ui_currentScannedPath = findChild<QLineEdit*>("currentScannedPath");
  assert(ui_currentScannedPath != NULL);
  ui_currentScannedPath->setText(model->path());
  QObject::connect(model, SIGNAL(scanning(QString)), ui_currentScannedPath, SLOT(setText(QString)));

  ui_closeButton = findChild<QPushButton*>("closeButton");
  assert(ui_closeButton != NULL);
  QObject::connect(model, SIGNAL(scanComplete()), this, SLOT(enableCloseButton()));
  if (model->completed())
    enableCloseButton();
}

ScanWidget::ScanWidget(ScanModel *model, QWidget *parent) :
  QWidget(parent),
  ui(new Ui::ScanWidget)
{
  ui->setupUi(this);

  doConnect(model);

  QTableView *ui_reportView = findChild<QTableView*>("reportView");
  assert(ui_reportView != NULL);
  ui_reportView->setModel(model->report());

#if QT_VERSION < 0x050000
  ui_reportView->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
#else
  ui_reportView->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
#endif
}

void ScanWidget::enableCloseButton() 
{
  ui_closeButton->setEnabled(true);
}

void ScanWidget::on_closeButton_clicked() 
{
  window()->close();
}

ScanWidget::~ScanWidget()
{
    delete ui;
}
