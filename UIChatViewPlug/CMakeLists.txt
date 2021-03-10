project(UIChatViewPlug)

include_directories(.)

# 打开Qt的Moc功能和ui文件QRC文件处理功能
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
if (APPLE)
    ADD_DEFINITIONS(-D_MACOS)
elseif (UNIX)
    ADD_DEFINITIONS(-D_LINUX)
else ()
    ADD_DEFINITIONS(-D_WINDOWS)
    ADD_DEFINITIONS(-DQ_NO_AMR)
    ADD_DEFINITIONS(-DUICHATVIEW_LIBRARY)
    #    find_package(OPENCORE-AMRWB opencore-amrwb)
endif ()

find_package(Qt5Widgets)
find_package(Qt5WebEngineWidgets)
find_package(Qt5MultimediaWidgets)

find_package(Qt5Multimedia)
find_package(Qt5Network)
find_package(Qt5Concurrent)
#
set(SOURCE_FILES
        MessageItems/CustomMenu.cpp
        MessageItems/CustomMenu.h
        MessageItems/EmojiMessItem.cpp
        MessageItems/EmojiMessItem.h
        MessageItems/FileRoundProgressBar.cpp
        MessageItems/FileRoundProgressBar.h
        MessageItems/FileSendReceiveMessItem.cpp
        MessageItems/FileSendReceiveMessItem.h
        MessageItems/ImageMessItem.cpp
        MessageItems/ImageMessItem.h
        MessageItems/MessageItemBase.cpp
        MessageItems/MessageItemBase.h
        MessageItems/TextBrowser.cpp
        MessageItems/TextBrowser.h
        MessageItems/TextMessItem.cpp
        MessageItems/TextMessItem.h
        MessageItems/MedalMind.h
        MessageItems/MedalMind.cpp
        ChatMainWgt.cpp
        ChatMainWgt.h
        chatview.json
        ChatViewMainPanel.cpp
        ChatViewMainPanel.h
        GroupChatSidebar.cpp
        GroupChatSidebar.h
        GroupMember.cpp
        GroupMember.h
        GroupTopic.cpp
        GroupTopic.h
        image1.qrc
        InputWgt.cpp
        InputWgt.h
        MessageManager.cpp
        MessageManager.h
        NativeChatStruct.h
        StatusWgt.cpp
        StatusWgt.h
        ToolWgt.cpp
        ToolWgt.h
        UIChatViewPlug.cpp
        UIChatViewPlug.h
        uichatviewplug_global.h
        MessageItems/TipMessageItem.cpp
        MessageItems/TipMessageItem.h
        GroupItemDelegate.cpp
        GroupItemDelegate.h
        AtMessageView.cpp
        AtMessageView.h
        blocks/AtBlock.h
        MessagePrompt.cpp
        MessagePrompt.h
        blocks/QuoteBlock.h
        NewMessageTip.cpp
        NewMessageTip.h
        AtMessageTip.cpp
        AtMessageTip.h
        MessageItems/CommonTrdInfoItem.cpp
        MessageItems/CommonTrdInfoItem.h
        blocks/ImageBlock.h
        blocks/block_define.h
        blocks/LinkBlock.h
        code/CodeShell.cpp
        code/CodeShell.h
        code/SendCodeWnd.cpp
        code/SendCodeWnd.h
        code/CodeEdit.cpp
        code/CodeEdit.h
        MessageItems/CodeItem.cpp
        MessageItems/CodeItem.h
        code/CodeShowWnd.cpp
        code/CodeShowWnd.h
        html.qrc
        MessageItems/VideoMessageItem.cpp
        MessageItems/VideoMessageItem.h
        VideoPlayWnd.cpp
        VideoPlayWnd.h
        MessageItems/AudioVideoItem.cpp
        MessageItems/AudioVideoItem.h
        ShareMessageFrm.cpp
        ShareMessageFrm.h
        SelectUserWnd.cpp
        SelectUserWnd.h
        QRcode/QRcode.cpp
        QRcode/QRcode.h
        QRcode/ScanQRcode.cpp
        QRcode/ScanQRcode.h
        QRcode/MakeQRcode.h
        QRcode/MakeQRcode.cpp
        QRcode/ScanMainFrm.cpp
        QRcode/ScanMainFrm.h
        QRcode/TipButton.cpp
        QRcode/TipButton.h
        MessageItems/VoiceMessageItem.cpp MessageItems/VoiceMessageItem.h
        MessageItems/NoteMessageItem.h MessageItems/NoteMessageItem.cpp
        MessageItems/NoticeMessageItem.h MessageItems/NoticeMessageItem.cpp
        MessageItems/SystemMessageItem.h MessageItems/SystemMessageItem.cpp
        MessageItems/MeetingRemindItem.h MessageItems/MeetingRemindItem.cpp
        GIFManager.cpp GIFManager.h
        ChatUtil.h ChatUtil.cpp
        MessageItems/HotLineTipItem.cpp
        MessageItems/HotLineTipItem.h
        MessageItems/HotLineAnswerItem.cpp
        MessageItems/HotLineAnswerItem.h
        search/LocalSearchMainWgt.cpp
        search/LocalSearchMainWgt.h
        search/SearchItemDelegate.cpp
        search/SearchItemDelegate.h
        search/ItemWgt.cpp
        search/ItemWgt.h
        search/MessageRecordManager.cpp
        search/MessageRecordManager.h
        search/MessageDelegate.cpp
        search/MessageDelegate.h
        search/SearchItemWgt.cpp
        search/SearchItemWgt.h
        MessageAnalysis.cpp
        MessageAnalysis.h
        search/FileRecordWnd.cpp
        search/FileRecordWnd.h
        NetImageLabel.cpp NetImageLabel.h
        search/SearchListDelegate.cpp
        search/SearchListDelegate.h
        search/MessageSortModel.cpp
        search/MessageSortModel.h
        search/SearchUserView.cpp
        search/SearchUserView.h
        search/ImageDelegate.cpp
        search/ImageDelegate.h
        ChatViewItem.cpp ChatViewItem.h
        ChatMainDelegate.cpp ChatMainDelegate.h
        ChatMainSoreModel.cpp ChatMainSoreModel.h
        DropWnd.cpp DropWnd.h
        GroupTopicEditor.cpp
        GroupTopicEditor.h)

