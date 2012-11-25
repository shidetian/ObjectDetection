PROJECT(objectdetector) 

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

# Where to search for cmake scripts
SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Build subdirectories
ADD_SUBDIRECTORY(thirdparty/ImageLib)
ADD_SUBDIRECTORY(thirdparty/libsvm-3.14)

# Building the project 
ADD_EXECUTABLE(objectdetector 
    Feature.cpp 
	SupportVectorMachine.cpp Utils.cpp
	ImageDatabase.cpp PrecisionRecall.cpp
	main.cpp)

INCLUDE_DIRECTORIES(${JPEG_INCLUDE_DIR} thirdparty/)
TARGET_LINK_LIBRARIES(objectdetector image svm ${JPEG_LIBRARY})
