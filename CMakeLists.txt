cmake_minimum_required(VERSION 3.0)

# KDE Application Version, managed by release script
set(KDE_APPLICATIONS_VERSION_MAJOR "19")
set(KDE_APPLICATIONS_VERSION_MINOR "12")
set(KDE_APPLICATIONS_VERSION_MICRO "3")
set(KDE_APPLICATIONS_VERSION "${KDE_APPLICATIONS_VERSION_MAJOR}.${KDE_APPLICATIONS_VERSION_MINOR}.${KDE_APPLICATIONS_VERSION_MICRO}")

project(keditbookmarks VERSION ${KDE_APPLICATIONS_VERSION})

# ECM setup
set(KF5_MIN_VERSION "5.28.0")
find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

set(QT_MIN_VERSION "5.6.0")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

include(ECMOptionalAddSubdirectory)
include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMAddTests)
include(ECMMarkNonGuiExecutable)

include(CMakePackageConfigHelpers)
include(FeatureSummary)
include(ECMAddAppIcon)
include(CheckFunctionExists)

set(KEDITBOOKMARKS_VERSION "5.97.0")

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS CoreAddons I18n KIO Bookmarks Parts WindowSystem IconThemes DocTools)

if(BUILD_TESTING)
    find_package(Qt5Test ${QT_REQUIRED_VERSION} CONFIG REQUIRED)
endif()

remove_definitions(-DQT_NO_CAST_FROM_ASCII)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
if (EXISTS "${CMAKE_SOURCE_DIR}/.git")
   add_definitions(-DQT_DISABLE_DEPRECATED_BEFORE=0x060000)
   add_definitions(-DKF_DISABLE_DEPRECATED_BEFORE_AND_AT=0x060000)
endif()

add_subdirectory(src)
add_subdirectory(doc)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

