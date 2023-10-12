configure_file(config-keditbookmarks.h.in
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
   Qt::Widgets
   KF6::KIOCore
   KF6::I18n
   KF6::Bookmarks
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
   kbookmarkexporter.h
   kbookmarkimporter.cpp
   kbookmarkimporter.h
   kbookmarkimporter_ie.cpp
   kbookmarkimporter_ie.h
   kbookmarkimporter_ns.cpp
   kbookmarkimporter_ns.h
   kbookmarkimporter_opera.cpp
   kbookmarkimporter_opera.h
   kbookmarkimporter_opera_p.h
   kbookmarkdombuilder.cpp
   kbookmarkdombuilder.h
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
   KF6::Parts
   KF6::Bookmarks
   KF6::WindowSystem
   KF6::IconThemes
   KF6::Completion
   KF6::I18n
   KF6::Codecs
   KF6::IconWidgets
   KF6::KIOGui)

install(TARGETS keditbookmarks          ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})

########### install files ###############

install( FILES keditbookmarks.kcfg  DESTINATION  ${KDE_INSTALL_KCFGDIR} )
install( PROGRAMS org.kde.keditbookmarks.desktop DESTINATION ${KDE_INSTALL_APPDIR} )

ecm_qt_install_logging_categories(
    EXPORT KEDITBOOKMARKS
    FILE keditbookmarks.categories
    DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)
