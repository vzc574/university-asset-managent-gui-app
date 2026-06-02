cmake_minimum_required(VERSION 3.16)

project(SmartCampusManager VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Sql
)

qt_standard_project_setup()
set(CMAKE_AUTORCC ON)

qt_add_executable(SmartCampusManager
    main.cpp

    widget.h
    widget.cpp
    widget.ui

    logindialog.h
    logindialog.cpp

    assetwindow.h
    assetwindow.cpp

    maintenancewindow.h
    maintenancewindow.cpp

    lostfoundwindow.h
    lostfoundwindow.cpp

    studentitemswindow.h
    studentitemswindow.cpp

    userswindow.h
    userswindow.cpp

    securitycheckwindow.h
    securitycheckwindow.cpp

    dsaengine.h
    dsaengine.cpp

    campusgraphwindow.h
    campusgraphwindow.cpp

)

qt_add_resources(SmartCampusManager "images"
    PREFIX "/"
    FILES
        dbu_logo.png
)

target_link_libraries(SmartCampusManager PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Sql
)