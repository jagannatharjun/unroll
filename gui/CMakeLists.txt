
set(SOURCES
        viewcontroller.hpp viewcontroller.cpp
        historystack.hpp historystack.cpp
	filebrowser.hpp filebrowser.cpp
	iconprovider.hpp iconprovider.cpp
        main.cpp
	qml/qml.qrc
	file-browser.rc
)


if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(file-browser
	MANUAL_FINALIZATION
	${SOURCES}
	preferences.hpp preferences.cpp
        pathhistorydb.hpp pathhistorydb.cpp
        directoryopener.hpp directoryopener.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET file-browser APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
	add_library(file-browser SHARED
	    ${PROJECT_SOURCES}
	)
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
	add_executable(file-browser
	  ${SOURCES}
        )
    endif()
endif()


find_package(Qt6 REQUIRED COMPONENTS Multimedia)

target_link_libraries(file-browser
  PRIVATE Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Quick
  Qt${QT_VERSION_MAJOR}::QuickControls2
  Qt${QT_VERSION_MAJOR}::Sql
  Qt${QT_VERSION_MAJOR}::StateMachine
  Qt${QT_VERSION_MAJOR}::Multimedia
)

target_link_libraries(file-browser
  PRIVATE core)

target_compile_definitions(file-browser PRIVATE QT_QML_DEBUG)

set_target_properties(file-browser PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)


if(QT_VERSION_MAJOR EQUAL 6)
    qt_import_qml_plugins(file-browser)
    qt_finalize_executable(file-browser)
endif()
