cmake_minimum_required(VERSION 3.9.3)

project(gandur)

set(CMAKE_CXX_STANDARD 11)

set(DARKNET ../darknet) 
set(CUDA /opt/cuda) 		#Arch cuda dir
#set(CUDA /usr/local/cuda)	#Ubuntu cuda dir

find_package( OpenCV REQUIRED )
find_package( Boost COMPONENTS filesystem REQUIRED )

include_directories(${DARKNET}/include)
include_directories(${DARKNET}/src)
include_directories(. examples)

<<<<<<< HEAD
# Remove these three lines if darknet lib was compiled without gpu
add_definitions(-DGPU)
add_definitions(-DCUDNN)
include_directories(${CUDA}/include)
=======
# Remove these three lines if darknet lib is compiled without gpu
# add_definitions(-DGPU)
# add_definitions(-DCUDNN)
# include_directories(${CUDA}/include)
>>>>>>> f5d0cb6810146b34f3c205d1e5fc1723f1ce0c30

link_directories(${DARKNET})
set(CVB ${OpenCV_LIBS} ${Boost_LIBRARIES})
set(ALL ${CVB} darknet)

add_executable(gandur examples/main.cpp gandur.cpp)
add_executable(videt examples/videt.cpp gandur.cpp)
add_executable(imgdet examples/imgdet.cpp gandur.cpp)
add_executable(small examples/small.cpp)
add_executable(trainlist examples/trainlist.cpp)
add_executable(valid examples/validate.cpp gandur.cpp)

add_executable(valid1 examples/validate1.cpp gandur.cpp)

add_library(gandr SHARED gandur.cpp)

target_link_libraries(gandur ${ALL})
target_link_libraries(videt ${ALL})
target_link_libraries(imgdet ${ALL})
target_link_libraries(trainlist ${CVB})
target_link_libraries(small ${CVB})
target_link_libraries(valid ${ALL})
target_link_libraries(valid1 ${ALL})
target_link_libraries(gandr ${ALL})