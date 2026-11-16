cmake_minimum_required(VERSION 3.22)
project(MASTERING_PRO_2 VERSION 2.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(MASTERING_PRO_2
    COMPANY_NAME "Studio Tools"
    BUNDLE_ID "com.studiotools.masteringpro2"
    PLUGIN_MANUFACTURER_CODE StTl
    PLUGIN_CODE Mp20
    FORMATS VST3 Standalone
    PRODUCT_NAME "MASTERING PRO 2.0"
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE COPY_PLUGIN_AFTER_BUILD TRUE)

target_sources(MASTERING_PRO_2 PRIVATE
    Source/PluginProcessor.cpp Source/PluginProcessor.h
    Source/PluginEditor.cpp Source/PluginEditor.h)
target_compile_definitions(MASTERING_PRO_2 PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(MASTERING_PRO_2 PRIVATE
    juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
