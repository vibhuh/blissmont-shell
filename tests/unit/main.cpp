// tests/unit/main.cpp — gtest entry with a QCoreApplication so QObject-based tests
// (models, view-models, signal/slot wiring) have a running event loop available.
#include <QCoreApplication>
#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
