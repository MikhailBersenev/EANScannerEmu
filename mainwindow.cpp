#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "generatedialog.h"
#include "cutils.h"

#include "cstringsenderlinuxwayland.h"
#include "cstringsenderlinuxx11.h"
#include "cstringsenderwin32.h"

#include <QMessageBox>
#include <QDesktopServices>
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <QFile>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTemporaryFile>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->TimeoutSlider->setSliderPosition(10);
    this->setWindowTitle(this->windowTitle() + " ver. " + CUtils::GetVersion());
    ui->SendReturnCheckBox->setChecked(true);
    setWindowIcon(QIcon(":/icon.ico"));
    qDebug() << "EANScannerEmu ver." << CUtils::GetVersion();
    m_pStringSender = nullptr;
    m_pSendTimer = nullptr;
    m_nCurrentBarcodeIndex = 0;
#ifdef Q_OS_LINUX
    QString sDisplayServer = QGuiApplication::platformName();
    if (sDisplayServer.contains("wayland", Qt::CaseInsensitive)) {
        qDebug() << "Display Server Wayland";
        m_pStringSender = new CStringSenderLinuxWayland(this);
        ShowWaylandUnsupportedMessage();
    } else if (sDisplayServer.contains("xcb", Qt::CaseInsensitive)) {
        qDebug() << "Display Server X11";
        m_pStringSender = new CStringSenderLinuxX11(this);
    }
    else {
        QMessageBox::critical(this, "Error", "Unknown display server");
        exit(0);
    }
#endif
#ifdef Q_OS_WIN
    m_pStringSender = new CStringSenderWin32(this);
#endif



}

MainWindow::~MainWindow()
{
    if(m_pStringSender)
        delete m_pStringSender;
    delete ui;
}

void MainWindow::on_CleanButton_clicked()
{
    ui->BarcodesMemo->clear();
    m_aBarcodes.clear();
    m_nCurrentBarcodeIndex = 0; // Reset index when clearing

}


void MainWindow::on_GenerateButton_clicked()
{
    GenerateDialog* pGenerateDialog = new GenerateDialog(this);
    pGenerateDialog->exec();
    const int nQnt = pGenerateDialog->GetQuantity();
    GenerateDialog::Type nType = pGenerateDialog->GetType();
    delete pGenerateDialog;
    QStringList aGeneratedItems;
    switch (nType) {
    case GenerateDialog::Type::EAN8:
        aGeneratedItems = CUtils::GenerateEAN8(nQnt);
        break;
    case GenerateDialog::Type::EAN13:
        aGeneratedItems = CUtils::GenerateEAN13(nQnt);
        break;
    default:
        return;
    }
    for(int i = 0; i < aGeneratedItems.size(); i++) {
        ui->BarcodesMemo->append(aGeneratedItems.at(i));
    }
}


void MainWindow::on_AboutButton_clicked()
{
    QMessageBox::information(this, "About", "EAN Code Scanner Generator\n Version " + CUtils::GetVersion());
}


void MainWindow::on_GitHubButton_clicked()
{
    QDesktopServices::openUrl(QUrl::fromUserInput("https://github.com/MikhailBersenev/EANScannerEmu"));
}


void MainWindow::on_ReadButton_clicked()
{
    QString strText = ui->BarcodesMemo->toPlainText();
    if(strText.isEmpty()) {
        QMessageBox::critical(this, "Error", "Barcode field is empty");
        return;
    }
    m_aBarcodes = strText.split('\n', Qt::SkipEmptyParts);
    m_nCurrentBarcodeIndex = 0; // Reset index for new barcodes
    QMessageBox::information(this, "EANScannerEmu", "Barcodes were successfully loaded");

}


