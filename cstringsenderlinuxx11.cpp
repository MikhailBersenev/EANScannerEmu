#include "cstringsenderlinuxx11.h"
#ifdef Q_OS_LINUX
#include <QProcess>
#include <QThread>

namespace {
bool SendStringViaXdotool(const QString &text, const QSharedPointer<CInfoMessageHandler> &pInfoMessageHandler)
{
    if (text.isEmpty()) {
        return false;
    }

    const int exitCode = QProcess::execute("xdotool", {"type", "--delay", "3", "--clearmodifiers", text});
    if (exitCode != 0) {
        if (!pInfoMessageHandler.isNull()) {
            pInfoMessageHandler->InfoMessage(QString("Unable to send string via xdotool. Exit code: %1").arg(exitCode));
        }
        return false;
    }

    if (!pInfoMessageHandler.isNull()) {
        pInfoMessageHandler->InfoMessage("Successfully sent string via xdotool fallback");
    }
    return true;
}

bool SendReturnViaXdotool(const QSharedPointer<CInfoMessageHandler> &pInfoMessageHandler)
{
    const int exitCode = QProcess::execute("xdotool", {"key", "--clearmodifiers", "Return"});
    if (exitCode != 0) {
        if (!pInfoMessageHandler.isNull()) {
            pInfoMessageHandler->InfoMessage(QString("Unable to send Return via xdotool. Exit code: %1").arg(exitCode));
        }
        return false;
    }

    if (!pInfoMessageHandler.isNull()) {
        pInfoMessageHandler->InfoMessage("Successfully sent Return via xdotool fallback");
    }
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
        if (!m_InfoMessageHandler.isNull()) {
            m_InfoMessageHandler->InfoMessage("CStringSenderLinuxX11::SendString(QString *pString) pString is null");
        }
        return false;
    }
    if (!m_pDisplay) {
        if (!m_InfoMessageHandler.isNull()) {
            m_InfoMessageHandler->InfoMessage("X11 display is not available, trying xdotool fallback");
        }
        return SendStringViaXdotool(*pString, m_InfoMessageHandler);
    }

    for(int i = 0; i < pString->length(); i++) {
        if (!SendUnicodeChar(pString->at(i).unicode(), true)) {
            if (!m_InfoMessageHandler.isNull()) {
                m_InfoMessageHandler->InfoMessage("X11 API failed to send character, trying xdotool fallback");
            }
            return SendStringViaXdotool(*pString, m_InfoMessageHandler);
        }
        if (!SendUnicodeChar(pString->at(i).unicode(), false)) {
            if (!m_InfoMessageHandler.isNull()) {
                m_InfoMessageHandler->InfoMessage("X11 API failed to release character, trying xdotool fallback");
            }
            return SendStringViaXdotool(*pString, m_InfoMessageHandler);
        }
        QThread::msleep(3);
    }
    XFlush(m_pDisplay);
    return true;
}

bool CStringSenderLinuxX11::SendReturn()
{
    if (!m_pDisplay) {
        if (!m_InfoMessageHandler.isNull()) {
            m_InfoMessageHandler->InfoMessage("X11 display is not available, trying xdotool fallback for Return");
        }
        return SendReturnViaXdotool(m_InfoMessageHandler);
    }

    if (!SendUnicodeChar(XK_Return, true)) {
        if (!m_InfoMessageHandler.isNull()) {
            m_InfoMessageHandler->InfoMessage("X11 API failed to send Return, trying xdotool fallback");
        }
        return SendReturnViaXdotool(m_InfoMessageHandler);
    }
    if (!SendUnicodeChar(XK_Return, false)) {
        if (!m_InfoMessageHandler.isNull()) {
            m_InfoMessageHandler->InfoMessage("X11 API failed to release Return, trying xdotool fallback");
        }
        return SendReturnViaXdotool(m_InfoMessageHandler);
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
        if (!m_InfoMessageHandler.isNull()) {
            m_InfoMessageHandler->InfoMessage("Unable to send XTestFakeKeyEvent");
        }
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
