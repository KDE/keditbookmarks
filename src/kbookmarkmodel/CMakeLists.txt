if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
include_directories(${CMAKE_BINARY_DIR}/src/)
add_library(kbookmarkmodel_private SHARED)
target_sources(kbookmarkmodel_private PRIVATE
  commandhistory.cpp
  commands.cpp
  commands_p.h
  model.cpp
  treeitem.cpp
  view.cpp
  commandhistory.h
  commands.h
  model.h
  treeitem_p.h
  kinsertionsort_p.h
  view.h
  ${debug_file_common_SRCS}
)
target_link_libraries(kbookmarkmodel_private
PUBLIC
   Qt::Widgets
   KF6::XmlGui # KActionCollection
PRIVATE
   KF6::Bookmarks
   KF6::I18n
)

include(GenerateExportHeader)
generate_export_header(kbookmarkmodel_private BASE_NAME kbookmarkmodel EXPORT_FILE_NAME kbookmarkmodel_export.h)

set_target_properties(kbookmarkmodel_private PROPERTIES
     VERSION ${KEDITBOOKMARKS_VERSION}
     SOVERSION "6"
)


install(TARGETS kbookmarkmodel_private ${KDE_INSTALL_TARGETS_DEFAULT_ARGS} LIBRARY NAMELINK_SKIP)
