enable_testing(true)

add_executable(test_filesystem test_filesystem.cpp)
add_test(NAME test_filesystem COMMAND test_filesystem)
target_link_libraries(test_filesystem PRIVATE core Qt${QT_VERSION_MAJOR}::Test)


add_executable(test_archivesystem test_archivesystem.cpp)
add_test(NAME test_archivesystem COMMAND test_archivesystem)
target_link_libraries(test_archivesystem PRIVATE core Qt${QT_VERSION_MAJOR}::Test)


add_executable(test_AsyncArchiveIODevice
    test_AsyncArchiveIODevice.cpp)
add_test(NAME test_AsyncArchiveIODevice COMMAND test_archivesystem)
target_link_libraries(test_AsyncArchiveIODevice PRIVATE core Qt${QT_VERSION_MAJOR}::Test)
target_link_libraries(test_AsyncArchiveIODevice
    PUBLIC
        "C:/local/libarchive/lib/archive.lib"
)

target_include_directories(test_AsyncArchiveIODevice
    PRIVATE
        "C:/local/libarchive/include"
)
add_executable(test_asyncarchivefilereader test_asyncarchivefilereader.cpp)
add_test(NAME test_asyncarchivefilereader COMMAND test_asyncarchivefilereader)
target_link_libraries(test_asyncarchivefilereader PRIVATE core Qt${QT_VERSION_MAJOR}::Test)
target_link_libraries(test_asyncarchivefilereader
    PUBLIC
        "C:/local/libarchive/lib/archive.lib"
)

target_include_directories(test_asyncarchivefilereader
    PRIVATE
        "C:/local/libarchive/include"
)

add_executable(test_ArchiveIODevice test_ArchiveIODevice.cpp)
add_test(NAME test_ArchiveIODevice COMMAND test_ArchiveIODevice)
target_link_libraries(test_ArchiveIODevice PRIVATE core Qt${QT_VERSION_MAJOR}::Test)
target_link_libraries(test_ArchiveIODevice
    PUBLIC
        "C:/local/libarchive/lib/archive.lib"
)

target_include_directories(test_ArchiveIODevice
    PRIVATE
        "C:/local/libarchive/include"
)



add_executable(test_directorysystemmodel test_directorysystemmodel.cpp)
add_test(NAME test_directorysystemmodel COMMAND test_directorysystemmodel)
target_link_libraries(test_directorysystemmodel PRIVATE core Qt${QT_VERSION_MAJOR}::Test)


