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

# TagLib — native audio metadata (ID3, Vorbis comments, FLAC, WAV, etc.)
# Fetched as a static library; examples, bindings, and tests are disabled
# to keep the dependency footprint minimal.
set(BUILD_SHARED_LIBS      OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES         OFF CACHE BOOL "" FORCE)
set(BUILD_BINDINGS         OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING          OFF CACHE BOOL "" FORCE)
FetchContent_Declare(taglib
    GIT_REPOSITORY https://github.com/taglib/taglib.git
    GIT_TAG        v2.0.2
)
FetchContent_MakeAvailable(taglib)

# Common include root: all targets include from src/ so headers use
# project-root-relative paths (e.g. #include "audio/AudioEngine.hpp").
set(TXPLAY_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src)

# TagLib doesn't export BUILD_INTERFACE include paths, so we add them manually
set(TAGLIB_INCLUDE_DIRS 
    ${taglib_SOURCE_DIR}/taglib
    ${taglib_SOURCE_DIR}/taglib/toolkit
    ${taglib_BINARY_DIR}/taglib
)

# Config Source Files (shared by all targets)
set(CONFIG_SRCS
    src/config/Config.cpp
)

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
    src/library/MetadataReader.cpp
)

# Library test runner executable
add_executable(library_test_runner src/library/library_test_runner.cpp ${LIBRARY_SRCS})
target_include_directories(library_test_runner PRIVATE ${TXPLAY_INCLUDE_DIRS} ${TAGLIB_INCLUDE_DIRS})
target_link_libraries(library_test_runner PRIVATE tag m pthread dl)

# Config test runner executable
add_executable(config_test_runner
    src/config/config_test_runner.cpp
    ${CONFIG_SRCS}
)
target_include_directories(config_test_runner PRIVATE ${TXPLAY_INCLUDE_DIRS})
target_link_libraries(config_test_runner PRIVATE m pthread dl)

# Application Source Files
set(APPLICATION_SRCS
    src/application/Application.cpp
)

# Application test runner executable
add_executable(application_test_runner
    src/application/application_test_runner.cpp
    ${APPLICATION_SRCS} ${LIBRARY_SRCS} ${AUDIO_SRCS} ${CONFIG_SRCS}
)
target_include_directories(application_test_runner PRIVATE ${TXPLAY_INCLUDE_DIRS} third_party/miniaudio ${TAGLIB_INCLUDE_DIRS})
target_link_libraries(application_test_runner PRIVATE tag m pthread dl)

# UI Source Files (FTXUI-dependent; excluded from test runners)
set(UI_SRCS
    src/ui/TxplayUI.cpp
    src/ui/Visualizer.cpp
    src/ui/InputAdapter.cpp
)

# InputAdapter test runner (requires FTXUI to construct Event values)
add_executable(input_adapter_test_runner
    src/ui/input_adapter_test_runner.cpp
    src/ui/InputAdapter.cpp
)
target_include_directories(input_adapter_test_runner PRIVATE ${TXPLAY_INCLUDE_DIRS})
target_link_libraries(input_adapter_test_runner PRIVATE ftxui::screen ftxui::dom ftxui::component m pthread dl)

# UI utility test runner (pure C++17, no FTXUI required)
add_executable(ui_util_test_runner src/ui/ui_util_test_runner.cpp)
target_include_directories(ui_util_test_runner PRIVATE ${TXPLAY_INCLUDE_DIRS})
target_link_libraries(ui_util_test_runner PRIVATE m pthread dl)

# Main executable
add_executable(txplay
    src/main.cpp
    ${CONFIG_SRCS} ${APPLICATION_SRCS} ${LIBRARY_SRCS} ${AUDIO_SRCS} ${UI_SRCS}
)
target_include_directories(txplay PRIVATE ${TXPLAY_INCLUDE_DIRS} third_party/miniaudio ${TAGLIB_INCLUDE_DIRS})
target_link_libraries(txplay PRIVATE tag ftxui::screen ftxui::dom ftxui::component m pthread dl)

# Layout preview (offline visual inspection, not shipped)
add_executable(layout_preview src/ui/layout_preview.cpp)
target_include_directories(layout_preview PRIVATE ${TXPLAY_INCLUDE_DIRS})
target_link_libraries(layout_preview PRIVATE ftxui::screen ftxui::dom m pthread dl)
