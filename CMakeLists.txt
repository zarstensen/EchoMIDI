if(MSVC)
	cmake_minimum_required (VERSION 3.21) # 3.21 -> has vs 2022
else()
	cmake_minimum_required (VERSION 3.0.0)
endif()

set(CMAKE_CXX_STANDARD 20)

project("EchoMIDI" VERSION 0.1.0 LANGUAGES CXX)

# place all predefined target projects into a filter
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "Targets")

option("EchoMIDI_CONAN_AUTO_INSTALL" OFF "Automaticly sets up conan directory and installs all the requirements in the conanfile.txt. Only works if conan is installed.")

set (SRC
	src/EchoMIDI.cpp
	src/Echoer.cpp
	src/FocusHook.cpp
)

set (INCLUDE
	include/Echoer.h
	include/FocusHook.h
)

# Add source to this project's executable.
add_executable ( ${PROJECT_NAME} 
	${INCLUDE} ${SRC}
)

if(MSVC)
  target_compile_options(${PROJECT_NAME}  PUBLIC "/ZI")
  target_link_options(${PROJECT_NAME}  PUBLIC "/INCREMENTAL")
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR} "include/" "ipp_src/")

# place files into the correct filters
source_group("src" FILES ${SRC})
source_group("include" FILES ${INCLUDE})

# handle packages

target_link_libraries(${PROJECT_NAME} winmm.lib)

# conan autoinstall

if(${EchoMIDI_CONAN_AUTO_INSTALL})

	message(${CMAKE_BINARY_DIR}/conan)

	# setup conan file inside build directory.
	execute_process(COMMAND mkdir "${CMAKE_BINARY_DIR}/conan" OUTPUT_QUIET ERROR_QUIET)

	message(STATUS "Running conan install command")
	execute_process(COMMAND conan install "${CMAKE_SOURCE_DIR}" --build missing WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/conan" RESULT_VARIABLE EchoMIDI_CONAN_INSTALL_RESULT)
	
	if(${EchoMIDI_CONAN_INSTALL_RESULT} EQUAL 0)
		message(STATUS "Finished conan install")
	elseif(${EchoMIDI_CONAN_INSTALL_RESULT})
		message(WARNING "Conan install finished with errors!")
	else()
		message(WARNING "Conan could not be found! Please install conan using pip (pip install conan)")
	endif()

endif()

include("${CMAKE_BINARY_DIR}/conan/conan_paths.cmake" OPTIONAL)

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-installer")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}")
include(CPack)
