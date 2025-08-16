#include "generatedialog.h"
#include "ui_generatedialog.h"
#include <QMessageBox>

GenerateDialog::GenerateDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GenerateDialog)
{
    ui->setupUi(this);
    nQnt = 0;
    nType = NOTHING;
}

int GenerateDialog::GetQuantity()
{
    return nQnt;
}

GenerateDialog::Type GenerateDialog::GetType()
{
    return nType;
}

GenerateDialog::~GenerateDialog()
{
    delete ui;
}

void GenerateDialog::on_buttonBox_accepted()
{
    int nQnt = ui->QntEdit->text().toInt();
    GenerateDialog::Type nType = static_cast<GenerateDialog::Type>(ui->comboBox->currentIndex() + 1); //add 1 because Type = 0 is NOTHING
    if(nQnt > 0)
    {
        this->nQnt = nQnt;
        this->nType = nType;
        close();
    }
    else {
        QMessageBox::warning(this, "Warning", "Check if all the fields filled correctly");
    }
}

