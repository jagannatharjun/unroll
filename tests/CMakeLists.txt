enable_testing(true)

add_executable(test_filesystem test_filesystem.cpp)
add_test(NAME test_filesystem COMMAND test_filesystem)
target_link_libraries(test_filesystem PRIVATE core Qt${QT_VERSION_MAJOR}::Test)


add_executable(test_archivesystem test_archivesystem.cpp)
add_test(NAME test_archivesystem COMMAND test_archivesystem)
target_link_libraries(test_archivesystem PRIVATE core Qt${QT_VERSION_MAJOR}::Test)

