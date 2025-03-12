# Following juce-webview-tutorial

cmake_minimum_required(VERSION 3.22)

project(JuceGPUDrum)

set(CMAKE_CXX_STANDARD 20)

# Possible choice: CPM_SOURCE_CACHE environment variable if you want to share modules between projects.
# Otherwise, do as the JUCE8 WebView tutorial:
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)

include(cmake/cpm.cmake)

CPMAddPackage(
    NAME JUCE
    GIT_TAG 8.0.6
    VERSION 8.0.6
    GITHUB_REPOSITORY juce-framework/JUCE
    SOURCE_DIR ${LIB_DIR}/juce
)

# Windows WebViews
if (MSVC)
  message(STATUS "Setting up WebView dependencies")
  execute_process(COMMAND powershell -NoProfile -File scripts/DownloadWebView2.ps1
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  RESULT_VARIABLE DOWNLOAD_WEBVIEW2_RESULT)

  if (NOT DOWNLOAD_WEBVIEW2_RESULT EQUAL 0)
      message(FATAL_ERROR "Failed to download Microsoft.Web.WebView2 NuGet package. Result: ${DOWNLOAD_WEBVIEW2_RESULT}")
  endif()
endif()

# Enables strict warnings and treats warnings as errors.
# This needs to be set up only for your projects, not 3rd party
if (MSVC)
  set(CXX_PROJECT_WARNINGS "/W4;/WX;/wd4820;/wd4514")
else()
  set(CXX_PROJECT_WARNINGS "-Wall;-Werror;-Wextra;-Wpedantic")
endif()

# Adds all the targets configured in the "plugin" folder.
add_subdirectory(plugin)