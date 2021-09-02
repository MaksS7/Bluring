#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->btnOpenDir, &QPushButton::clicked,
            this, &MainWindow::openDir);
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::openDir()
{
    QDir path = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("Find Path"), QDir::homePath()));


    return true;
}

bool MainWindow::openFile()
{
    QFile file(QFileDialog::getOpenFileName(this, tr("Open \"Name class\" file"), QDir::homePath()));
    return true;
}

