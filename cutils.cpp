#include "cutils.h"
#include <QRandomGenerator>
CUtils::CUtils(QObject *parent)
    : QObject{parent}
{}

QStringList CUtils::GenerateEAN8(const int &rQnt)
{
    QStringList AResult;
    for(int i = 0; i < rQnt; i++) {
        QString strCode;
        for (int j = 0; j < 7; ++j)
            strCode.append(QString::number(QRandomGenerator::global()->bounded(10)));
        int nCheck = EanChecksum(strCode);
         AResult.append(strCode + QString::number(nCheck));
    }
    return AResult;
}

QStringList CUtils::GenerateEAN13(const int &rQnt)
{
    QStringList AResult;
    for(int i = 0; i < rQnt; i++) {
        QString strCode;
        for (int j = 0; j < 12; ++j)
            strCode.append(QString::number(QRandomGenerator::global()->bounded(10)));
        int nCheck = EanChecksum(strCode);
        AResult.append(strCode + QString::number(nCheck));
    }
    return AResult;
}

QString CUtils::GetVersion()
{
    const QString sVersion = "1.0";
    return sVersion;
}

int CUtils::EanChecksum(const QString &rStr)
{
    int nLen = rStr.length();
    if (nLen != 7 && nLen != 12) return -1; // EAN-8: 7 цифр, EAN-13: 12 цифр

    int nSum = 0;
    for (int iIndex = 0; iIndex < nLen; ++iIndex) {
        int nDigit = rStr[iIndex].digitValue();
        if (nLen == 12) // EAN-13
            nSum += (iIndex % 2 == 0 ? nDigit : 3*nDigit);
        else // EAN-8
            nSum += (iIndex % 2 == 0 ? 3*nDigit : nDigit);
    }
    return (10 - (nSum % 10)) % 10;
}
