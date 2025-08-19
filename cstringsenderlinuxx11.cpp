#include "cstringsenderlinuxx11.h"
#ifdef Q_OS_LINUX
#include <QDebug>
#include <QThread>
CStringSenderLinuxX11::CStringSenderLinuxX11(QObject *parent)
    : CStringSender{parent}
{
    m_pDisplay = XOpenDisplay(nullptr);
}

bool CStringSenderLinuxX11::SendString(QString *pString)
{
    if(!pString) {
        qDebug() << "CStringSenderLinuxX11::SendString(QString *pString) pString is null";
        return false;
    }
    for(int i = 0; i < pString->length(); i++) {
        if (!SendUnicodeChar(pString->at(i).unicode(), true)) return false;
        if (!SendUnicodeChar(pString->at(i).unicode(), false)) return false;
        QThread::msleep(3);
    }
    XFlush(m_pDisplay);
    return true;
}

bool CStringSenderLinuxX11::SendReturn()
{
    if (!SendUnicodeChar(XK_Return, true)) return false;
    if (!SendUnicodeChar(XK_Return, false)) return false;
    XFlush(m_pDisplay);
    return true;
}

CStringSenderLinuxX11::~CStringSenderLinuxX11()
{
    if (m_pDisplay) {
        // Release all the keys before closing
        for (int i = 0; i < 256; i++) {
            XTestFakeKeyEvent(m_pDisplay, i, false, CurrentTime);
        }
        FlushAndCloseDisplay();
    }
    m_pDisplay = nullptr;
}

bool CStringSenderLinuxX11::SendUnicodeChar(qint32 uCode, bool bRelease)
{
    KeySym keySym = uCode;
    KeyCode keyCode = XKeysymToKeycode(m_pDisplay, keySym);
    if (!keyCode) {
        return false;
    }
    if(!XTestFakeKeyEvent(m_pDisplay, keyCode, bRelease, CurrentTime)) {
        qDebug() << "Unable to send XTestFakeKeyEvent";
        return false;
    }
    XFlush(m_pDisplay);
    return true;
}

void CStringSenderLinuxX11::FlushAndCloseDisplay()
{
    XFlush(m_pDisplay);
    XCloseDisplay(m_pDisplay);
}
#endif
