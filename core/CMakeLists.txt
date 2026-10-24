# core/CMakeLists.txt

add_library(peerchat_core STATIC
    crypto/CryptoUtils.hpp
    crypto/CryptoUtils.cpp

    identity/IIdentityProvider.hpp
    identity/SodiumIdentityProvider.hpp
    identity/SodiumIdentityProvider.cpp

    networking/IPeerTransport.hpp
    networking/FramedSocket.hpp
    networking/FramedSocket.cpp
    networking/DirectTcpTransport.hpp
    networking/DirectTcpTransport.cpp
    networking/RelayTransport.hpp
    networking/RelayTransport.cpp

    discovery/IPeerDiscovery.hpp
    discovery/LanDiscovery.hpp
    discovery/LanDiscovery.cpp

    storage/IMessageStore.hpp
    storage/SqliteMessageStore.hpp
    storage/SqliteMessageStore.cpp

    transfers/IFileTransfer.hpp
    transfers/FileTransferManager.hpp
    transfers/FileTransferManager.cpp

    messaging/ProtocolMessage.hpp
    messaging/ProtocolMessage.cpp
    messaging/MessagingEngine.hpp
    messaging/MessagingEngine.cpp

    diagnostics/DiagnosticsService.hpp
    diagnostics/DiagnosticsService.cpp
)

target_include_directories(peerchat_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/crypto
    ${CMAKE_CURRENT_SOURCE_DIR}/identity
    ${CMAKE_CURRENT_SOURCE_DIR}/networking
    ${CMAKE_CURRENT_SOURCE_DIR}/discovery
    ${CMAKE_CURRENT_SOURCE_DIR}/storage
    ${CMAKE_CURRENT_SOURCE_DIR}/transfers
    ${CMAKE_CURRENT_SOURCE_DIR}/messaging
    ${CMAKE_CURRENT_SOURCE_DIR}/diagnostics
)

target_link_libraries(peerchat_core PUBLIC
    Qt6::Core
    Qt6::Network
    Qt6::Sql
    libsodium::libsodium
)

if(TARGET SQLite3::SQLite3)
    target_link_libraries(peerchat_core PUBLIC SQLite3::SQLite3)
endif()
