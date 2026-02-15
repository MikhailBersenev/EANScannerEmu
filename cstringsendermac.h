#ifndef CSTRINGSENDERMAC_H
#define CSTRINGSENDERMAC_H

#include "cstringsender.h"

#ifdef Q_OS_MACOS

class CStringSenderMac : public CStringSender
{
public:
    explicit CStringSenderMac(QObject *parent = nullptr);
    virtual bool SendString(QString *pString) override;
    virtual bool SendReturn() override;

private:
    bool SendUnicodeChar(ushort uCode, bool bKeyDown);
};

#endif // Q_OS_MACOS

#endif // CSTRINGSENDERMAC_H
