#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include "cstringsender.h"
#include "cinfomessagehandler.h"
#include <QMainWindow>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QFileInfo>
#include <QResource>
#include <QSoundEffect>
#include <QScopedPointer>
#include <QSharedPointer>


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

    void on_SendSelectionButton_clicked();

    void on_StopButton_clicked();

    void on_WebsiteButton_clicked();

    void on_MessagesCheckBox_stateChanged(int state);

    void on_SendReturnCheckBox_stateChanged(int state);

    void on_TimeoutSlider_valueChanged(int value);

private:
    QScopedPointer<Ui::MainWindow> ui;
    QStringList m_aBarcodes;
    unsigned int m_nIterator;
    QScopedPointer<CStringSender> m_pStringSender;
    bool SendBarcodeByIterator(int nIt);
    bool SendBarcodeString(QString sBarcode);
    void ShowWaylandWarningMessage();
    void PlayScanSound();
    void PlayScanSoundAlternative(); // Альтернативная функция с QSoundEffect
    void TestSoundResources(); // Добавляем тестовую функцию
    void LoadBarcodesFromFile(); // Загрузка штрихкодов из файла
    QSharedPointer <CInfoMessageHandler> m_InfoMessageHandler;

    
    // Timer-related members
    QScopedPointer<QTimer> m_pSendTimer;
    int m_nCurrentBarcodeIndex;
};
#endif // MAINWINDOW_H
