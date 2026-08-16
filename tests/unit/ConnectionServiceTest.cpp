// tests/unit/ConnectionServiceTest.cpp — config-freshness surfacing, headless.
//
// The engine can now tell the shell that its terminal config has stopped
// refreshing (contracts v1.18.0). This pins that the shell SHOWS it, and shows the
// two cases differently.
//
// Why this file exists at all: before v1.18.0 a terminal that booted while the
// backend was unreachable held its config forever, with no retry and no alarm.
// Adding the retry without a surface would have been half a fix — retrying silently
// forever is still silent. So "the verdict reaches a human" is the property under
// test, and it is worth a test precisely because nothing else would notice if the
// badge quietly stopped being populated.
#include <gtest/gtest.h>

#include <QSignalSpy>
#include <QString>

#include "services/ConnectionService.hpp"

using blissmont::services::ConnectionService;

namespace {
constexpr bool kOnline = true;
constexpr bool kOffline = false;
constexpr bool kStale = true;
constexpr bool kFresh = false;
}  // namespace

TEST(ConnectionService, HealthyConfigShowsNoWarning) {
    ConnectionService conn;
    conn.setConnected(true);
    conn.applySyncStatus(kOnline, 0, kFresh, QStringLiteral("2026-08-16T10:00:00Z"));

    EXPECT_FALSE(conn.configStale());
    EXPECT_TRUE(conn.configStatusText().isEmpty())
        << "a healthy till must show nothing: a badge that is always present is a badge "
           "nobody reads when it matters";
}

TEST(ConnectionService, StaleWhileOnlineReadsAsAFault) {
    ConnectionService conn;
    conn.setConnected(true);
    conn.applySyncStatus(kOnline, 0, kStale, QStringLiteral("2026-08-16T09:00:00Z"));

    EXPECT_TRUE(conn.configStale());
    const QString text = conn.configStatusText();
    EXPECT_FALSE(text.isEmpty());
    // Online + stale means the server is REACHABLE and the config still will not
    // refresh. That is a defect to escalate, not an outage to wait out, and the
    // wording has to send someone to support rather than to the wifi router.
    EXPECT_TRUE(text.contains(QStringLiteral("support"), Qt::CaseInsensitive))
        << "online-and-stale must read as a fault to escalate, got: " << text.toStdString();
}

TEST(ConnectionService, StaleWhileOfflineReadsAsAnOutage) {
    ConnectionService conn;
    conn.setConnected(true);
    conn.applySyncStatus(kOffline, 0, kStale, QStringLiteral("2026-08-14T09:00:00Z"));

    const QString text = conn.configStatusText();
    EXPECT_FALSE(text.isEmpty());
    // The discriminating half. If both cases produced the same string, the two
    // thresholds the engine goes to the trouble of distinguishing would collapse at
    // the surface, and a real server-side fault would be triaged as flaky shop wifi.
    EXPECT_FALSE(text.contains(QStringLiteral("support"), Qt::CaseInsensitive))
        << "offline staleness must not read as a support case, got: " << text.toStdString();
    EXPECT_TRUE(text.contains(QStringLiteral("2026-08-14T09:00:00Z")))
        << "an offline till should show WHEN it last synced — that is the fact the shop "
           "can act on, got: "
        << text.toStdString();
}

TEST(ConnectionService, NeverSyncedIsDistinctFromStaleWithATimestamp) {
    ConnectionService conn;
    conn.setConnected(true);
    conn.applySyncStatus(kOffline, 0, kStale, QString());

    // A terminal that has NEVER pulled config is materially different from one whose
    // config is merely old: it is running on defaults nobody chose. Rendering an
    // empty timestamp into "Config last synced " would hide exactly that.
    EXPECT_TRUE(conn.configStatusText().contains(QStringLiteral("never"), Qt::CaseInsensitive))
        << "got: " << conn.configStatusText().toStdString();
}

TEST(ConnectionService, FreshnessChangeNotifiesTheView) {
    ConnectionService conn;
    conn.applySyncStatus(kOnline, 0, kFresh, QStringLiteral("2026-08-16T10:00:00Z"));

    QSignalSpy spy(&conn, &ConnectionService::changed);
    conn.applySyncStatus(kOnline, 0, kStale, QStringLiteral("2026-08-16T10:00:00Z"));
    EXPECT_EQ(spy.count(), 1)
        << "the config going stale must notify: without it the badge appears only when "
           "some unrelated field happens to change";

    // And an unchanged status must NOT notify — the engine emits SyncStatusChanged on
    // every tick (every 5s), so a service that signalled each time would repaint the
    // whole status bar continuously.
    conn.applySyncStatus(kOnline, 0, kStale, QStringLiteral("2026-08-16T10:00:00Z"));
    EXPECT_EQ(spy.count(), 1) << "an identical status re-emitted must not signal";
}
