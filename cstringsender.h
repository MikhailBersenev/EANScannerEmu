#ifndef CSTRINGSENDER_H
#define CSTRINGSENDER_H

#include <QObject>

class CStringSender : public QObject
{
    Q_OBJECT
public:
    explicit CStringSender(QObject *parent = nullptr);
    virtual  bool SendString(QString* pString) = 0;
    virtual  bool SendReturn()  = 0;

signals:
};

#endif // CSTRINGSENDER_H
