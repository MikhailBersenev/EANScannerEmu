#include "cstringsenderlinuxwayland.h"
#include <QApplication>
#include <QProcess>
#include <QClipboard>
#include <QDebug>
#include <QThread>

CStringSenderLinuxWayland::CStringSenderLinuxWayland(QObject *parent)
    : CStringSender{parent}
{}

bool CStringSenderLinuxWayland::SendString(QString* pString) const
{
    if(pString->isEmpty()) {
        qDebug() << "CStringSenderLinuxWayland::SendString - given string is empty";
        return false;
    }
    qDebug() << "Attempting to send text: \"" + *pString + "\"";
    
    // Try to send text normally first
    if(QProcess::execute("ydotool type \"" + *pString + "\"") == 0) {
        qDebug() << "Successfully sent string via ydotool:" << *pString;
        return true;
    } else {
        qDebug() << "Unable to send string via ydotool";
    }

    // Method 2: Fallback - copy to clipboard
    qDebug() << "All methods failed, copying to clipboard only";
    QClipboard *pClipboard = QApplication::clipboard();
    pClipboard->setText(*pString);
    qDebug() << "String copied to clipboard:" << *pString;

    return false;
}

bool CStringSenderLinuxWayland::SendReturn() const
{
    qDebug() << "Attempting to send Return key...";
    if(QProcess::execute("which ydotool") == 0) {
        if(QProcess::execute("ydotool key 28") == 0) {
            qDebug() << "Successfully sent Return key via ydotool";
            return true;
        }
    }
    qDebug() << "Failed to send Return key via any method";
    return false;
}
