include(ECMAddTests)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

###### kbookmarkmodeltest ######

ecm_add_test(kbookmarkmodeltest.cpp
    TEST_NAME kbookmarkmodeltest
    LINK_LIBRARIES kbookmarkmodel_private KF5::Bookmarks Qt::Test
)
