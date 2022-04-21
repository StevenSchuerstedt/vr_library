############################
#
# VR_Library
#
############################



cmake_minimum_required (VERSION 3.8)

# Fügen Sie der ausführbaren Datei dieses Projekts eine Quelle hinzu.
add_library (VR_Library "VR_Library.cpp")

target_link_libraries (VR_Library openvr_api)

# when building openvr directly, add openvr_api64

target_include_directories (VR_Library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_include_directories (VR_Library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/openvr/headers)

target_link_directories(VR_Library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/openvr/lib/win64)

#add_subdirectory ("openvr")
