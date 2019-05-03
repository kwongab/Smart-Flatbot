SET(execName ZED_Project)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
PROJECT(${execName})
 
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 OLD)
    cmake_policy(SET CMP0015 OLD)
endif(COMMAND cmake_policy)
 
SET(EXECUTABLE_OUTPUT_PATH ".")
 
SET(VERSION_REQ_CUDA "8.0")
 
IF(WIN32) # Windows
    if (CMAKE_CL_64) 
        SET(ZED_INCLUDE_DIRS $ENV{ZED_INCLUDE_DIRS})
        SET(ZED_LIBRARIES $ENV{ZED_LIBRARIES_64})
        SET(ZED_LIBRARY_DIR $ENV{ZED_LIBRARY_DIR})
    else()
        message(FATAL_ERROR "You've selected the 32bit version of ${CMAKE_GENERATOR}. \n Please delete the cache (file->Delete Cache) and use the 64bit version. (${CMAKE_GENERATOR} Win64)")
    endif()
ELSE() # Linux
    find_package(ZED 2 REQUIRED)
    SET(SPECIAL_OS_LIBS "pthread")
ENDIF(WIN32)
 
find_package(CUDA ${VERSION_REQ_CUDA} REQUIRED)
  
include_directories(${CUDA_INCLUDE_DIRS})
include_directories(${ZED_INCLUDE_DIRS})
#include_directories()
include_directories(/home/nvidia/Documents/ZED_example_get_image/)
 
link_directories(${ZED_LIBRARY_DIR})
link_directories(${CUDA_LIBRARY_DIRS})
set(SOURCES zedInit.cpp /home/nvidia/Documents/ZED_example_get_image/zedInit.h)
 
SET(SRC_FOLDER src)
FILE(GLOB_RECURSE SRC_FILES "${SRC_FOLDER}/*.cpp")  
find_package( Boost 1.40 REQUIRED COMPONENTS system filesystem)    
#add_executable( ZED_Project Jetson_ZED_ballTracker2.cpp )   
ADD_EXECUTABLE(${execName} "./main.cpp" ${SOURCES})
#ADD_EXECUTABLE(test ${SOURCES})

add_definitions(-std=c++0x -g -O3)

# Include opencv
if(WIN32)
  set(OpenCV_DIR "d:/libs/opencv-2.4.8/build")
else()
  set(OpenCV_DIR "/usr/lib/opencv")
endif()
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui)

target_link_libraries(${execName} ${OpenCV_LIBS})
 
# Add the required libraries for linking:
TARGET_LINK_LIBRARIES(${execName}
                        ${ZED_LIBRARIES}
                        ${SPECIAL_OS_LIBS}
                        ${CUDA_LIBRARIES} ${CUDA_npps_LIBRARY}
                        ${ZED_Project}    ${Boost_LIBRARIES})
#TARGET_LINK_LIBRARIES(test ${ZED_LIBRARIES})
# SET(CMAKE_CXX_FLAGS "`pkg-config --cflags --libs opencv`" )
