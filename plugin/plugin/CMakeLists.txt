cmake_minimum_required(VERSION 3.22)

# Version is needed by JUCE.
project(DrumGpu VERSION 0.1.0)

# Adds a plugin target (that's basically what the Projucer does).
juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME travissk
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE TMSC
    PLUGIN_CODE DGPU

    FORMATS Standalone VST3 # AU
    JUCE_COPY_PLUGIN_AFTER_BUILD TRUE

    PRODUCT_NAME "drum.gpu"
    NEEDS_WEBVIEW2 TRUE  # Windows
)

get_target_property(PRODUCT_NAME ${PROJECT_NAME} JUCE_PRODUCT_NAME)
get_target_property(COMPANY_NAME ${PROJECT_NAME} JUCE_COMPANY_NAME)
target_compile_definitions(${PROJECT_NAME} PRIVATE
  JUCE_PRODUCT_NAME="${PRODUCT_NAME}"
  JUCE_COMPANY_NAME="${COMPANY_NAME}"
  JUCE_PRODUCT_VERSION="${PROJECT_VERSION}")

# Native code sources.
set(SOURCES
        source/ModeLoader.cpp
        source/PluginEditor.cpp
        source/PluginProcessor.cpp)
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/JuceGPUDrum")

target_sources(${PROJECT_NAME}
    PRIVATE
        ${SOURCES}
        ${INCLUDE_DIR}/ModeLoader.h
        ${INCLUDE_DIR}/PluginEditor.h
        ${INCLUDE_DIR}/PluginProcessor.h
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${JUCE_MODULES_DIR})

#### WebUI Sources

# Included directly from sibling directory for easiest build-ability and debug-ability in devtools.
# For production, consider looking at the included npm commands for minifying etc.; they're in the webui directory.
set(WEBVIEW_FILES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../webui/src")
file(COPY "${JUCE_MODULES_DIR}/juce_gui_extra/native/javascript/" DESTINATION "${WEBVIEW_FILES_SOURCE_DIR}/js/juce/")

set(WEBVIEW_FILES_ZIP_NAME "webview_files.zip")
set(TARGET_WEBVIEW_FILES_ZIP_PATH "${CMAKE_BINARY_DIR}/${WEBVIEW_FILES_ZIP_NAME}")
get_filename_component(PUBLIC_PATH "${WEBVIEW_FILES_SOURCE_DIR}" ABSOLUTE)

file(
  ARCHIVE_CREATE
  OUTPUT
  "${TARGET_WEBVIEW_FILES_ZIP_PATH}"
  PATHS
  "${PUBLIC_PATH}"
  FORMAT
  zip
  VERBOSE
)

juce_add_binary_data(WebViewFiles
    HEADER_NAME WebViewFiles.h
    NAMESPACE webview_files
    SOURCES ${TARGET_WEBVIEW_FILES_ZIP_PATH}
)


#### JUCE Modules
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        WebViewFiles
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1  # Use WebView2 on Windows
)

set_source_files_properties(${SOURCES} PROPERTIES COMPILE_OPTIONS "${CXX_PROJECT_WARNINGS}")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/..)  # For Visual Studio
