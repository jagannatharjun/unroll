
add_library(core directorysystem.hpp
    filesystem.hpp filesystem.cpp
    archivesystem.hpp archivesystem.cpp
    hybriddirsystem.hpp hybriddirsystem.cpp
    directorysystemmodel.hpp directorysystemmodel.cpp
    directorysortmodel.hpp directorysortmodel.cpp
    filetype.hpp filetype.cpp
    filehistorydb.hpp filehistorydb.cpp
    filehistorydb_p.hpp
    asyncdirectorysystem.hpp asyncdirectorysystem.cpp
    dbutil.hpp
    persistenthash.hpp persistenthash.cpp
    asyncarchivefilereader.h asyncarchivefilereader.cpp
    asyncarchiveiodevice.h asyncarchiveiodevice.cpp)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Concurrent Sql)
# Find the Boost libraries, specifically the 'circular_buffer' component.
# The COMPONENTS keyword tells CMake which parts you need.
find_package(Boost 1.70 REQUIRED COMPONENTS system )

target_link_libraries(core PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Concurrent
    Qt${QT_VERSION_MAJOR}::Sql
    Boost::system)

target_link_libraries(core
    PUBLIC
        "C:/local/libarchive/lib/archive.lib"
)

target_include_directories(core
    PRIVATE
        "C:/local/libarchive/include"
)
