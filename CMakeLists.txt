cmake_minimum_required(VERSION 3.16)
project(sound_recording LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(APP_SOURCES
    main.cpp
        Core/Src/Crc32Mpeg2.cpp
        Core/Src/Protocol.cpp
        Core/Src/FrameParser.cpp
        Core/Src/WavWriter.cpp
        Core/Src/PacketLog.cpp
        Core/Src/SerialLink.cpp
        Core/Src/RecordSession.cpp
    App/ui/MainWindow.cpp
)

set(APP_HEADERS
    App/ui/MainWindow.h
    Core/Inc/Crc32Mpeg2.h
    Core/Inc/FrameParser.h
    Core/Inc/PacketLog.h
    Core/Inc/Protocol.h
    Core/Inc/RecordSession.h
    Core/Inc/SerialLink.h
    Core/Inc/WavWriter.h
)

add_executable(sound_recording ${APP_SOURCES} ${APP_HEADERS})
target_link_libraries(sound_recording PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets)

enable_testing()
set(CORE_TEST_HEADERS
    Core/Inc/Crc32Mpeg2.h
    Core/Inc/Protocol.h
    Core/Inc/WavWriter.h
)

add_executable(core_tests
    tests/core_tests.cpp
        Core/Src/Crc32Mpeg2.cpp
        Core/Src/FrameParser.cpp
        Core/Src/Protocol.cpp
        Core/Src/WavWriter.cpp
        ${CORE_TEST_HEADERS}
)
target_link_libraries(core_tests PRIVATE Qt6::Core Qt6::Test)
add_test(NAME core_tests COMMAND core_tests)

set(QT_PLUGIN_ROOT "${Qt6_DIR}/../../../plugins")
set(QT_WINDOWS_PLATFORM "${QT_PLUGIN_ROOT}/platforms/qwindows.dll")
set_tests_properties(core_tests PROPERTIES ENVIRONMENT
    "QT_QPA_PLATFORM=offscreen;QT_PLUGIN_PATH=${QT_PLUGIN_ROOT};PATH=C:/Qt/Tools/mingw1310_64/bin;C:/Qt/6.12.0/mingw_64/bin;$ENV{PATH};C:/Users/Pobed/CLionProjects/sound-recording/build")

foreach(target sound_recording core_tests)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${target}>/plugins/platforms"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_WINDOWS_PLATFORM}" "$<TARGET_FILE_DIR:${target}>/plugins/platforms/qwindows.dll"
    )
endforeach()
