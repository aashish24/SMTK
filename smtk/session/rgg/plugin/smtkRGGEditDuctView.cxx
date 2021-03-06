//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#include "smtk/session/rgg/plugin/smtkRGGEditDuctView.h"
#include "smtk/session/rgg/plugin/ui_smtkRGGEditDuctParameters.h"

#include "smtkRGGViewHelper.h"

#include "smtk/attribute/Attribute.h"
#include "smtk/attribute/DoubleItem.h"
#include "smtk/attribute/GroupItem.h"
#include "smtk/attribute/IntItem.h"
#include "smtk/attribute/ReferenceItem.h"
#include "smtk/attribute/StringItem.h"
#include "smtk/attribute/VoidItem.h"

#include "smtk/extension/qt/qtActiveObjects.h"
#include "smtk/extension/qt/qtAttribute.h"
#include "smtk/extension/qt/qtBaseView.h"
#include "smtk/extension/qt/qtItem.h"
#include "smtk/extension/qt/qtModelOperationWidget.h"
#include "smtk/extension/qt/qtModelView.h"
#include "smtk/extension/qt/qtUIManager.h"

#include "smtk/io/Logger.h"

#include "smtk/model/AuxiliaryGeometry.h"

#include "smtk/session/rgg/operators/CreateDuct.h"
#include "smtk/session/rgg/operators/CreateModel.h"
#include "smtk/session/rgg/operators/EditDuct.h"

#include "smtk/view/View.h"

#include "pqActiveObjects.h"
#include "pqApplicationCore.h"
#include "pqPresetDialog.h"
#include "pqRenderView.h"
#include "pqServer.h"
#include "pqSettings.h"

#include <QComboBox>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

using namespace smtk::model;
using namespace smtk::extension;
using namespace smtk::session::rgg;
const int numberOfSegmentTableColumns = 2;

class smtkRGGEditDuctViewInternals : public Ui::RGGEditDuctParameters
{
public:
  smtkRGGEditDuctViewInternals() {}

  ~smtkRGGEditDuctViewInternals()
  {
    if (CurrentAtt)
      delete CurrentAtt;
  }

  qtAttribute* createAttUI(smtk::attribute::AttributePtr att,
    const smtk::view::View::Component& comp, QWidget* pw, qtBaseView* view)
  {
    if (att && att->numberOfItems() > 0)
    {
      qtAttribute* attInstance = new qtAttribute(att, comp, pw, view);
      if (attInstance && attInstance->widget())
      {
        //Without any additional info lets use a basic layout with model associations
        // if any exists
        attInstance->createBasicLayout(true);
        attInstance->widget()->setObjectName("RGGDuctEditor");
        QVBoxLayout* parentlayout = static_cast<QVBoxLayout*>(pw->layout());
        parentlayout->insertWidget(0, attInstance->widget());
      }
      return attInstance;
    }
    return NULL;
  }

  QPointer<qtAttribute> CurrentAtt;

  smtk::weak_ptr<smtk::operation::Operation> CurrentOp;
};

smtkRGGEditDuctView::smtkRGGEditDuctView(const ViewInfo& info)
  : qtBaseView(info)
{
  this->Internals = new smtkRGGEditDuctViewInternals();
}

smtkRGGEditDuctView::~smtkRGGEditDuctView()
{
  delete this->Internals;
}

qtBaseView* smtkRGGEditDuctView::createViewWidget(const ViewInfo& info)
{
  smtkRGGEditDuctView* view = new smtkRGGEditDuctView(info);
  view->buildUI();
  return view;
}

bool smtkRGGEditDuctView::displayItem(smtk::attribute::ItemPtr item)
{
  return this->qtBaseView::displayItem(item);
}

void smtkRGGEditDuctView::requestModelEntityAssociation()
{
  this->updateAttributeData();
}

void smtkRGGEditDuctView::valueChanged(smtk::attribute::ItemPtr /*optype*/)
{
  this->requestOperation(this->Internals->CurrentOp.lock());
}

