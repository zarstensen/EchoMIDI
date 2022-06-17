project("EchoMIDIApp" VERSION 0.3.1)

set(SRC
	src/EchoMIDI.cpp
)

add_executable(${PROJECT_NAME} ${SRC})

target_link_libraries(${PROJECT_NAME} EchoMIDI)
