#include "cstringsender.h"

CStringSender::CStringSender(QObject *parent)
    : QObject{parent}
{}

void CStringSender::SetInfoMessageHandler(const QSharedPointer<CInfoMessageHandler> &pInfoMessageHandler)
{
    m_InfoMessageHandler = pInfoMessageHandler;
}