void MainWindow::on_SendAllButton_clicked()
{
    qDebug() << "MainWindow::on_SendAllButton_clicked()";
    if(m_aBarcodes.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No barcodes to send");
        return;
    }
    
    // Initialize timer and start sending barcodes
    m_nCurrentBarcodeIndex = 0;
    m_pSendTimer = new QTimer(this);
    m_pSendTimer->setSingleShot(true);
    m_pSendTimer->setInterval(ui->TimeoutSlider->value() * 1000);
    
    connect(m_pSendTimer, &QTimer::timeout, this, &MainWindow::sendNextBarcode);
    
    // Start with first barcode
    QTimer::singleShot(ui->TimeoutSlider->value() * 1000, this, [this]() {
        if(SendBarcodeByIterator(m_nCurrentBarcodeIndex)) {
            m_nCurrentBarcodeIndex++;
            // Start timer for next barcode
            if(m_pSendTimer) {
                m_pSendTimer->start();
            }
        }
    });
}

bool MainWindow::SendBarcodeByIterator(int nIt)
{
    QString sCurBarcode = m_aBarcodes.at(nIt);
    if(sCurBarcode.isEmpty()) {
        qDebug() << "m_aBarcodes.at(" + QString::number(nIt) + ") is empty";
        return false;
    }
    
    // Send the barcode immediately

    if(!m_pStringSender->SendString(&sCurBarcode)) {
        qDebug() << "unable to send string " << sCurBarcode;
        return false;
    }
    if(ui->SendReturnCheckBox->isChecked()) {
        if(!m_pStringSender->SendReturn()) {
            qDebug() << "unable to send Return " << sCurBarcode;
            return false;
        }
    }
    
    qDebug() << "Sent barcode at index " << nIt << ": " << sCurBarcode;
    PlayScanSound();
    
    // Check if this is the last barcode
    if(nIt == m_aBarcodes.size() - 1) {
        // All barcodes sent
        qDebug() << "All barcodes sent successfully!";
        // Clean up timer
        if(m_pSendTimer) {
            m_pSendTimer->deleteLater();
            m_pSendTimer = nullptr;
        }
        return true;
    }
    
    // Schedule next barcode with timer
    if(m_pSendTimer) {
        int timeoutSeconds = ui->TimeoutSlider->value();
        qDebug() << "Scheduling next barcode in" << timeoutSeconds << "seconds";
        if(ui->MessagesCheckBox->isChecked()) {
            // Show progress message
            QMessageBox::information(this, "Progress",
                                     QString("Barcode %1 of %2 sent! You have %3 seconds to switch windows.\n"
                                             "Next barcode will be sent automatically.").arg(nIt + 1).arg(m_aBarcodes.size()).arg(timeoutSeconds));
        }
        // Start timer for next barcode (only if not the last one)
        if(nIt < m_aBarcodes.size() - 1) {
            m_pSendTimer->start();
        }
    }
    
    return true;
}

void MainWindow::ShowWaylandUnsupportedMessage()
{
    QMessageBox::critical(this, "You are using Wayland", "Wayland session is not supported at this moment. Switch to X.org session and try again");
    exit(0);
}

void MainWindow::PlayScanSound()
{
    QFile file(":/sounds/sound.wav");
    if (!file.open(QIODevice::ReadOnly))
        return;

    QTemporaryFile* tempFile = new QTemporaryFile(this);
    tempFile->setAutoRemove(false);
    if (tempFile->open()) {
        tempFile->write(file.readAll());
        tempFile->close();
    }
    QMediaPlayer* pPlayer = new QMediaPlayer(this);
    QAudioOutput* pAudioOutput = new QAudioOutput(this);

    pPlayer->setAudioOutput(pAudioOutput);
    pPlayer->setSource(QUrl::fromLocalFile(tempFile->fileName()));
    pAudioOutput->setVolume(1.0); //Qt6
    pPlayer->play();

    //Clean everything
    connect(pPlayer, &QMediaPlayer::mediaStatusChanged, this, [=](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            pPlayer->deleteLater();
            pAudioOutput->deleteLater();
            QFile::remove(tempFile->fileName()); // удаляем временный wav
            tempFile->deleteLater();
        }
    });

}

