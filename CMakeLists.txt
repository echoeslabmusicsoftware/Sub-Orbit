cmake_minimum_required(VERSION 3.24)

project(SubOrbit VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set_property(GLOBAL PROPERTY USE_FOLDERS YES)

include(FetchContent)

FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.12
  GIT_SHALLOW TRUE
)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.7.1
  GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(juce Catch2)

option(SUB_ORBIT_ENABLE_PLUGINVAL "Add pluginval validation to the CTest suite." OFF)
set(SUB_ORBIT_PLUGINVAL_EXECUTABLE "" CACHE FILEPATH "Path to the pluginval executable.")
set(SUB_ORBIT_PLUGINVAL_STRICTNESS 5 CACHE STRING "pluginval strictness level.")
set(SUB_ORBIT_PLUGINVAL_SAMPLE_RATES "44100,48000,96000" CACHE STRING "Comma-separated sample rates for pluginval.")
set(SUB_ORBIT_PLUGINVAL_BLOCK_SIZES "1,64,512" CACHE STRING "Comma-separated block sizes for pluginval.")
set(SUB_ORBIT_PLUGINVAL_LOG_DIR "${CMAKE_CURRENT_BINARY_DIR}/pluginval-logs" CACHE PATH "Directory for pluginval logs.")

juce_add_binary_data(BinaryData
  SOURCES
    Resources/Fonts/PxPlus_IBM_VGA_9x16.ttf
    Resources/Brand/echoes-lab-eye-green.svg
    Resources/Brand/echoes-lab-eye-pink.svg
)

add_library(suborbit_core
  Source/SubOrbitParameterSpecs.h
  Source/SubOrbitTypes.h
  Source/SubOrbitParameters.h
  Source/ScopeDataFifo.h
  Source/DSP/QuadratureAllpassPair.h
  Source/DSP/QuadratureAllpassPair.cpp
  Source/DSP/LowBandAnalyzer.h
  Source/DSP/SidechainEnvelopeFollower.h
  Source/DSP/SubOrbitEngine.h
  Source/DSP/SubOrbitEngine.cpp
)

target_include_directories(suborbit_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Source
)

target_link_libraries(suborbit_core
  PUBLIC
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp
)

target_compile_definitions(suborbit_core
  PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

juce_add_plugin(SubOrbit
  COMPANY_NAME "Echoes Lab"
  BUNDLE_ID com.echoeslab.suborbit
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  PLUGIN_MANUFACTURER_CODE EchL
  PLUGIN_CODE SbOr
  FORMATS AU VST3
  PRODUCT_NAME "SUB ORBIT"
)

target_sources(SubOrbit
  PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp
    Source/UI/CRTLookAndFeel.h
    Source/UI/UIConstants.h
    Source/UI/CRTLookAndFeel.cpp
    Source/UI/FieldMonitorPanel.h
    Source/UI/FieldMonitorPanel.cpp
    Source/UI/OrbitPanel.h
    Source/UI/OrbitPanel.cpp
    Source/UI/SidechainPanel.h
    Source/UI/SidechainPanel.cpp
    Source/UI/TipsOverlay.h
    Source/UI/TipsOverlay.cpp
)

target_link_libraries(SubOrbit
  PRIVATE
    suborbit_core
    BinaryData
    juce::juce_audio_utils
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags
)

target_compile_definitions(SubOrbit
  PRIVATE
    JUCE_VST3_CAN_REPLACE_VST2=0
)

enable_testing()

add_executable(SubOrbitTests
  tests/SubOrbitEngineTests.cpp
  tests/QuadratureAllpassPairTests.cpp
  tests/SubOrbitStateTests.cpp
  tests/MonoCheckClickTests.cpp
  tests/ScopeDataFifoTests.cpp
  tests/LowBandAnalyzerTests.cpp
  tests/SidechainEnvelopeFollowerTests.cpp
)

target_link_libraries(SubOrbitTests
  PRIVATE
    SubOrbit
    BinaryData
    suborbit_core
    Catch2::Catch2WithMain
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
)

include(CTest)
include(Catch)
catch_discover_tests(SubOrbitTests)

if (SUB_ORBIT_ENABLE_PLUGINVAL)
  if (SUB_ORBIT_PLUGINVAL_EXECUTABLE)
    set(_sub_orbit_pluginval_program "${SUB_ORBIT_PLUGINVAL_EXECUTABLE}")
  else()
    find_program(_sub_orbit_pluginval_program pluginval DOC "pluginval executable")
  endif()

  if (NOT _sub_orbit_pluginval_program)
    message(FATAL_ERROR "SUB_ORBIT_ENABLE_PLUGINVAL is ON but pluginval was not found. Set SUB_ORBIT_PLUGINVAL_EXECUTABLE or install pluginval.")
  endif()

  file(MAKE_DIRECTORY "${SUB_ORBIT_PLUGINVAL_LOG_DIR}")

  function(sub_orbit_add_pluginval_test format_name target_name)
    if (NOT TARGET ${target_name})
      return()
    endif()

    get_target_property(_sub_orbit_plugin_artefact ${target_name} JUCE_PLUGIN_ARTEFACT_FILE)
    if (NOT _sub_orbit_plugin_artefact)
      message(FATAL_ERROR "Unable to locate the ${format_name} artefact for pluginval validation.")
    endif()

    add_test(
      NAME SubOrbit.pluginval.${format_name}
      COMMAND "${_sub_orbit_pluginval_program}"
              --strictness-level "${SUB_ORBIT_PLUGINVAL_STRICTNESS}"
              --sample-rates "${SUB_ORBIT_PLUGINVAL_SAMPLE_RATES}"
              --block-sizes "${SUB_ORBIT_PLUGINVAL_BLOCK_SIZES}"
              --output-dir "${SUB_ORBIT_PLUGINVAL_LOG_DIR}/${format_name}"
              --validate "${_sub_orbit_plugin_artefact}"
    )

    set_tests_properties(
      SubOrbit.pluginval.${format_name}
      PROPERTIES
        REQUIRED_FILES "${_sub_orbit_plugin_artefact}"
    )
  endfunction()

  sub_orbit_add_pluginval_test(VST3 SubOrbit_VST3)
endif()
