project("EchoMIDIApp" VERSION 0.4.0)

set(SRC
	src/EchoMIDI.cpp
)

set(INCLUDE
	include/EchoManager.h
	include/EchoMIDI.h
	include/SaveFile.h
)

add_executable(${PROJECT_NAME} WIN32 ${SRC} ${INCLUDE})

target_link_libraries(${PROJECT_NAME} EchoMIDI)

# handle packages


find_package(RapidJSON REQUIRED)

target_link_libraries(${PROJECT_NAME} RapidJSON::RapidJSON)

# set(wxBUILD_SHARED OFF CACHE BOOL "Build wxWidgets as a shared library" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/modules/wxWidgets")

target_link_libraries(${PROJECT_NAME} ${RapidJSON} wx::core wx::base)

# set warning level
target_compile_options(${PROJECT_NAME} PRIVATE /external:anglebrackets /external:W0)