void smtkRGGEditDuctView::requestOperation(const smtk::operation::OperationPtr& op)
{
  if (!op || !op->parameters())
  {
    return;
  }
  this->uiManager()->activeModelView()->requestOperation(op, false);
}

void smtkRGGEditDuctView::cancelOperation(const smtk::operation::OperationPtr& op)
{
  if (!op || !this->Widget || !this->Internals->CurrentAtt)
  {
    return;
  }
  // Reset widgets here
}

void smtkRGGEditDuctView::clearSelection()
{
  this->uiManager()->activeModelView()->clearSelection();
}

void smtkRGGEditDuctView::attributeModified()
{
  // Always enable apply button here
}

void smtkRGGEditDuctView::onAttItemModified(smtk::extension::qtItem* item)
{
  smtk::attribute::ItemPtr itemPtr = item->item();
  // only changing duct would update edit duct panel
  if (itemPtr->name() == "duct" && itemPtr->type() == smtk::attribute::Item::Type::ModelEntityType)
  {
    this->updateEditDuctPanel();
  }
}

void smtkRGGEditDuctView::apply()
{
  // Fill the attribute - read all data from UI
  smtk::attribute::AttributePtr att = this->Internals->CurrentAtt->attribute();
  smtk::attribute::StringItemPtr nameI = att->findString("name");
  if (nameI)
  {
    nameI->setValue(this->Internals->ductName->text().toStdString());
  }

  smtk::attribute::VoidItemPtr crossSectionI = att->findVoid("cross section");

  if (crossSectionI)
  {
    crossSectionI->setIsEnabled(this->Internals->isCrossSectionButton->isChecked());
  }

  smtk::attribute::GroupItemPtr segsI = att->findGroup("duct segments");
  segsI->setNumberOfGroups(1); // Clear the existing groups
  QTableWidget* dst = this->Internals->ductSegmentTable;
  for (int i = 0; i < dst->rowCount(); i++)
  {
    if (i > 0)
    { // Attribute would create an empty group since required value is set to 1
      segsI->appendGroup();
    }
    smtk::attribute::DoubleItemPtr zValuesI =
      smtk::dynamic_pointer_cast<smtk::attribute::DoubleItem>(segsI->item(i, 0));
    smtk::attribute::IntItemPtr materialsI =
      smtk::dynamic_pointer_cast<smtk::attribute::IntItem>(segsI->item(i, 1));
    materialsI->setNumberOfValues(1); // Clear previous value
    smtk::attribute::DoubleItemPtr thicknessesI =
      smtk::dynamic_pointer_cast<smtk::attribute::DoubleItem>(segsI->item(i, 2));
    thicknessesI->setNumberOfValues(1); // Clear previous value
    // Fill z values first
    zValuesI->setValue(0, dst->item(i, 0)->text().toDouble());
    zValuesI->setValue(1, dst->item(i, 1)->text().toDouble());

    smtk::model::IntegerList currentMs;
    smtk::model::FloatList currentTs;
    QTableWidget* mlt = dynamic_cast<QTableWidget*>(this->Internals->materialLayerStack->widget(i));
    for (int j = 0; j < mlt->rowCount(); j++)
    {
      int materialIndex = dynamic_cast<QComboBox*>(mlt->cellWidget(j, 0))->currentIndex();
      currentMs.push_back(materialIndex);
      double radiusN1 = mlt->item(j, 1)->text().toDouble();
      if (this->Internals->ductPitchX->isHidden())
      {
        currentTs.push_back(radiusN1);
        currentTs.push_back(radiusN1);
      }
      else
      {
        currentTs.push_back(radiusN1);
        double radiusN2 = mlt->item(j, 2)->text().toDouble();
        currentTs.push_back(radiusN2);
      }
    }
    materialsI->setValues(currentMs.begin(), currentMs.end());
    thicknessesI->setValues(currentTs.begin(), currentTs.end());
  }

  this->requestOperation(this->Internals->CurrentOp.lock());
}

