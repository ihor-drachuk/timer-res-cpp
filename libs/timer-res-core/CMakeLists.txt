# timer-res-core - shared timer-resolution + autostart backend.
add_library(timer-res-core STATIC
    timer_backend.cpp
    autostart.cpp
)

target_compile_features(timer-res-core PUBLIC cxx_std_23)
target_include_directories(timer-res-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(timer-res-core PUBLIC
    UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_compile_options(timer-res-core PUBLIC
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive- /utf-8 /EHsc>)
target_link_libraries(timer-res-core PUBLIC advapi32 shell32 user32)
