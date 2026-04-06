set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(server
    ClientSession.h
    ClientSession.cpp
    DeviceServer.cpp
    DeviceServer.h
    LockScreenWidget.cpp
  
)

target_include_directories(server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
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