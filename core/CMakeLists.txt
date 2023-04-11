
add_library(core directorysystem.hpp
    filesystem.hpp filesystem.cpp
    archivesystem.hpp archivesystem.cpp
    asyncdirsystem.hpp asyncdirsystem.cpp
    multidirsystem.hpp multidirsystem.cpp
    directorysystemmodel.hpp directorysystemmodel.cpp)

target_link_libraries(core PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Concurrent)

target_link_libraries(core
    PUBLIC
        "C:/local/libarchive/lib/archive.lib"
)

target_include_directories(core
    PRIVATE
        "C:/local/libarchive/include"
)
