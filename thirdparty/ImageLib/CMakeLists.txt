CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

ADD_SUBDIRECTORY(thirdparty/JPEG)

ADD_LIBRARY(image STATIC
	Convert.cpp
	Convolve.cpp
	FileIO.cpp
	Image.cpp
	ImageProc.cpp
	Pyramid.cpp
	RefCntMem.cpp
	Transform.cpp
	WarpImage.cpp)	

INCLUDE_DIRECTORIES(thirdparty/)
TARGET_LINK_LIBRARIES(image jpegrw)