void smtkRGGEditDuctView::updateAttributeData()
{
  smtk::view::ViewPtr view = this->getObject();
  if (!view || !this->Widget)
  {
    return;
  }

  if (this->Internals->CurrentAtt)
  {
    delete this->Internals->CurrentAtt;
  }

  int i = view->details().findChild("AttributeTypes");
  if (i < 0)
  {
    return;
  }
  smtk::view::View::Component& comp = view->details().child(i);
  std::string defName;
  for (std::size_t ci = 0; ci < comp.numberOfChildren(); ++ci)
  {
    smtk::view::View::Component& attComp = comp.child(ci);
    if (attComp.name() != "Att")
    {
      continue;
    }
    std::string optype;
    if (attComp.attribute("Type", optype) && !optype.empty())
    {
      if (optype == "edit duct")
      {
        defName = optype;
        break;
      }
    }
  }
  if (defName.empty())
  {
    return;
  }

  smtk::operation::OperationPtr createDuctOp =
    this->uiManager()->activeModelView()->operatorsWidget()->existingOperation(defName);
  this->Internals->CurrentOp = createDuctOp;

  smtk::attribute::AttributePtr att = createDuctOp->parameters();
  this->Internals->CurrentAtt = this->Internals->createAttUI(att, comp, this->Widget, this);
  if (this->Internals->CurrentAtt)
  {
    QObject::connect(this->Internals->CurrentAtt, &qtAttribute::modified, this,
      &smtkRGGEditDuctView::attributeModified);
    QObject::connect(this->Internals->CurrentAtt, &qtAttribute::itemModified, this,
      &smtkRGGEditDuctView::onAttItemModified);
    // Associate duct combobox with the latest duct
    smtk::attribute::ReferenceItemPtr ductI =
      this->Internals->CurrentAtt->attribute()->associations();

    if (!ductI)
    {
      smtkErrorMacro(
        smtk::io::Logger::instance(), "Edit duct operator does not have a duct association");
      return;
    }
    smtk::model::Model model = qtActiveObjects::instance().activeModel();
    if (model.hasStringProperty("latest duct"))
    {
      smtk::model::EntityRef latestDuct = smtk::model::EntityRef(
        model.resource(), smtk::common::UUID(model.stringProperty("latest duct")[0]));
      ductI->setObjectValue(latestDuct.component());
    }
    this->updateEditDuctPanel();
  }
}

void smtkRGGEditDuctView::createWidget()
{
  smtk::view::ViewPtr view = this->getObject();
  if (!view)
  {
    return;
  }

  QVBoxLayout* parentLayout = dynamic_cast<QVBoxLayout*>(this->parentWidget()->layout());

  // Delete any pre-existing widget
  if (this->Widget)
  {
    if (parentLayout)
    {
      parentLayout->removeWidget(this->Widget);
    }
    delete this->Widget;
  }

  // Create a new frame and lay it out
  this->Widget = new QFrame(this->parentWidget());
  QVBoxLayout* layout = new QVBoxLayout(this->Widget);
  layout->setMargin(0);
  this->Widget->setLayout(layout);
  this->Widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

  // QUESTION: You might need to keep tracking of the widget
  QWidget* tempWidget = new QWidget(this->parentWidget());
  this->Internals->setupUi(tempWidget);
  tempWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
  layout->addWidget(tempWidget, 1);
  // Make sure that we have enough space for the custom widget
  this->Internals->scrollArea->setMinimumHeight(650);

  QObject::disconnect(this->uiManager()->activeModelView());
  QObject::connect(this->uiManager()->activeModelView(),
    &smtk::extension::qtModelView::operationCancelled, this, &smtkRGGEditDuctView::cancelOperation);

  // Duct height is modified at core level
  this->Internals->z1LineEdit->setEnabled(false);
  this->Internals->z2LineEdit->setEnabled(false);

  QObject::connect(this->Internals->isHexButton, &QCheckBox::stateChanged, this, [=](int status) {
    this->Internals->ductPitchX->setHidden(status);
    this->Internals->ductPitchXLabel->setHidden(status);
    // Always show pitch y lineEdit widget
    this->Internals->ductPitchY->setToolTip(
      QString::fromStdString(status ? "Hex duct's pitch" : "Rect duct's pitch along y axis"));
    this->Internals->ductPitchYLabel->setHidden(status);
  });
  this->Internals->ductPitchX->setToolTip(QString::fromStdString("Rect duct's pitch along x axis"));

  this->createDuctSegmentsTable();
  // Material layers buttons
  QObject::connect(this->Internals->addMaterialLayerBefore, &QPushButton::released, this,
    &smtkRGGEditDuctView::onAddMaterialLayerBefore);
  QObject::connect(this->Internals->addMaterialLayerAfter, &QPushButton::released, this,
    &smtkRGGEditDuctView::onAddMaterialLayerAfter);
  QObject::connect(this->Internals->deleteMaterialLayer, &QPushButton::released, this,
    &smtkRGGEditDuctView::onDeleteMaterialLayer);

  // Show help when the info button is clicked.
  QObject::connect(
    this->Internals->infoButton, &QPushButton::released, this, &smtkRGGEditDuctView::onInfo);

  QObject::connect(
    this->Internals->applyButton, &QPushButton::released, this, &smtkRGGEditDuctView::apply);

  this->updateAttributeData();
}

