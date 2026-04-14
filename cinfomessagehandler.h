#ifndef CINFOMESSAGEHANDLER_H
#define CINFOMESSAGEHANDLER_H

#include <QObject>
#include <QString>

class CInfoMessageHandler : public QObject
{
    Q_OBJECT
public:
    CInfoMessageHandler(QObject *parent = nullptr, bool bShowMessages = true);
    void InfoMessage(const QString &sMsg);
    void SetMode(bool bShowMessages);
    bool GetMode() const;
private:
    bool m_bShowMessages;
    void LogMessage(const QString &sMsg) const;
    void ShowMessage(const QString &sMsg);

signals:
};

#endif // CINFOMESSAGEHANDLER_H
