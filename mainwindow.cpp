#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTextStream>
#include <opencv4/opencv2/opencv.hpp>
#include <QDebug>
#include <QRegularExpression>
#include <QProgressDialog>
#include <QMessageBox>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , indexBlureClass(-1)
    , deleteClassBlur(true)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->btnOpenDir, &QPushButton::clicked,
            this, &MainWindow::openDir);
    connect(ui->btnOpenNamesFile, &QPushButton::clicked,
            this, &MainWindow::openFile);
    connect(ui->btnBlur, &QPushButton::clicked,
            this, &MainWindow::bluringImage);
    connect(ui->rbDeleteClassBlur, &QRadioButton::toggled, [this](bool set) { deleteClassBlur = set; });
}

MainWindow::~MainWindow()
{
    delete ui;
}

bool MainWindow::openDir()
{
    pathDirWithImage = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("Find Path"), QDir::homePath()));
    if (!pathDirWithImage.exists())
        return false;

    QStringList filters;
    filters << "*.jpg" << "*.png";
    listImage = pathDirWithImage.entryInfoList(filters, QDir::Files);
    ui->leCountImages->setText(QString::number(listImage.size()));

    if (listImage.empty())
        return false;

    return true;
}

bool MainWindow::openFile()
{
    QFile file(QFileDialog::getOpenFileName(this, tr("Open \"Name class\" file"), QDir::homePath()));

    if (!file.exists())
        return false;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    //TODO: добавить выбор класса для блюра в GUI
    QTextStream streamFromFile(&file);
    QStringList tempList = streamFromFile.readAll().split("\n");
    indexBlureClass = tempList.indexOf("blure");
    file.close();


    if (deleteClassBlur) {
        tempList.removeAt(indexBlureClass);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
            return false;

        for (const QString &str : qAsConst(tempList)) {
            streamFromFile << str + "\n";
        }
        file.close();
    }

    if (indexBlureClass < 0)
        return false;
    return true;
}

bool MainWindow::bluringImage()
{
    if (indexBlureClass >= 0) {
        QFile fileTxt;
        QRegularExpression re("^(" + QString::number(indexBlureClass) + ") ");

        QProgressDialog progressDialog(this);
        progressDialog.setCancelButtonText(tr("&Cancel"));
        progressDialog.setRange(0, listImage.size());
        progressDialog.setWindowTitle(tr("BLURING files"));

        progressDialog.show();
        int loopIndex = 0;

        for (const QFileInfo &imagePath : qAsConst(listImage)) {
            progressDialog.setValue(++loopIndex);
            progressDialog.setLabelText(tr("Bluring number %1 of %n...", nullptr, listImage.size()).arg(loopIndex));
            QCoreApplication::processEvents();

            if (progressDialog.wasCanceled()) {
                break;
            }

            cv::Mat tempImage = cv::imread(imagePath.filePath().toStdString());
            fileTxt.setFileName(imagePath.path() + "/" + imagePath.baseName() + ".txt");

            if (!fileTxt.open(QIODevice::ReadOnly | QIODevice::Text))
                return false;

            QTextStream stream(&fileTxt);
            QStringList listCoordinates = stream.readAll().split("\n").filter(re);

            if (deleteClassBlur) {
                //add delete class from file
            }

            fileTxt.close();

            for (const QString &box : qAsConst(listCoordinates)) { //TODO: перенос в отдельную функцию
                 QStringList coordinates = box.split(" ");
                 coordinates.removeAt(0);

                 float const width = coordinates[2].toFloat() * tempImage.cols;
                 float const height = coordinates[3].toFloat() * tempImage.rows;
                 float const x = (coordinates[0].toFloat() * tempImage.cols) - width / 2.f;
                 float const y = (coordinates[1].toFloat() * tempImage.rows) - height / 2.f;

                 cv::GaussianBlur(tempImage(cv::Rect(x, y, width, height)), tempImage(cv::Rect(x, y, width, height)), cv::Size(0,0), 10); //TODO: добавить параметр силы замыливания
            }
            cv::imwrite(imagePath.filePath().toStdString(), tempImage);
            tempImage.release();
        }
        progressDialog.close();
        QMessageBox msgBox;
        msgBox.setText("WELL DONE.");
        msgBox.exec();
        return true;
    }
    return false;
}

