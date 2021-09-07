#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QTextStream>
#include <opencv4/opencv2/opencv.hpp>
#include <QDebug>
#include <QRegularExpression>
#include <QProgressDialog>
#include <QMessageBox>
#include <QDesktopServices>

//TODO: переделать сигнал удаления данных из файлов
// Вынести из функции удаление данных из файлов в одную отдельную функцию
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , indexBlureClass(-1)
    , deleteClassBlur(true)
    , nameClassBlurInFile("blure")
    , alreadyDeletedClassName(false)
    , alreadyDeletedCoordinates(false)
{
    ui->setupUi(this);
    ui->leNameClassForBlure->setText(nameClassBlurInFile);
    connect(ui->btnOpenDir, &QPushButton::clicked,
            this, &MainWindow::openDir);
    connect(ui->btnOpenNamesFile, &QPushButton::clicked,
            this, &MainWindow::openFile);
    connect(ui->btnBlur, &QPushButton::clicked,
            this, &MainWindow::bluringImage);
    connect(ui->rbDeleteClassBlur, &QRadioButton::toggled,
            [this](bool set) { deleteClassBlur = set; });
    connect(ui->leNameClassForBlure, &QLineEdit::textEdited,
            this, &MainWindow::setNameBlurInFile);
    connect(ui->btnDeleteAllBlure, &QPushButton::clicked, this, &MainWindow::deleteClassBlurAndCoordinates);
    //TODO: доделать выбор

//    connect(ui->comboBoxNameClassForBlure, &QComboBox::currentTextChanged, this, &MainWindow::setNameBlurInFile);

    connect(this, &MainWindow::classBlurFound,
            ui->cbClassBlurFound, &QCheckBox::setChecked);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setNameBlurInFile(const QString &name)
{
    nameClassBlurInFile = name;
}

QString MainWindow::getNameBlurInFile() const
{
    return nameClassBlurInFile;
}

bool MainWindow::openDir()
{
    alreadyDeletedCoordinates = false;
    ui->leCountImages->clear();
    pathDirWithImage = QDir::toNativeSeparators(QFileDialog::getExistingDirectory(this, tr("Find Path"), QDir::homePath()));
    if (!pathDirWithImage.exists()) {
        QMessageBox::warning(this, tr("File no exist"), tr("The selected file does not exist"));
        return false;
    }

    QStringList filters;
    filters << "*.jpg" << "*.png";
    listImage = pathDirWithImage.entryInfoList(filters, QDir::Files);
    ui->leCountImages->setText(QString::number(listImage.size()));

    if (listImage.empty()) {
        QMessageBox::warning(this, tr("Folder error"), tr("There are no files with images or the folder is empty"));
        return false;
    }
    emit updateDirInfo();
    return true;
}

bool MainWindow::openFile() //TODO: переделать с возможностью выбора из combobox
{
    alreadyDeletedClassName = false;
    ui->cbClassBlurFound->setChecked(false);
    pathToFileNameClass = QFileDialog::getOpenFileName(this, tr("Open \"Name class\" file"), QDir::homePath());
    QFile file(pathToFileNameClass);

    if (!file.exists()) {
        QMessageBox::warning(this, tr("File error"), tr("The file with the class names cannot be opened"));
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("File error"), tr("The file with the class names cannot be opened"));
        return false;
    }
    listOfClass.clear();
    QTextStream streamFromFile(&file);
    listOfClass = streamFromFile.readAll().split("\n");
    listOfClass.removeAll("");


    indexBlureClass = listOfClass.indexOf(getNameBlurInFile());
    ui->comboBoxNameClassForBlure->clear();
    ui->comboBoxNameClassForBlure->addItems(listOfClass);

    file.close();
    emit updateFileInfo(); //TODO: check
    emit classBlurFound(true);
    if (indexBlureClass < 0) {
        QMessageBox::information(this, tr("Class blur"), tr("The class for blurring was not found"));
        return false;
    }

    if (deleteClassBlur && !alreadyDeletedClassName) {
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QMessageBox::warning(this, tr("File delete error"), tr("The file with the class names cannot be opened"));
            return false;
        }
        listOfClass.removeAt(indexBlureClass);
        ui->comboBoxNameClassForBlure->removeItem(indexBlureClass);
        QTextStream streamToFile(&file);

        for (const QString &str : qAsConst(listOfClass)) {
            streamToFile << str + "\n";
        }
        file.close();
        alreadyDeletedClassName = true;
    }


    ui->comboBoxNameClassForBlure->setCurrentIndex(indexBlureClass);

    return true;
}

bool MainWindow::bluringImage()
{
    if (indexBlureClass >= 0 && !listImage.isEmpty()) {
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

            if (!fileTxt.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox::warning(this, tr("File error"), tr("The file with the coordinates cannot be opened"));
                return false;
            }

            QTextStream stream(&fileTxt);
            listAllCoordinates = stream.readAll().split("\n");
            listAllCoordinates.removeAll("");

            QStringList listCoordinates = listAllCoordinates.filter(re);

            fileTxt.close();

            if (deleteClassBlur && !alreadyDeletedCoordinates) {
                if (!fileTxt.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                    QMessageBox::warning(this, tr("File delete error"), tr("The file with the coordinates cannot be opened"));
                    return false;
                }
                for (const QString &str : qAsConst(listAllCoordinates)) {
                    QStringList tempStrList = str.split(" ");
                    if (tempStrList[0].toInt() != indexBlureClass && (tempStrList.size() > 1)){
                        stream << str + "\n";
                    }
                }
                fileTxt.close();
                alreadyDeletedCoordinates = true;
            }


            for (const QString &box : qAsConst(listCoordinates)) { //TODO: перенос в отдельную функцию для потока
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
    QMessageBox::warning(this, tr("Bluring error"), tr("The class for blurring was not found or the list of images is empty"));
    return false;
}

int MainWindow::deleteClassBlurAndCoordinates()
{
    int st = 0;
    if (!listOfClass.empty() && !alreadyDeletedClassName) {
        QFile file(pathToFileNameClass);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QMessageBox::warning(this, tr("File delete error"), tr("The file with the class names cannot be opened"));
            return -1;
        }
        listOfClass.removeAt(indexBlureClass);
        ui->comboBoxNameClassForBlure->removeItem(indexBlureClass);
        QTextStream streamToFile(&file);

        for (const QString &str : qAsConst(listOfClass)) {
            streamToFile << str + "\n";
        }
        file.close();
        alreadyDeletedClassName = true;
        st++;
    }

    if (!listAllCoordinates.empty() && !alreadyDeletedCoordinates) {
        QFile fileTxt;
        for (const QFileInfo &imagePath : qAsConst(listImage)) {
            fileTxt.setFileName(imagePath.path() + "/" + imagePath.baseName() + ".txt");

            if (!fileTxt.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
                QMessageBox::warning(this, tr("File delete error"), tr("The file with the coordinates cannot be opened"));
                return -1;
            }
            QTextStream stream(&fileTxt);
            for (const QString &str : qAsConst(listAllCoordinates)) {
                QStringList tempStrList = str.split(" ");
                if (tempStrList[0].toInt() != indexBlureClass && (tempStrList.size() > 1)){
                    stream << str + "\n";
                }
            }
            fileTxt.close();
            alreadyDeletedCoordinates = true;
        }
        st++;
    }

    return st;
}

