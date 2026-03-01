# Auto-apply patches to submodules
# Naming: <submodule>-<description>.patch -> libraries/<Submodule>

set(SUBMODULE_MAP
    "dynohook:DynoHook"
    "dyncall:dyncall"
    "funchook:funchook"
    "spdlog:spdlog"
    "asmjit:asmjit"
)

function(apply_submodule_patch PATCH_FILE)
    get_filename_component(PATCH_NAME ${PATCH_FILE} NAME_WE)
    string(REGEX MATCH "^([^-]+)" SUBMODULE_LOWER ${PATCH_NAME})

    set(SUBMODULE_DIR ${SUBMODULE_LOWER})
    foreach(ENTRY ${SUBMODULE_MAP})
        string(REGEX MATCH "^([^:]+):(.+)$" _ ${ENTRY})
        if(CMAKE_MATCH_1 STREQUAL ${SUBMODULE_LOWER})
            set(SUBMODULE_DIR ${CMAKE_MATCH_2})
            break()
        endif()
    endforeach()

    set(TARGET_DIR "${CMAKE_SOURCE_DIR}/libraries/${SUBMODULE_DIR}")
    if(NOT EXISTS ${TARGET_DIR})
        message(WARNING "Patch target not found: ${TARGET_DIR}")
        return()
    endif()

    execute_process(
        COMMAND git apply --check --reverse ${PATCH_FILE}
        WORKING_DIRECTORY ${TARGET_DIR}
        RESULT_VARIABLE IS_APPLIED
        OUTPUT_QUIET ERROR_QUIET
    )

    if(NOT IS_APPLIED EQUAL 0)
        execute_process(
            COMMAND git apply ${PATCH_FILE}
            WORKING_DIRECTORY ${TARGET_DIR}
            RESULT_VARIABLE RESULT
            OUTPUT_QUIET ERROR_QUIET
        )
        if(RESULT EQUAL 0)
            message(STATUS "Applied patch '${PATCH_NAME}' to ${SUBMODULE_DIR}")
        else()
            message(WARNING "Failed to apply patch '${PATCH_NAME}' to ${SUBMODULE_DIR}")
        endif()
    endif()
endfunction()

file(GLOB PATCH_FILES "${CMAKE_SOURCE_DIR}/patches/*.patch")
foreach(PATCH_FILE ${PATCH_FILES})
    apply_submodule_patch(${PATCH_FILE})
endforeach()