void smtkRGGEditDuctView::updateEditDuctPanel()
{
  smtk::model::EntityRefArray ents = this->Internals->CurrentAtt->attribute()
                                       ->associatedModelEntities<smtk::model::EntityRefArray>();
  bool isEnabled(true);
  if ((ents.size() == 0) || (!ents[0].hasStringProperty("rggType")) ||
    (ents[0].stringProperty("rggType")[0] != SMTK_SESSION_RGG_DUCT) ||
    (ents[0].embeddedEntities<smtk::model::EntityRefArray>().size() == 0))
  { // Its type is not rgg duct
    isEnabled = false;
  }
  if (this->Internals)
  {
    this->Internals->scrollArea->setEnabled(isEnabled);
  }

  if (isEnabled)
  {
    // Populate the panel
    AuxiliaryGeometry duct = ents[0].as<AuxiliaryGeometry>();
    // Name
    this->Internals->ductName->setText(QString::fromStdString(duct.name()));

    // IsHex
    bool isHex;
    this->Internals->isHexButton->setEnabled(true);
    if (duct.owningModel().hasIntegerProperty("hex"))
    {
      isHex = duct.owningModel().integerProperty("hex")[0];
      this->Internals->isHexButton->setChecked(isHex);
    }
    // isHex would have an impact on pitch options.
    // For now it's specified at core creation time
    this->Internals->isHexButton->setEnabled(false);

    // isCrossSection
    if (duct.hasIntegerProperty("cross section"))
    {
      bool isCS = duct.integerProperty("cross section")[0];
      this->Internals->isCrossSectionButton->setChecked(isCS);
    }

    // Pitch
    if (duct.owningModel().hasFloatProperty("duct thickness"))
    {
      FloatList pitches = duct.owningModel().floatProperty("duct thickness");
      if (isHex)
      { // Since hex pitch has two same values, we only show one
        this->Internals->ductPitchY->setText(QString::number(pitches[0]));
      }
      else
      {
        this->Internals->ductPitchX->setText(QString::number(pitches[0]));
        this->Internals->ductPitchY->setText(QString::number(pitches[1]));
      }
    }

    // Duct height
    FloatList ductHeight = duct.owningModel().floatProperty("duct height");
    if (ductHeight.size() == 2)
    {
      this->Internals->z1LineEdit->setText(QString::number(ductHeight[0]));
      this->Internals->z2LineEdit->setText(QString::number(ductHeight[1]));
    }
    else
    {
      smtkErrorMacro(smtk::io::Logger::instance(), "duct height does not have"
                                                   "two values!");
    }

    smtk::model::IntegerList numMaterialsPerSeg;
    if (duct.hasIntegerProperty("material nums per segment"))
    {
      numMaterialsPerSeg = duct.integerProperty("material nums per segment");
    }

    smtk::model::FloatList zValues; // [z1,z2,z1,z2,...]
    if (duct.hasFloatProperty("z values"))
    {
      zValues = duct.floatProperty("z values");
    }
    // Clear the duct segment table first then populate it
    this->Internals->ductSegmentTable->clearSpans();
    this->Internals->ductSegmentTable->model()->removeRows(
      0, this->Internals->ductSegmentTable->rowCount());
    for (size_t index = 0; index < zValues.size() / 2; index++)
    {
      this->addSegmentToTable(static_cast<int>(index), zValues[2 * index], zValues[2 * index + 1]);
    }

    smtk::model::IntegerList materials;
    if (duct.hasIntegerProperty("materials"))
    {
      materials = duct.integerProperty("materials");
    }

    smtk::model::FloatList thicknessesN;
    if (duct.hasFloatProperty("thicknesses(normalized)"))
    {
      thicknessesN = duct.floatProperty("thicknesses(normalized)");
    }

    // Clear the material layer stack widget first
    while (this->Internals->materialLayerStack->count() > 0)
    {
      QWidget* tmp = this->Internals->materialLayerStack->widget(
        this->Internals->materialLayerStack->count() - 1);
      this->Internals->materialLayerStack->removeWidget(tmp);
    }
    // Populate the material layer tables
    size_t offset(0);
    for (size_t index = 0; index < numMaterialsPerSeg.size(); index++)
    {
      this->createMaterialLayersTable(
        index, offset, numMaterialsPerSeg[index], materials, thicknessesN);
      offset += numMaterialsPerSeg[index];
    }
    this->Internals->materialLayerStack->setCurrentIndex(0);
  }
  this->updateButtonStatus();
}

