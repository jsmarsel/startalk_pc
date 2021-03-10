project(StarTalk)

include_directories(.)
# 打开Qt的Moc功能和ui文件QRC文件处理功能
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Widgets Gui Qml Quick QuickWidgets Network Concurrent QuickControls2 REQUIRED)
include_directories(../breakpad)

if (APPLE)
    ADD_DEFINITIONS(-D_MACOS)
    set(PLATFORM_SOURCE
            HotKey/qhotkey_mac.cpp
            MacApp.h
            MacApp.mm)
    link_libraries("-framework Carbon")
    link_libraries("-framework AppKit")
    link_libraries("-framework AVFoundation")

elseif (UNIX)
    ADD_DEFINITIONS(-D_LINUX)
    find_package(Qt5X11Extras)
    find_package(X11)
    link_libraries(Qt5::X11Extras)
    link_libraries(X11)
    set(PLATFORM_SOURCE
            HotKey/qhotkey_x11.cpp)
else ()
    ADD_DEFINITIONS(-D_WINDOWS)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup")
    set(PLATFORM_SOURCE
            HotKey/qhotkey_win.cpp)

    file(GLOB SourceFiles *.cpp icon/Startalk.rc)
endif ()


set(SOURCE_FILES
        ${SourceFiles}
        main.cpp
        MainWindow.cpp
        MainWindow.h
        MessageManager.cpp
        MessageManager.h
        QTalk.qrc
        QTalkApp.cpp
        QTalkApp.h
        QtStruct.h
        style.qrc
        SystemTray.cpp
        SystemTray.h
        HotKey/qhotkey.cpp
        HotKey/qhotkey.h
        HotKey/qhotkey_p.h
        HotKey/QHotkey
        PlugManager.cpp
        PlugManager.h
        StyleManager.cpp
        StyleManager.h
        GlobalManager.cpp
        GlobalManager.h
        NoOperationThread.cpp
        NoOperationThread.h
        LocalServer.cpp
        LocalServer.h 
        SystemTrayPopWnd.cpp 
        SystemTrayPopWnd.h
        ProcessInfo.h
        ProcessInfo.cpp
        qml/qml.qrc
        qml/QmlView.cpp
        qml/QmlView.h
        qml/PkgDownload.cpp
        qml/PkgDownload.h)

set(EXECUTABLE_OUTPUT_PATH ../bin)
set(CMAKE_DEBUG_POSTFIX "d")
IF (CMAKE_BUILD_TYPE MATCHES "Debug")
    ADD_DEFINITIONS(-D_DEBUG)
endif ()


add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${PLATFORM_SOURCE})
IF (WIN32)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:windows")
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:windows")
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:windows")
    SET_TARGET_PROPERTIES(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:windows")
ENDIF (WIN32)

    target_link_libraries(${PROJECT_NAME}
            Qt5::Core
            Qt5::Widgets
            Qt5::Network
            Qt5::Concurrent
            Qt5::Qml
            Qt5::Quick
            Qt5::QuickWidgets
            Qt5::QuickControls2
            CustomUi
            QtUtil
            EventBus
            Platform
            LogicManager
            SUICom
            quazip
            breakpad)

#添加链接库
if (WIN32)
    target_link_libraries(${PROJECT_NAME} libcurl)
else ()
    target_link_libraries(${PROJECT_NAME} pthread)
endif ()


if (APPLE)
    if (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
        set_target_properties(${PROJECT_NAME} PROPERTIES
                MACOSX_BUNDLE TRUE
                )
        set_property(TARGET ${PROJECT_NAME} PROPERTY MACOSX_BUNDLE_ICON_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.icos)
        set_property(TARGET ${PROJECT_NAME} PROPERTY MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/startalk.info.plist)
        install(TARGETS ${PROJECT_NAME} DESTINATION ${PROJECT_NAME}.app)
    endif ()
endif ()
