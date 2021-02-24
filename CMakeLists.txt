cmake_minimum_required(VERSION 3.0)

# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "21")
set(RELEASE_SERVICE_VERSION_MINOR "03")
set(RELEASE_SERVICE_VERSION_MICRO "70")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(keditbookmarks VERSION ${RELEASE_SERVICE_VERSION})

# ECM setup
set(KF5_MIN_VERSION "5.71.0")
find_package(ECM ${KF5_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

set(QT_MIN_VERSION "5.12.0")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)

include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMAddTests)
include(ECMQtDeclareLoggingCategory)
include(FeatureSummary)
include(ECMAddAppIcon)
if(ECM_VERSION VERSION_GREATER_EQUAL 5.79)
    include(KDEClangFormat)
    include(KDEGitCommitHooks)
endif()

set(KEDITBOOKMARKS_VERSION "5.97.0")

find_package(Qt5 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS CoreAddons I18n KIO Bookmarks Parts WindowSystem IconThemes DocTools)

if(BUILD_TESTING)
    find_package(Qt5Test ${QT_REQUIRED_VERSION} CONFIG REQUIRED)
endif()

remove_definitions(-DQT_NO_CAST_FROM_ASCII)
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
add_definitions(-DQT_NO_FOREACH)

add_subdirectory(src)
add_subdirectory(doc)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

if(ECM_VERSION VERSION_GREATER_EQUAL 5.79)
    file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
    kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})
    kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
endif()
