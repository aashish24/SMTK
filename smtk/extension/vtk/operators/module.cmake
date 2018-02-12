set (__dependencies)

vtk_module(vtkSMTKOperationsExt
  DEPENDS
    vtkCommonCore
  PRIVATE_DEPENDS
    vtkCommonDataModel
    vtkCommonExecutionModel
    vtkFiltersGeneral
    vtkIOGDAL
    vtkIOXML
    vtkSMTKFilterExt
    vtkSMTKReaderExt
    vtkSMTKSourceExt
    ${__dependencies}
  TEST_DEPENDS
  EXCLUDE_FROM_PYTHON_WRAPPING
)
