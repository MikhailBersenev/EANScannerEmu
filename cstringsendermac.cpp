#include "cstringsendermac.h"

#ifdef Q_OS_MACOS

#include <QThread>
#include <QDebug>

#include <ApplicationServices/ApplicationServices.h>
#include <Carbon/Carbon.h>

CStringSenderMac::CStringSenderMac(QObject *parent)
    : CStringSender{parent}
{}

bool CStringSenderMac::SendUnicodeChar(ushort uCode, bool bKeyDown)
{
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (!source) {
        qDebug() << "CStringSenderMac: CGEventSourceCreate failed";
        return false;
    }

    CGEventRef event = CGEventCreateKeyboardEvent(source, 0, bKeyDown);
    if (!event) {
        CFRelease(source);
        return false;
    }

    UniChar ch = static_cast<UniChar>(uCode);
    CGEventKeyboardSetUnicodeString(event, 1, &ch);

    CGEventPost(kCGHIDEventTap, event);

    CFRelease(event);
    CFRelease(source);
    return true;
}

bool CStringSenderMac::SendString(QString *pString)
{
    if (!pString) {
        qDebug() << "CStringSenderMac::SendString: pString is null";
        return false;
    }

    for (int i = 0; i < pString->length(); ++i) {
        ushort uCode = pString->at(i).unicode();
        if (!SendUnicodeChar(uCode, true))
            return false;
        if (!SendUnicodeChar(uCode, false))
            return false;
        QThread::msleep(3);
    }
    return true;
}

bool CStringSenderMac::SendReturn()
{
    CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
    if (!source) {
        qDebug() << "CStringSenderMac: CGEventSourceCreate failed";
        return false;
    }

    CGEventRef keyDown = CGEventCreateKeyboardEvent(source, kVK_Return, true);
    CGEventRef keyUp   = CGEventCreateKeyboardEvent(source, kVK_Return, false);
    if (!keyDown || !keyUp) {
        if (keyDown) CFRelease(keyDown);
        if (keyUp)   CFRelease(keyUp);
        CFRelease(source);
        return false;
    }

    CGEventPost(kCGHIDEventTap, keyDown);
    CGEventPost(kCGHIDEventTap, keyUp);

    CFRelease(keyDown);
    CFRelease(keyUp);
    CFRelease(source);
    return true;
}

#endif // Q_OS_MACOS
