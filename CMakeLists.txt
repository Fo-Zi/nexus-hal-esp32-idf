# hal-esp32/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

set(WEST_WORKSPACE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../")
set(HAL_INTERFACE_INCLUDE_PATH "${WEST_WORKSPACE_PATH}/hal-interface/include")

# Detect if we're building within ESP-IDF
if(DEFINED IDF_PATH)
    # ESP-IDF build - access IDF components
    find_package(idf REQUIRED)

    add_library(hal-esp32 STATIC ${SOURCES})
    target_include_directories(hal-esp32 PUBLIC
        include
        ${HAL_INTERFACE_INCLUDE_PATH}
    )
    target_link_libraries(hal-esp32 PUBLIC hal-interface)

    # Link ESP-IDF components
    target_link_libraries(hal-esp32 PRIVATE
        idf::driver
        idf::esp_timer
        idf::freertos
        idf::esp_common
    )
else()
    message(FATAL_ERROR "ESP-IDF not found")
endif()
