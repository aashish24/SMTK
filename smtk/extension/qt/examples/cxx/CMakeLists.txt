##
## Create browseModel to test browsing model entity tree:

qt5_wrap_ui(browseModel_UI_BUILT_SOURCES ModelBrowser.ui)

add_executable(browseModel MACOSX_BUNDLE
  browseModel.cxx
  ModelBrowser.cxx
  ${browseModel_UI_BUILT_SOURCES})
target_link_libraries(browseModel
  smtkCore
  smtkCoreModelTesting
  smtkQtExt
)

set_target_properties(
  browseModel PROPERTIES AUTOMOC TRUE
  INSTALL_NAME_DIR "${CMAKE_INSTALL_PREFIX}/bin")

##
## Create an application to test attribute editing

#add in the attribute preview executable
add_executable(qtAttributePreview MACOSX_BUNDLE qtAttributePreview.cxx)
target_link_libraries(qtAttributePreview
  smtkQtExt
)

##
## Test model browsing (for crashes only, not behavior)

if (NOT WIN32 AND SMTK_DATA_DIR)
  add_test(
    NAME browseModel
    COMMAND
      $<TARGET_FILE:browseModel>
      "${SMTK_DATA_DIR}/model/2d/smtk/test2D.json"
      0xffffffff
  )
endif()
