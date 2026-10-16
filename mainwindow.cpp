#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "generatedialog.h"
#include "aboutdialog.h"
#include "cutils.h"
#include "cappsettings.h"
#include "sendhistorymodel.h"

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
#include <QtGlobal>
#include <QDialog>
#include <QHeaderView>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>
#include <QClipboard>
#include <QApplication>
#include <QStatusBar>
#include <utility>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    m_sendHistory = new SendHistoryModel(
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/send-history.json", this);
    ReportHistoryError(m_sendHistory->load());
    {
        CAppSettings settings(this);
        const int saved = settings.timeoutSeconds();
        ui->TimeoutSlider->setValue(qBound(ui->TimeoutSlider->minimum(), saved,
                                           ui->TimeoutSlider->maximum()));
        ui->MessagesCheckBox->setChecked(settings.showInfoMessages());
        ui->SendReturnCheckBox->setChecked(settings.sendReturnAfterBarcode());
    }
    this->setWindowTitle(this->windowTitle() + " ver. " + CUtils::GetVersion());
    m_InfoMessageHandler = QSharedPointer<CInfoMessageHandler>::create(nullptr, ui->MessagesCheckBox->isChecked());
#if defined(Q_OS_WIN)
    setWindowIcon(QIcon(":/icon.ico"));
#else
    setWindowIcon(QIcon(":/icon.png"));
#endif
    m_InfoMessageHandler->InfoMessage(QString("EANScannerEmu ver. %1").arg(CUtils::GetVersion()));
    m_nCurrentBarcodeIndex = 0;
    
    // Загрузка штрихкодов из файла при запуске
    LoadBarcodesFromFile();
    
#ifdef Q_OS_LINUX
    QString sDisplayServer = QGuiApplication::platformName();
    if (sDisplayServer.contains("wayland", Qt::CaseInsensitive)) {
        m_InfoMessageHandler->InfoMessage("Display Server Wayland");
        m_pStringSender.reset(new CStringSenderLinuxWayland(this));
        ShowWaylandWarningMessage();
    } else if (sDisplayServer.contains("xcb", Qt::CaseInsensitive)) {
        m_InfoMessageHandler->InfoMessage("Display Server X11");
        m_pStringSender.reset(new CStringSenderLinuxX11(this));
        CAppSettings appSettings(this);
        appSettings.removeWaylandMessageIfPresent();
    }
    else {
        QMessageBox::critical(this, "Error", "Unknown display server");
        exit(0);
        ShowWaylandWarningMessage();
    }
#endif
#ifdef Q_OS_WIN
    m_pStringSender.reset(new CStringSenderWin32(this));
#endif
#ifdef Q_OS_MACOS
    m_pStringSender.reset(new CStringSenderMac(this));
#endif

    if (m_pStringSender) {
        m_pStringSender->SetInfoMessageHandler(m_InfoMessageHandler);
    }

}

