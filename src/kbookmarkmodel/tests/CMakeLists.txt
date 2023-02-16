include(ECMAddTests)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

###### kbookmarkmodeltest ######

ecm_add_test(kbookmarkmodeltest.cpp
    TEST_NAME kbookmarkmodeltest
    LINK_LIBRARIES kbookmarkmodel_private KF${KF_MAJOR_VERSION}::Bookmarks Qt::Test
)
