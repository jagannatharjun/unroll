
add_library(core directorysystem.hpp
    filesystem.hpp filesystem.cpp
    archivesystem.hpp archivesystem.cpp)

target_link_libraries(core PRIVATE Qt${QT_VERSION_MAJOR}::Core)

target_link_libraries(core
    PUBLIC
        "C:/local/libarchive/lib/archive.lib"
)

target_include_directories(core
    PRIVATE
        "C:/local/libarchive/include"
)
