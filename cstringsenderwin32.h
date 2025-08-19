#ifndef CSTRINGSENDERWIN32_H
#define CSTRINGSENDERWIN32_H

#include "cstringsender.h"

class CStringSenderWin32 : public CStringSender
{
public:
    explicit CStringSenderWin32(QObject *parent = nullptr);
    virtual  bool SendString(QString* pString) override;
    virtual  bool SendReturn() override;

private:
    bool SendUnicodeChar(wchar_t ch, bool bKeyUp) const;


};

#endif // CSTRINGSENDERWIN32_H
