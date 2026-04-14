# ColourBrickwallScream — Harmonic Saturator & Brickwall Limiter
# Native JUCE UI | Cross-platform: Windows (VST3), macOS (VST3/AU), Linux (VST3/LV2)

# ============================================
# PLATFORM-SPECIFIC CONFIGURATION
# ============================================
set(AU_MAIN_TYPE "")
set(LV2_URI "")

if(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
elseif(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
    set(AU_MAIN_TYPE kAudioUnitType_Effect)
elseif(UNIX)
    set(PLUGIN_FORMATS VST3 LV2 Standalone)
    set(LV2_URI "https://github.com/noizefield/audio-plugin-coder/ColourBrickwallScream")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

message(STATUS "ColourBrickwallScream: Building for ${PLUGIN_FORMATS}")

# ============================================
# PLUGIN DEFINITION
# ============================================
juce_add_plugin(ColourBrickwallScream
    COMPANY_NAME "Noizefield"
    PLUGIN_MANUFACTURER_CODE Nfld
    PLUGIN_CODE CbSc
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "ColourBrickwallScream"
    VST3_CATEGORIES Fx Dynamics Distortion
    AU_MAIN_TYPE ${AU_MAIN_TYPE}
    LV2URI ${LV2_URI}
)

# ============================================
# SOURCE FILES
# ============================================
target_sources(ColourBrickwallScream
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# ============================================
# LINK LIBRARIES
# ============================================
target_link_libraries(ColourBrickwallScream
    PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ============================================
# COMPILE DEFINITIONS
# ============================================
target_compile_definitions(ColourBrickwallScream
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        _USE_MATH_DEFINES=1
)

if(UNIX AND NOT APPLE)
    target_compile_definitions(ColourBrickwallScream
        PUBLIC
            JUCE_JACK=1
    )
endif()

# ============================================
# MACOS UNIVERSAL BINARY
# ============================================
# Handled via CMAKE_OSX_ARCHITECTURES at configure time
