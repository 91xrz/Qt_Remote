

add_executable(client WIN32)

target_sources(client PRIVATE
    main.cpp

    core/MainController.h
    core/MainController.cpp
    core/RemoteConnection.h
    core/RemoteConnection.cpp

    controller/ClientCommandHandler.h
    controller/ClientCommandHandler.cpp
    controller/FileManagerController.h
    controller/FileManagerController.cpp
    controller/RemoteDesktopController.h
    controller/RemoteDesktopController.cpp

    view/Qt_Remote.h
    view/Qt_Remote.cpp
    view/FileManagerWidget.h
    view/FileManagerWidget.cpp
    view/RemoteDesktopWidget.h
    view/RemoteDesktopWidget.cpp
    view/Qt_Remote.ui
    
    res/Qt_Remote.qrc
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

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES
    main.cpp

    core/MainController.h
    core/MainController.cpp
    core/RemoteConnection.h
    core/RemoteConnection.cpp

    controller/ClientCommandHandler.h
    controller/ClientCommandHandler.cpp
    controller/FileManagerController.h
    controller/FileManagerController.cpp
    controller/RemoteDesktopController.h
    controller/RemoteDesktopController.cpp

    view/Qt_Remote.h
    view/Qt_Remote.cpp
    view/FileManagerWidget.h
    view/FileManagerWidget.cpp
    view/RemoteDesktopWidget.h
    view/RemoteDesktopWidget.cpp
    view/Qt_Remote.ui
    
    res/Qt_Remote.qrc
)