find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)

find_package( VTK REQUIRED )
include_directories(${VTK_INCLUDE_DIRS})

set(VXLWidgetsHeaders
  pqTerrainExtractionManager.h
   )
set(VXLWidgetsSrcs
  pqTerrainExtractionManager.cxx
   )
set(VXLWidgetsUI
   )

#install the headers
smtk_public_headers(smtkVXLWidgetsExt ${VXLWidgetsHeaders})

qt5_wrap_cpp(MOC_BUILT_SOURCES ${VXLWidgetsHeaders})
qt5_wrap_ui(UI_BUILT_SOURCES ${VXLWidgetsUI})

source_group("Qt_Source" FILES ${VXLWidgetsSrcs} ${VXLWidgetsUI})
source_group("Qt_Generated" FILES ${MOC_BUILT_SOURCES} ${UI_BUILT_SOURCES} ${RCS_RESOURCES})
source_group("Qt_Header" FILES ${VXLWidgetsMocHeaders})

add_library(smtkVXLWidgetsExt
  ${VXLWidgetsSrcs}
  ${MOC_BUILT_SOURCES}
  ${UI_BUILT_SOURCES}
  ${RCS_RESOURCES}
)

target_include_directories(smtkVXLWidgetsExt PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(smtkVXLWidgetsExt LINK_PUBLIC
  pqApplicationComponents
  smtkCore
  smtkPQWidgetsExt
  vtkSMTKSourceExt
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
LINK_PRIVATE
  ${VTK_LIBRARIES}
  smtkQtExt
)

smtk_export_header(smtkVXLWidgetsExt Exports.h)

#install the library and exports the library when used from a build tree
smtk_install_library(smtkVXLWidgetsExt)

if (SMTK_ENABLE_TESTING)
  # Link *everything* in this directory to VTK modules
  # This is required for the TestBuild targets.
  # We must test whether each of the targets below exist
  # because they are required when built into VTK but
  # may not be present, especially since the OpenGL2
  # backend became available.
  set(SMTK_VTK_TARGET_LIST)
  foreach(target
    vtkInteractionStyle
    vtkRenderingFreeType
    vtkRenderingOpenGL2
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    )
    if (TARGET ${target})
      list(APPEND SMTK_VTK_TARGET_LIST ${target})
    endif()
  endforeach()
endif()