void smtkRGGEditDuctView::setInfoToBeDisplayed()
{
  this->m_infoDialog->displayInfo(this->getObject());
}

void smtkRGGEditDuctView::createDuctSegmentsTable()
{
  QTableWidget* dsT = this->Internals->ductSegmentTable;
  dsT->clear();
  dsT->horizontalHeader()->setStretchLastSection(true);
  dsT->setColumnCount(numberOfSegmentTableColumns);
  dsT->setHorizontalHeaderLabels(QStringList() << "Z1"
                                               << "Z2");

  // Split
  QObject::connect(
    this->Internals->segSplit, &QPushButton::pressed, this, &smtkRGGEditDuctView::onSegmentSplit);

  // Delete up
  QObject::connect(this->Internals->segDeleteUp, &QPushButton::pressed, this, [=]() {
    QTableWidget* dst = this->Internals->ductSegmentTable;
    // If there is no selected item then the button would be disabled. we can
    // safely ask for the 1st item
    QTableWidgetItem* selected = dst->selectedItems().value(0);
    int currentRow = selected->row();
    if (currentRow)
    {
      dst->blockSignals(true);
      dst->removeRow(currentRow - 1);
      dst->blockSignals(false);
    }
    if (currentRow == 1)
    { // Update the lower bound
      dst->item(0, 0)->setText(this->Internals->z1LineEdit->text());
    }
    // Remove the corresponding table widget
    this->Internals->materialLayerStack->removeWidget(
      this->Internals->materialLayerStack->widget(currentRow - 1));
    this->updateButtonStatus();

  });

  // Delete down
  QObject::connect(this->Internals->segDeleteDown, &QPushButton::pressed, this, [=]() {
    QTableWidget* dst = this->Internals->ductSegmentTable;
    // If there is no selected item then the button would be disabled. we can
    // safely ask for the 1st item
    QTableWidgetItem* selected = dst->selectedItems().value(0);
    int currentRow = selected->row(), rowC = dst->rowCount();
    if (currentRow < (rowC - 1))
    {
      dst->blockSignals(true);
      dst->removeRow(currentRow + 1);
      dst->blockSignals(false);
    }
    else
    {
      return;
    }
    if (currentRow == (rowC - 2))
    { // Update the lower bound
      dst->item(dst->rowCount() - 1, 1)->setText(this->Internals->z2LineEdit->text());
    }
    // Remove the corresponding table widget
    this->Internals->materialLayerStack->removeWidget(
      this->Internals->materialLayerStack->widget(currentRow + 1));
    this->updateButtonStatus();
  });

  // Selection
  QObject::connect(this->Internals->ductSegmentTable, &QTableWidget::itemSelectionChanged, this,
    &smtkRGGEditDuctView::updateButtonStatus);
  // Bring up the material layer table associated with current row
  QObject::connect(
    this->Internals->ductSegmentTable, &QTableWidget::itemSelectionChanged, this, [=]() {
      int row = this->Internals->ductSegmentTable->currentRow();
      this->Internals->materialLayerStack->setCurrentIndex(row);
    });
}

