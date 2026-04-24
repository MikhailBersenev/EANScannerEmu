#ifndef CSTRINGSENDERLINUXWAYLAND_H
#define CSTRINGSENDERLINUXWAYLAND_H

#include <QtGlobal>
#ifdef Q_OS_LINUX

#include "cstringsender.h"

class CStringSenderLinuxWayland : public CStringSender
{
public:
    explicit CStringSenderLinuxWayland(QObject *parent = nullptr);
    bool SendString(QString* pString) override;
    bool SendReturn() override;

};

#endif
#endif // CSTRINGSENDERLINUXWAYLAND_H
