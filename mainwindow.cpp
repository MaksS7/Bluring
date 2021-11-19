#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , indexBlureClass(-1)
    , deleteClassBlur(false)
    , nameClassBlurInFile("blure")
    , alreadyDeletedClassName(false)
    , alreadyDeletedCoordinates(false)
    , blureGain(10.0)
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

    auto frameIdTextChanged = [this](const QString &text) {
        if (!text.isEmpty() && text != getNameBlurInFile()) {
            setNameBlurInFile(text);
            setBlureIndex(listOfClass.lastIndexOf(text));
            ui->leNameClassForBlure->setText(text);
        }
    };

    connect(ui->comboBoxNameClassForBlure, QOverload<const QString &>::of(&QComboBox::activated),
        [=](const QString &text){ frameIdTextChanged(text);});

    connect(this, &MainWindow::classBlurFound,
            ui->cbClassBlurFound, &QCheckBox::setChecked);
    connect(ui->btnTest, &QPushButton::clicked, this, &MainWindow::startWorkInAThread);

    connect(ui->dSpinBoxGainBluring, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::setGainBlure);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setNameBlurInFile(const QString &name)
{
    nameClassBlurInFile = name;
}

void MainWindow::setBlureIndex(const int &num)
{
    indexBlureClass = num;
}

QString MainWindow::getNameBlurInFile() const
{
    return nameClassBlurInFile;
}

bool MainWindow::openDir()
{
    alreadyDeletedCoordinates = false;
    ui->cbImagesFound->setChecked(false);
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
    ui->cbImagesFound->setChecked(true);
    return true;
}

bool MainWindow::openFile()
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

    int tempIndex = -1;
    tempIndex = listOfClass.indexOf(getNameBlurInFile());
    ui->comboBoxNameClassForBlure->clear();
    ui->comboBoxNameClassForBlure->addItems(listOfClass);

    if (tempIndex < 0) {
        QMessageBox::information(this, tr("Class blur"), tr("The class for blurring was not found"));
        return false;
    } else {
        setBlureIndex(tempIndex);
        ui->comboBoxNameClassForBlure->setCurrentIndex(tempIndex);
    }

    file.close();
    emit updateFileInfo(); //TODO: check
    emit classBlurFound(true);
    return true;
}

bool MainWindow::bluringImage()
{
    int tempIndex = getBlurIndex();
    if (tempIndex >= 0 && !listImage.isEmpty()) {
        QFile fileTxt;
        QRegularExpression re("^(" + QString::number(tempIndex) + ") ");

        QProgressDialog progressDialog(this);
        progressDialog.setCancelButtonText(tr("&Cancel"));
        progressDialog.setRange(0, listImage.size());
        progressDialog.setWindowTitle(tr("BLURING files"));

        progressDialog.show();
        int loopIndex = 0;
        QString fileWithTxtInfoName;
        for (const QFileInfo &imagePath : qAsConst(listImage)) {
            progressDialog.setValue(++loopIndex);
            progressDialog.setLabelText(tr("Bluring number %1 of %n...", nullptr, listImage.size()).arg(loopIndex));
            QCoreApplication::processEvents();

            if (progressDialog.wasCanceled()) {
                break;
            }

            cv::Mat tempImage = cv::imread(imagePath.filePath().toStdString());
            fileWithTxtInfoName = (imagePath.path() + "/" + imagePath.baseName() + ".txt");
            fileTxt.setFileName(fileWithTxtInfoName);

            if (!fileTxt.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMessageBox::warning(this, tr("File error"), tr("The file with the coordinates cannot be opened"));
                return false;
            }

            QTextStream stream(&fileTxt);
            listAllCoordinates = stream.readAll().split("\n");
            listAllCoordinates.removeAll("");

            QStringList listCoordinates = listAllCoordinates.filter(re);

            fileTxt.close();

            for (const QString &box : qAsConst(listCoordinates)) { //TODO: перенос в отдельную функцию для потока
                 QStringList coordinates = box.split(" ");
                 coordinates.removeAt(0);

                 float const width = coordinates[2].toFloat() * tempImage.cols;
                 float const height = coordinates[3].toFloat() * tempImage.rows;
                 float const x = (coordinates[0].toFloat() * tempImage.cols) - width / 2.f;
                 float const y = (coordinates[1].toFloat() * tempImage.rows) - height / 2.f;

                 //TODO: Добавить выбор типа замыливания
                 cv::GaussianBlur(tempImage(cv::Rect(x, y, width, height)),
                                  tempImage(cv::Rect(x, y, width, height)), cv::Size(0,0), blureGain);
            }
            cv::imwrite(imagePath.filePath().toStdString(), tempImage);
            tempImage.release();

            if (deleteClassBlur) {
                if (!deleteCoordinatesFromFile(fileWithTxtInfoName, tempIndex)) {
                        QMessageBox::warning(this, tr("File error"), tr("Error in deleting coordinates from a file: %1")
                                             .arg(fileWithTxtInfoName));
                }
            }
        }
        //TODO: удаление класса
        if (deleteClassBlur) {
            alreadyDeletedCoordinates = true;
            int stDeleted = deleteClassBlurAndCoordinates();
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
    int tempIndex = getBlurIndex();
    if (!listOfClass.empty() && !alreadyDeletedClassName) {
        QFile file(pathToFileNameClass);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
            QMessageBox::warning(this, tr("File delete error"), tr("The file with the class names cannot be opened"));
            return -1;
        }
        listOfClass.removeAt(tempIndex);
        ui->comboBoxNameClassForBlure->setCurrentIndex(0);
        ui->comboBoxNameClassForBlure->removeItem(tempIndex);
        QTextStream streamToFile(&file);

        for (const QString &str : qAsConst(listOfClass)) {
            streamToFile << str + "\n";
        }
        file.close();
        alreadyDeletedClassName = true;
        st++;
    }

    if (!listAllCoordinates.empty() && !alreadyDeletedCoordinates) {
        for (const QFileInfo &imagePath : qAsConst(listImage)) {
            if (!deleteCoordinatesFromFile(imagePath.path() + "/" + imagePath.baseName() + ".txt", tempIndex)) {
                QMessageBox::warning(this, tr("File error"), tr("Error in deleting coordinates from a file: %1")
                                     .arg(imagePath.path() + "/" + imagePath.baseName() + ".txt", tempIndex));
            }
        }
        alreadyDeletedCoordinates = true;
        st++;
    }
    return st;
}



void MainWindow::startWorkInAThread()
{
    static int toThread = 0;
    WorkerThread *workerThread = new WorkerThread(this, toThread++);
//    connect(workerThread, &WorkerThread::resultReady, this, &Main::handleResults);
    connect(workerThread, &WorkerThread::finished, workerThread, &QObject::deleteLater);
    workerThread->start();
}

bool MainWindow::deleteCoordinatesFromFile(const QString &name, const int &index)
{
    if (name.isEmpty() || index < 0)
        return false;

    QFile fileTxt(name);
    if (!fileTxt.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QMessageBox::warning(this, tr("File delete error"), tr("The file with the coordinates cannot be opened"));
        return false;
    }
    QTextStream stream(&fileTxt);
    for (const QString &str : qAsConst(listAllCoordinates)) {
        QStringList tempStrList = str.split(" ");
        if (tempStrList[0].toInt() != index && (tempStrList.size() > 1)){
            stream << str + "\n";
        }
    }
    fileTxt.close();
    return true;
}

WorkerThread::WorkerThread(QObject *parent, const int &number): QThread(parent)
  ,num(number){

}
