#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "generatedialog.h"
#include "cutils.h"

#include "cstringsenderlinuxwayland.h"
#include "cstringsenderlinuxx11.h"
#include "cstringsenderwin32.h"
#include "cstringsendermac.h"

#include <QMessageBox>
#include <QDesktopServices>
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <QFile>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTemporaryFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QSettings>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->TimeoutSlider->setSliderPosition(10);
    this->setWindowTitle(this->windowTitle() + " ver. " + CUtils::GetVersion());
    ui->SendReturnCheckBox->setChecked(true);
    m_InfoMessageHandler = QSharedPointer<CInfoMessageHandler>::create(nullptr, ui->MessagesCheckBox->isChecked());
    // На macOS PNG отображается корректнее, чем .ico (док, заголовок окна)
#if defined(Q_OS_MACOS)
    setWindowIcon(QIcon(":/icon.png"));
#else
    setWindowIcon(QIcon(":/icon.ico"));
#endif
    m_InfoMessageHandler->InfoMessage(QString("EANScannerEmu ver. %1").arg(CUtils::GetVersion()));
    m_pStringSender = nullptr;
    m_pSendTimer = nullptr;
    m_nCurrentBarcodeIndex = 0;
    
    // Загрузка штрихкодов из файла при запуске
    LoadBarcodesFromFile();
    
#ifdef Q_OS_LINUX
    QString sDisplayServer = QGuiApplication::platformName();
    if (sDisplayServer.contains("wayland", Qt::CaseInsensitive)) {
        m_InfoMessageHandler->InfoMessage("Display Server Wayland");
        m_pStringSender = new CStringSenderLinuxWayland(this);
        ShowWaylandWarningMessage();
    } else if (sDisplayServer.contains("xcb", Qt::CaseInsensitive)) {
        m_InfoMessageHandler->InfoMessage("Display Server X11");
        m_pStringSender = new CStringSenderLinuxX11(this);
        QSettings settings("Mikhail Bersenev", "EANScammerEmu", this);
        if(settings.contains("waylandMessage")) {
            settings.remove("waylandMessage");
        }
    }
    else {
        QMessageBox::critical(this, "Error", "Unknown display server");
        exit(0);
        ShowWaylandWarningMessage();
    }
#endif
#ifdef Q_OS_WIN
    m_pStringSender = new CStringSenderWin32(this);
#endif
#ifdef Q_OS_MACOS
    m_pStringSender = new CStringSenderMac(this);
#endif

    if (m_pStringSender) {
        m_pStringSender->SetInfoMessageHandler(m_InfoMessageHandler);
    }

}

MainWindow::~MainWindow()
{
    if(m_pStringSender)
        delete m_pStringSender;
    delete ui;
}

void MainWindow::LoadBarcodesFromFile()
{
    const QString sBarcodesFileName = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/barcodes.txt";
    QFile file(sBarcodesFileName);
    
    if (!file.exists()) {
        m_InfoMessageHandler->InfoMessage(QString("Barcodes file not found: %1").arg(sBarcodesFileName));
        return;
    }
    
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString strText = in.readAll();
        file.close();
        
        if (!strText.isEmpty()) {
            // Загружаем в поле ввода
            ui->BarcodesMemo->setPlainText(strText);
            
            // Загружаем в массив
            m_aBarcodes = strText.split('\n', Qt::SkipEmptyParts);
            m_nCurrentBarcodeIndex = 0;
            
            m_InfoMessageHandler->InfoMessage(QString("Loaded %1 barcodes from %2").arg(m_aBarcodes.size()).arg(sBarcodesFileName));
        }
    } else {
        m_InfoMessageHandler->InfoMessage(QString("Failed to open barcodes file: %1").arg(file.errorString()));
    }
}

void MainWindow::on_CleanButton_clicked()
{
    ui->BarcodesMemo->clear();
    m_aBarcodes.clear();
    m_nCurrentBarcodeIndex = 0; // Reset index when clearing
    
    // Удаление txt файла со штрихкодами
    const QString sBarcodesFileName = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/barcodes.txt";
    if (QFile::exists(sBarcodesFileName)) {
        if (QFile::remove(sBarcodesFileName)) {
            m_InfoMessageHandler->InfoMessage(QString("Barcodes file deleted: %1").arg(sBarcodesFileName));
        } else {
            m_InfoMessageHandler->InfoMessage(QString("Failed to delete barcodes file: %1").arg(sBarcodesFileName));
        }
    }

}


