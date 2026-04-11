add_library(common STATIC
    NetworkData.h
    PacketStreamParser.h
    PacketStreamParser.cpp
    PacketStreamWriter.h
    PacketStreamWriter.cpp
)

target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(common PUBLIC
    Qt6::Network
)
