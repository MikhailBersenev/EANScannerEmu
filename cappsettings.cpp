#include "cappsettings.h"

#include <QLatin1String>

namespace {
const QLatin1String kWaylandMessageKey("waylandMessage");
const QLatin1String kTimeoutSecondsKey("timeoutSeconds");
const QLatin1String kShowInfoMessagesKey("showInfoMessages");
const QLatin1String kSendReturnAfterBarcodeKey("sendReturnAfterBarcode");
constexpr int kDefaultTimeoutSeconds = 10;
}

QString CAppSettings::organizationName()
{
    return QStringLiteral("Mikhail Bersenev");
}

QString CAppSettings::applicationName()
{
    // Совместимость с уже сохранёнными настройками (имя как в прежнем QSettings)
    return QStringLiteral("EANScammerEmu");
}

CAppSettings::CAppSettings(QObject *parent)
    : QObject(parent)
    , m_settings(QSettings::NativeFormat, QSettings::UserScope, organizationName(), applicationName(), this)
{
}

bool CAppSettings::hasAcknowledgedWaylandMessage() const
{
    return m_settings.contains(kWaylandMessageKey);
}

void CAppSettings::acknowledgeWaylandMessage()
{
    m_settings.setValue(kWaylandMessageKey, QStringLiteral("1"));
}

void CAppSettings::removeWaylandMessageIfPresent()
{
    if (m_settings.contains(kWaylandMessageKey)) {
        m_settings.remove(kWaylandMessageKey);
    }
}

int CAppSettings::timeoutSeconds() const
{
    const QVariant v = m_settings.value(kTimeoutSecondsKey);
    if (!v.isValid()) {
        return kDefaultTimeoutSeconds;
    }
    bool ok = false;
    const int n = v.toInt(&ok);
    if (!ok) {
        return kDefaultTimeoutSeconds;
    }
    return n;
}

void CAppSettings::setTimeoutSeconds(int seconds)
{
    m_settings.setValue(kTimeoutSecondsKey, seconds);
}

bool CAppSettings::showInfoMessages() const
{
    const QVariant v = m_settings.value(kShowInfoMessagesKey);
    if (!v.isValid()) {
        return false;
    }
    return v.toBool();
}

void CAppSettings::setShowInfoMessages(bool enabled)
{
    m_settings.setValue(kShowInfoMessagesKey, enabled);
}

bool CAppSettings::sendReturnAfterBarcode() const
{
    const QVariant v = m_settings.value(kSendReturnAfterBarcodeKey);
    if (!v.isValid()) {
        return true;
    }
    return v.toBool();
}

void CAppSettings::setSendReturnAfterBarcode(bool enabled)
{
    m_settings.setValue(kSendReturnAfterBarcodeKey, enabled);
}
