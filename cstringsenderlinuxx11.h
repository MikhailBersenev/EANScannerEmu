#ifndef CSTRINGSENDERLINUXX11_H
#define CSTRINGSENDERLINUXX11_H

#include "cstringsender.h"

#include <QTextStream>

#ifdef Q_OS_LINUX

#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

class CStringSenderLinuxX11 : public CStringSender
{
public:
    explicit CStringSenderLinuxX11(QObject *parent = nullptr);
    virtual  bool SendString(QString* pString) override;
    virtual  bool SendReturn() override;
    ~ CStringSenderLinuxX11();
private:
    Display* m_pDisplay;
    bool SendUnicodeChar(qint32 uCode, bool bRelease);
    void FlushAndCloseDisplay();

};
#endif
#endif // CSTRINGSENDERLINUXX11_H
