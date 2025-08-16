#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_CleanButton_clicked();

    void on_GenerateButton_clicked();

    void on_AboutButton_clicked();

    void on_GitHubButton_clicked();

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
