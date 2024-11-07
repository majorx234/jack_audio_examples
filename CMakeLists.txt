cmake_minimum_required(VERSION 3.16)
project(jack_audio_examples)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

find_package(Jack REQUIRED)

set(JACK_SINE_OUT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/jack_sine_out.c
)

add_executable(jack_sine_out)
target_sources(jack_sine_out
 PRIVATE
  ${JACK_SINE_OUT_SOURCES}
)
target_include_directories(jack_sine_out
 PRIVATE
  ${JACK_INCLUDE_DIRS}
)
target_link_libraries(jack_sine_out
  ${JACK_LIBRARIES}
  m
)