void smtkRGGEditDuctView::addSegmentToTable(int row, double z1, double z2)
{
  QTableWidget* dst = this->Internals->ductSegmentTable;
  if (row < 0 || row > dst->rowCount())
  { // invalid input
    return;
  }
  dst->blockSignals(true);
  dst->insertRow(row);
  // z1
  QTableWidgetItem* z1Item = new dSTableItem();
  z1Item->setText(QString::number(z1));
  dst->setItem(row, 0, z1Item);
  // z2
  QTableWidgetItem* z2Item = new dSTableItem();
  z2Item->setText(QString::number(z2));
  dst->setItem(row, 1, z2Item);
  dst->blockSignals(false);
  this->updateButtonStatus();
}

void smtkRGGEditDuctView::onSegmentSplit()
{
  QTableWidget* dst = this->Internals->ductSegmentTable;
  // If there is no selected item then the buttons would be disabled. we can
  // safely ask for the 1st item
  QTableWidgetItem* selected = dst->selectedItems().value(0);
  QTableWidgetItem* z1 = dst->item(selected->row(), 0);
  QTableWidgetItem* z2 = dst->item(selected->row(), 1);
  // Calcuate average and update z1
  double min(z1->text().toDouble()), max(z2->text().toDouble()), average = (min + max) / 2;
  z1->setText(QString::number(average));
  this->addSegmentToTable(selected->row(), min, average);
  // Create the corresponding material table based on selected row
  IntegerList materials;
  FloatList thicknessesN;
  // Extract the info from current material layers table
  QTableWidget* mlt =
    dynamic_cast<QTableWidget*>(this->Internals->materialLayerStack->currentWidget());
  for (int i = 0; i < mlt->rowCount(); i++)
  {
    QComboBox* box = dynamic_cast<QComboBox*>(mlt->cellWidget(i, 0));
    materials.push_back(box ? box->currentIndex() : 0);
    thicknessesN.push_back(mlt->item(i, 1)->text().toDouble());
    if (this->Internals->ductPitchX->isHidden())
    { // Hex
      thicknessesN.push_back(mlt->item(i, 1)->text().toDouble());
    }
    else
    {
      thicknessesN.push_back(mlt->item(i, 2)->text().toDouble());
    }
  }
  this->createMaterialLayersTable(selected->row(), 0, materials.size(), materials, thicknessesN);
}

