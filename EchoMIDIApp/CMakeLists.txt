project("EchoMIDIApp" VERSION 0.4.2)


set(SRC
	src/EchoManager.cpp
	src/EchoMIDI.cpp
)

set(INCLUDE
	include/EchoManager.h
	include/EchoMIDI.h
)

add_executable(${PROJECT_NAME} WIN32 ${SRC} ${INCLUDE})

# handle packages

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/modules/wxWidgets")

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/modules/json")

set(JSON_BuildTests OFF CACHE INTERNAL "")

target_link_libraries(${PROJECT_NAME} PRIVATE EchoMIDI nlohmann_json::nlohmann_json wx::core wx::base)

# set warning level
target_compile_options(${PROJECT_NAME} PRIVATE /external:anglebrackets /external:W0)

