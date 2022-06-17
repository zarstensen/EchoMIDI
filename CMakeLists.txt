if(MSVC)
	cmake_minimum_required (VERSION 3.21) # 3.21 -> has vs 2022
else()
	cmake_minimum_required (VERSION 3.0.0)
endif()

set(CMAKE_CXX_STANDARD 20)

project("EchoMIDI" VERSION 0.3.0 LANGUAGES CXX)

# place all predefined target projects into a filter
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(PREDEFINED_TARGETS_FOLDER "Targets")

option("${PROJECT_NAME}_CONAN_AUTO_INSTALL" OFF "Automaticly sets up conan directory and installs all the requirements in the conanfile.txt. Only works if conan is installed.")
option("${PROJECT_NAME}_GEN_DOCS" OFF "Generate docs using Doxygen. (requires Doxygen to be installed)")

set (SRC
	src/Echoer.cpp
	src/FocusHook.cpp
)

set (INCLUDE
	include/Echoer.h
	include/FocusHook.h
)

# Add source to this project's executable.
add_library ( ${PROJECT_NAME} 
	${INCLUDE} ${SRC}
)

# create __RELATIVE_FILE__ macro for log output

foreach(SRC_FILE ${SRC} ${INCLUDE})

	message(${SRC_FILE})

	get_property(COMP_DEFS SOURCE ${SRC_FILE} PROPERTY COMPILE_DEFINITIONS)

	get_filename_component(ABS_PATH ${SRC_FILE} ABSOLUTE ${CMAKE_CURRENT_SOURCE_DIR})
	get_filename_component(REL_ROOT ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)

	string(REPLACE ${REL_ROOT} "" REL_PATH ${ABS_PATH})

	list(APPEND COMP_DEFS "__RELATIVE_FILE__=\"${REL_PATH}\"")

	set_property(SOURCE ${SRC_FILE} PROPERTY COMPILE_DEFINITIONS ${COMP_DEFS})
endforeach()

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

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/EchoMIDIApp")

# doxygen and docs generation

if(${${PROJECT_NAME}_GEN_DOCS})
	find_package(Doxygen REQUIRED doxygen)

	set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/docs")
	set(DOXYGEN_CREATE_SUBDIRS YES)
	set(DOXYGEN_EXTRACT_ALL YES)
	set(DOXYGEN_SHOW_USED_FILES NO)
	set(DOXYGEN_RECURSIVE NO)
	
	set(DOXYGEN_GENERATE_HTML YES)
	
	set(DOXYGEN_DISABLE_INDEX YES)
	set(DOXYGEN_GENERATE_TREEVIEW YES)

	set(DOXYGEN_GENERATE_LATEX NO)

	doxygen_add_docs(${PROJECT_NAME}_DOCS
		"."
		"src/"
		"include/"
		ALL
	)

endif()

# conan autoinstall

if(${${PROJECT_NAME}_CONAN_AUTO_INSTALL})

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

# CURRENTLY UNUSED

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "${CMAKE_PROJECT_NAME}")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-installer")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}")
include(CPack)
