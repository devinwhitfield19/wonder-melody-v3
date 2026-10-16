cmake_minimum_required(VERSION 3.22)
project(WonderMelodyGenerator VERSION 0.2.0)
if(NOT DEFINED JUCE_DIR)
  message(FATAL_ERROR "Set JUCE_DIR to your local JUCE checkout, e.g. -DJUCE_DIR=C:/JUCE")
endif()
add_subdirectory("${JUCE_DIR}" JUCE)
juce_add_plugin(WonderMelodyGenerator
  COMPANY_NAME "Wonder Audio"
  BUNDLE_ID "audio.wonder.melodygenerator"
  PLUGIN_MANUFACTURER_CODE Wndr
  PLUGIN_CODE WmG2
  FORMATS VST3 Standalone
  PRODUCT_NAME "Wonder Melody Generator"
  IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT TRUE
  IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS TRUE
  COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(WonderMelodyGenerator PRIVATE
  Source/PluginProcessor.cpp Source/PluginProcessor.h
  Source/PluginEditor.cpp Source/PluginEditor.h)
  juce_generate_juce_header(WonderMelodyGenerator)
target_compile_features(WonderMelodyGenerator PRIVATE cxx_std_17)
target_compile_definitions(WonderMelodyGenerator PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(WonderMelodyGenerator PRIVATE
  juce::juce_audio_utils juce::juce_audio_processors juce::juce_dsp
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)
