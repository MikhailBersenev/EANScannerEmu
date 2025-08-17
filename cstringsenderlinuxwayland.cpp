#include "cstringsenderlinuxwayland.h"
#include <QApplication>
#include <QProcess>
#include <QClipboard>

CStringSenderLinuxWayland::CStringSenderLinuxWayland(QObject *parent)
    : CStringSender{parent}
{}

bool CStringSenderLinuxWayland::SendString(QString &sString) const
{
    if(sString.isEmpty()) {
        qDebug() << "CStringSenderLinuxWayland::SendString - given string is empty";
        return false;
    }
    QClipboard *pClipboard = QApplication::clipboard();
    pClipboard->setText(sString);
    if(!QProcess::execute("ydotool type \"" + sString + "\"")) {
        qDebug() << "CStringSenderLinuxWayland::SendString - ydotool error";
        return false;
    }
    return true;
}

bool CStringSenderLinuxWayland::SendReturn() const
{
    if(!QProcess::execute("ydotool key 28")) {
        qDebug() << "CStringSenderLinuxWayland::SendReturn - ydotool error";
        return false;
    }
    return true;
}
