#ifndef CUTILS_H
#define CUTILS_H

#include <QObject>

class CUtils : public QObject
{
    Q_OBJECT
public:
    explicit CUtils(QObject *parent = nullptr);
    static QStringList GenerateEAN8(const int& rQnt);
    static QStringList GenerateEAN8(const int& rQnt, const QString& rPrefix);
    static QStringList GenerateEAN13(const int& rQnt);
    static QStringList GenerateEAN13(const int& rQnt, const QString& rPrefix);
    static QString GetVersion();
private:
    static int EanChecksum(const QString& rStr);

signals:
};

#endif // CUTILS_H