void MainWindow::on_GenerateButton_clicked()
{
    GenerateDialog* pGenerateDialog = new GenerateDialog(this);
    pGenerateDialog->exec();
    const int nQnt = pGenerateDialog->GetQuantity();
    GenerateDialog::Type nType = pGenerateDialog->GetType();
    QString sPrefix = pGenerateDialog->GetPrefix();
    delete pGenerateDialog;
    QStringList aGeneratedItems;
    switch (nType) {
    case GenerateDialog::Type::EAN8:
        if (sPrefix.isEmpty()) {
            aGeneratedItems = CUtils::GenerateEAN8(nQnt);
        } else {
            aGeneratedItems = CUtils::GenerateEAN8(nQnt, sPrefix);
        }
        break;
    case GenerateDialog::Type::EAN13:
        if (sPrefix.isEmpty()) {
            aGeneratedItems = CUtils::GenerateEAN13(nQnt);
        } else {
            aGeneratedItems = CUtils::GenerateEAN13(nQnt, sPrefix);
        }
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
    
    // Сохранение штрихкодов в txt файл
    const QString sBarcodesFileName = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/barcodes.txt";
    QFile file(sBarcodesFileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (const QString &barcode : m_aBarcodes) {
            out << barcode << "\n";
        }
        file.close();
        m_InfoMessageHandler->InfoMessage(QString("Barcodes saved to %1").arg(sBarcodesFileName));
    } else {
        m_InfoMessageHandler->InfoMessage(QString("Failed to save barcodes to file: %1").arg(file.errorString()));
        QMessageBox::warning(this, "Warning", "Failed to save barcodes to file: " + file.errorString());
    }
    
    QMessageBox::information(this, "EANScannerEmu", "Barcodes were successfully loaded");

}


void MainWindow::on_SendAllButton_clicked()
{
    m_InfoMessageHandler->InfoMessage("MainWindow::on_SendAllButton_clicked()");
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

bool MainWindow::SendBarcodeString(QString sBarcode)
{
    if (sBarcode.isEmpty()) {
        return false;
    }
    if (!m_pStringSender->SendString(&sBarcode)) {
        m_InfoMessageHandler->InfoMessage(QString("unable to send string %1").arg(sBarcode));
        return false;
    }
    if (ui->SendReturnCheckBox->isChecked()) {
        if (!m_pStringSender->SendReturn()) {
            m_InfoMessageHandler->InfoMessage(QString("unable to send Return %1").arg(sBarcode));
            return false;
        }
    }
    PlayScanSound();
    return true;
}

bool MainWindow::SendBarcodeByIterator(int nIt)
{
    QString sCurBarcode = m_aBarcodes.at(nIt);
    if(sCurBarcode.isEmpty()) {
        m_InfoMessageHandler->InfoMessage("m_aBarcodes.at(" + QString::number(nIt) + ") is empty");
        return false;
    }

    if (!SendBarcodeString(sCurBarcode)) {
        return false;
    }

    m_InfoMessageHandler->InfoMessage(QString("Sent barcode at index %1: %2").arg(nIt).arg(sCurBarcode));
    
    // Check if this is the last barcode
    if(nIt == m_aBarcodes.size() - 1) {
        // All barcodes sent
        m_InfoMessageHandler->InfoMessage("All barcodes sent successfully!");
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
        m_InfoMessageHandler->InfoMessage(QString("Scheduling next barcode in %1 seconds").arg(timeoutSeconds));
        if (m_InfoMessageHandler->GetMode()) {
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

void MainWindow::ShowWaylandWarningMessage()
{
    QSettings settings("Mikhail Bersenev", "EANScammerEmu", this);
    if(!settings.contains("waylandMessage")) {
        QMessageBox::warning(this, "Warning",
            "Wayland mode detected.\n\n"
            "Keyboard emulation works through ydotool. Please make sure:\n"
            "1) ydotool is installed and доступен in PATH\n"
            "2) ydotoold daemon is running\n"
            "3) your user has permission to access /dev/uinput\n\n"
            "If input still does not work in your compositor, try an X11 session as fallback.");
        settings.setValue("waylandMessage", "1");
    }
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

    QSoundEffect* pSound = new QSoundEffect(this);
    pSound->setSource(QUrl::fromLocalFile(tempFile->fileName()));
    pSound->setVolume(1.0);
    pSound->play();

    //Clean everything
    connect(pSound, &QSoundEffect::playingChanged, this, [=]() {
        if (!pSound->isPlaying()) {
            QFile::remove(tempFile->fileName());
            tempFile->deleteLater();
            pSound->deleteLater();
        }
    });

}

void MainWindow::sendNextBarcode()
{
    if(m_nCurrentBarcodeIndex < m_aBarcodes.size()) {
        m_InfoMessageHandler->InfoMessage(QString("Sending barcode %1 of %2").arg(m_nCurrentBarcodeIndex + 1).arg(m_aBarcodes.size()));
        if(SendBarcodeByIterator(m_nCurrentBarcodeIndex)) {
            m_nCurrentBarcodeIndex++;
            // Start timer for next barcode if there are more
            if(m_nCurrentBarcodeIndex < m_aBarcodes.size() && m_pSendTimer) {
                m_pSendTimer->start();
            }
        } else {
            m_InfoMessageHandler->InfoMessage(QString("Failed to send barcode at index %1").arg(m_nCurrentBarcodeIndex));
            QMessageBox::warning(this, "Error", "Failed to send barcode at index " + QString::number(m_nCurrentBarcodeIndex));

            // Clean up timer on error
            if(m_pSendTimer) {
                m_pSendTimer->deleteLater();
                m_pSendTimer = nullptr;
            }
        }
    }
}


void MainWindow::on_SendSelectionButton_clicked()
{
    QString sSel = ui->BarcodesMemo->textCursor().selectedText();
    sSel.replace(QChar(0x2029), QLatin1Char('\n'));
    sSel = sSel.trimmed();
    if (sSel.contains(QLatin1Char('\n'))) {
        const QStringList parts = sSel.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
        sSel = parts.isEmpty() ? QString() : parts.first().trimmed();
    }
    if (sSel.isEmpty()) {
        QMessageBox::warning(this, tr("Warning"), tr("Select a barcode in the list (highlight text)."));
        return;
    }

    QTimer::singleShot(ui->TimeoutSlider->value() * 1000, this, [this, sSel]() {
        if (!SendBarcodeString(sSel)) {
            QMessageBox::warning(this, tr("Error"), tr("Failed to send the selection."));
        }
    });
}

void MainWindow::on_SendNextButton_clicked()
{
    m_InfoMessageHandler->InfoMessage("MainWindow::on_SendNextButton_clicked()");
    
    if(m_aBarcodes.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No barcodes loaded");
        return;
    }
    
    // If no current index is set, start from beginning
    if(m_nCurrentBarcodeIndex >= m_aBarcodes.size()) {
        m_nCurrentBarcodeIndex = 0;
        m_InfoMessageHandler->InfoMessage("Resetting to first barcode");
    }
    // Send current barcode
    QTimer::singleShot(ui->TimeoutSlider->value() * 1000, this, [this]() {
        if(SendBarcodeByIterator(m_nCurrentBarcodeIndex)) {
            m_nCurrentBarcodeIndex++;

            // Show progress
            if(m_nCurrentBarcodeIndex < m_aBarcodes.size()) {
                if (m_InfoMessageHandler->GetMode()) {
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
    m_InfoMessageHandler->InfoMessage("MainWindow::on_SendPreviousButton_clicked()");
    
    if(m_aBarcodes.isEmpty()) {
        QMessageBox::warning(this, "Warning", "No barcodes loaded");
        return;
    }
    
    // Decrease index to go to previous barcode
    m_nCurrentBarcodeIndex--;
    
    // If we went below 0, wrap around to the last barcode
    if(m_nCurrentBarcodeIndex < 0) {
        m_nCurrentBarcodeIndex = m_aBarcodes.size() - 1;
        m_InfoMessageHandler->InfoMessage("Wrapped around to last barcode");
    }
    // Send the previous barcode
    QTimer::singleShot(ui->TimeoutSlider->value() * 1000, this, [this]() {
        if(SendBarcodeByIterator(m_nCurrentBarcodeIndex)) {
            // Show progress
            if (m_InfoMessageHandler->GetMode()) {
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
    m_InfoMessageHandler->InfoMessage("MainWindow::on_StopButton_clicked()");
    
    // Stop the timer if it's running
    if(m_pSendTimer && m_pSendTimer->isActive()) {
        m_InfoMessageHandler->InfoMessage("Stopping send timer...");
        m_pSendTimer->stop();
        m_pSendTimer->deleteLater();
        m_pSendTimer = nullptr;
        
        QMessageBox::information(this, "Stopped", "Sending process has been stopped.");
    } else {
        m_InfoMessageHandler->InfoMessage("No active timer to stop");
        QMessageBox::information(this, "Info", "No active sending process to stop.");
    }
    
    // Reset the current barcode index
    m_nCurrentBarcodeIndex = 0;
}


void MainWindow::on_WebsiteButton_clicked()
{
    QDesktopServices::openUrl(QUrl::fromUserInput("http://mbersenev.ph/"));
}

void MainWindow::on_MessagesCheckBox_checkStateChanged(const Qt::CheckState &arg1)
{
    if (m_InfoMessageHandler.isNull()) {
        return;
    }

    const bool bShouldShowMessages = (arg1 == Qt::Checked);
    if (bShouldShowMessages != m_InfoMessageHandler->GetMode()) {
        m_InfoMessageHandler->SetMode(bShouldShowMessages);
    }
}

