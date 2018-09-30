cmake_minimum_required (VERSION 3.0.2)
project (rpi-intercom)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

# To fix "relocation R_X86_64_32 against `.rodata.str1.1' can not be used when making a PIE object" error on Unbuntu 18.04
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -no-pie")

set(BOOST_MIN_VERSION "1.39.0")
find_package(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS system)

set (SOURCE_FILES
    audio.cpp;
    config.cpp;
    http.cpp;
    httpconnection.cpp;
    httprequest.cpp;
    httpserver.cpp;
    io.cpp;
    pushsafer.cpp;
    main.cpp;
    requesthandler.cpp;
    udp.cpp;
)

add_executable(rpi-intercom ${SOURCE_FILES})
target_link_libraries(rpi-intercom pthread boost_system portaudio)