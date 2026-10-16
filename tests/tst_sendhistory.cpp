#include "sendhistorymodel.h"

#include <QAbstractItemModelTester>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QtTest>
#include <QLockFile>

class SendHistoryTest : public QObject
{
    Q_OBJECT
private slots:
    void roundTrip();
    void retention();
    void corruptFile();
    void persistenceFailure();
    void failedClearPreservesEntries();
    void multipleInstancesPreserveEntries();
    void staleInstanceDoesNotRestoreClearedEntries();
    void lockContentionRetainsPendingEntries();
};

void SendHistoryTest::roundTrip()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    const QString path = directory.filePath("nested/history.json");
    SendHistoryModel model(path);
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QVERIFY(model.load().isEmpty());
    const QDateTime time = QDateTime::fromString("2026-10-17T01:02:03.456+08:00", Qt::ISODateWithMs);
    const QString barcode = QString::fromUtf8("000123\n\"тест\"\t");
    QVERIFY(model.append({time, barcode, "Send Selection", true, "Sent"}).isEmpty());
    QVERIFY(model.append({time.addSecs(1), "000456", "Send All", false, "Text sent; Enter failed"}).isEmpty());
    SendHistoryModel restored(path);
    QVERIFY(restored.load().isEmpty());
    QCOMPARE(restored.rowCount(), 2);
    QCOMPARE(restored.index(0, 1).data().toString(), "000456");
    QCOMPARE(restored.index(0, 3).data().toString(), "No");
    QCOMPARE(restored.index(0, 4).data().toString(), "Text sent; Enter failed");
    QCOMPARE(restored.index(1, 1).data().toString(), barcode);
    QCOMPARE(restored.index(1, 2).data().toString(), "Send Selection");
    QCOMPARE(restored.index(1, 3).data().toString(), "Yes");
    QCOMPARE(restored.index(1, 0).data().toString(), time.toLocalTime().toString("yyyy-MM-dd HH:mm:ss.zzz"));
    QVERIFY(restored.clear().isEmpty());
    QCOMPARE(restored.rowCount(), 0);
    SendHistoryModel cleared(path);
    QVERIFY(cleared.load().isEmpty());
    QCOMPARE(cleared.rowCount(), 0);
}

void SendHistoryTest::retention()
{
    QTemporaryDir directory;
    const QString path = directory.filePath("history.json");
    // Seed a full history to test eviction without performing 1,000 disk writes.
    QJsonArray entries;
    for (int i = 0; i < SendHistoryModel::MaximumEntries; ++i)
        entries.append(QJsonObject{{"timestamp", "2026-10-16T17:02:03.456Z"},
                                   {"barcode", QString::number(i)}, {"action", "Send All"},
                                   {"sendReturn", true}, {"result", "Sent"}});
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write(QJsonDocument(entries).toJson());
    file.close();
    SendHistoryModel model(path);
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QVERIFY(model.load().isEmpty());
    QVERIFY(model.append({QDateTime::currentDateTimeUtc(), "newest", "Send Next", true, "Sent"}).isEmpty());
    QCOMPARE(model.rowCount(), SendHistoryModel::MaximumEntries);
    QCOMPARE(model.index(0, 1).data().toString(), "newest");
    QCOMPARE(model.index(999, 1).data().toString(), "998");
    SendHistoryModel restored(path);
    QVERIFY(restored.load().isEmpty());
    QCOMPARE(restored.rowCount(), SendHistoryModel::MaximumEntries);
    QCOMPARE(restored.index(0, 1).data().toString(), "newest");
}

void SendHistoryTest::corruptFile()
{
    QTemporaryDir directory;
    const QString path = directory.filePath("history.json");
    QFile file(path);
    QVERIFY(file.open(QIODevice::WriteOnly));
    file.write("broken history");
    file.close();
    SendHistoryModel model(path);
    QVERIFY(!model.load().isEmpty());
    QVERIFY(!model.append({QDateTime::currentDateTimeUtc(), "123", "Send All", false, "Sent"}).isEmpty());
    QCOMPARE(model.rowCount(), 1);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(file.readAll(), QByteArray("broken history"));
    file.close();
    QVERIFY(model.clear().isEmpty());
    QVERIFY(model.append({QDateTime::currentDateTimeUtc(), "456", "Send Next", true, "Sent"}).isEmpty());
    SendHistoryModel restored(path);
    QVERIFY(restored.load().isEmpty());
    QCOMPARE(restored.rowCount(), 1);
    QCOMPARE(restored.index(0, 1).data().toString(), "456");
}

