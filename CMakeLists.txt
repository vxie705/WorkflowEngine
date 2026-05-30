cmake_minimum_required(VERSION 3.20)
project(WorkflowEngine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- Include directories ----
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/external)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/plugins)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/tests)

# ---- Core library (src/) ----
add_library(workflow_engine_core STATIC
    src/DataPacket.cpp
    src/DataBus.cpp
    src/ConsoleLogger.cpp
    src/WorkflowConfig.cpp
    src/WorkflowSchema.cpp
    src/WorkflowEngine.cpp
    src/PluginLoader.cpp
)

target_link_libraries(workflow_engine_core PUBLIC
    # nlohmann/json is header-only; linking not required
)

# ---- Plugin library (plugins/) ----
add_library(workflow_engine_plugins STATIC
    plugins/EchoCommand.cpp
    plugins/DelayCommand.cpp
    plugins/TransformCommand.cpp
)

target_link_libraries(workflow_engine_plugins PUBLIC
    workflow_engine_core
)

# ---- Tests ----
enable_testing()

# Helper: build a test binary and register it with CTest
function(add_workflow_test target_name source_file)
    add_executable(${target_name} ${source_file})
    target_link_libraries(${target_name} PRIVATE workflow_engine_core workflow_engine_plugins)
    target_force_whole_archive(${target_name} workflow_engine_plugins)
    add_test(NAME ${target_name} COMMAND ${target_name})
endfunction()

# Force /WHOLEARCHIVE for MSVC (or --whole-archive for GCC/Clang) so that
# static REGISTER_COMMAND globals in .lib are not discarded by the linker.
function(target_force_whole_archive target lib)
    if(MSVC)
        target_link_options(${target} PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:${lib}>")
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        target_link_options(${target} PRIVATE
            "LINKER:--whole-archive,$<TARGET_FILE:${lib}>,--no-whole-archive")
    endif()
endfunction()

add_workflow_test(test_result        tests/test_result.cpp)
add_workflow_test(test_datapacket    tests/test_datapacket.cpp)
add_workflow_test(test_engine        tests/test_workflow_engine.cpp)
add_workflow_test(test_observability tests/test_observability.cpp)

# ---- Demo binary ----
add_executable(workflow_demo src/main.cpp)
target_link_libraries(workflow_demo PRIVATE
    workflow_engine_core
    workflow_engine_plugins
)
target_force_whole_archive(workflow_demo workflow_engine_plugins)