set(LIBRARY_OUTPUT_PATH ../bin)
set(CMAKE_DEBUG_POSTFIX "d")

if (APPLE)
    set(MAC_SOURCE_FILE
            sound/PlayAudioSound.mm
            sound/PlayAudioSound.h
            MacNotification.mm
            MacNotification.h )
    #    set_property(SOURCE lib/http/asihttprequest/ASIDataCompressor.m APPEND_STRING PROPERTY COMPILE_FLAGS "-fno-objc-arc")
    FIND_LIBRARY(AudioToolbox_Library AudioToolbox)
    if (NOT AudioToolbox_Library)
        message(FATAL_ERROR "AudioToolbox not found")
    endif ()

    FIND_LIBRARY(CoreFoundation_Library CoreFoundation)
    if (NOT CoreFoundation_Library)
        message(FATAL_ERROR "CoreFoundation not found")
    endif ()

    FIND_LIBRARY(Foundation_Library Foundation)
    if (NOT Foundation_Library)
        message(FATAL_ERROR "Foundation not found")
    endif ()

    find_library(COCOA Cocoa)

    ADD_LIBRARY(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${MAC_SOURCE_FILE})

    target_link_libraries(${PROJECT_NAME}
            ${AudioToolbox_Library}
            ${CoreFoundation_Library}
            ${Foundation_Library}
            Qt5::Widgets
            Qt5::WebEngineWidgets
            Qt5::MultimediaWidgets
            Qt5::Network
            Qt5::Concurrent
            Platform
            QtUtil
            EventBus
            CustomUi
            Screenshot
            SUICom
            Emoticon
            quazip
            WebService
            qzxing
            ${COCOA}
            )

elseif (WIN32)
    include_directories(${PROJECT_SOURCE_DIR}/../Depends/amr/include/)
    set(WIN_SOURCE_FILE
            sound/amr/VoiceHelper.cpp
            sound/amr/VoiceHelper.h
            sound/amr/WavOutput.cpp
            sound/amr/WavOutput.h
            sound/amr/wavreader.c
            sound/amr/wavreader.h)

    if (PLATFORM_32BIT STREQUAL "ON")
        link_directories(Depends/amr/lib/x86)
    else()
        link_directories(Depends/amr/lib/x64)
    endif()

    ADD_LIBRARY(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${WIN_SOURCE_FILE} ${DOU_TU_SOURCE})

    target_link_libraries(${PROJECT_NAME}
            Qt5::Widgets
            Qt5::WebEngineWidgets
            Qt5::MultimediaWidgets
            Qt5::Network
            Qt5::Concurrent
            Qt5::Multimedia
            libssl
            libcrypto
            Platform
            QtUtil
            EventBus
            CustomUi
            Screenshot
            SUICom
            Emoticon
            quazip
            WebService
            qzxing
            libopencore-amrnb.a)
else ()
    ADD_LIBRARY(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${DOU_TU_SOURCE})

    target_link_libraries(${PROJECT_NAME}
            Qt5::Widgets
            Qt5::WebEngineWidgets
            Qt5::MultimediaWidgets
            Qt5::Network
            Qt5::Concurrent
            Platform
            QtUtil
            EventBus
            CustomUi
            Screenshot
            SUICom
            Emoticon
            quazip
            WebService
            qzxing)
endif ()


#添加链接库
