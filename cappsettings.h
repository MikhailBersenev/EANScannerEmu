#ifndef CAPPSETTINGS_H
#define CAPPSETTINGS_H

#include <QObject>
#include <QSettings>

/** Обёртка над QSettings: единые organization/application и типизированные ключи. */
class CAppSettings : public QObject
{
public:
    explicit CAppSettings(QObject *parent = nullptr);

    static QString organizationName();
    static QString applicationName();

    bool hasAcknowledgedWaylandMessage() const;
    void acknowledgeWaylandMessage();
    void removeWaylandMessageIfPresent();

    int timeoutSeconds() const;
    void setTimeoutSeconds(int seconds);

    bool showInfoMessages() const;
    void setShowInfoMessages(bool enabled);

    bool sendReturnAfterBarcode() const;
    void setSendReturnAfterBarcode(bool enabled);

private:
    QSettings m_settings;
};

#endif // CAPPSETTINGS_H
