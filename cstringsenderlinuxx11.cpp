#include "cstringsenderlinuxx11.h"
#ifdef Q_OS_LINUX
#include <QDebug>
#include <QProcess>
#include <QThread>

namespace {
bool SendStringViaXdotool(const QString &text)
{
    if (text.isEmpty()) {
        return false;
    }

    const int exitCode = QProcess::execute("xdotool", {"type", "--delay", "3", "--clearmodifiers", text});
    if (exitCode != 0) {
        qDebug() << "Unable to send string via xdotool. Exit code:" << exitCode;
        return false;
    }

    qDebug() << "Successfully sent string via xdotool fallback";
    return true;
}

bool SendReturnViaXdotool()
{
    const int exitCode = QProcess::execute("xdotool", {"key", "--clearmodifiers", "Return"});
    if (exitCode != 0) {
        qDebug() << "Unable to send Return via xdotool. Exit code:" << exitCode;
        return false;
    }

    qDebug() << "Successfully sent Return via xdotool fallback";
    return true;
}
}

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
    if (!m_pDisplay) {
        qDebug() << "X11 display is not available, trying xdotool fallback";
        return SendStringViaXdotool(*pString);
    }

    for(int i = 0; i < pString->length(); i++) {
        if (!SendUnicodeChar(pString->at(i).unicode(), true)) {
            qDebug() << "X11 API failed to send character, trying xdotool fallback";
            return SendStringViaXdotool(*pString);
        }
        if (!SendUnicodeChar(pString->at(i).unicode(), false)) {
            qDebug() << "X11 API failed to release character, trying xdotool fallback";
            return SendStringViaXdotool(*pString);
        }
        QThread::msleep(3);
    }
    XFlush(m_pDisplay);
    return true;
}

bool CStringSenderLinuxX11::SendReturn()
{
    if (!m_pDisplay) {
        qDebug() << "X11 display is not available, trying xdotool fallback for Return";
        return SendReturnViaXdotool();
    }

    if (!SendUnicodeChar(XK_Return, true)) {
        qDebug() << "X11 API failed to send Return, trying xdotool fallback";
        return SendReturnViaXdotool();
    }
    if (!SendUnicodeChar(XK_Return, false)) {
        qDebug() << "X11 API failed to release Return, trying xdotool fallback";
        return SendReturnViaXdotool();
    }
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
    if (!m_pDisplay) {
        return false;
    }
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
