//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#include "jsonDefinition.h"
#include "smtk/PublicPointerDefs.h"

#include "nlohmann/json.hpp"

#include "smtk/attribute/Definition.h"
#include "smtk/attribute/RefItem.h"
#include "smtk/attribute/ReferenceItem.h"
#include "smtk/attribute/ReferenceItemDefinition.h"
#include "smtk/attribute/Resource.h"

#include "smtk/attribute/json/jsonAttribute.h"
#include "smtk/attribute/json/jsonHelperFunction.h"
#include "smtk/attribute/json/jsonItem.h"
#include "smtk/attribute/json/jsonReferenceItemDefinition.h"

#include <exception>
#include <string>

namespace smtk
{
namespace attribute
{
using ItemExpressionDefInfo = std::pair<smtk::attribute::ValueItemDefinitionPtr, std::string>;

using AttRefDefInfo = std::pair<smtk::attribute::RefItemDefinitionPtr, std::string>;

SMTKCORE_EXPORT void to_json(nlohmann::json& j, const smtk::attribute::DefinitionPtr& defPtr)
{
  j["Type"] = defPtr->type();
  if (!defPtr->label().empty())
  {
    j["Label"] = defPtr->label();
  }
  if (defPtr->baseDefinition())
  {
    j["BaseType"] = defPtr->baseDefinition()->type();
  }
  else
  {
    j["BaseType"] = "";
  }
  j["Version"] = defPtr->version();
  if (defPtr->isAbstract())
  {
    j["Abstract"] = true;
  }
  if (defPtr->advanceLevel())
  {
    j["AdvanceLevel"] = defPtr->advanceLevel();
  }
  if (defPtr->isUnique())
  { // true is the defPtrault
    j["Unique"] = true;
  }
  else
  {
    j["Unique"] = false;
  }
  if (defPtr->rootName() != defPtr->type())
  {
    j["RootName"] = defPtr->rootName();
  }
  if (defPtr->isNodal())
  {
    j["Nodal"] = true;
  }
  // Save Color Information
  if (defPtr->isNotApplicableColorSet())
  {
    const double* rgba = defPtr->notApplicableColor();
    j["NotApplicableColor"] = { rgba[0], rgba[1], rgba[2], rgba[3] };
  }
  if (defPtr->isDefaultColorSet())
  {
    const double* rgba = defPtr->defaultColor();
    j["DefaultColor"] = { rgba[0], rgba[1], rgba[2], rgba[3] };
  }

  auto assocRule = defPtr->localAssociationRule();
  if (assocRule)
  {
    // Create association element if we need to.
    j["AssociationsDef"] = assocRule;
  }

  if (!defPtr->briefDescription().empty())
  {
    j["BriefDescription"] = defPtr->briefDescription();
  }
  if (!defPtr->detailedDescription().empty())
  {
    j["DetailedDescription"] = defPtr->detailedDescription();
  }
  // Now lets process its items
  std::size_t n = defPtr->numberOfItemDefinitions();
  // Does this defPtrinition have items not derived from its base defPtr?
  if (n != defPtr->itemOffset())
  {
    json itemDefs;
    for (std::size_t i = defPtr->itemOffset(); i < n; i++)
    {
      json itemDef;
      smtk::attribute::ItemDefinitionPtr itemDPtr = defPtr->itemDefinition(static_cast<int>(i));
      smtk::attribute::JsonHelperFunction::processItemDefinitionTypeToJson(itemDef, itemDPtr);
      // Same type definitions can occur multiple times
      itemDefs.push_back(itemDef);
    }
    j["ItemDefinitions"] = itemDefs;
  }
}

SMTKCORE_EXPORT void from_json(const nlohmann::json& j, smtk::attribute::DefinitionPtr& defPtr,
  std::vector<ItemExpressionDefInfo>& expressionDefInfo, std::vector<AttRefDefInfo>& attRefDefInfo)
{
  // The caller should make sure that defPtr is valid since it's not default constructible
  if (!defPtr.get())
  {
    return;
  }
  smtk::attribute::ResourcePtr colPtr =
    std::dynamic_pointer_cast<smtk::attribute::Resource>(defPtr->resource());
  if (colPtr == nullptr)
  {
    std::cerr << "When converting json, definition " << defPtr->label()
              << " has an invalid resourcePtr" << std::endl;
    return;
  }
  // Same logic in XmlDocV1Parser::processDefinition
  try
  {
    if (!j.at("Label").is_null())
    {
      defPtr->setLabel(j.at("Label"));
    }
  }
  catch (std::exception& /*e*/)
  {
  }

  try
  {
    if (!j.at("Version").is_null())
    {
      defPtr->setVersion(j.at("Version"));
    }
  }
  catch (std::exception& /*e*/)
  {
  }
  // Nlohmann would thow an exception if the key does not exist. Using []
  // would cause underfined behavior
  try
  {
    defPtr->setIsAbstract(j.at("Abstract"));
  }
  catch (std::exception& /*e*/)
  {
  }
  try
  {
    defPtr->setAdvanceLevel(j.at("AdvanceLevel"));
  }
  catch (std::exception& /*e*/)
  {
  }
  try
  {
    defPtr->setIsUnique(j.at("Unique"));
  }
  catch (std::exception& /*e*/)
  {
  }
  try
  {
    defPtr->setIsNodal(j.at("Nodal"));
  }
  catch (std::exception& /*e*/)
  {
  }
  // Read old-style association mask first.  Note that the association is set
  // as extensible.
  // It will be overwritten if a new-style AssociationsDef
  // is also provided.
  // Reference: XmlDocV1Parser:: L744
  try
  {
    nlohmann::json associations = j.at("Associations");
    if (!associations.is_null())
    {
      smtk::model::BitFlags mask = smtk::model::Entity::specifierStringToFlag(associations);
      defPtr->setLocalAssociationMask(mask);
      defPtr->localAssociationRule()->setIsExtensible(true);
    }
  }
  catch (std::exception& /*e*/)
  {
  }
  try
  {
    std::vector<double> rgba = j.at("NotApplicableColor");
    defPtr->setNotApplicableColor(rgba[0], rgba[1], rgba[2], rgba[3]);
  }
  catch (std::exception& /*e*/)
  {
  }
  try
  {
    std::vector<double> rgba = j.at("DefaultColor");
    defPtr->setDefaultColor(rgba[0], rgba[1], rgba[2], rgba[3]);
  }
  catch (std::exception& /*e*/)
  {
  }
  try
  {
    defPtr->setBriefDescription(j.at("BriefDescription"));
  }
  catch (std::exception& /*e*/)
  {
  }
  try
  {
    defPtr->setDetailedDescription(j.at("DetailedDescription"));
  }
  catch (std::exception& /*e*/)
  {
  }
  try
  {
    nlohmann::json associationsDef = j.at("AssociationsDef");
    if (!associationsDef.is_null())
    {
      // TODO: Check XmlDocV1Parser 789
      std::string assocName = associationsDef.at("Name");
      if (assocName.empty())
      {
        assocName = defPtr->type() + "Associations";
      }
      auto assocRule = smtk::attribute::ReferenceItemDefinition::New(assocName);
      smtk::attribute::from_json(associationsDef, assocRule);
      defPtr->setLocalAssociationRule(assocRule);
    }
  }
  catch (std::exception& /*e*/)
  {
  }
  try
  {
    nlohmann::json itemDefs = j.at("ItemDefinitions");
    if (!itemDefs.is_null())
    {
      smtk::attribute::ResourcePtr resource =
        std::dynamic_pointer_cast<smtk::attribute::Resource>(defPtr->resource());
      // Reference: Check XmlDocV1Parser 789
      for (json::iterator iter = itemDefs.begin(); iter != itemDefs.end(); iter++)
      {
        smtk::attribute::JsonHelperFunction::processItemDefinitionTypeFromJson(
          iter, defPtr, resource, expressionDefInfo, attRefDefInfo);
      }
    }
  }
  catch (std::exception& /*e*/)
  {
  }
  try
  {
    defPtr->setRootName(j.at("RootName"));
  }
  catch (std::exception& /*e*/)
  {
  }
}
}
}
