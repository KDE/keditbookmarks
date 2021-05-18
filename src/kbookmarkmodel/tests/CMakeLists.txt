include(ECMAddTests)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

###### kbookmarkmodeltest ######

set(kbookmarkmodeltest_SRCS kbookmarkmodeltest.cpp)
ecm_add_test(${kbookmarkmodeltest_SRCS}
    TEST_NAME kbookmarkmodeltest
    LINK_LIBRARIES kbookmarkmodel_private KF5::Bookmarks Qt::Test
)