void MainWindow::sendNextBarcode()
{
    if(m_nCurrentBarcodeIndex < m_aBarcodes.size()) {
        qDebug() << "Sending barcode" << m_nCurrentBarcodeIndex + 1 << "of" << m_aBarcodes.size();
        if(SendBarcodeByIterator(m_nCurrentBarcodeIndex)) {
            m_nCurrentBarcodeIndex++;
            // Start timer for next barcode if there are more
            if(m_nCurrentBarcodeIndex < m_aBarcodes.size() && m_pSendTimer) {
                m_pSendTimer->start();
            }
        } else {
            qDebug() << "Failed to send barcode at index" << m_nCurrentBarcodeIndex;
            QMessageBox::warning(this, "Error", "Failed to send barcode at index " + QString::number(m_nCurrentBarcodeIndex));

            // Clean up timer on error
            if(m_pSendTimer) {
                m_pSendTimer->deleteLater();
                m_pSendTimer = nullptr;
            }
        }
    }
}


void MainWindow::on_SendNextButton_clicked()
{
    qDebug() << "MainWindow::on_SendNextButton_clicked()";
    
    if(m_aBarcodes.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No barcodes loaded");
        return;
    }
    
    // If no current index is set, start from beginning
    if(m_nCurrentBarcodeIndex >= m_aBarcodes.size()) {
        m_nCurrentBarcodeIndex = 0;
        qDebug() << "Resetting to first barcode";
    }
    // Send current barcode
    QTimer::singleShot(ui->TimeoutSlider->value() * 1000, this, [this]() {
        if(SendBarcodeByIterator(m_nCurrentBarcodeIndex)) {
            m_nCurrentBarcodeIndex++;

            // Show progress
            if(m_nCurrentBarcodeIndex < m_aBarcodes.size()) {
                if(ui->MessagesCheckBox->isChecked()) {
                    QMessageBox::information(this, "Progress",
                                             QString("Barcode %1 of %2 sent successfully!\n"
                                                     "Click 'Send Next' again to send the next barcode.").arg(m_nCurrentBarcodeIndex).arg(m_aBarcodes.size()));
                }
            } else {
                // QMessageBox::information(this, "Complete", "All barcodes have been sent!");
                m_nCurrentBarcodeIndex = 0; // Reset for next use
            }
        } else {
            QMessageBox::warning(this, "Error", "Failed to send barcode at index " + QString::number(m_nCurrentBarcodeIndex));
            // Don't increase index if sending failed
        }
    });
}


void MainWindow::on_SendPreviousButton_clicked()
{
    qDebug() << "MainWindow::on_SendPreviousButton_clicked()";
    
    if(m_aBarcodes.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No barcodes loaded");
        return;
    }
    
    // Decrease index to go to previous barcode
    m_nCurrentBarcodeIndex--;
    
    // If we went below 0, wrap around to the last barcode
    if(m_nCurrentBarcodeIndex < 0) {
        m_nCurrentBarcodeIndex = m_aBarcodes.size() - 1;
        qDebug() << "Wrapped around to last barcode";
    }
    // Send the previous barcode
    QTimer::singleShot(ui->TimeoutSlider->value() * 1000, this, [this]() {
        if(SendBarcodeByIterator(m_nCurrentBarcodeIndex)) {
            // Show progress
            if(ui->MessagesCheckBox->isChecked()) {
                QMessageBox::information(this, "Progress",
                                         QString("Previous barcode %1 of %2 sent successfully!\n"
                                                 "Current position: %3").arg(m_nCurrentBarcodeIndex + 1).arg(m_aBarcodes.size()).arg(m_nCurrentBarcodeIndex + 1));
            }
        } else {
            QMessageBox::warning(this, "Error", "Failed to send barcode at index " + QString::number(m_nCurrentBarcodeIndex));
            // Restore the index to where it was before
            m_nCurrentBarcodeIndex++;
        }
    });
}


void MainWindow::on_StopButton_clicked()
{
    qDebug() << "MainWindow::on_StopButton_clicked()";
    
    // Stop the timer if it's running
    if(m_pSendTimer && m_pSendTimer->isActive()) {
        qDebug() << "Stopping send timer...";
        m_pSendTimer->stop();
        m_pSendTimer->deleteLater();
        m_pSendTimer = nullptr;
        
        QMessageBox::information(this, "Stopped", "Sending process has been stopped.");
    } else {
        qDebug() << "No active timer to stop";
        QMessageBox::information(this, "Info", "No active sending process to stop.");
    }
    
    // Reset the current barcode index
    m_nCurrentBarcodeIndex = 0;
}

