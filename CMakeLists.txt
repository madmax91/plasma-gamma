cmake_minimum_required(VERSION 3.22)
project(plasma-gamma VERSION 0.1.0 LANGUAGES CXX)

set(QT_MIN_VERSION 6.6.0)
set(KF_MIN_VERSION 6.0.0)

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(FeatureSummary)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Widgets DBus)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS
    CoreAddons
    Config
    DBusAddons
    GlobalAccel
    StatusNotifierItem
    XmlGui
)
find_package(KF6Screen REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LCMS2 REQUIRED IMPORTED_TARGET lcms2)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

add_executable(plasma-gamma
    src/main.cpp
    src/gammacontroller.cpp
    src/iccprofile.cpp
    src/trayapp.cpp
)

target_compile_definitions(plasma-gamma PRIVATE PROJECT_VERSION="${PROJECT_VERSION}")

target_link_libraries(plasma-gamma PRIVATE
    Qt6::Widgets
    Qt6::DBus
    KF6::CoreAddons
    KF6::ConfigCore
    KF6::DBusAddons
    KF6::GlobalAccel
    KF6::StatusNotifierItem
    KF6::XmlGui
    KF6::Screen
    PkgConfig::LCMS2
)

install(TARGETS plasma-gamma ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES org.madmax91.plasma-gamma.desktop DESTINATION ${KDE_INSTALL_APPDIR})
