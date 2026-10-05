project(dear_imgui)

# Set path anchors
set(DEAR_IMGUI_DIR third_party)

# List build targets
add_library(${PROJECT_NAME} STATIC
    # Build Dear ImGui and SDL3/OpenGL backends
    ${DEAR_IMGUI_DIR}/imgui.cpp
    ${DEAR_IMGUI_DIR}/imgui_draw.cpp
    ${DEAR_IMGUI_DIR}/imgui_tables.cpp
    ${DEAR_IMGUI_DIR}/imgui_widgets.cpp
    ${DEAR_IMGUI_DIR}/backends/imgui_impl_sdl3.cpp
    ${DEAR_IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    ${DEAR_IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp
)

# List dependencies
target_link_libraries(${PROJECT_NAME} PRIVATE
    SDL3::Headers
)

# List include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${DEAR_IMGUI_DIR}
    ${DEAR_IMGUI_DIR}/backends
)

# Set language modes
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Set warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
endif()
