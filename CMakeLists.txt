cmake_minimum_required(VERSION 3.22)

project(Oblivium VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Static MSVC runtime: the plugin does not depend on the Visual C++ redistributable.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set(OBLIVIUM_JUCE_DIR "" CACHE PATH "Local JUCE checkout. Leave empty to download JUCE automatically.")
set(OBLIVIUM_JUCE_TAG "8.0.15" CACHE STRING "JUCE git tag used when downloading JUCE.")
set(OBLIVIUM_VST2_SDK_DIR "" CACHE PATH "Legacy VST2 SDK folder. When set, a VST2 build is produced as well.")
option(OBLIVIUM_COPY_AFTER_BUILD "Copy the plugin to the system plugin folder after building" OFF)

if(OBLIVIUM_JUCE_DIR)
    add_subdirectory("${OBLIVIUM_JUCE_DIR}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        ${OBLIVIUM_JUCE_TAG}
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(OBLIVIUM_FORMATS VST3 Standalone)
if(OBLIVIUM_VST2_SDK_DIR)
    juce_set_vst2_sdk_path("${OBLIVIUM_VST2_SDK_DIR}")
    list(APPEND OBLIVIUM_FORMATS VST)
endif()

juce_add_plugin(Oblivium
    COMPANY_NAME                "Myauti"
    PRODUCT_NAME                "Oblivium"
    VERSION                     ${PROJECT_VERSION}
    PLUGIN_MANUFACTURER_CODE    Myau
    PLUGIN_CODE                 Oblv
    FORMATS                     ${OBLIVIUM_FORMATS}
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             Fx Distortion
    VST2_CATEGORY               kPlugCategEffect
    COPY_PLUGIN_AFTER_BUILD     ${OBLIVIUM_COPY_AFTER_BUILD})

juce_add_binary_data(ObliviumAssets
    NAMESPACE ObliviumBinary
    SOURCES
        Resources/Fonts/CoveredByYourGrace-Regular.ttf
        Resources/Fonts/Montserrat-Medium.ttf
        Resources/Fonts/Montserrat-Bold.ttf)

target_sources(Oblivium PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Parameters.cpp
    Source/DSP/ObliviumEngine.cpp
    Source/UI/InputDeviceSelector.cpp
    Source/UI/ObliviumLookAndFeel.cpp
    Source/UI/Widgets.cpp)

target_compile_definitions(Oblivium PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Oblivium
    PRIVATE
        ObliviumAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
