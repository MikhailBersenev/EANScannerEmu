#include "cinfomessagehandler.h"
#include <QMessageBox>
#include <QTextStream>
#include <QWidget>
CInfoMessageHandler::CInfoMessageHandler(QObject *parent, bool bShowMessages)
    : QObject{parent},
    m_bShowMessages(bShowMessages)
{}

void CInfoMessageHandler::InfoMessage(const QString &sMsg)
{
    LogMessage(sMsg);
    ShowMessage(sMsg);
}

void CInfoMessageHandler::SetMode(bool bShowMessages)
{
    m_bShowMessages = bShowMessages;
}

bool CInfoMessageHandler::GetMode() const
{
    return m_bShowMessages;
}


void CInfoMessageHandler::LogMessage(const QString &sMsg) const
{
    QTextStream(stdout) << sMsg << Qt::endl;
}

void CInfoMessageHandler::ShowMessage(const QString &sMsg)
{
    if (m_bShowMessages) {
        QMessageBox msgBox(qobject_cast<QWidget *>(parent()));
        msgBox.setWindowTitle("Information");
        msgBox.setText(sMsg);
        msgBox.exec();
    }
}
