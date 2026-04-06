set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(client
     main.cpp
    Qt_Remote.cpp
    Qt_Remote.h
    Qt_Remote.ui
    Qt_Remote.qrc
    FileManagerWidget.h
    FileManagerWidget.cpp
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