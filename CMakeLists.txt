cmake_minimum_required(VERSION 3.16)
project(jack_audio_examples)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH}" "${PROJECT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(include)

set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
find_package(Jack REQUIRED)

set(JACK_SINE_OUT_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/jack_sine_out.c
)
set(JACK_MIDI_IN_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/src/jack_midi_in.c
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
 PRIVATE
  ${JACK_LIBRARIES}
  m
)

add_executable(jack_midi_in)
target_sources(jack_midi_in
 PRIVATE
  ${JACK_MIDI_IN_SOURCES}
)
target_include_directories(jack_midi_in
 PRIVATE
  ${JACK_INCLUDE_DIRS}
)
target_link_libraries(jack_midi_in
 PRIVATE
  ${JACK_LIBRARIES}
  m
  Threads::Threads
)

