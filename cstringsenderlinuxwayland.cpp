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
    
    qDebug() << "Attempting to send string:" << *pString;
    
    // Method 1: Try wtype (native Wayland tool)
    if(QProcess::execute("which wtype") == 0) {
        qDebug() << "Found wtype, trying to send text...";
        if(QProcess::execute("wtype \"" + *pString + "\"") == 0) {
            qDebug() << "Successfully sent string via wtype:" << *pString;
            return true;
        } else {
            qDebug() << "wtype failed, trying alternative methods...";
        }
    } else {
        qDebug() << "wtype not found, trying alternative methods...";
    }
    
    // Method 2: Try ydotool with proper cleanup
    qDebug() << "Trying ydotool...";
    
    // Kill any existing ydotoold processes
    QProcess::execute("pkill -f ydotoold");
    QThread::msleep(1000);
    
    // Start fresh ydotoold
    QProcess::startDetached("ydotoold");
    QThread::msleep(2000);
    
    if(QProcess::execute("ydotool type \"" + *pString + "\"") == 0) {
        qDebug() << "Successfully sent string via ydotool:" << *pString;
        return true;
    } else {
        qDebug() << "ydotool failed";
    }
    
    // Method 3: Fallback - copy to clipboard
    qDebug() << "All methods failed, copying to clipboard only";
    QClipboard *pClipboard = QApplication::clipboard();
    pClipboard->setText(*pString);
    qDebug() << "String copied to clipboard:" << *pString;
    
    return false;
}

bool CStringSenderLinuxWayland::SendReturn() const
{
    qDebug() << "Attempting to send Return key...";
    
    // Method 1: Try wtype (native Wayland tool)
    if(QProcess::execute("which wtype") == 0) {
        if(QProcess::execute("wtype -k Return") == 0) {
            qDebug() << "Successfully sent Return key via wtype";
            return true;
        }
    }
    
    // Method 2: Try ydotool
    if(QProcess::execute("which ydotool") == 0) {
        if(QProcess::execute("ydotool key 28") == 0) {
            qDebug() << "Successfully sent Return key via ydotool";
            return true;
        }
    }
    
    qDebug() << "Failed to send Return key via any method";
    return true;
}
