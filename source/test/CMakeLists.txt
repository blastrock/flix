add_executable(multitask multitask.cpp)
target_link_libraries(multitask flix)
target_include_directories(multitask PRIVATE ..)
make_flix_image(multitask)
