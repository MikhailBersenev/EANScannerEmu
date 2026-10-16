#include "sendhistorymodel.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QLockFile>
#include <algorithm>

SendHistoryModel::SendHistoryModel(const QString &filePath, QObject *parent)
    : QAbstractTableModel(parent), m_filePath(filePath)
{}

int SendHistoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_entries.size();
}

int SendHistoryModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 5;
}

QVariant SendHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_entries.size()
        || (role != Qt::DisplayRole && role != Qt::ToolTipRole))
        return {};

    const Entry &entry = m_entries.at(index.row());
    switch (index.column()) {
    case 0: return entry.timestamp.toLocalTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    case 1: return entry.barcode;
    case 2: return entry.action;
    case 3: return entry.sendReturn ? tr("Yes") : tr("No");
    case 4: return entry.result;
    default: return {};
    }
}

QVariant SendHistoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractTableModel::headerData(section, orientation, role);
    switch (section) {
    case 0: return tr("Time");
    case 1: return tr("Barcode");
    case 2: return tr("Action");
    case 3: return tr("Enter");
    case 4: return tr("Result");
    default: return {};
    }
}

QString SendHistoryModel::load()
{
    QList<Entry> entries;
    const QString error = read(entries);
    if (!error.isEmpty())
        return error;
    beginResetModel();
    m_entries = entries;
    endResetModel();
    return {};
}

QString SendHistoryModel::read(QList<Entry> &entries) const
{
    QFile file(m_filePath);
    if (!file.exists())
        return {};
    if (!file.open(QIODevice::ReadOnly))
        return file.errorString();
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isArray())
        return tr("Invalid history file. Clear History to reset it.");

    for (const QJsonValue &value : document.array()) {
        const QJsonObject object = value.toObject();
        const QDateTime time = QDateTime::fromString(object.value("timestamp").toString(), Qt::ISODateWithMs);
        if (!value.isObject() || !time.isValid() || !object.value("barcode").isString()
            || !object.value("action").isString() || !object.value("sendReturn").isBool()
            || !object.value("result").isString())
            return tr("Invalid history entry. Clear History to reset it.");
        if (entries.size() < MaximumEntries)
            entries.append({time, object.value("barcode").toString(), object.value("action").toString(),
                            object.value("sendReturn").toBool(), object.value("result").toString()});
    }
    return {};
}

QString SendHistoryModel::append(const Entry &entry)
{
    m_pendingEntries.prepend(entry);
    if (m_pendingEntries.size() > MaximumEntries)
        m_pendingEntries.removeLast();
    beginInsertRows(QModelIndex(), 0, 0);
    m_entries.prepend(entry);
    endInsertRows();
    if (m_entries.size() > MaximumEntries) {
        beginRemoveRows(QModelIndex(), MaximumEntries, m_entries.size() - 1);
        m_entries.removeLast();
        endRemoveRows();
    }
    return persistPending();
}

QString SendHistoryModel::persistPending()
{
    if (!QDir().mkpath(QFileInfo(m_filePath).absolutePath()))
        return tr("Could not create the history directory.");
    QLockFile lock(m_filePath + ".lock");
    if (!lock.tryLock(1000))
        return tr("History is busy. New entries are kept for this session until a save succeeds.");
    QList<Entry> savedEntries;
    const QString readError = read(savedEntries);
    if (!readError.isEmpty())
        return readError;
    QList<Entry> merged = m_pendingEntries;
    merged.append(savedEntries);
    std::stable_sort(merged.begin(), merged.end(), [](const Entry &left, const Entry &right) {
        return left.timestamp > right.timestamp;
    });
    if (merged.size() > MaximumEntries)
        merged.resize(MaximumEntries);
    const QString error = save(merged);
    if (!error.isEmpty())
        return error;
    m_pendingEntries.clear();
    if (merged != m_entries) {
        beginResetModel();
        m_entries = merged;
        endResetModel();
    }
    return {};
}

QString SendHistoryModel::clear()
{
    if (!QDir().mkpath(QFileInfo(m_filePath).absolutePath()))
        return tr("Could not create the history directory.");
    QLockFile lock(m_filePath + ".lock");
    if (!lock.tryLock(1000))
        return tr("History is busy. Could not clear history.");
    const QString error = save({});
    if (!error.isEmpty())
        return error;
    beginResetModel();
    m_entries.clear();
    m_pendingEntries.clear();
    endResetModel();
    return {};
}

QString SendHistoryModel::save(const QList<Entry> &entries) const
{
    QJsonArray array;
    for (const Entry &entry : entries) {
        array.append(QJsonObject{{"timestamp", entry.timestamp.toUTC().toString(Qt::ISODateWithMs)},
                                 {"barcode", entry.barcode}, {"action", entry.action},
                                 {"sendReturn", entry.sendReturn}, {"result", entry.result}});
    }
    QSaveFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly))
        return file.errorString();
    const QByteArray contents = QJsonDocument(array).toJson(QJsonDocument::Compact);
    if (file.write(contents) != contents.size() || !file.commit())
        return file.errorString();
    return {};
}
