#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "generatedialog.h"
#include "cutils.h"
#include <QMessageBox>
#include <QDesktopServices>
#include <QTimer>
#include <QDebug>
#include <QThread>
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->TimeoutSlider->setSliderPosition(30);
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
    
    // Start with first barcode - send immediately and start timer for next
    if(SendBarcodeByIterator(m_nCurrentBarcodeIndex)) {
        m_nCurrentBarcodeIndex++;
        // Start timer for next barcode
        if(m_pSendTimer) {
            m_pSendTimer->start();
        }
    }
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
    if(!m_pStringSender->SendReturn()) {
        qDebug() << "unable to send Return " << sCurBarcode;
        return false;
    }
    
    qDebug() << "Sent barcode at index " << nIt << ": " << sCurBarcode;
    
    // Check if this is the last barcode
    if(nIt == m_aBarcodes.size() - 1) {
        // All barcodes sent
        qDebug() << "All barcodes sent successfully!";
        QMessageBox::information(this, "Send All", "All barcodes sent successfully!");
        
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
#if 0
        // Show progress message
        QMessageBox::information(this, "Progress", 
            QString("Barcode %1 of %2 sent! You have %3 seconds to switch windows.\n"
                   "Next barcode will be sent automatically.").arg(nIt + 1).arg(m_aBarcodes.size()).arg(timeoutSeconds));
#endif
        // Start timer for next barcode (only if not the last one)
        if(nIt < m_aBarcodes.size() - 1) {
            m_pSendTimer->start();
        }
    }
    
    return true;
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
    QThread::sleep(ui->TimeoutSlider->value());
    // Send current barcode
    if(SendBarcodeByIterator(m_nCurrentBarcodeIndex)) {
        m_nCurrentBarcodeIndex++;
        
        // Show progress
        if(m_nCurrentBarcodeIndex < m_aBarcodes.size()) {
            QMessageBox::information(this, "Progress", 
                QString("Barcode %1 of %2 sent successfully!\n"
                       "Click 'Send Next' again to send the next barcode.").arg(m_nCurrentBarcodeIndex).arg(m_aBarcodes.size()));
        } else {
            QMessageBox::information(this, "Complete", "All barcodes have been sent!");
            m_nCurrentBarcodeIndex = 0; // Reset for next use
        }
    } else {
        QMessageBox::warning(this, "Error", "Failed to send barcode at index " + QString::number(m_nCurrentBarcodeIndex));
        // Don't increase index if sending failed
    }
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
    QThread::sleep(ui->TimeoutSlider->value());
    // Send the previous barcode
    if(SendBarcodeByIterator(m_nCurrentBarcodeIndex)) {
        // Show progress
        QMessageBox::information(this, "Progress", 
            QString("Previous barcode %1 of %2 sent successfully!\n"
                   "Current position: %3").arg(m_nCurrentBarcodeIndex + 1).arg(m_aBarcodes.size()).arg(m_nCurrentBarcodeIndex + 1));
    } else {
        QMessageBox::warning(this, "Error", "Failed to send barcode at index " + QString::number(m_nCurrentBarcodeIndex));
        // Restore the index to where it was before
        m_nCurrentBarcodeIndex++;
    }
}

