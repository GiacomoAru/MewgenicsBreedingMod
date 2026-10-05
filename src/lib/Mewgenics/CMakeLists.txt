# just the SDL symbols that Mewgenics.exe exports
project(Mewgenics)

# List build targets
add_custom_target(${PROJECT_NAME} ALL DEPENDS
    ${CMAKE_CURRENT_BINARY_DIR}/Mewgenics_exports.lib
)
add_library(Mewgenics_exports INTERFACE)

# List dependencies
target_link_libraries(Mewgenics_exports INTERFACE
    ${CMAKE_CURRENT_BINARY_DIR}/Mewgenics_exports.lib
)
add_dependencies(Mewgenics_exports
    ${PROJECT_NAME}
)

# recipe to have CMake generate an import lib from a def
# https://gitlab.kitware.com/cmake/cmake/-/work_items/24511
if(MSVC)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Mewgenics_exports.lib
        COMMAND ${CMAKE_AR}
                /def:${CMAKE_CURRENT_SOURCE_DIR}/Mewgenics.def
                /out:${CMAKE_CURRENT_BINARY_DIR}/Mewgenics_exports.lib
                ${CMAKE_STATIC_LINKER_FLAGS}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Mewgenics.def
    )
elseif(MINGW)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Mewgenics_exports.lib
        COMMAND dlltool
                -d ${CMAKE_CURRENT_SOURCE_DIR}/Mewgenics.def
                -l ${CMAKE_CURRENT_BINARY_DIR}/Mewgenics_exports.lib
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/Mewgenics.def
    )
endif()
