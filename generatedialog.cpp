#include "generatedialog.h"
#include "ui_generatedialog.h"
#include <QMessageBox>

GenerateDialog::GenerateDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::GenerateDialog)
{
    ui->setupUi(this);
    m_nQnt = 0;
    m_nType = NOTHING;
    m_sPrefix = "";

    // Fill prefixes map
    m_prefixes["None - Random"] = "";
    m_prefixes["Russia (460-469)"] = "460";
    m_prefixes["USA (000-139)"] = "000";
    m_prefixes["Germany (400-440)"] = "400";
    m_prefixes["France (300-379)"] = "300";
    m_prefixes["UK (500-509)"] = "500";
    m_prefixes["Italy (800-839)"] = "800";
    m_prefixes["Spain (840-849)"] = "840";
    m_prefixes["China (690-699)"] = "690";
    m_prefixes["Japan (450-499)"] = "450";
    m_prefixes["Canada (754-755)"] = "754";
    m_prefixes["Australia (930-939)"] = "930";

    // Fill comboBox
    for (auto it = m_prefixes.begin(); it != m_prefixes.end(); ++it) {
        ui->PrefixComboBox->addItem(it.key(), it.value());
    }
}

int GenerateDialog::GetQuantity()
{
    return m_nQnt;
}

QString GenerateDialog::GetPrefix()
{
    return m_sPrefix;
}

GenerateDialog::Type GenerateDialog::GetType()
{
    return m_nType;
}

GenerateDialog::~GenerateDialog()
{
    delete ui;
}

void GenerateDialog::on_buttonBox_accepted()
{
    int nQnt = ui->QntEdit->text().toInt();
    GenerateDialog::Type nType = static_cast<GenerateDialog::Type>(ui->comboBox->currentIndex() + 1); //add 1 because Type = 0 is NOTHING
    QString sPrefix = ui->PrefixComboBox->currentData().toString();
    if(nQnt > 0 && nQnt <= 100)
    {
        m_nQnt = nQnt;
        m_nType = nType;
        m_sPrefix = sPrefix;
        close();
    }
    else {
        QMessageBox::warning(this, "Warning", "Check if all the fields filled correctly");
    }
}

