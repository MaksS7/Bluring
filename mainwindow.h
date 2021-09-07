#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>

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

};
#endif // MAINWINDOW_H
