#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QTextStream>
#include <opencv4/opencv2/opencv.hpp>
#include <QDebug>
#include <QRegularExpression>
#include <QProgressDialog>
#include <QMessageBox>
#include <QDesktopServices>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString getNameBlurInFile() const;
    void setNameBlurInFile(const QString &name);
    void setBlureIndex(const int &num);
    void setGainBlure(const double &number) {
        blureGain = number;
        qDebug() << blureGain;
    }
    int getBlurIndex() const {
        return indexBlureClass;
    }

signals:
    void classBlurFound(bool state);
    void updateDirInfo();
    void updateFileInfo();

private slots:
    bool openDir();
    bool openFile();
    bool bluringImage();
    int deleteClassBlurAndCoordinates();

private:
    enum blureType {
        medianBlur = 0,
        gaussianBlur = 8,
        bilateralFilter = 16,
        boxFilter = 24,
        sqrBoxFilter = 32,
        blur = 40
    };
    void startWorkInAThread();
    bool deleteClassNameFromFile();
    bool deleteCoordinatesFromFile(const QString &name, const int &index);
    Ui::MainWindow *ui;

    QDir pathDirWithImage;
    QStringList listOfClass;
    int indexBlureClass;
    QFileInfoList listImage;
    bool deleteClassBlur;
    QString nameClassBlurInFile;
    QString pathToFileNameClass;
    QStringList listAllCoordinates;
    bool alreadyDeletedClassName;
    bool alreadyDeletedCoordinates;
    double blureGain;

};

class WorkerThread : public QThread
{
    Q_OBJECT
public:
    explicit WorkerThread(QObject *parent = nullptr, const int &number = 10);

    void run() override {
        QString result;
        qDebug() << num;
        emit resultReady(result);
    }

private:
    int num;
signals:
    void resultReady(const QString &s);
};


#endif // MAINWINDOW_H
