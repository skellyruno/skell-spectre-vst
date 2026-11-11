cmake_minimum_required(VERSION 3.22)
project(SkellSpectre VERSION 1.8.3 LANGUAGES C CXX)

# Retains the SKELL quickstart's JUCE_PATH override and pinned JUCE 8.0.12,
# along with its unique manufacturer namespace and Windows VST3 workflow.
option(SKELL_BUILD_PLUGIN "Build the JUCE VST3 and standalone application" ON)
option(SKELL_BUILD_TESTS "Build the JUCE-free DSP regression/benchmark targets" ON)

if(SKELL_BUILD_TESTS)
  enable_testing()
  add_executable(SkellDSPTests tests/DSPTests.cpp)
  add_executable(SkellDSPBenchmark tests/DSPBenchmark.cpp)
  target_compile_features(SkellDSPTests PRIVATE cxx_std_17)
  target_compile_features(SkellDSPBenchmark PRIVATE cxx_std_17)
  add_test(NAME SkellDSPRegression COMMAND SkellDSPTests)
endif()

if(SKELL_BUILD_PLUGIN)
  include(FetchContent)
  if(DEFINED JUCE_PATH AND EXISTS "${JUCE_PATH}/CMakeLists.txt")
    add_subdirectory("${JUCE_PATH}" JUCE)
  else()
    FetchContent_Declare(JUCE
      GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG 8.0.12
      GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
  endif()

  # Header-only fetch: retain existing SkellSpectre DSP and JUCE targets.
  FetchContent_Declare(SkellGuidedTourHeaders
    GIT_REPOSITORY https://github.com/skellyruno/skell-vst-template.git
    GIT_TAG 3306ad495c93e062e39b8abbf079534b0b53da4e
    GIT_SHALLOW FALSE
    SOURCE_SUBDIR _guided_tour_headers_only)
  FetchContent_MakeAvailable(SkellGuidedTourHeaders)
  juce_add_plugin(SkellSpectre
    COMPANY_NAME "Skell Audio"
    BUNDLE_ID "audio.skell.spectre"
    PLUGIN_MANUFACTURER_CODE SkAu
    PLUGIN_CODE Spct
    FORMATS VST3 Standalone
    PRODUCT_NAME "SKELL SPECTRE"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

  juce_generate_juce_header(SkellSpectre)
  target_sources(SkellSpectre PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/DSP.h
    Source/DriveOversampler.h)
  target_compile_features(SkellSpectre PRIVATE cxx_std_17)
  target_compile_definitions(SkellSpectre PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_include_directories(SkellSpectre PRIVATE "${skellguidedtourheaders_SOURCE_DIR}/SkellCore/include")
  target_link_libraries(SkellSpectre PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra)

  # Exercise actual offscreen JUCE rendering and A/B state restore on Windows CI.
  if(WIN32 AND SKELL_BUILD_TESTS)
    juce_add_console_app(SkellUISmoke PRODUCT_NAME "Skell Spectre UI Smoke")
    target_sources(SkellUISmoke PRIVATE tests/UISmoke.cpp)
    target_include_directories(SkellUISmoke PRIVATE "${skellguidedtourheaders_SOURCE_DIR}/SkellCore/include")
    # JUCE's generated compatibility header belongs to each CMake target.
    # Linking the plugin core alone does not export its private JuceHeader.h.
    target_link_libraries(SkellUISmoke PRIVATE SkellSpectre
      juce::juce_audio_utils juce::juce_audio_processors
      juce::juce_dsp juce::juce_gui_extra)
    juce_generate_juce_header(SkellUISmoke)
    target_compile_features(SkellUISmoke PRIVATE cxx_std_17)
    add_test(NAME SkellUIRegression COMMAND SkellUISmoke)
  endif()

  # Visual regression harness: actual editor + processor, never included in the VST3.
  option(SKELL_BUILD_UI_RENDERER "Build SkellCore real JUCE editor screenshot executable" OFF)
  if(SKELL_BUILD_UI_RENDERER)
    FetchContent_Declare(SkellCoreDependency
      GIT_REPOSITORY https://github.com/skellyruno/skell-vst-template.git
      GIT_TAG 3306ad495c93e062e39b8abbf079534b0b53da4e
      SOURCE_SUBDIR SkellCore)
    FetchContent_MakeAvailable(SkellCoreDependency)
    add_executable(SkellSpectreUIRenderer tests/UIRenderer.cpp)
    target_include_directories(SkellSpectreUIRenderer PRIVATE
      "${CMAKE_CURRENT_BINARY_DIR}/SkellSpectre_artefacts/JuceLibraryCode"
      "${skellguidedtourheaders_SOURCE_DIR}/SkellCore/include")
    target_link_libraries(SkellSpectreUIRenderer PRIVATE
      SkellSpectre SkellCore::UIRenderer
      juce::juce_audio_utils juce::juce_audio_processors
      juce::juce_dsp juce::juce_gui_extra)
    target_compile_definitions(SkellSpectreUIRenderer PRIVATE
      JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
    target_compile_features(SkellSpectreUIRenderer PRIVATE cxx_std_17)
    add_test(NAME SkellRendererRegression
      COMMAND SkellSpectreUIRenderer --output "${CMAKE_CURRENT_BINARY_DIR}/ui-screenshots")
  endif()
endif()
