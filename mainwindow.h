#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "cstringsender.h"
#include "cstringsenderlinuxwayland.h"
#include "cstringsenderwin32.h"
#include <QMainWindow>
#include <QTimer>


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

    void on_ReadButton_clicked();

    void on_SendAllButton_clicked();
    
    void sendNextBarcode();

    void on_SendNextButton_clicked();
    
    void on_SendPreviousButton_clicked();

private:
    Ui::MainWindow *ui;
    QStringList m_aBarcodes;
    unsigned int m_nIterator;
    CStringSender* m_pStringSender;
    bool SendBarcodeByIterator(int nIt);
    
    // Timer-related members
    QTimer* m_pSendTimer;
    int m_nCurrentBarcodeIndex;
};
#endif // MAINWINDOW_H
