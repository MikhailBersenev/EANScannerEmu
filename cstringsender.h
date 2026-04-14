#ifndef CSTRINGSENDER_H
#define CSTRINGSENDER_H

#include "cinfomessagehandler.h"

#include <QObject>
#include <QSharedPointer>

class CStringSender : public QObject
{
    Q_OBJECT
public:
    explicit CStringSender(QObject *parent = nullptr);
    virtual  bool SendString(QString* pString) = 0;
    virtual  bool SendReturn()  = 0;
    void SetInfoMessageHandler(const QSharedPointer<CInfoMessageHandler> &pInfoMessageHandler);

signals:
protected:
    QSharedPointer<CInfoMessageHandler> m_InfoMessageHandler;
};

#endif // CSTRINGSENDER_H
