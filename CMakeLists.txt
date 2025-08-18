# nhal-esp32/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(WEST_WORKSPACE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../")
set(NHAL_INTERFACE_INCLUDE_PATH "${WEST_WORKSPACE_PATH}/hal-interface/include")

# Detect if we're building within ESP-IDF
if(DEFINED IDF_PATH)
    # ESP-IDF build - access IDF components
    find_package(idf REQUIRED)

    add_library(nhal-esp32 STATIC ${SOURCES})
    target_include_directories(nhal-esp32 PUBLIC
        include
        ${NHAL_INTERFACE_INCLUDE_PATH}
    )
    target_link_libraries(nhal-esp32 PUBLIC nhal-interface)

    # Link ESP-IDF components
    target_link_libraries(nhal-esp32 PRIVATE
        idf::driver
        idf::esp_timer
        idf::freertos
        idf::esp_common
    )
else()
    message(FATAL_ERROR "ESP-IDF not found")
endif()
