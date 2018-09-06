cmake_minimum_required (VERSION 3.0.2)
project (rpi-intercom)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

set(BOOST_MIN_VERSION "1.39.0")
find_package(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS system)

set (SOURCE_FILES
    config.cpp;
    http.cpp;
    httpconnection.cpp;
    httprequest.cpp;
    httpserver.cpp;
    io.cpp;
    main.cpp;
    requesthandler.cpp;
    udp.cpp;
)

set (SOURCE_FILES_CLIENT
    main.cpp;
    udp.cpp;
)

add_executable(rpi-intercom ${SOURCE_FILES})
target_link_libraries(rpi-intercom pthread boost_system)
target_compile_definitions(rpi-intercom PRIVATE RPI_INTERCOM_SERVER)

add_executable(rpi-intercom-client ${SOURCE_FILES_CLIENT})
target_compile_definitions(rpi-intercom-client PRIVATE RPI_INTERCOM_CLIENT)
target_link_libraries(rpi-intercom-client pthread boost_system)
