cmake_minimum_required (VERSION 3.18)

project(SimScanning)

if(MSVC)
    option(STATIC_WINDOWS_RUNTIME "Use static (MT/MTd) Windows runtime" ON)
    if(STATIC_WINDOWS_RUNTIME)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    else()
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    endif()
    set(CMAKE_CXX_FLAGS_RELEASE "/Z7")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF")

    set(CMAKE_CXX_FLAGS 
    "${CMAKE_CXX_FLAGS} 
    /Wv:18 
    /wd4201 /wd4310 /wd4505 /wd4127 /wd4146 
    /wd4189 /wd4324 /wd4706 /wd4100 /wd4702 
    /wd4244 /wd4245 /wd4267 /wd4305 /wd4756
    /wd4100 /wd4127 /wd4150 /wd4191 /wd4242 
    /wd4244 /wd4251 /wd4305 /wd4365 /wd4512 
    /wd4514 /wd4548 /wd4571 /wd4619 /wd4625
    /wd4626 /wd4628 /wd4668 /wd4710 /wd4711 
    /wd4738 /wd4820 /wd4946 /wd4986 /wd4819 
    /wd4996 /wd4267 /wd4018 /wd4028 /wd4056 
    /wd4101 /wd4334 /wd4477 /wd4700 /wd4723 ")
    add_definitions(-DNDEBUG)
    add_definitions(-DNOMINMAX)
    add_definitions(-D_CONSOLE)
    add_definitions(-D_USE_MATH_DEFINES)
endif()

find_package(assimp REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenCV CONFIG REQUIRED)
find_package(K4A CONFIG REQUIRED)

add_definitions     ( ${EIGEN3_DEFINITIONS} )
include_directories ( ${EIGEN3_INCLUDE_DIRS} )

set(ModelLoading ModelLoading.h ModelLoading.cpp)
set(Shader TexturedTriangleMesh.vertexshader TexturedTriangleMesh.fragmentshader)

add_executable(SimScanning "Source.cpp" ${ModelLoading} ${Shader})
target_link_libraries(SimScanning opencv_world assimp::assimp GLEW::GLEW glfw k4a::k4a)

source_group("ModelLoading" FILES ${ModelLoading})
source_group("Shader" FILES ${Shader})