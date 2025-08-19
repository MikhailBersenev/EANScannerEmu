#ifndef CSTRINGSENDERLINUXWAYLAND_H
#define CSTRINGSENDERLINUXWAYLAND_H

#include "cstringsender.h"

class CStringSenderLinuxWayland : public CStringSender
{
public:
    explicit CStringSenderLinuxWayland(QObject *parent = nullptr);
    bool SendString(QString* pString) override;
    bool SendReturn() override;

};

#endif // CSTRINGSENDERLINUXWAYLAND_H
