project(eyedetect)
cmake_minimum_required(VERSION 3.5)

IF(CMAKE_BUILD_TYPE MATCHES Release)
    message("Release mode")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
ENDIF()

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    message("Debug mode")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall")
ENDIF()


# In case cmake didn't find OpenCV set variable OpenCV_DIR correctly
set(OpenCV_DIR ~/software/opencv/installed/3.1.0/share/OpenCV)
find_package(OpenCV REQUIRED)

set(SOURCES
	eyedetector.h
	main.cpp
) # SOURCES


add_executable(${CMAKE_PROJECT_NAME} 	${SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME} ${OpenCV_LIBS})
