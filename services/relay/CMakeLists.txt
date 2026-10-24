# services/relay/CMakeLists.txt

add_executable(peerchat-relay
    RelayServer.hpp
    RelayServer.cpp
    main.cpp
)

target_link_libraries(peerchat-relay PRIVATE
    peerchat_core
    Qt6::Core
    Qt6::Network
)
