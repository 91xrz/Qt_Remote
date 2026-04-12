set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(server
    main.cpp

    core/MainController.h
    core/MainController.cpp
    core/DeviceServer.h
    core/DeviceServer.cpp
    core/ClientSession.h
    core/ClientSession.cpp

    controller/CommandHandler.h
    controller/CommandHandler.cpp

    view/ServerWindow.h
    view/ServerWindow.cpp
    view/LockScreenWidget.h
    view/LockScreenWidget.cpp
)

target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/core
    ${CMAKE_CURRENT_SOURCE_DIR}/controller
    ${CMAKE_CURRENT_SOURCE_DIR}/view
    ${CMAKE_SOURCE_DIR}/common
)

target_link_libraries(server PRIVATE
    common
    Qt6::Widgets
    Qt6::Network
)

if(MSVC)
    target_compile_options(server PRIVATE /utf-8)
endif()

if(WIN32)
    set_target_properties(server PROPERTIES WIN32_EXECUTABLE ON)
endif()
