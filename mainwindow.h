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

    void setNameBlurInFile(const QString &name);
    QString getNameBlurInFile() const;

private slots:
    bool openDir();
    bool openFile();
    bool bluringImage();

private:
    QDir pathDirWithImage;
    QStringList listOfClass;
    int indexBlureClass;
    QFileInfoList listImage;
    bool deleteClassBlur;
    QString nameClassBlurInFile;

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
