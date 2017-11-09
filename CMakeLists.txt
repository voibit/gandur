cmake_minimum_required(VERSION 3.9.3)

project(gandur)

set(CMAKE_CXX_STANDARD 11)
set(DARKNET ../darknet)

find_package( OpenCV REQUIRED )
find_package( Boost COMPONENTS filesystem REQUIRED )

include_directories(${DARKNET}/include)
include_directories(${DARKNET}/src)
include_directories(. examples)

link_directories(${DARKNET})
set(CVB ${OpenCV_LIBS} ${Boost_LIBRARIES})
set(LIBS ${CVB} darknet)

add_executable(gandur examples/main.cpp gandur.cpp)
add_executable(videt examples/videt.cpp gandur.cpp)
add_executable(imgdet examples/imgdet.cpp gandur.cpp)
add_executable(small examples/small.cpp)
add_executable(trainlist examples/trainlist.cpp)
add_executable(valid examples/validate.cpp gandur.cpp)

target_link_libraries(gandur ${LIBS})
target_link_libraries(videt ${LIBS})
target_link_libraries(imgdet ${LIBS})
target_link_libraries(trainlist ${CVB})
target_link_libraries(small ${CVB})
target_link_libraries(valid ${LIBS})