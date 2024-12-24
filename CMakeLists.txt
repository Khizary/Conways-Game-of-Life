cmake_minimum_required(VERSION 3.10.0)
project(conway VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 2.6.x
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SFML)

add_executable(conway main.cpp)
file(COPY ${CMAKE_SOURCE_DIR}/font2.ttf DESTINATION ${CMAKE_BINARY_DIR}/resources)
file(COPY ${CMAKE_SOURCE_DIR}/revive.wav DESTINATION ${CMAKE_BINARY_DIR}/resources)
file(COPY ${CMAKE_SOURCE_DIR}/nuke2.wav DESTINATION ${CMAKE_BINARY_DIR}/resources)
file(COPY ${CMAKE_SOURCE_DIR}/backgroud_music.ogg DESTINATION ${CMAKE_BINARY_DIR}/resources)


target_link_libraries(conway PRIVATE sfml-graphics sfml-window sfml-audio)
target_compile_features(conway PRIVATE cxx_std_17)



include(CTest)
enable_testing()


if(WIN32)
    add_custom_command(
        TARGET conway
        COMMENT "Copy OpenAL DLL"
        PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${SFML_SOURCE_DIR}/extlibs/bin/$<IF:$<EQUAL:${CMAKE_SIZEOF_VOID_P},8>,x64,x86>/openal32.dll $<TARGET_FILE_DIR:conway>
        VERBATIM)
endif()