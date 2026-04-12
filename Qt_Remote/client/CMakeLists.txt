set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(client WIN32
     main.cpp
    Qt_Remote.cpp
    Qt_Remote.h
    Qt_Remote.ui
    Qt_Remote.qrc
    MainController.h
    MainController.cpp
    FileManagerController.h
    FileManagerController.cpp
    RemoteDesktopController.h
    RemoteDesktopController.cpp
    FileManagerWidget.h
    FileManagerWidget.cpp
    RemoteConnection.h
    RemoteConnection.cpp
    ClientCommandHandler.h
    ClientCommandHandler.cpp
    RemoteDesktopWidget.h
    RemoteDesktopWidget.cpp

)

target_include_directories(client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/common
)

target_link_libraries(client PRIVATE
    common
    Qt6::Widgets
    Qt6::Network
)
