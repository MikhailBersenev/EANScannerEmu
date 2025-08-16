#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "generatedialog.h"
#include "cutils.h"
#include <QMessageBox>
#include <QDesktopServices>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/icon.ico"));
    qDebug() << "EANScannerEmu ver." << CUtils::GetVersion();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_CleanButton_clicked()
{
    ui->BarcodesMemo->clear();
    aBarcodes.clear();

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
    aBarcodes = strText.split('\n', Qt::SkipEmptyParts);
    QMessageBox::information(this, "EANScannerEmu", "Barcodes were successfuly loaded");

}

