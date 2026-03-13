cmake_minimum_required(VERSION 3.16)

project(Qt_Remote VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt 自动处理 moc / uic / rcc
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 让 VS 里按分组显示
set_property(GLOBAL PROPERTY USE_FOLDERS ON)


# 找 Qt6
find_package(Qt6 REQUIRED COMPONENTS Widgets Network)

set(SRC_FILES
    main.cpp
    ClientSession.cpp
    DeviceServer.cpp
    Qt_Remote.cpp
)

set(HDR_FILES
    ClientSession.h
    DeviceServer.h
    NetworkData.h
    Qt_Remote.h
)

set(UI_FILES
    Qt_Remote.ui
)

set(QRC_FILES
    Qt_Remote.qrc
)

add_executable(${PROJECT_NAME}
    ${SRC_FILES}
    ${HDR_FILES}
    ${UI_FILES}
    ${QRC_FILES}
)

source_group("Source Files" FILES ${SRC_FILES})
source_group("Header Files" FILES ${HDR_FILES})
source_group("Forms" FILES ${UI_FILES})
source_group("Resources" FILES ${QRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Widgets
    Qt6::Network
)
if(MSVC)
    target_compile_options(Qt_Remote PRIVATE /utf-8)
endif()

# Windows 下可选：避免控制台窗口（如果你是纯界面程序）
if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        WIN32_EXECUTABLE ON
    )
endif()