############ subdir ###################
set(debug_file_common_SRCS)
ecm_qt_declare_logging_category(debug_file_common_SRCS
    HEADER keditbookmarks_debug.h
    IDENTIFIER KEDITBOOKMARKS_LOG
    CATEGORY_NAME org.kde.keditbookmarks
    DESCRIPTION "KEditBookmarks"
    EXPORT KEDITBOOKMARKS
)


add_subdirectory(kbookmarkmodel)

########### next target ###############

ecm_setup_version(${RELEASE_SERVICE_VERSION} VARIABLE_PREFIX KEDITBOOKMARKS VERSION_HEADER keditbookmarks_version.h)

set(kbookmarkmerger_SRCS kbookmarkmerger.cpp ${debug_file_common_SRCS})

add_executable(kbookmarkmerger ${kbookmarkmerger_SRCS})

target_link_libraries(kbookmarkmerger
   KF5::KIOCore
   KF5::I18n
   KF5::Bookmarks
)

install(TARGETS kbookmarkmerger ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


########### next target ###############

set(keditbookmarks_SRCS
   main.cpp
   toplevel.cpp
   globalbookmarkmanager.cpp
   actionsimpl.cpp
   importers.cpp
   bookmarkiterator.cpp
   testlink.cpp
   favicons.cpp
   faviconupdater.cpp
   exporters.cpp
   bookmarkinfowidget.cpp
   kebsearchline.cpp
   bookmarklistview.cpp
   ${debug_file_common_SRCS}
   keditbookmarks.qrc
)
set(toplevel_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.keditbookmarks.xml)
qt_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/toplevel.h org.kde.keditbookmarks.xml )

qt_add_dbus_interface( keditbookmarks_SRCS ${toplevel_xml} toplevel_interface )

kconfig_add_kcfg_files(keditbookmarks_SRCS settings.kcfgc )

add_executable( keditbookmarks ${keditbookmarks_SRCS})

target_link_libraries(keditbookmarks kbookmarkmodel_private
   KF5::Parts
   KF5::Bookmarks
   KF5::WindowSystem
   KF5::IconThemes
   KF5::KIOGui)

install(TARGETS keditbookmarks          ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES keditbookmarks.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
install( PROGRAMS org.kde.keditbookmarks.desktop DESTINATION ${KDE_INSTALL_APPDIR} )

ecm_qt_install_logging_categories(
    EXPORT KEDITBOOKMARKS
    FILE keditbookmarks.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
