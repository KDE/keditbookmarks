configure_file(config-keditbookmarks.h.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/config-keditbookmarks.h)

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

add_executable(kbookmarkmerger kbookmarkmerger.cpp ${debug_file_common_SRCS})

target_link_libraries(kbookmarkmerger
   KF${KF_MAJOR_VERSION}::KIOCore
   KF${KF_MAJOR_VERSION}::I18n
   KF${KF_MAJOR_VERSION}::Bookmarks
)

install(TARGETS kbookmarkmerger ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})


########### next target ###############
add_executable( keditbookmarks)
set(toplevel_xml ${CMAKE_CURRENT_BINARY_DIR}/org.kde.keditbookmarks.xml)
qt_generate_dbus_interface( ${CMAKE_CURRENT_SOURCE_DIR}/toplevel.h org.kde.keditbookmarks.xml )

qt_add_dbus_interface( keditbookmarks_SRCS ${toplevel_xml} toplevel_interface )

kconfig_add_kcfg_files(keditbookmarks_SRCS settings.kcfgc )
target_sources(keditbookmarks PRIVATE
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
   toplevel.h
   globalbookmarkmanager.h
   actionsimpl.h
   importers.h
   bookmarkiterator.h
   testlink.h
   favicons.h
   faviconupdater.h
   exporters.h
   bookmarkinfowidget.h
   kebsearchline.h
   bookmarklistview.h
   ${debug_file_common_SRCS}
   ${keditbookmarks_SRCS}
   keditbookmarks.qrc
)

target_link_libraries(keditbookmarks kbookmarkmodel_private
   KF${KF_MAJOR_VERSION}::Parts
   KF${KF_MAJOR_VERSION}::Bookmarks
   KF${KF_MAJOR_VERSION}::WindowSystem
   KF${KF_MAJOR_VERSION}::IconThemes
   KF${KF_MAJOR_VERSION}::Completion
   KF${KF_MAJOR_VERSION}::I18n
   KF${KF_MAJOR_VERSION}::KIOGui)

if (QT_MAJOR_VERSION STREQUAL "6")
   target_link_libraries( keditbookmarks KF6::IconWidgets)
endif()

install(TARGETS keditbookmarks          ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES keditbookmarks.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
install( PROGRAMS org.kde.keditbookmarks.desktop DESTINATION ${KDE_INSTALL_APPDIR} )

ecm_qt_install_logging_categories(
    EXPORT KEDITBOOKMARKS
    FILE keditbookmarks.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
