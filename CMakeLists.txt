cmake_minimum_required(VERSION 3.22)

project(QY70Controller VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(QY70_BUILD_TESTS "Build protocol tests" ON)
option(QY70_LAUNCH_STANDALONE_AFTER_BUILD
       "Launch the macOS standalone app after a successful build"
       ON)

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    message(FATAL_ERROR "JUCE is missing. Run: git submodule update --init --recursive")
endif()

juce_add_plugin(QY70Controller
    COMPANY_NAME "Ilya Orange"
    BUNDLE_ID "com.ilyaorange.qy70controller"
    PLUGIN_MANUFACTURER_CODE IOrn
    PLUGIN_CODE Qy70
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "QY70 Controller"
    DESCRIPTION "MIDI editor and controller for Yamaha QY70"
    IS_SYNTH TRUE
    IS_MIDI_EFFECT FALSE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    NEEDS_AUDIO_INPUT FALSE
    NEEDS_AUDIO_OUTPUT TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3")

juce_generate_juce_header(QY70Controller)

if (APPLE AND QY70_LAUNCH_STANDALONE_AFTER_BUILD)
    add_custom_command(TARGET QY70Controller_Standalone POST_BUILD
        COMMAND /usr/bin/open "$<TARGET_BUNDLE_DIR:QY70Controller_Standalone>"
        COMMENT "Launching QY70 Controller standalone app")
endif()

target_sources(QY70Controller PRIVATE
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/QY70Midi.cpp
    Source/QY70Midi.h
    Source/VoiceCatalog.cpp
    Source/VoiceCatalog.h)

target_compile_definitions(QY70Controller PUBLIC
    JUCE_REPORT_APP_USAGE=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(QY70Controller PRIVATE
    juce::juce_audio_utils
    juce::juce_gui_extra
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

if (QY70_BUILD_TESTS)
    enable_testing()

    juce_add_console_app(QY70ProtocolTests
        PRODUCT_NAME "QY70 Protocol Tests")

    target_sources(QY70ProtocolTests PRIVATE
        Source/QY70Midi.cpp
        Source/QY70Midi.h
        Source/VoiceCatalog.cpp
        Source/VoiceCatalog.h
        Tests/QY70MidiTests.cpp)

    target_link_libraries(QY70ProtocolTests PRIVATE
        juce::juce_audio_basics
        juce::juce_core
        PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

    add_test(NAME QY70ProtocolTests COMMAND QY70ProtocolTests)
endif()
