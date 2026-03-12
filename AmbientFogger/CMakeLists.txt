cmake_minimum_required(VERSION 3.22)
project(AmbientFogger VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Prefer local JUCE checkout if available to avoid network dependency.
if (EXISTS "${CMAKE_SOURCE_DIR}/../juce/CMakeLists.txt")
    add_subdirectory("${CMAKE_SOURCE_DIR}/../juce" juce)
else()
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(AmbientFogger
    COMPANY_NAME "OpenAI"
    BUNDLE_ID com.openai.ambientfogger
    PLUGIN_MANUFACTURER_CODE OaiA
    PLUGIN_CODE AmFg
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Instrument Synth
    PRODUCT_NAME "AmbientFogger")

juce_generate_juce_header(AmbientFogger)

target_sources(AmbientFogger PRIVATE
    src/PluginProcessor.cpp
    src/PluginProcessor.h
    src/PluginEditor.cpp
    src/PluginEditor.h)

target_compile_definitions(AmbientFogger
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(AmbientFogger
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
