//=============================================================================
// Copyright (c) Kitware, Inc.
// All rights reserved.
// See LICENSE.txt for details.
//
// This software is distributed WITHOUT ANY WARRANTY; without even
// the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE.  See the above copyright notice for more information.
//=============================================================================
#include "smtk/session/rgg/operators/CreatePin.h"

#include "smtk/session/rgg/Session.h"

#include "smtk/io/Logger.h"

#include "smtk/PublicPointerDefs.h"
#include "smtk/attribute/Attribute.h"
#include "smtk/attribute/DoubleItem.h"
#include "smtk/attribute/GroupItem.h"
#include "smtk/attribute/IntItem.h"
#include "smtk/attribute/ModelEntityItem.h"
#include "smtk/attribute/StringItem.h"
#include "smtk/attribute/VoidItem.h"
#include "smtk/model/AuxiliaryGeometry.h"
#include "smtk/model/Model.h"
#include "smtk/model/Resource.h"

#include "smtk/session/rgg/operators/CreateModel.h"

#include "smtk/session/rgg/CreatePin_xml.h"

#include <string> // std::to_string
using namespace smtk::model;

namespace smtk
{
namespace session
{
namespace rgg
{

CreatePin::Result CreatePin::operateInternal()
{
  Result result = this->createResult(smtk::operation::Operation::Outcome::FAILED);
  EntityRefArray entities = this->parameters()->associatedModelEntities<EntityRefArray>();
  // Since we are in creating mode, it must be a model
  if (entities.empty() || !entities[0].isModel())
  {
    smtkErrorMacro(this->log(), "An invalid model is provided for create pin op");
    return this->createResult(smtk::operation::Operation::Outcome::FAILED);
  }
  EntityRef parent = entities[0];

  smtk::model::AuxiliaryGeometry auxGeom;
  // A list contains all subparts and layers of the pin
  std::vector<EntityRef> subAuxGeoms;
  auxGeom = parent.resource()->addAuxiliaryGeometry(parent.as<smtk::model::Model>(), 3);
  // Update the latest pin in the model
  parent.setStringProperty("latest pin", auxGeom.entity().toString());

  CreatePin::populatePin(this, auxGeom, subAuxGeoms, true);

  result = this->createResult(smtk::operation::Operation::Outcome::SUCCEEDED);

  result->findComponent("modified")->appendValue(parent.component());
  smtk::attribute::ComponentItem::Ptr createdItem = result->findComponent("created");
  createdItem->appendValue(auxGeom.component());
  for (auto& c : subAuxGeoms)
  {
    createdItem->appendValue(c.component());
  }

  return result;
}

void CreatePin::populatePin(smtk::operation::Operation* op, smtk::model::AuxiliaryGeometry& auxGeom,
  std::vector<smtk::model::EntityRef>& subAuxGeoms, bool isCreation)
{
  auxGeom.setStringProperty("rggType", SMTK_SESSION_RGG_PIN);

  // Cut away the pin?
  smtk::attribute::VoidItemPtr isCutAway = op->parameters()->findVoid("cut away");
  if (isCutAway->isEnabled())
  {
    auxGeom.setIntegerProperty("cut away", 1);
  }
  else
  {
    auxGeom.setIntegerProperty("cut away", 0);
  }

  smtk::attribute::StringItemPtr nameItem = op->parameters()->findString("name");
  std::string pinName;
  if (nameItem != nullptr && !nameItem->value(0).empty())
  {
    pinName = nameItem->value(0);
    auxGeom.setName(nameItem->value(0));
  }

  smtk::attribute::StringItemPtr labelItem = op->parameters()->findString("label");
  if (labelItem != nullptr && !labelItem->value(0).empty())
  {
    // Make sure that the label is unique
    smtk::model::Model model = auxGeom.owningModel();
    std::string labelValue = labelItem->value(0);
    if ((auxGeom.hasStringProperty("label") && auxGeom.stringProperty("label")[0] != labelValue) ||
      isCreation)
    { // Only check and generate new label if user provides a new label
      if (model.hasStringProperty("pin labels list"))
      {
        smtk::model::StringList& labels = model.stringProperty("pin labels list");
        int count = 0;
        while (std::find(std::begin(labels), std::end(labels), labelValue) != std::end(labels))
        { // need to generate a new label
          labelValue = "PC" + std::to_string(count);
          count++;
        }
        // Update the label list
        labels.push_back(labelValue);
      }
      else
      {
        model.setStringProperty("pin labels list", labelValue);
      }
    }
    auxGeom.setStringProperty(labelItem->name(), labelValue);
  }

  smtk::attribute::DoubleItemPtr colorI = op->parameters()->findDouble("color");
  if (colorI)
  {
    smtk::model::FloatList color = smtk::model::FloatList{ colorI->begin(), colorI->end() };
    if (color.size() != 4 && (color.size() != 0))
    {
      smtkErrorMacro(op->log(), "Color item does not have 4 values");
    }
    else
    {
      auxGeom.setColor(color);
    }
  }

  smtk::attribute::IntItemPtr pinMaterialItem = op->parameters()->findInt("cell material");
  size_t materialIndex(0);
  if (pinMaterialItem != nullptr && pinMaterialItem->numberOfValues() == 1)
  {
    auxGeom.setIntegerProperty(pinMaterialItem->name(), pinMaterialItem->value(0));
    materialIndex = pinMaterialItem->value(0);
  }

  smtk::attribute::DoubleItemPtr zOriginItem = op->parameters()->findDouble("z origin");
  if (zOriginItem != nullptr && zOriginItem->numberOfValues() == 1)
  {
    auxGeom.setFloatProperty(zOriginItem->name(), zOriginItem->value(0));
  }

  smtk::attribute::GroupItemPtr piecesGItem = op->parameters()->findGroup("pieces");
  size_t numParts;
  if (piecesGItem != nullptr)
  {
    numParts = piecesGItem->numberOfGroups();
    IntegerList pieceSegType;
    FloatList typeParas;
    for (std::size_t index = 0; index < numParts; index++)
    {
      smtk::attribute::IntItemPtr segmentType =
        piecesGItem->findAs<smtk::attribute::IntItem>(index, "segment type");
      // Length, base radius, top radius
      smtk::attribute::DoubleItemPtr typePara =
        piecesGItem->findAs<smtk::attribute::DoubleItem>(index, "type parameters");
      pieceSegType.insert(pieceSegType.end(), segmentType->begin(), segmentType->end());
      typeParas.insert(typeParas.end(), typePara->begin(), typePara->end());
    }
    auxGeom.setIntegerProperty(piecesGItem->name(), pieceSegType);
    auxGeom.setFloatProperty(piecesGItem->name(), typeParas);
    // Get the max radius and cache it for create assembly purpose
    double maxRadius(-1);
    for (size_t i = 0; i < typeParas.size() / 3; i++)
    {
      maxRadius = std::max(maxRadius, std::max(typeParas[i * 3 + 1], typeParas[i * 3 + 2]));
    }
    auxGeom.setFloatProperty("max radius", maxRadius);
  }

  smtk::attribute::GroupItemPtr layerMaterialsItem = op->parameters()->findGroup("layer materials");
  size_t numLayers;
  IntegerList subMaterials;
  if (layerMaterialsItem != nullptr)
  {
    numLayers = layerMaterialsItem->numberOfGroups();
    FloatList radiusNs;
    for (std::size_t index = 0; index < numLayers; index++)
    {
      smtk::attribute::IntItemPtr subMaterial =
        layerMaterialsItem->findAs<smtk::attribute::IntItem>(index, "sub material");
      smtk::attribute::DoubleItemPtr radiusN =
        layerMaterialsItem->findAs<smtk::attribute::DoubleItem>(index, "radius(normalized)");
      subMaterials.insert(subMaterials.end(), subMaterial->begin(), subMaterial->end());
      radiusNs.insert(radiusNs.end(), radiusN->begin(), radiusN->end());
    }
    auxGeom.setIntegerProperty(layerMaterialsItem->name(), subMaterials);
    auxGeom.setFloatProperty(layerMaterialsItem->name(), radiusNs);
  }

  auto assignColor = [](size_t index, smtk::model::AuxiliaryGeometry& aux) {
    smtk::model::FloatList rgba;
    smtk::session::rgg::CreateModel::getMaterialColor(index, rgba, aux.owningModel());
    aux.setColor(rgba);
  };
  // Create auxgeom placeholders for layers and parts
  for (std::size_t i = 0; i < numParts; i++)
  {
    for (std::size_t j = 0; j < numLayers; j++)
    {
      // Create an auxo_geom for current each unit part&layer
      AuxiliaryGeometry subLayer = auxGeom.resource()->addAuxiliaryGeometry(auxGeom, 3);
      std::string subLName = pinName + SMTK_SESSION_RGG_PIN_SUBPART + std::to_string(i) +
        SMTK_SESSION_RGG_PIN_LAYER + std::to_string(j);
      subLayer.setName(subLName);
      subLayer.setStringProperty("rggType", SMTK_SESSION_RGG_PIN);
      assignColor(subMaterials[j], subLayer);
      subAuxGeoms.push_back(subLayer.as<EntityRef>());
    }
    if (materialIndex) // 0 index means no material
    {                  // Append a material layer after the last layer
      AuxiliaryGeometry materialLayer = auxGeom.resource()->addAuxiliaryGeometry(auxGeom, 3);
      std::string materialName =
        pinName + SMTK_SESSION_RGG_PIN_SUBPART + std::to_string(i) + SMTK_SESSION_RGG_PIN_MATERIAL;
      materialLayer.setName(materialName);
      materialLayer.setStringProperty("rggType", SMTK_SESSION_RGG_PIN);
      assignColor(materialIndex, materialLayer);
      subAuxGeoms.push_back(materialLayer.as<EntityRef>());
    }
  }
}

const char* CreatePin::xmlDescription() const
{
  return CreatePin_xml;
}
} // namespace rgg
} //namespace session
} // namespace smtk
