cmake_minimum_required(VERSION 3.22)

project(Bassman5F6ACircuitModel VERSION 1.0.1 LANGUAGES CXX)

option(BASSMAN_BUILD_PLUGIN "Build the JUCE plug-in and standalone app" ON)
option(BASSMAN_BUILD_TESTS "Build the circuit-model tests" ON)
set(JUCE_SOURCE_DIR "" CACHE PATH "Path to a local JUCE source tree")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(BassmanCircuit STATIC
    JUCE/Source/DSP/AmpCircuit.cpp
    JUCE/Source/DSP/AmpCircuit.h
    JUCE/Source/DSP/LinearAlgebra.h
    JUCE/Source/DSP/NodalDKModel.cpp
    JUCE/Source/DSP/NodalDKModel.h
    JUCE/Source/DSP/TriodeModel.cpp
    JUCE/Source/DSP/TriodeModel.h
    JUCE/Source/DSP/VolumeBrightFilter.cpp
    JUCE/Source/DSP/VolumeBrightFilter.h)

target_include_directories(BassmanCircuit PUBLIC JUCE/Source)
target_compile_features(BassmanCircuit PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(BassmanCircuit PRIVATE /W4 /permissive-)
else()
    target_compile_options(BassmanCircuit PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(BASSMAN_BUILD_PLUGIN)
    if(JUCE_SOURCE_DIR)
        if(NOT EXISTS "${JUCE_SOURCE_DIR}/CMakeLists.txt")
            message(FATAL_ERROR "JUCE_SOURCE_DIR does not point to a JUCE source tree: ${JUCE_SOURCE_DIR}")
        endif()
        add_subdirectory("${JUCE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/JUCE" EXCLUDE_FROM_ALL)
    else()
        include(FetchContent)
        FetchContent_Declare(
            JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG 8.0.8
            GIT_SHALLOW TRUE)
        FetchContent_MakeAvailable(JUCE)
    endif()

    juce_add_plugin(Bassman5F6ACircuitModel
        COMPANY_NAME "Circuit Sim Research"
        BUNDLE_ID "audio.circuitsim.bassman5f6a"
        PLUGIN_MANUFACTURER_CODE CSim
        PLUGIN_CODE Bm5A
        FORMATS VST3 Standalone
        PRODUCT_NAME "Bassman 5F6-A Circuit Model"
        DESCRIPTION "Circuit-derived 5F6-A preamp and tone-stack model"
        VERSION ${PROJECT_VERSION}
        IS_SYNTH FALSE
        NEEDS_MIDI_INPUT FALSE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        COPY_PLUGIN_AFTER_BUILD FALSE)

    juce_generate_juce_header(Bassman5F6ACircuitModel)

    target_sources(Bassman5F6ACircuitModel PRIVATE
        JUCE/Source/PluginProcessor.cpp
        JUCE/Source/PluginProcessor.h)

    target_link_libraries(Bassman5F6ACircuitModel PRIVATE
        BassmanCircuit
        juce::juce_audio_utils
        juce::juce_dsp
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

    target_compile_definitions(Bassman5F6ACircuitModel PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)
endif()

if(BASSMAN_BUILD_TESTS)
    include(CTest)
    add_executable(BassmanCircuitTests Tests/Automated/CircuitTests.cpp)
    target_link_libraries(BassmanCircuitTests PRIVATE BassmanCircuit)
    add_test(NAME BassmanCircuitTests COMMAND BassmanCircuitTests)

    if(BASSMAN_BUILD_PLUGIN)
        add_executable(BassmanPluginTests Tests/Plugin/ProcessorTests.cpp)
        target_include_directories(BassmanPluginTests PRIVATE
            JUCE/Source
            "${CMAKE_CURRENT_BINARY_DIR}/Bassman5F6ACircuitModel_artefacts/JuceLibraryCode")
        target_link_libraries(BassmanPluginTests PRIVATE
            Bassman5F6ACircuitModel
            juce::juce_audio_utils
            juce::juce_dsp)
        add_test(NAME BassmanPluginTests COMMAND BassmanPluginTests)
    endif()
endif()
