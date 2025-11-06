cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME cutefish-dock)
project(${PROJECT_NAME})

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets DBus Quick)
find_package(Qt6 COMPONENTS LinguistTools REQUIRED)

# KF6 - 使用具体的组件包而不是通用的 KF6 包
find_package(KF6CoreAddons REQUIRED)
find_package(KF6WindowSystem REQUIRED)
find_package(KF6Config REQUIRED)

# 如果你需要其他组件，也这样添加：
# find_package(KF6GuiAddons REQUIRED)
# find_package(KF6I18n REQUIRED)

set(SRCS
    src/applicationitem.h
    src/applicationmodel.cpp
    src/docksettings.cpp
    src/iconthemeimageprovider.cpp
    src/main.cpp
    src/mainwindow.cpp
    src/systemappmonitor.cpp
    src/systemappitem.cpp
    src/processprovider.cpp
    src/trashmanager.cpp
    src/utils.cpp
    src/xwindowinterface.cpp
    src/activity.cpp
    src/fakewindow.cpp
)

set(RESOURCES
    resources.qrc
)

qt6_add_dbus_adaptor(DBUS_SOURCES
                     src/com.cutefish.Dock.xml
                     src/mainwindow.h MainWindow)
set_source_files_properties(${DBUS_SOURCES} PROPERTIES SKIP_AUTOGEN ON)

add_executable(${PROJECT_NAME} ${SRCS} ${DBUS_SOURCES} ${RESOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Quick
    Qt6::DBus
    KF6::CoreAddons
    KF6::WindowSystem
    Qt6::CorePrivate  # 这会自动处理私有头文件路径
    Qt6::GuiPrivate   # 这会自动处理私有头文件路径
)

file(GLOB TS_FILES translations/*.ts)

set(QM_FILES "")
foreach(_ts ${TS_FILES})
    get_filename_component(_name ${_ts} NAME_WE)
    set(_qm ${CMAKE_CURRENT_BINARY_DIR}/${_name}.qm)

    add_custom_command(
        OUTPUT ${_qm}
        COMMAND Qt6::lrelease ${_ts} -qm ${_qm}
        DEPENDS ${_ts}
        COMMENT "Generating ${_qm}"
    )

    list(APPEND QM_FILES ${_qm})
endforeach()

add_custom_target(translations ALL DEPENDS ${QM_FILES})
add_dependencies(${PROJECT_NAME} translations)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${QM_FILES} DESTINATION /usr/share/${PROJECT_NAME}/translations)
install(FILES cutefish-dock-list.conf DESTINATION /etc)
