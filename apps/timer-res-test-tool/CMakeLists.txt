# Console test tool for the backend (not shipped). Produces timer-res-test-tool.exe.
add_executable(timer-res-test-tool cli_main.cpp)
target_link_libraries(timer-res-test-tool PRIVATE timer-res-core)
