cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # build/compile_commands.json

project(BComp LANGUAGES CXX)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS "src/*.cpp")

add_executable(b ${SRC})

target_include_directories(b PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

if(MSVC)
  target_compile_options(b PRIVATE /W4 /nologo)
else()
  target_compile_options(b PRIVATE -Wall -Wextra -Wno-missing-field-initializers -Wno-unused-function -Werror=return-type)
endif()
