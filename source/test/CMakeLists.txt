add_executable(multitask multitask.cpp)
target_link_libraries(multitask flix)
target_include_directories(multitask PRIVATE ..)
make_flix_image(multitask TEST)

add_executable(mutex mutex.cpp)
target_link_libraries(mutex flix)
target_include_directories(mutex PRIVATE ..)
make_flix_image(mutex TEST)

add_executable(alloc alloc.cpp)
target_link_libraries(alloc flix)
target_include_directories(alloc PRIVATE ..)
make_flix_image(alloc TEST)
