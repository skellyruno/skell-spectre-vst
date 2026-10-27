cmake_minimum_required(VERSION 3.22)
project(SkellSpectre VERSION 1.1.0 LANGUAGES C CXX)

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
    Source/DSP.h)
  target_compile_features(SkellSpectre PRIVATE cxx_std_17)
  target_compile_definitions(SkellSpectre PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_link_libraries(SkellSpectre PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_gui_extra)
endif()
