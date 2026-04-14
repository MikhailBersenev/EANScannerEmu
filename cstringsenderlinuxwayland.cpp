#include "cstringsenderlinuxwayland.h"
#include <QApplication>
#include <QProcess>
#include <QClipboard>
#include <QThread>

CStringSenderLinuxWayland::CStringSenderLinuxWayland(QObject *parent)
    : CStringSender{parent}
{}

bool CStringSenderLinuxWayland::SendString(QString* pString)
{
    if(pString->isEmpty()) {
        if (!m_InfoMessageHandler.isNull()) {
            m_InfoMessageHandler->InfoMessage("CStringSenderLinuxWayland::SendString - given string is empty");
        }
        return false;
    }
    if (!m_InfoMessageHandler.isNull()) {
        m_InfoMessageHandler->InfoMessage("Attempting to send text: \"" + *pString + "\"");
    }
    
    // Try to send text normally first
    if(QProcess::execute("ydotool type \"" + *pString + "\"") == 0) {
        if (!m_InfoMessageHandler.isNull()) {
            m_InfoMessageHandler->InfoMessage("Successfully sent string via ydotool: " + *pString);
        }
        return true;
    } else {
        if (!m_InfoMessageHandler.isNull()) {
            m_InfoMessageHandler->InfoMessage("Unable to send string via ydotool");
        }
    }

    // Method 2: Fallback - copy to clipboard
    if (!m_InfoMessageHandler.isNull()) {
        m_InfoMessageHandler->InfoMessage("All methods failed, copying to clipboard only");
    }
    QClipboard *pClipboard = QApplication::clipboard();
    pClipboard->setText(*pString);
    if (!m_InfoMessageHandler.isNull()) {
        m_InfoMessageHandler->InfoMessage("String copied to clipboard: " + *pString);
    }

    return false;
}

bool CStringSenderLinuxWayland::SendReturn()
{
    if (!m_InfoMessageHandler.isNull()) {
        m_InfoMessageHandler->InfoMessage("Attempting to send Return key...");
    }
    if(QProcess::execute("which ydotool") == 0) {
        if(QProcess::execute("ydotool key 28") == 0) {
            if (!m_InfoMessageHandler.isNull()) {
                m_InfoMessageHandler->InfoMessage("Successfully sent Return key via ydotool");
            }
            return true;
        }
    }
    if (!m_InfoMessageHandler.isNull()) {
        m_InfoMessageHandler->InfoMessage("Failed to send Return key via any method");
    }
    return false;
}
