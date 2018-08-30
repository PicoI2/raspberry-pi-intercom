cmake_minimum_required (VERSION 3.0.2)
project (rpi-intercom)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_BUILD_TYPE Debug)

set(BOOST_MIN_VERSION "1.39.0")
find_package(Boost ${BOOST_MIN_VERSION} REQUIRED COMPONENTS system)

set (SOURCE_FILES
   main.cpp;
   doorbell.cpp;
   udplisten.cpp;
)

add_executable(rpi-intercom ${SOURCE_FILES})

target_link_libraries(rpi-intercom pthread boost_system)
