cmake_minimum_required(VERSION 3.14)
project(InstrumentTimbreIniFileTest)

set(CMAKE_CXX_STANDARD 11)

find_package(QT NAMES Qt6 Qt5 COMPONENTS Core Gui REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Core Gui REQUIRED)

set(INSTRUMENTTIMBREINIFILE_TEST_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(CHIPTUNE_QT_DIR "${INSTRUMENTTIMBREINIFILE_TEST_DIR}/../..")
set(CHIPTUNE_ROOT_DIR "${CHIPTUNE_QT_DIR}/..")

set(INSTRUMENTTIMBREINIFILE_TEST_DEPENDENCY_SOURCES
    "${CHIPTUNE_QT_DIR}/ChiptuneMidiValues.cpp"
)

add_executable(InstrumentTimbreIniFileTest
    main.cpp
    "${CHIPTUNE_QT_DIR}/InstrumentTimbreIniFile.cpp"
    ${INSTRUMENTTIMBREINIFILE_TEST_DEPENDENCY_SOURCES}
)

target_include_directories(InstrumentTimbreIniFileTest PRIVATE
    "${CHIPTUNE_QT_DIR}"
    "${CHIPTUNE_ROOT_DIR}"
)

target_link_libraries(InstrumentTimbreIniFileTest
    PRIVATE Qt${QT_VERSION_MAJOR}::Core
            Qt${QT_VERSION_MAJOR}::Gui
)