void SendHistoryTest::persistenceFailure()
{
    QTemporaryDir directory;
    // A regular file cannot act as the parent directory for history.
    QFile blocker(directory.filePath("blocker"));
    QVERIFY(blocker.open(QIODevice::WriteOnly));
    blocker.close();
    SendHistoryModel model(blocker.fileName() + "/history.json");
    QVERIFY(!model.append({QDateTime::currentDateTimeUtc(), "123", "Send All", true, "Sent"}).isEmpty());
    QCOMPARE(model.rowCount(), 1);
}

void SendHistoryTest::failedClearPreservesEntries()
{
    QTemporaryDir directory;
    const QString path = directory.filePath("history.json");
    SendHistoryModel model(path);
    QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::QtTest);
    QVERIFY(model.append({QDateTime::currentDateTimeUtc(), "123", "Send Previous", true, "Sent"}).isEmpty());
    QVERIFY(QFile::remove(path));
    QVERIFY(QDir().mkdir(path));
    QVERIFY(!model.clear().isEmpty());
    QCOMPARE(model.rowCount(), 1);
    QCOMPARE(model.index(0, 1).data().toString(), "123");
}

void SendHistoryTest::multipleInstancesPreserveEntries()
{
    QTemporaryDir directory;
    const QString path = directory.filePath("history.json");
    SendHistoryModel first(path), second(path);
    QVERIFY(first.load().isEmpty());
    QVERIFY(second.load().isEmpty());
    const auto now = QDateTime::currentDateTimeUtc();
    QVERIFY(first.append({now, "first", "Resend", true, "Sent"}).isEmpty());
    QVERIFY(second.append({now.addSecs(1), "second", "Resend", true, "Sent"}).isEmpty());
    QVERIFY(first.append({now.addSecs(2), "third", "Resend", true, "Sent"}).isEmpty());
    SendHistoryModel restored(path);
    QVERIFY(restored.load().isEmpty());
    QCOMPARE(restored.rowCount(), 3);
    QCOMPARE(restored.index(0, 1).data().toString(), "third");
    QCOMPARE(restored.index(1, 1).data().toString(), "second");
    QCOMPARE(restored.index(2, 1).data().toString(), "first");
}

void SendHistoryTest::staleInstanceDoesNotRestoreClearedEntries()
{
    QTemporaryDir directory;
    const QString path = directory.filePath("history.json");
    SendHistoryModel first(path), second(path);
    const auto now = QDateTime::currentDateTimeUtc();
    QVERIFY(first.append({now, "old", "Send All", true, "Sent"}).isEmpty());
    QVERIFY(second.load().isEmpty());
    QVERIFY(first.clear().isEmpty());
    QVERIFY(second.append({now.addSecs(1), "new", "Resend", true, "Sent"}).isEmpty());
    SendHistoryModel restored(path);
    QVERIFY(restored.load().isEmpty());
    QCOMPARE(restored.rowCount(), 1);
    QCOMPARE(restored.index(0, 1).data().toString(), "new");
}

void SendHistoryTest::lockContentionRetainsPendingEntries()
{
    QTemporaryDir directory;
    const QString path = directory.filePath("history.json");
    SendHistoryModel first(path), second(path);
    const auto now = QDateTime::currentDateTimeUtc();
    QLockFile lock(path + ".lock");
    QVERIFY(lock.tryLock());
    QVERIFY(!first.append({now, "pending", "Resend", true, "Sent"}).isEmpty());
    QCOMPARE(first.rowCount(), 1);
    QVERIFY(!first.clear().isEmpty());
    QCOMPARE(first.rowCount(), 1);
    lock.unlock();
    QVERIFY(second.append({now.addSecs(1), "other", "Resend", false, "Sent"}).isEmpty());
    QVERIFY(first.append({now.addSecs(2), "retry", "Resend", true, "Sent"}).isEmpty());
    SendHistoryModel restored(path);
    QVERIFY(restored.load().isEmpty());
    QCOMPARE(restored.rowCount(), 3);
    QCOMPARE(restored.index(0, 1).data().toString(), "retry");
    QCOMPARE(restored.index(1, 1).data().toString(), "other");
    QCOMPARE(restored.index(2, 1).data().toString(), "pending");
}

QTEST_GUILESS_MAIN(SendHistoryTest)
#include "tst_sendhistory.moc"
