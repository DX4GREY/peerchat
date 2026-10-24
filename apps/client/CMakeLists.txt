# apps/client/CMakeLists.txt

qt_policy(SET QTP0004 OLD)

qt_add_executable(PeerChat
    src/main.cpp
    src/ChatController.hpp
    src/ChatController.cpp
    src/ContactModel.hpp
    src/ContactModel.cpp
    src/MessageModel.hpp
    src/MessageModel.cpp
    src/TransferModel.hpp
    src/TransferModel.cpp
)

qt_add_qml_module(PeerChat
    URI peerchat
    VERSION 1.0
    RESOURCE_PREFIX "/peerchat"
    QML_FILES
        qml/Main.qml
        qml/Theme.qml
        qml/components/RouteBadge.qml
        qml/components/MessageBubble.qml
        qml/components/TransferItem.qml
        qml/components/EmptyState.qml
        qml/views/ContactsView.qml
        qml/views/ChatView.qml
        qml/views/TransfersView.qml
        qml/views/SettingsView.qml
)

target_include_directories(PeerChat PRIVATE
    src
    ${CMAKE_SOURCE_DIR}/core
)

target_link_libraries(PeerChat PRIVATE
    peerchat_core
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Network
    Qt6::Sql
)

if(ANDROID)
    set_target_properties(PeerChat PROPERTIES
        QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/platform/android"
    )
endif()
