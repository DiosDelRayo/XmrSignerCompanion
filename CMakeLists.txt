cmake_minimum_required(VERSION 3.5)

project(XmrSigner VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools Network Multimedia MultimediaWidgets Svg SvgWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools Network Multimedia MultimediaWidgets Svg SvgWidgets)
find_package(ZXing REQUIRED)

set(TS_FILES XmrSigner_en_US.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
	components.h
	components.cpp
        mainwindow.h
        mainwindow.ui
        qdotprogressindicator.h
        qdotprogressindicator.cpp
        qaddresslineedit.h
        qaddresslineedit.cpp
        qamountlineedit.h
        qamountlineedit.cpp
        qrcode/scanner/QrCodeScanWidget.h
        qrcode/scanner/QrCodeScanWidget.cpp
        qrcode/scanner/QrCodeScanWidget.ui
        qrcode/scanner/QrScanThread.h
        qrcode/scanner/QrScanThread.cpp
        bcur/ur-decoder.cpp
        bcur/ur-decoder.hpp
        ${TS_FILES}
)

add_definitions(-DZX_USE_UTF8)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(XmrSigner
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        qdotprogressindicator.h qdotprogressindicator.cpp
        qaddresslineedit.h qaddresslineedit.cpp
        qamountlineedit.h qamountlineedit.cpp
        icons.qrc
    )

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(XmrSigner SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(XmrSigner
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_link_libraries(XmrSigner PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::MultimediaWidgets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::SvgWidgets
    ZXing::ZXing
)

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/qrcode
    ${CMAKE_CURRENT_SOURCE_DIR}/qrcode/scanner
    ${CMAKE_CURRENT_SOURCE_DIR}/bcur
    ${QtMultimedia_INCLUDE_DIRS}
    ${QtMultimediaWidgets_INCLUDE_DIRS}
    ${ZXING_INCLUDE_DIRS}
)

target_compile_definitions(XmrSigner PUBLIC
        ${QtMultimedia_DEFINITIONS}
        ${QtMultimediaWidgets_DEFINITIONS}
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.xmrsigner.XmrSigner)
endif()
set_target_properties(XmrSigner PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS XmrSigner
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(XmrSigner)
endif()
