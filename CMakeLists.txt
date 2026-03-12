cmake_minimum_required(VERSION 3.22)
project(FlambiAtmosphere VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  juce
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 8.0.3
)
FetchContent_MakeAvailable(juce)

juce_add_plugin(FlambiAtmosphere
    COMPANY_NAME "Flambi"
    PLUGIN_MANUFACTURER_CODE Flmb
    PLUGIN_CODE FAmb
    FORMATS VST3 Standalone
    PRODUCT_NAME "Flambi Atmosphere"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(FlambiAtmosphere)

target_sources(FlambiAtmosphere PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/SynthVoice.cpp
)

target_include_directories(FlambiAtmosphere PRIVATE Source)

target_compile_definitions(FlambiAtmosphere
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
)

target_link_libraries(FlambiAtmosphere
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
