cmake_minimum_required(VERSION 3.22)
project(UniversalReference VERSION 0.1.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.10 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(UniversalReference
    COMPANY_NAME "One Sound Records"
    BUNDLE_ID "com.onesoundrecords.universalreference"
    PLUGIN_MANUFACTURER_CODE OsRc
    PLUGIN_CODE UnRf
    FORMATS AU VST3
    PRODUCT_NAME "Universal Reference"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(UniversalReference PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_include_directories(UniversalReference PRIVATE Source)
target_compile_definitions(UniversalReference PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(UniversalReference PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_gui_extra juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
enable_testing()
add_executable(DSPTests Tests/DSPTests.cpp)
target_include_directories(DSPTests PRIVATE Source)
add_test(NAME dsp COMMAND DSPTests)
