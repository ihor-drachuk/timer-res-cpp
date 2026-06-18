# GUI front-end. WIN32 -> /SUBSYSTEM:WINDOWS + wWinMain. Produces timer-res.exe.
add_executable(timer-res WIN32
    gui_main.cpp
    timer-res.rc
)
target_include_directories(timer-res PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(timer-res PRIVATE timer-res-core)
