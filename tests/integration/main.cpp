// tests/integration/main.cpp — gtest entry with a QCoreApplication event loop for the
// bridge's queued event marshaling.
#include <QCoreApplication>
#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
