#include "cstringsenderlinuxwayland.h"
#include <QApplication>
#include <QProcess>
#include <QClipboard>
#include <QStandardPaths>
#include <QSharedPointer>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <unistd.h>

namespace {
QString GetYdotoolSocketPath()
{
    const QString runtimeDir = QProcessEnvironment::systemEnvironment().value("XDG_RUNTIME_DIR");
    if (!runtimeDir.isEmpty()) {
        return runtimeDir + "/.ydotool_socket";
    }
    return "/run/user/" + QString::number(getuid()) + "/.ydotool_socket";
}

bool IsYdotoolSocketAvailable(const QSharedPointer<CInfoMessageHandler> &pInfoMessageHandler)
{
    const QString socketPath = GetYdotoolSocketPath();
    const QFileInfo socketInfo(socketPath);
    if (socketInfo.exists()) {
        return true;
    }

    if (!pInfoMessageHandler.isNull()) {
        pInfoMessageHandler->InfoMessage(
            QString("ydotoold socket not found: %1. Start daemon: systemctl --user enable --now ydotoold.service")
                .arg(socketPath));
    }
    return false;
}

bool RunYdotool(const QStringList &args, const QSharedPointer<CInfoMessageHandler> &pInfoMessageHandler)
{
    QProcess process;
    process.start("ydotool", args);
    if (!process.waitForStarted(2000)) {
        if (!pInfoMessageHandler.isNull()) {
            pInfoMessageHandler->InfoMessage(QString("Failed to start ydotool: %1").arg(process.errorString()));
        }
        return false;
    }

    if (!process.waitForFinished(5000)) {
        process.kill();
        process.waitForFinished(1000);
        if (!pInfoMessageHandler.isNull()) {
            pInfoMessageHandler->InfoMessage("ydotool timed out");
        }
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (!pInfoMessageHandler.isNull()) {
            pInfoMessageHandler->InfoMessage(QString("ydotool failed (exit %1)").arg(process.exitCode()));
        }
        return false;
    }

    return true;
}
}

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

    if (QStandardPaths::findExecutable("ydotool").isEmpty()) {
        if (!m_InfoMessageHandler.isNull()) {
            m_InfoMessageHandler->InfoMessage("ydotool is not found in PATH");
        }
        return false;
    }
    if (!IsYdotoolSocketAvailable(m_InfoMessageHandler)) {
        return false;
    }

    if (RunYdotool({"type", *pString}, m_InfoMessageHandler)) {
        if (!m_InfoMessageHandler.isNull()) {
            m_InfoMessageHandler->InfoMessage("Successfully sent string via ydotool: " + *pString);
        }
        return true;
    }

    if (!m_InfoMessageHandler.isNull()) {
        m_InfoMessageHandler->InfoMessage("Unable to send string via ydotool");
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

    if (QStandardPaths::findExecutable("ydotool").isEmpty()) {
        if (!m_InfoMessageHandler.isNull()) {
            m_InfoMessageHandler->InfoMessage("ydotool is not found in PATH");
        }
        return false;
    }
    if (!IsYdotoolSocketAvailable(m_InfoMessageHandler)) {
        return false;
    }

    if (RunYdotool({"key", "28:1", "28:0"}, m_InfoMessageHandler)) {
        if (!m_InfoMessageHandler.isNull()) {
            m_InfoMessageHandler->InfoMessage("Successfully sent Return key via ydotool");
        }
        return true;
    }

    if (!m_InfoMessageHandler.isNull()) {
        m_InfoMessageHandler->InfoMessage("Failed to send Return key via ydotool");
    }
    return false;
}
