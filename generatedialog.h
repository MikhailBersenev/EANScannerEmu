#ifndef GENERATEDIALOG_H
#define GENERATEDIALOG_H

#include <QDialog>
#include <QMap>

namespace Ui {
class GenerateDialog;
}

class GenerateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GenerateDialog(QWidget *parent = nullptr);
    int GetQuantity();
    QString GetPrefix();
    enum Type {
        NOTHING,
        EAN8,
        EAN13,
    };
    Type GetType();

    ~GenerateDialog();

private slots:
    void on_buttonBox_accepted();

private:
    Ui::GenerateDialog *ui;
    int m_nQnt;
    QString m_sPrefix;
    QMap<QString, QString> m_prefixes;

    Type m_nType; // 0 - Nothing 1 - EAN8 2 - EAN13
};

#endif // GENERATEDIALOG_H
