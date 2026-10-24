# tests/CMakeLists.txt

add_executable(peerchat_tests
    main.cpp
    unit/test_identity.cpp
    unit/test_protocol.cpp
    unit/test_storage.cpp
    unit/test_sanitization.cpp
    transfers/test_transfers.cpp
    networking/test_networking.cpp
)

target_link_libraries(peerchat_tests PRIVATE
    peerchat_core
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    GTest::gtest
    libsodium::libsodium
)

include(GoogleTest)
gtest_discover_tests(peerchat_tests)
