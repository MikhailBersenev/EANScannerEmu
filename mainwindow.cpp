#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "generatedialog.h"
#include "cutils.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_CleanButton_clicked()
{
    ui->BarcodesMemo->clear();

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

