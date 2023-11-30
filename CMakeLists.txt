cmake_minimum_required(VERSION 3.16)

# KDE Application Version, managed by release script
set(RELEASE_SERVICE_VERSION_MAJOR "23")
set(RELEASE_SERVICE_VERSION_MINOR "08")
set(RELEASE_SERVICE_VERSION_MICRO "4")
set(RELEASE_SERVICE_VERSION "${RELEASE_SERVICE_VERSION_MAJOR}.${RELEASE_SERVICE_VERSION_MINOR}.${RELEASE_SERVICE_VERSION_MICRO}")

project(keditbookmarks VERSION ${RELEASE_SERVICE_VERSION})

# ECM setup
set(KF_MIN_VERSION "5.91.0")
find_package(ECM ${KF_MIN_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

set(QT_MIN_VERSION "5.15.2")

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)

include(ECMInstallIcons)
include(ECMSetupVersion)
include(ECMAddTests)
include(ECMQtDeclareLoggingCategory)
include(FeatureSummary)
include(ECMAddAppIcon)
include(KDEGitCommitHooks)
include(KDEClangFormat)
include(ECMDeprecationSettings)
if (QT_MAJOR_VERSION STREQUAL "6")
    set(QT_REQUIRED_VERSION "6.4.0")
    set(KF_MIN_VERSION "5.240.0")
    set(KF_MAJOR_VERSION "6")
else()
    set(KF_MAJOR_VERSION "5")
endif()
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

set(KEDITBOOKMARKS_VERSION "5.97.0")

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} REQUIRED COMPONENTS Core)
if (QT_MAJOR_VERSION STREQUAL "6")
    find_package(Qt6Core5Compat)
endif()
find_package(KF${KF_MAJOR_VERSION} ${KF_MIN_VERSION} REQUIRED COMPONENTS CoreAddons I18n KIO Bookmarks Parts WindowSystem IconThemes)
find_package(KF${KF_MAJOR_VERSION}DocTools ${KF_MIN_VERSION})
set_package_properties(KF${KF_MAJOR_VERSION}DocTools PROPERTIES DESCRIPTION
    "Tools to generate documentation"
    TYPE OPTIONAL
)
if(BUILD_TESTING)
    find_package(Qt${QT_MAJOR_VERSION}Test ${QT_REQUIRED_VERSION} CONFIG REQUIRED)
endif()

ecm_set_disabled_deprecation_versions(QT 6.4
    KF 5.240.0
    )

if(NOT APPLE)
    find_package(X11)
    set(HAVE_X11 ${X11_FOUND})
    if (HAVE_X11)
       set_package_properties(X11 PROPERTIES TYPE OPTIONAL)
    endif()
endif()

add_subdirectory(src)
ki18n_install(po)
if (KF${KF_MAJOR_VERSION}DocTools_FOUND)
    kdoctools_install(po)
    add_subdirectory(doc)
endif()

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)

kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)
