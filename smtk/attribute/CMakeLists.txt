set(jsonAttributeHeaders
  json/jsonAttribute.h
  json/jsonComponentItem.h
  json/jsonComponentItemDefinition.h
  json/jsonResource.h
  json/jsonDateTimeItem.h
  json/jsonDateTimeItemDefinition.h
  json/jsonDirectoryItem.h
  json/jsonDirectoryItemDefinition.h
  json/jsonDoubleItem.h
  json/jsonDoubleItemDefinition.h
  json/jsonDefinition.h
  json/jsonFileItem.h
  json/jsonFileItemDefinition.h
  json/jsonFileSystemItem.h
  json/jsonFileSystemItemDefinition.h
  json/jsonGroupItem.h
  json/jsonGroupItemDefinition.h
  json/jsonHelperFunction.h
  json/jsonIntItem.h
  json/jsonIntItemDefinition.h
  json/jsonItem.h
  json/jsonItemDefinition.h
  json/jsonMeshItem.h
  json/jsonMeshItemDefinition.h
  json/jsonMeshSelectionItem.h
  json/jsonMeshSelectionItemDefinition.h
  json/jsonModelEntityItem.h
  json/jsonModelEntityItemDefinition.h
  json/jsonStringItem.h
  json/jsonStringItemDefinition.h
  json/jsonReferenceItem.h
  json/jsonReferenceItemDefinition.h
  json/jsonRefItem.h
  json/jsonRefItemDefinition.h
  json/jsonValueItem.h
  json/jsonValueItemDefinition.h
  json/jsonVoidItem.h
  json/jsonVoidItemDefinition.h
)

set(jsonAttributeSrcs
  json/jsonAttribute.cxx
  json/jsonComponentItem.cxx
  json/jsonComponentItemDefinition.cxx
  json/jsonResource.cxx
  json/jsonDateTimeItem.cxx
  json/jsonDateTimeItemDefinition.cxx
  json/jsonDirectoryItem.cxx
  json/jsonDirectoryItemDefinition.cxx
  json/jsonDoubleItem.cxx
  json/jsonDoubleItemDefinition.cxx
  json/jsonDefinition.cxx
  json/jsonFileItem.cxx
  json/jsonFileItemDefinition.cxx
  json/jsonFileSystemItem.cxx
  json/jsonFileSystemItemDefinition.cxx
  json/jsonGroupItem.cxx
  json/jsonGroupItemDefinition.cxx
  json/jsonHelperFunction.cxx
  json/jsonIntItem.cxx
  json/jsonIntItemDefinition.cxx
  json/jsonItem.cxx
  json/jsonItemDefinition.cxx
  json/jsonMeshItem.cxx
  json/jsonMeshItemDefinition.cxx
  json/jsonMeshSelectionItem.cxx
  json/jsonMeshSelectionItemDefinition.cxx
  json/jsonModelEntityItem.cxx
  json/jsonModelEntityItemDefinition.cxx
  json/jsonStringItem.cxx
  json/jsonStringItemDefinition.cxx
  json/jsonReferenceItem.cxx
  json/jsonReferenceItemDefinition.cxx
  json/jsonRefItem.cxx
  json/jsonRefItemDefinition.cxx
  json/jsonValueItem.cxx
  json/jsonValueItemDefinition.cxx
  json/jsonVoidItem.cxx
  json/jsonVoidItemDefinition.cxx
)

set(attributeHeaders
  ${jsonAttributeHeaders}
  Attribute.h
  Resource.h
  ComponentItem.h
  ComponentItemDefinition.h
  Definition.h
  DateTimeItem.h
  DateTimeItemDefinition.h
  DirectoryInfo.h
  DirectoryItem.h
  DirectoryItemDefinition.h
  DoubleItem.h
  DoubleItemDefinition.h
  FileItem.h
  FileItemDefinition.h
  FileSystemItem.h
  FileSystemItemDefinition.h
  GroupItem.h
  GroupItemDefinition.h
  IntItem.h
  IntItemDefinition.h
  Item.h
  ItemDefinition.h
  MeshItem.h
  MeshItemDefinition.h
  MeshSelectionItem.h
  MeshSelectionItemDefinition.h
  ModelEntityItem.h
  ModelEntityItemDefinition.h
  ReferenceItem.h
  ReferenceItemDefinition.h
  RefItem.h
  RefItemDefinition.h
  Registrar.h
  ResourceItem.h
  ResourceItemDefinition.h
  SearchStyle.h
  StringItem.h
  StringItemDefinition.h
  Tag.h
  ValueItem.h
  ValueItemDefinition.h
  ValueItemDefinitionTemplate.h
  ValueItemTemplate.h
  VoidItem.h
  VoidItemDefinition.h

  operators/Associate.h
  operators/Read.h
  operators/Write.h
)

set(attributeSrcs
  ${jsonAttributeSrcs}
  Attribute.cxx
  Resource.cxx
  ComponentItem.cxx
  ComponentItemDefinition.cxx
  DateTimeItem.cxx
  DateTimeItemDefinition.cxx
  Definition.cxx
  DirectoryInfo.cxx
  DirectoryItem.cxx
  DirectoryItemDefinition.cxx
  DoubleItem.cxx
  DoubleItemDefinition.cxx
  FileItem.cxx
  FileItemDefinition.cxx
  FileSystemItem.cxx
  FileSystemItemDefinition.cxx
  GroupItem.cxx
  GroupItemDefinition.cxx
  IntItem.cxx
  IntItemDefinition.cxx
  Item.cxx
  ItemDefinition.cxx
  MeshItem.cxx
  MeshItemDefinition.cxx
  MeshSelectionItem.cxx
  MeshSelectionItemDefinition.cxx
  ModelEntityItem.cxx
  ModelEntityItemDefinition.cxx
  RefItem.cxx
  RefItemDefinition.cxx
  ReferenceItem.cxx
  ReferenceItemDefinition.cxx
  Registrar.cxx
  ResourceItem.cxx
  ResourceItemDefinition.cxx
  StringItem.cxx
  StringItemDefinition.cxx
  ValueItem.cxx
  ValueItemDefinition.cxx
  VoidItem.cxx
  VoidItemDefinition.cxx

  operators/Associate.cxx
  operators/Read.cxx
  operators/Write.cxx
)

#construct operator inputs
smtk_operation_xml("${CMAKE_CURRENT_SOURCE_DIR}/operators/Associate.sbt" defOpXML)
smtk_operation_xml("${CMAKE_CURRENT_SOURCE_DIR}/operators/Read.sbt" defOpXML)
smtk_operation_xml("${CMAKE_CURRENT_SOURCE_DIR}/operators/Write.sbt" defOpXML)

#install the headers
smtk_public_headers(smtkCore ${attributeHeaders})

if (SMTK_ENABLE_PARAVIEW_SUPPORT)
  add_smtk_plugin(
    smtkAttributePlugin "1.0"
    REGISTRAR smtk::attribute::Registrar
    MANAGERS smtk::operation::Manager smtk::resource::Manager
    LIBRARIES smtkCore)
  smtk_install_library(smtkAttributePlugin)
endif()

if (SMTK_ENABLE_PYTHON_WRAPPING)
  add_subdirectory(pybind11)
endif()

if(SMTK_ENABLE_TESTING)
  add_subdirectory(testing)
endif(SMTK_ENABLE_TESTING)
