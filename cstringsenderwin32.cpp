#include "cstringsenderwin32.h"
#ifdef Q_OS_WIN
#include "windows.h"
#include <QThread>

CStringSenderWin32::CStringSenderWin32(QObject *parent)
    : CStringSender{parent}
{}

bool CStringSenderWin32::SendString(QString *pString)
{
    for (QChar ch : *pString)
    {
        if(!SendUnicodeChar(ch.unicode(), false))
            return false;
        if(!SendUnicodeChar(ch.unicode(), true))
            return false;

        QThread::msleep(3);
    }
    return true;
}

bool CStringSenderWin32::SendReturn()
{
    INPUT in[2] = {};
    in[0].type = INPUT_KEYBOARD;
    in[0].ki.wVk = VK_RETURN;

    in[1].type = INPUT_KEYBOARD;
    in[1].ki.wVk = VK_RETURN;
    in[1].ki.dwFlags = KEYEVENTF_KEYUP;
    if(!SendInput(2, in, sizeof(INPUT)))
        return false;
    return true;
}

bool CStringSenderWin32::SendUnicodeChar(wchar_t ch, bool bKeyUp)
{
    INPUT in = {};
    in.type = INPUT_KEYBOARD;
    in.ki.wVk = 0;
    in.ki.wScan = ch;
    in.ki.dwFlags = KEYEVENTF_UNICODE | (bKeyUp ? KEYEVENTF_KEYUP : 0);
    if(!SendInput(1, &in, sizeof(INPUT))) {
        return false;
    }
    return true;

}
#endif
