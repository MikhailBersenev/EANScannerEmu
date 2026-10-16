#ifndef SENDHISTORYMODEL_H
#define SENDHISTORYMODEL_H

#include <QAbstractTableModel>
#include <QDateTime>
#include <QList>

class SendHistoryModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    struct Entry {
        QDateTime timestamp;
        QString barcode;
        QString action;
        bool sendReturn;
        QString result;
        bool operator==(const Entry &other) const {
            return timestamp == other.timestamp && barcode == other.barcode
                && action == other.action && sendReturn == other.sendReturn && result == other.result;
        }
    };
    static constexpr int MaximumEntries = 1000;

    explicit SendHistoryModel(const QString &filePath, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QString load();
    QString append(const Entry &entry);
    QString clear();

private:
    QString read(QList<Entry> &entries) const;
    QString save(const QList<Entry> &entries) const;
    QString persistPending();
    QString m_filePath;
    QList<Entry> m_entries;
    QList<Entry> m_pendingEntries;
};

#endif
