# tests/CMakeLists.txt — unit + integration suites. Both link libshell (NO GUI), proving the
# library-first split (spec §2): all logic is testable without a display or QML.
find_package(GTest CONFIG REQUIRED)

# ── Unit tests: pure logic + headless QObject/model/viewmodel checks ───────────
qt_add_executable(shell_unit_tests
  unit/main.cpp
  unit/MoneyTest.cpp
  unit/CartLineModelTest.cpp
  unit/BillingViewModelTest.cpp
  unit/ConfigServiceTest.cpp)
target_link_libraries(shell_unit_tests PRIVATE shell GTest::gtest Qt6::Core)
add_test(NAME shell_unit_tests COMMAND shell_unit_tests)

# ── Integration: bridge against a live engine / fakepos (opt-in) ──────────────
# Requires a running TerminalEngine on localhost; skipped unless BLISSMONT_ENGINE_TARGET is
# set in the environment, so the suite stays green on a build box with no engine.
qt_add_executable(shell_integration_tests
  integration/main.cpp
  integration/BridgeSmokeTest.cpp
  integration/ConfigRehydrateTest.cpp)
target_link_libraries(shell_integration_tests PRIVATE shell GTest::gtest Qt6::Core)
add_test(NAME shell_integration_tests COMMAND shell_integration_tests)