void smtkRGGEditDuctView::createMaterialLayersTable(const size_t index, const size_t offSet,
  const size_t numberOfMaterials, const IntegerList& materials, const FloatList& thicknessesN)
{
  // Create the table widget
  QTableWidget* mlt = new QTableWidget;
  // Selection
  QObject::connect(
    mlt, &QTableWidget::itemSelectionChanged, this, &smtkRGGEditDuctView::updateButtonStatus);

  if (this->Internals->ductPitchX->isHidden())
  { // If we hide the ductPitchX then it's a hex duct.
    mlt->setColumnCount(2);
    mlt->setHorizontalHeaderLabels(QStringList() << "Material"
                                                 << "Raidus\n(normalized)");
  }
  else
  {
    mlt->setColumnCount(3);
    mlt->setHorizontalHeaderLabels(QStringList() << "Material"
                                                 << "Raidus\n(normalized)"
                                                 << "Raidus\n(normalized)");
  }
  mlt->horizontalHeader()->setStretchLastSection(true);
  mlt->setSelectionBehavior(QAbstractItemView::SelectRows);

  for (size_t mI = 0; mI < numberOfMaterials; mI++)
  {
    this->addMaterialLayerToTable(mlt, static_cast<int>(mI), materials[offSet + mI],
      thicknessesN[2 * (offSet + mI)], thicknessesN[2 * (offSet + mI) + 1]);
  }

  this->Internals->materialLayerStack->insertWidget(static_cast<int>(index), mlt);
}

void smtkRGGEditDuctView::onAddMaterialLayerBefore()
{
  QTableWidget* mlt =
    dynamic_cast<QTableWidget*>(this->Internals->materialLayerStack->currentWidget());
  int row;
  if (mlt->selectedItems().count() != 0)
  {
    row = mlt->selectedItems()[0]->row();
  }
  else
  {
    return;
  }
  // generate the new thickness1 and thickness2
  double beforeTN0, afterTN0, beforeTN1, afterTN1, averageTN0, averageTN1;
  beforeTN0 = (row == 0) ? 0 : mlt->item(row - 1, 1)->text().toDouble();
  afterTN0 = mlt->item(row, 1)->text().toDouble();
  averageTN0 = (beforeTN0 + afterTN0) / 2;
  if (this->Internals->ductPitchX->isHidden())
  { // Hex has only one normalized thicknesses
    this->addMaterialLayerToTable(mlt, row, 0, averageTN0, averageTN0);
  }
  else
  {
    beforeTN1 = (row == 0) ? 0 : mlt->item(row - 1, 2)->text().toDouble();
    afterTN1 = mlt->item(row, 2)->text().toDouble();
    averageTN1 = (beforeTN1 + afterTN1) / 2;
    this->addMaterialLayerToTable(mlt, row, 0, averageTN0, averageTN1);
  }
  this->updateButtonStatus();
}

void smtkRGGEditDuctView::onAddMaterialLayerAfter()
{
  QTableWidget* mlt =
    dynamic_cast<QTableWidget*>(this->Internals->materialLayerStack->currentWidget());
  int row;
  if (mlt->selectedItems().count() != 0)
  {
    row = mlt->selectedItems()[0]->row();
  }
  else
  {
    return;
  }
  // generate the new thickness1 and thickness2
  double beforeTN0, afterTN0, beforeTN1, afterTN1, averageTN0, averageTN1;
  beforeTN0 = mlt->item(row, 1)->text().toDouble();
  if (row == (mlt->rowCount() - 1))
  { // Cannot add after the last item
    return;
  }
  afterTN0 = mlt->item(row + 1, 1)->text().toDouble();
  averageTN0 = (beforeTN0 + afterTN0) / 2;
  if (this->Internals->ductPitchX->isHidden())
  { // Hex has only one normalized thicknesses
    this->addMaterialLayerToTable(mlt, row, 0, averageTN0, averageTN0);
  }
  else
  {
    beforeTN1 = mlt->item(row, 2)->text().toDouble();
    afterTN1 = mlt->item(row + 1, 2)->text().toDouble();
    averageTN1 = (beforeTN1 + afterTN1) / 2;
    this->addMaterialLayerToTable(mlt, row + 1, 0, averageTN0, averageTN1);
  }
  this->updateButtonStatus();
}

