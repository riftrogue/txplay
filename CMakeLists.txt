cmake_minimum_required(VERSION 3.11)
project(txplay VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI
    GIT_TAG v7.0.3
)
FetchContent_MakeAvailable(ftxui)

# Common include root: all targets include from src/ so headers use
# project-root-relative paths (e.g. #include "audio/AudioEngine.hpp").
set(TXPLAY_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src)

# Audio Engine sources
set(AUDIO_SRCS
    src/audio/miniaudio_impl.cpp
    src/audio/AudioEngine.cpp
    src/audio/AudioDevice.cpp
    src/audio/DecoderThread.cpp
    src/audio/Analyzer.cpp
)

# Audio test runner executable
add_executable(audio_test_runner src/audio/audio_test_runner.cpp ${AUDIO_SRCS})
target_include_directories(audio_test_runner PRIVATE ${TXPLAY_INCLUDE_DIRS} third_party/miniaudio)
target_link_libraries(audio_test_runner PRIVATE m pthread dl)

# Library Source Files
set(LIBRARY_SRCS
    src/library/Library.cpp
    src/library/Scanner.cpp
)

# Library test runner executable
add_executable(library_test_runner src/library/library_test_runner.cpp ${LIBRARY_SRCS})
target_include_directories(library_test_runner PRIVATE ${TXPLAY_INCLUDE_DIRS})
target_link_libraries(library_test_runner PRIVATE m pthread dl)

# Application Source Files
set(APPLICATION_SRCS
    src/application/Application.cpp
)

# Application test runner executable
add_executable(application_test_runner
    src/application/application_test_runner.cpp
    ${APPLICATION_SRCS} ${LIBRARY_SRCS} ${AUDIO_SRCS}
)
target_include_directories(application_test_runner PRIVATE ${TXPLAY_INCLUDE_DIRS} third_party/miniaudio)
target_link_libraries(application_test_runner PRIVATE m pthread dl)

# UI Source Files (FTXUI-dependent; excluded from test runners)
set(UI_SRCS
    src/ui/TxplayUI.cpp
)

# Main executable
add_executable(txplay
    src/main.cpp
    src/config/Config.cpp
    ${APPLICATION_SRCS} ${LIBRARY_SRCS} ${AUDIO_SRCS} ${UI_SRCS}
)
target_include_directories(txplay PRIVATE ${TXPLAY_INCLUDE_DIRS} third_party/miniaudio)
target_link_libraries(txplay PRIVATE ftxui::screen ftxui::dom ftxui::component m pthread dl)