MainWindow::~MainWindow()
{
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
    QScopedPointer<GenerateDialog> pGenerateDialog(new GenerateDialog(this));
    pGenerateDialog->exec();
    const int nQnt = pGenerateDialog->GetQuantity();
    GenerateDialog::Type nType = pGenerateDialog->GetType();
    QString sPrefix = pGenerateDialog->GetPrefix();
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
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::ReportHistoryError(const QString &error)
{
    m_historyError = error.isEmpty() ? QString() : tr("History: %1").arg(error);
    if (m_historyErrorLabel) {
        m_historyErrorLabel->setText(m_historyError);
        m_historyErrorLabel->setVisible(!m_historyError.isEmpty());
    }
    if (!error.isEmpty()) {
        qWarning().noquote() << m_historyError;
        statusBar()->showMessage(m_historyError, 10000);
    }
}

void MainWindow::on_HistoryButton_clicked()
{
    if (m_historyDialog) {
        m_historyDialog->show();
        m_historyDialog->raise();
        m_historyDialog->activateWindow();
        return;
    }
    auto *dialog = new QDialog(this);
    m_historyDialog = dialog;
    dialog->setObjectName("HistoryDialog");
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setWindowTitle(tr("Sending History"));
    dialog->resize(960, 480);
    auto *layout = new QVBoxLayout(dialog);
    auto *description = new QLabel(tr("Latest 1,000 send attempts, newest first. Results report keyboard emulation; target receipt is not verified."), dialog);
    description->setWordWrap(true);
    layout->addWidget(description);
    auto *count = new QLabel(dialog);
    const auto updateCount = [this, count] {
        count->setText(m_sendHistory->rowCount() == 0 ? tr("No send attempts yet.")
                         : tr("%1 send attempts").arg(m_sendHistory->rowCount()));
    };
    updateCount();
    connect(m_sendHistory, &QAbstractItemModel::rowsInserted, dialog, updateCount);
    connect(m_sendHistory, &QAbstractItemModel::rowsRemoved, dialog, updateCount);
    connect(m_sendHistory, &QAbstractItemModel::modelReset, dialog, updateCount);
    layout->addWidget(count);
    auto *errorLabel = new QLabel(m_historyError, dialog);
    m_historyErrorLabel = errorLabel;
    errorLabel->setWordWrap(true);
    errorLabel->setVisible(!m_historyError.isEmpty());
    layout->addWidget(errorLabel);
    auto *table = new QTableView(dialog);
    table->setObjectName("HistoryTable");
    table->setModel(m_sendHistory);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setAlternatingRowColors(true);
    table->setWordWrap(false);
    table->verticalHeader()->hide();
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    table->setColumnWidth(0, 205);
    table->setColumnWidth(1, 225);
    table->setColumnWidth(2, 130);
    table->setColumnWidth(3, 65);
    table->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(table);
    auto *buttons = new QHBoxLayout;
    auto *resend = new QPushButton(tr("Resend"), dialog);
    resend->setObjectName("HistoryResendButton");
    resend->setAutoDefault(false);
    resend->setDefault(false);
    resend->setToolTip(tr("Resend the selected barcode after the current timeout, using the current Send Return setting. Switch focus to the target application."));
    resend->setEnabled(false);
    auto *copy = new QPushButton(tr("Copy Barcode"), dialog);
    copy->setAutoDefault(false);
    copy->setEnabled(false);
    const auto updateSelectionActions = [table, copy, resend] {
        const bool selected = table->selectionModel()->hasSelection();
        copy->setEnabled(selected);
        resend->setEnabled(selected);
    };
    connect(table->selectionModel(), &QItemSelectionModel::selectionChanged, dialog, updateSelectionActions);
    connect(m_sendHistory, &QAbstractItemModel::modelReset, dialog, updateSelectionActions);
    connect(resend, &QPushButton::clicked, dialog, [this, table] {
        const auto rows = table->selectionModel()->selectedRows(1);
        if (rows.isEmpty())
            return;
        // Capture the value now: new history entries or clearing history can move/remove the row.
        const QString barcode = rows.first().data().toString();
        if (barcode.isEmpty())
            return;
        auto *timer = new QTimer(this);
        timer->setSingleShot(true);
        m_pendingResends.append(timer);
        connect(timer, &QTimer::timeout, this, [this, barcode, timer] {
            m_pendingResends.removeOne(timer);
            timer->deleteLater();
            if (!SendBarcodeString(barcode, tr("Resend")))
                QMessageBox::warning(this, tr("Error"), tr("Failed to resend the barcode."));
        });
        timer->start(ui->TimeoutSlider->value() * 1000);
    });
    buttons->addWidget(resend);
    connect(copy, &QPushButton::clicked, dialog, [table] {
        const auto rows = table->selectionModel()->selectedRows(1);
        if (!rows.isEmpty())
            QApplication::clipboard()->setText(rows.first().data().toString());
    });
    buttons->addWidget(copy);
    auto *clear = new QPushButton(tr("Clear History"), dialog);
    clear->setAutoDefault(false);
    connect(clear, &QPushButton::clicked, dialog, [this, dialog] {
        if (QMessageBox::question(dialog, tr("Clear History"),
                tr("Delete all sending history?"), QMessageBox::Yes | QMessageBox::No,
                QMessageBox::No) == QMessageBox::Yes)
            ReportHistoryError(m_sendHistory->clear());
    });
    buttons->addWidget(clear);
    buttons->addStretch();
    auto *close = new QPushButton(tr("Close"), dialog);
    close->setAutoDefault(false);
    connect(close, &QPushButton::clicked, dialog, &QDialog::close);
    buttons->addWidget(close);
    layout->addLayout(buttons);
    dialog->show();
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
    m_pSendTimer.reset(new QTimer(this));
    m_pSendTimer->setSingleShot(true);
    m_pSendTimer->setInterval(ui->TimeoutSlider->value() * 1000);
    
    connect(m_pSendTimer.data(), &QTimer::timeout, this, &MainWindow::sendNextBarcode);
    
    // Start with first barcode
    QTimer::singleShot(ui->TimeoutSlider->value() * 1000, this, [this]() {
        if(SendBarcodeByIterator(m_nCurrentBarcodeIndex, tr("Send All"))) {
            m_nCurrentBarcodeIndex++;
            // Start timer for next barcode
            if(m_pSendTimer) {
                m_pSendTimer->start();
            }
        }
    });
}

bool MainWindow::SendBarcodeString(QString sBarcode, const QString &action)
{
    if (sBarcode.isEmpty()) {
        return false;
    }
    const QDateTime timestamp = QDateTime::currentDateTimeUtc();
    const bool sendReturn = ui->SendReturnCheckBox->isChecked();
    const auto record = [this, &sBarcode, &action, timestamp, sendReturn](const QString &result) {
        ReportHistoryError(m_sendHistory->append({timestamp, sBarcode, action, sendReturn, result}));
    };
    if (!m_pStringSender) {
        record(tr("Failed: sender unavailable"));
        m_InfoMessageHandler->InfoMessage("String sender is not initialized");
        return false;
    }
    if (!m_pStringSender->SendString(&sBarcode)) {
        record(tr("Failed: text send failed (may be partial)"));
        m_InfoMessageHandler->InfoMessage(QString("unable to send string %1").arg(sBarcode));
        return false;
    }
    if (sendReturn) {
        if (!m_pStringSender->SendReturn()) {
            record(tr("Text sent; Enter failed"));
            m_InfoMessageHandler->InfoMessage(QString("unable to send Return %1").arg(sBarcode));
            return false;
        }
    }
    record(tr("Sent"));
    PlayScanSound();
    return true;
}

bool MainWindow::SendBarcodeByIterator(int nIt, const QString &action)
{
    if (nIt < 0 || nIt >= m_aBarcodes.size())
        return false;
    QString sCurBarcode = m_aBarcodes.at(nIt);
    if(sCurBarcode.isEmpty()) {
        m_InfoMessageHandler->InfoMessage("m_aBarcodes.at(" + QString::number(nIt) + ") is empty");
        return false;
    }

    if (!SendBarcodeString(sCurBarcode, action)) {
        return false;
    }

    m_InfoMessageHandler->InfoMessage(QString("Sent barcode at index %1: %2").arg(nIt).arg(sCurBarcode));
    
    // Check if this is the last barcode
    if(nIt == m_aBarcodes.size() - 1) {
        // All barcodes sent
        m_InfoMessageHandler->InfoMessage("All barcodes sent successfully!");
        // Clean up timer
        if(m_pSendTimer) {
            m_pSendTimer.reset();
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
    CAppSettings settings(this);
    if (!settings.hasAcknowledgedWaylandMessage()) {
        QMessageBox::warning(this, "Warning",
            "Wayland mode detected.\n\n"
            "Keyboard emulation works through ydotool. Please make sure:\n"
            "1) ydotool is installed and available in PATH\n"
            "2) ydotoold daemon is running\n"
            "3) your user has permission to access /dev/uinput\n\n"
            "If input still does not work in your compositor, try an X11 session as fallback.");
        settings.acknowledgeWaylandMessage();
    }
}

void MainWindow::PlayScanSound()
{
    QFile file(":/sounds/sound.wav");
    if (!file.open(QIODevice::ReadOnly))
        return;

    QScopedPointer<QTemporaryFile> tempFile(new QTemporaryFile(this));
    tempFile->setAutoRemove(false);
    if (tempFile->open()) {
        tempFile->write(file.readAll());
        tempFile->close();
    }

    QScopedPointer<QSoundEffect> sound(new QSoundEffect(this));
    sound->setSource(QUrl::fromLocalFile(tempFile->fileName()));
    sound->setVolume(1.0);
    sound->play();

    // Передаем владение Qt parent'у, а удаление по завершению делаем через сигнал.
    connect(sound.data(), &QSoundEffect::playingChanged, this, [tempFile = tempFile.data(), sound = sound.data()]() {
        if (!sound->isPlaying()) {
            QFile::remove(tempFile->fileName());
            tempFile->deleteLater();
            sound->deleteLater();
        }
    });
    tempFile.take();
    sound.take();
}

void MainWindow::sendNextBarcode()
{
    if(m_nCurrentBarcodeIndex < m_aBarcodes.size()) {
        m_InfoMessageHandler->InfoMessage(QString("Sending barcode %1 of %2").arg(m_nCurrentBarcodeIndex + 1).arg(m_aBarcodes.size()));
        if(SendBarcodeByIterator(m_nCurrentBarcodeIndex, tr("Send All"))) {
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
                m_pSendTimer.reset();
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
        if (!SendBarcodeString(sSel, tr("Send Selection"))) {
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
        if(SendBarcodeByIterator(m_nCurrentBarcodeIndex, tr("Send Next"))) {
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
        if(SendBarcodeByIterator(m_nCurrentBarcodeIndex, tr("Send Previous"))) {
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
    // Cancel all pending work before opening any messages with nested event loops.
    const bool hadPendingResends = !m_pendingResends.isEmpty();
    for (QTimer *timer : std::as_const(m_pendingResends)) {
        timer->stop();
        timer->deleteLater();
    }
    m_pendingResends.clear();
    const bool hadActiveBatch = m_pSendTimer && m_pSendTimer->isActive();
    if (hadActiveBatch) {
        m_pSendTimer->stop();
        m_pSendTimer.reset();
    }
    m_InfoMessageHandler->InfoMessage("MainWindow::on_StopButton_clicked()");
    if (hadPendingResends || hadActiveBatch) {
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

void MainWindow::on_MessagesCheckBox_stateChanged(int state)
{
    const bool bShouldShowMessages = (state == Qt::Checked);
    CAppSettings settings(this);
    settings.setShowInfoMessages(bShouldShowMessages);

    if (m_InfoMessageHandler.isNull()) {
        return;
    }

    if (bShouldShowMessages != m_InfoMessageHandler->GetMode()) {
        m_InfoMessageHandler->SetMode(bShouldShowMessages);
    }
}

void MainWindow::on_SendReturnCheckBox_stateChanged(int state)
{
    CAppSettings settings(this);
    settings.setSendReturnAfterBarcode(state == Qt::Checked);
}

void MainWindow::on_TimeoutSlider_valueChanged(int value)
{
    CAppSettings settings(this);
    settings.setTimeoutSeconds(value);
}