void smtkRGGEditDuctView::onDeleteMaterialLayer()
{
  QTableWidget* mlt =
    dynamic_cast<QTableWidget*>(this->Internals->materialLayerStack->currentWidget());
  int row = mlt->currentRow();
  mlt->blockSignals(true);
  if (row == (mlt->rowCount() - 1))
  { // Upper bound of normalized radius should always be 1
    mlt->item(row - 1, 1)->setText(QString::number(1.0));
    if (!this->Internals->ductPitchX->isHidden())
    { // Rect duct has two different normalized thicknesses
      mlt->item(row - 1, 2)->setText(QString::number(1.0));
    }
  }
  mlt->removeRow(row);
  mlt->blockSignals(false);
  this->updateButtonStatus();
}

void smtkRGGEditDuctView::addMaterialLayerToTable(
  QTableWidget* mlt, int row, int subMaterial, double thickness1, double thickness2)
{
  mlt->blockSignals(true);
  mlt->insertRow(row);
  // Create the material combobox first
  QComboBox* materialCom = new QComboBox(mlt);
  materialCom->setObjectName("DuctMaterialBox_" + QString::number(row));
  mlt->setCellWidget(row, 0, materialCom);
  materialCom->blockSignals(true);
  this->setupMaterialComboBox(materialCom);
  materialCom->blockSignals(false);
  materialCom->setCurrentIndex(subMaterial);

  // Normalized thicknessnesses
  rangeItem* radiusNItem = new rangeItem();
  radiusNItem->setText(QString::number(thickness1));
  mlt->setItem(row, 1, radiusNItem);

  if (!this->Internals->ductPitchX->isHidden())
  { // For a rect duct, we have radius along length and width
    rangeItem* radiusNItem2 = new rangeItem();
    radiusNItem2->setText(QString::number(thickness2));
    mlt->setItem(row, 2, radiusNItem2);
  }
  mlt->blockSignals(false);
}

void smtkRGGEditDuctView::updateButtonStatus()
{
  bool deleteDuctStatus = this->Internals->ductSegmentTable->rowCount() > 1;
  this->Internals->segDeleteUp->setEnabled(deleteDuctStatus);
  this->Internals->segDeleteDown->setEnabled(deleteDuctStatus);
  if (this->Internals->ductSegmentTable->selectedItems().size() > 0)
  {
    this->Internals->segSplit->setEnabled(true);
    int row = this->Internals->ductSegmentTable->selectedItems()[0]->row();
    this->Internals->segDeleteUp->setEnabled((row != 0));
    this->Internals->segDeleteDown->setEnabled(
      (row != (this->Internals->ductSegmentTable->rowCount() - 1)));
  }
  else
  {
    this->Internals->segSplit->setEnabled(false);
  }

  QTableWidget* mlt =
    dynamic_cast<QTableWidget*>(this->Internals->materialLayerStack->currentWidget());
  bool deleteMaterialStatus(false), hasSelection(false), canAddAfter(false);
  if (mlt)
  {
    hasSelection = mlt->selectedItems().size() > 0;
    deleteMaterialStatus = (mlt->rowCount() > 1) && hasSelection;
    if (hasSelection)
    { // Can not add after the last row
      canAddAfter = mlt->selectedItems()[0]->row() != (mlt->rowCount() - 1);
    }
  }
  this->Internals->addMaterialLayerBefore->setEnabled(hasSelection);
  this->Internals->addMaterialLayerAfter->setEnabled(canAddAfter);
  this->Internals->deleteMaterialLayer->setEnabled(deleteMaterialStatus);
}

void smtkRGGEditDuctView::setupMaterialComboBox(QComboBox* box, bool isCell)
{
  box->clear();
  smtk::model::Model model = qtActiveObjects::instance().activeModel();
  size_t matN = smtk::session::rgg::CreateModel::materialNum(model);
  for (size_t i = 0; i < matN; i++)
  {
    std::string name;
    smtk::session::rgg::CreateModel::getMaterial(i, name, model);
    box->addItem(QString::fromStdString(name));
  }
  if (isCell)
  {
    // In this condition, the part does not need to have a material. Change the item text
  }
}
