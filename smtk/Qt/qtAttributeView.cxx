/*=========================================================================

Copyright (c) 1998-2003 Kitware Inc. 469 Clifton Corporate Parkway,
Clifton Park, NY, 12065, USA.

All rights reserved. No part of this software may be reproduced, distributed,
or modified, in any form or by any means, without permission in writing from
Kitware Inc.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

=========================================================================*/

#include "smtk/Qt/qtAttributeView.h"

#include "smtk/Qt/qtUIManager.h"
#include "smtk/Qt/qtTableWidget.h"
#include "smtk/Qt/qtAttribute.h"
#include "smtk/Qt/qtAttributeRefItem.h"
#include "smtk/Qt/qtAssociationWidget.h"
#include "smtk/Qt/qtReferencesWidget.h"
#include "smtk/Qt/qtItem.h"
#include "smtk/Qt/qtVoidItem.h"

#include "smtk/attribute/Attribute.h"
#include "smtk/attribute/RefItem.h"
#include "smtk/attribute/Definition.h"
#include "smtk/attribute/ItemDefinition.h"
#include "smtk/attribute/Manager.h"
#include "smtk/attribute/GroupItem.h"
#include "smtk/attribute/GroupItemDefinition.h"
#include "smtk/attribute/ValueItem.h"
#include "smtk/attribute/ValueItemDefinition.h"
#include "smtk/attribute/VoidItem.h"

#include "smtk/view/Attribute.h"

#include <QGridLayout>
#include <QComboBox>
#include <QTableWidgetItem>
#include <QVariant>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QKeyEvent>
#include <QModelIndex>
#include <QModelIndexList>
#include <QMessageBox>
#include <QSplitter>
#include <QPointer>
#include <QBrush>
#include <QColorDialog>
#include <QHeaderView>

#include <iostream>
#include <set>
using namespace smtk::attribute;

//----------------------------------------------------------------------------
class qtAttributeViewInternals
{
public:

  const QList<smtk::attribute::DefinitionPtr> getCurrentDefs(
    const QString strCategory) const
  {

    if(this->AttDefMap.keys().contains(strCategory))
      {
      return this->AttDefMap[strCategory];
      }
    return this->AllDefs;
  }
  qtTableWidget* ListTable;
  qtTableWidget* ValuesTable;

  QPushButton* AddButton;
  QComboBox* DefsCombo;
  QPushButton* DeleteButton;
  QPushButton* CopyButton;

  QFrame* FiltersFrame;
  QFrame* ButtonsFrame;
  QFrame* TopFrame; // top
  QFrame* BottomFrame; // bottom

  QComboBox* ViewByCombo;
  QComboBox* PropDefsCombo;

  QPointer<qtAssociationWidget> AssociationsWidget;
  QPointer<qtReferencesWidget> ReferencesWidget;

  // <category, AttDefinitions>
  QMap<QString, QList<smtk::attribute::DefinitionPtr> > AttDefMap;

  // All definitions list
  QList<smtk::attribute::DefinitionPtr> AllDefs;

  // Attribute widget
  QPointer<qtAttribute> CurrentAtt;
  QPointer<QFrame> AttFrame;
  QMap<std::string, Qt::CheckState> AttProperties;
  // Compared <attribute, table-row > list
  // QMap<smtk::attribute::AttributePtr, int> ComparedPropMap;

};

//----------------------------------------------------------------------------
qtAttributeView::
qtAttributeView(smtk::view::BasePtr dataObj, QWidget* p, qtUIManager* uiman) :
  qtBaseView(dataObj, p, uiman)
{
  this->Internals = new qtAttributeViewInternals;
  this->createWidget( );
}

//----------------------------------------------------------------------------
qtAttributeView::~qtAttributeView()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
const QMap<QString, QList<smtk::attribute::DefinitionPtr> > &qtAttributeView::
 attDefinitionMap() const
 {
   return this->Internals->AttDefMap;
 }

//----------------------------------------------------------------------------
void qtAttributeView::createWidget( )
{
  if(!this->getObject())
    {
    return;
    }
  // Create a frame to contain all gui components for this object
  // Create a list box for the group entries
  // Create a table widget
  // Add link from the listbox selection to the table widget
  // A common add/delete/(copy/paste ??) widget

  QSplitter* frame = new QSplitter(this->parentWidget());
  //this panel looks better in a over / under layout, rather than left / right
  frame->setOrientation( Qt::Vertical );

  QFrame* TopFrame = new QFrame(frame);
  QFrame* BottomFrame = new QFrame(frame);

  this->Internals->TopFrame = TopFrame;
  this->Internals->BottomFrame = BottomFrame;
  QSizePolicy sizeFixedPolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  QVBoxLayout* TopLayout = new QVBoxLayout(TopFrame);
  TopLayout->setMargin(0);
  TopFrame->setSizePolicy(sizeFixedPolicy);
  QVBoxLayout* BottomLayout = new QVBoxLayout(BottomFrame);
  BottomLayout->setMargin(0);

  // create a filter-frame with ViewBy-combo
  this->Internals->FiltersFrame = new QFrame(frame);
  QGridLayout* filterLayout = new QGridLayout(this->Internals->FiltersFrame);
  filterLayout->setMargin(0);
  this->Internals->FiltersFrame->setSizePolicy(sizeFixedPolicy);

  this->Internals->ViewByCombo = new QComboBox(this->Internals->FiltersFrame);
  this->Internals->ViewByCombo->addItem("Attribute");
  this->Internals->ViewByCombo->addItem("Property");
  QLabel* labelViewBy = new QLabel("View By: ", this->Internals->FiltersFrame);
  filterLayout->addWidget(labelViewBy, 0, 0);
  filterLayout->addWidget(this->Internals->ViewByCombo, 0, 1);

  this->Internals->PropDefsCombo = new QComboBox(TopFrame);
  this->Internals->PropDefsCombo->setVisible(false);
  filterLayout->addWidget(this->Internals->PropDefsCombo, 0, 2);
  QObject::connect(this->Internals->PropDefsCombo,  SIGNAL(currentIndexChanged(int)),
    this, SLOT(onPropertyDefSelected()), Qt::QueuedConnection);

  const Manager* attMan = this->uiManager()->attManager();
  std::set<std::string>::const_iterator it;
  const std::set<std::string> &cats = attMan->categories();
  this->Internals->AttDefMap.clear();
  for (it = cats.begin(); it != cats.end(); it++)
    {
    QList<smtk::attribute::DefinitionPtr> attdeflist;
    this->Internals->AttDefMap[it->c_str()] = attdeflist;
    }

  QSizePolicy tableSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  // create a list box for all the entries
  this->Internals->ListTable = new qtTableWidget(frame);
  this->Internals->ListTable->setSelectionMode(QAbstractItemView::SingleSelection);
  this->Internals->ListTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Internals->ListTable->setSizePolicy(tableSizePolicy);

  // Buttons frame
  this->Internals->ButtonsFrame = new QFrame(frame);
  QHBoxLayout* buttonLayout = new QHBoxLayout(this->Internals->ButtonsFrame);
  buttonLayout->setMargin(0);
  this->Internals->ButtonsFrame->setSizePolicy(sizeFixedPolicy);

  this->Internals->AddButton = new QPushButton("New", this->Internals->ButtonsFrame);
  this->Internals->AddButton->setSizePolicy(sizeFixedPolicy);
  this->Internals->DeleteButton = new QPushButton("Delete", this->Internals->ButtonsFrame);
  this->Internals->DeleteButton->setSizePolicy(sizeFixedPolicy);
  this->Internals->CopyButton = new QPushButton("Copy", this->Internals->ButtonsFrame);
  this->Internals->CopyButton->setSizePolicy(sizeFixedPolicy);
  this->Internals->DefsCombo = new QComboBox(this->Internals->ButtonsFrame);
  this->Internals->DefsCombo->setVisible(false);
  buttonLayout->addWidget(this->Internals->DefsCombo);

  buttonLayout->addWidget(this->Internals->AddButton);
  buttonLayout->addWidget(this->Internals->CopyButton);
  buttonLayout->addWidget(this->Internals->DeleteButton);

  // Attribute table
  this->Internals->ValuesTable = new qtTableWidget(frame);
  this->Internals->ValuesTable->setSizePolicy(tableSizePolicy);

  TopLayout->addWidget(this->Internals->FiltersFrame);
  TopLayout->addWidget(this->Internals->ListTable);
  TopLayout->addWidget(this->Internals->ButtonsFrame);

  BottomLayout->addWidget(this->Internals->ValuesTable);
  this->Internals->ValuesTable->setVisible(0);

  // Attribte frame
  this->Internals->AttFrame = new QFrame(frame);
  new QVBoxLayout(this->Internals->AttFrame);
  BottomLayout->addWidget(this->Internals->AttFrame);
//  this->Internals->AttFrame->setVisible(0);

  frame->addWidget(TopFrame);
  frame->addWidget(BottomFrame);

  // the association widget
  this->Internals->ReferencesWidget = new qtReferencesWidget(frame);
  this->Internals->AssociationsWidget = new qtAssociationWidget(frame, this);
  this->updateAssociationEnableState(smtk::attribute::AttributePtr());
  BottomLayout->addWidget(this->Internals->AssociationsWidget);
  BottomLayout->addWidget(this->Internals->ReferencesWidget);

  this->Internals->ListTable->horizontalHeader()->setResizeMode(
    QHeaderView::ResizeToContents);
  this->Internals->ValuesTable->horizontalHeader()->setResizeMode(
    QHeaderView::ResizeToContents);
  this->Internals->ValuesTable->verticalHeader()->setResizeMode(
    QHeaderView::ResizeToContents);
  this->Internals->ListTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
  this->Internals->ValuesTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

  // signals/slots
  QObject::connect(this->Internals->AssociationsWidget,
    SIGNAL(attAssociationChanged()), this, SIGNAL(attAssociationChanged()));

  QObject::connect(this->Internals->ViewByCombo,
    SIGNAL(currentIndexChanged(int)), this, SLOT(onViewBy(int)));

  QObject::connect(this->Internals->ListTable,
    SIGNAL(itemClicked (QTableWidgetItem*)),
    this, SLOT(onListBoxClicked(QTableWidgetItem*)));
  QObject::connect(this->Internals->ListTable,
    SIGNAL(itemSelectionChanged ()),
    this, SLOT(onListBoxSelectionChanged()));
  QObject::connect(this->Internals->ListTable,
    SIGNAL(itemChanged (QTableWidgetItem *)),
    this, SLOT(onAttributeNameChanged(QTableWidgetItem * )));
  // we need this so that the attribute name will also be changed
  // when a recorded test is play back, which is using setText
  // on the underline QLineEdit of the cell.
  QObject::connect(this->Internals->ListTable,
    SIGNAL(cellChanged (int, int)),
    this, SLOT(onAttributeCellChanged(int, int)));

  QObject::connect(this->Internals->AddButton,
    SIGNAL(clicked()), this, SLOT(onCreateNew()));
  QObject::connect(this->Internals->CopyButton,
    SIGNAL(clicked()), this, SLOT(onCopySelected()));
  QObject::connect(this->Internals->DeleteButton,
    SIGNAL(clicked()), this, SLOT(onDeleteSelected()));

  QObject::connect(this->Internals->ValuesTable,
    SIGNAL(itemChanged (QTableWidgetItem *)),
    this, SLOT(onAttributeValueChanged(QTableWidgetItem * )));
  //QObject::connect(this->Internals->ValuesTable,
  //  SIGNAL(keyPressed (QKeyEvent *)),
  //  this, SLOT(onAttributeTableKeyPress(QKeyEvent * )));
  this->Internals->ValuesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->Internals->ValuesTable->setSelectionMode(QAbstractItemView::SingleSelection);

  this->Internals->ValuesTable->setVisible(0);

  this->Widget = frame;
  if(this->parentWidget()->layout())
    {
    this->parentWidget()->layout()->addWidget(frame);
    }

  this->getAllDefinitions();

  this->updateModelAssociation();
}

//----------------------------------------------------------------------------
void qtAttributeView::updateModelAssociation()
{
  this->onShowCategory();
}
//-----------------------------------------------------------------------------
smtk::attribute::AttributePtr qtAttributeView::getAttributeFromItem(
  QTableWidgetItem * item)
{
  if(this->Internals->ViewByCombo->currentIndex() == VIEWBY_Attribute)
    {
    Attribute* rawPtr = item ?
      static_cast<Attribute*>(item->data(Qt::UserRole).value<void *>()) : NULL;
    return rawPtr ? rawPtr->pointer() : smtk::attribute::AttributePtr();
    }
  return smtk::attribute::AttributePtr();
}
//-----------------------------------------------------------------------------
smtk::attribute::ItemPtr qtAttributeView::getAttributeItemFromItem(
  QTableWidgetItem * item)
{
  if(this->Internals->ViewByCombo->currentIndex() == VIEWBY_PROPERTY)
    {
    Item* rawPtr = item ?
      static_cast<Item*>(item->data(Qt::UserRole).value<void *>()) : NULL;
    return rawPtr ? rawPtr->pointer() : smtk::attribute::ItemPtr();
    }
  return smtk::attribute::ItemPtr();
}
//-----------------------------------------------------------------------------
smtk::attribute::AttributePtr qtAttributeView::getSelectedAttribute()
{
  return this->getAttributeFromItem(this->getSelectedItem());
}

//-----------------------------------------------------------------------------
QTableWidgetItem *qtAttributeView::getSelectedItem()
{
  return this->Internals->ListTable->selectedItems().count()>0 ?
    this->Internals->ListTable->selectedItems().value(0) : NULL;
}

//----------------------------------------------------------------------------
void qtAttributeView::updateAssociationEnableState(
  smtk::attribute::AttributePtr theAtt)
{
  bool rvisible=false, avisible=false;
  if(theAtt)
    {
    if(theAtt->definition()->associationMask())
      {
      avisible = true;
      this->Internals->AssociationsWidget->showEntityAssociation(theAtt);
      }
    }
  this->Internals->AssociationsWidget->setVisible(avisible);
  this->Internals->ReferencesWidget->setVisible(rvisible);
}

//----------------------------------------------------------------------------
void qtAttributeView::onListBoxSelectionChanged()
{
  if(this->Internals->ViewByCombo->currentIndex() == VIEWBY_PROPERTY)
    {
    return;
    }

  this->Internals->ValuesTable->blockSignals(true);
  this->Internals->ValuesTable->clear();
  this->Internals->ValuesTable->setRowCount(0);
  this->Internals->ValuesTable->setColumnCount(0);
  QTableWidgetItem* current = this->getSelectedItem();

  if(current)
    {
    smtk::attribute::AttributePtr dataItem = this->getAttributeFromItem(current);
    this->updateAssociationEnableState(dataItem);
    if(dataItem)
      {
      this->updateTableWithAttribute(dataItem);
      }
    }
  else
    {
    this->updateAssociationEnableState(smtk::attribute::AttributePtr());
    }

  this->Internals->ValuesTable->blockSignals(false);
  this->Internals->ValuesTable->resizeColumnsToContents();
  this->Internals->ValuesTable->resizeRowsToContents();
}

//----------------------------------------------------------------------------
void qtAttributeView::onAttributeNameChanged(QTableWidgetItem* item)
{
  smtk::attribute::AttributePtr aAttribute = this->getAttributeFromItem(item);
  if(aAttribute && item->text().toStdString() != aAttribute->name())
    {
    Manager *attManager = aAttribute->definition()->manager();
    attManager->rename(aAttribute, item->text().toStdString());
    //aAttribute->definition()->setLabel(item->text().toAscii().constData());
    }
}
//----------------------------------------------------------------------------
void qtAttributeView::onAttributeCellChanged(int row, int col)
{
  if(col == 0)
    {
    if(this->Internals->ViewByCombo->currentIndex() == VIEWBY_Attribute)
      {
      QTableWidgetItem* item = this->Internals->ListTable->item(row, col);
      this->onAttributeNameChanged(item);
      }
    else
      {
      this->onPropertySelectionChanged(row, col);
      }
    }
}
//----------------------------------------------------------------------------
void qtAttributeView::updateTableWithProperties()
{
  if(this->Internals->ViewByCombo->currentIndex() == VIEWBY_Attribute)
    {
    return;
    }

  this->Internals->ValuesTable->blockSignals(true);
  this->Internals->ValuesTable->clear();
  this->Internals->ValuesTable->setRowCount(0);
  this->Internals->ValuesTable->setColumnCount(0);
//  this->Internals->ComparedPropMap.clear();
  this->updateAssociationEnableState(smtk::attribute::AttributePtr());

  Definition* rawPtr = static_cast<Definition*>(
    this->Internals->PropDefsCombo->itemData(
    this->Internals->PropDefsCombo->currentIndex(), Qt::UserRole).value<void *>());
  if(!rawPtr)
    {
    this->Internals->ValuesTable->blockSignals(false);
    return;
    }

  std::vector<smtk::attribute::AttributePtr> result;
  Manager *attManager = rawPtr->pointer()->manager();
  attManager->findAttributes(rawPtr->pointer(), result);
  if(result.size() == 0)
    {
    this->Internals->ValuesTable->blockSignals(false);
    return;
    }

  // create column headers
  QTableWidget* vtWidget = this->Internals->ValuesTable;
  vtWidget->setColumnCount(result.size() + 1);
  vtWidget->setHorizontalHeaderItem(0, new QTableWidgetItem("Property"));

  std::vector<smtk::attribute::AttributePtr>::iterator it;
  int j = 1;
  for (it=result.begin(); it!=result.end(); ++it, ++j)
    {
    vtWidget->setHorizontalHeaderItem(j, new QTableWidgetItem((*it)->name().c_str()));

    if(rawPtr->pointer()->advanceLevel())
      {
      vtWidget->horizontalHeaderItem(j)->setFont(this->uiManager()->advancedFont());
      }
    }

  for(int r=0; r<this->Internals->ListTable->rowCount(); r++)
    {
    QTableWidgetItem* current = this->Internals->ListTable->item(r, 0);
    if(current && current->checkState() == Qt::Checked)
      {
      smtk::attribute::ItemPtr attItem = this->getAttributeItemFromItem(current);
      smtk::attribute::AttributePtr att = attItem->attribute();
      this->addComparativeProperty(current, att->definition());
      }
    }

  this->Internals->ValuesTable->blockSignals(false);
  this->Internals->ValuesTable->resizeColumnsToContents();
  this->Internals->ValuesTable->resizeRowsToContents();
}
//----------------------------------------------------------------------------
void qtAttributeView::onPropertySelectionChanged(int row, int col)
{
  if(this->Internals->ViewByCombo->currentIndex() == VIEWBY_Attribute)
    {
    return;
    }

  this->Internals->ValuesTable->blockSignals(true);
//  this->Internals->ValuesTable->clear();
  QTableWidgetItem* current = this->Internals->ListTable->item(row, col);

  if(current)
    {
//    this->Internals->ValuesTable->setRowCount(0);
//    this->Internals->ValuesTable->setColumnCount(0);
//    this->updateAssociationEnableState(smtk::attribute::AttributePtr());
    smtk::attribute::ItemPtr attItem = this->getAttributeItemFromItem(current);
    smtk::attribute::AttributePtr att = attItem->attribute();
    std::string keyName = att->definition()->type() + attItem->name();
    this->Internals->AttProperties[keyName] = current->checkState();
    if(current->checkState() == Qt::Checked)
      {
      this->addComparativeProperty(current, att->definition());
      }
    else
      {
      this->removeComparativeProperty(current->text());
      }
    }

  this->Internals->ValuesTable->blockSignals(false);
  this->Internals->ValuesTable->resizeColumnsToContents();
  this->Internals->ValuesTable->resizeRowsToContents();
}

//----------------------------------------------------------------------------
void qtAttributeView::onAttributeValueChanged(QTableWidgetItem* item)
{
  Item* linkedData = item ?
    static_cast<Item*>(item->data(Qt::UserRole).value<void *>()) : NULL;

  if(linkedData && linkedData->isOptional())
    {
    linkedData->setIsEnabled(item->checkState()==Qt::Checked);
    this->updateChildWidgetsEnableState(
     linkedData->pointer(), item);
    }
/*
  smtk::attribute::ValueItemPtr dataItem = this->getSelectedArrayData();
  if(!dataItem)
    {
    return;
    }
  //this->uiManager()->updateArrayDataValue(dataItem, item);
*/
}
//----------------------------------------------------------------------------
void qtAttributeView::updateChildWidgetsEnableState(
  smtk::attribute::ItemPtr attItem, QTableWidgetItem* item)
{
  if(!item || !attItem || !attItem->isOptional())
    {
    return;
    }
  bool bEnabled = attItem->isEnabled();
  int startRow = item->row();

  if(attItem->type() == smtk::attribute::Item::GROUP)
    {
    smtk::attribute::GroupItemPtr grpItem = dynamic_pointer_cast<GroupItem>(attItem);
    if(grpItem)
      {
      int numItems = static_cast<int>(grpItem->numberOfItemsPerGroup());
      for (int j = 0; j < numItems; j++) // expecting one item for each column
        {
        this->updateItemWidgetsEnableState(
          grpItem->item(j), startRow,bEnabled);
        }
      }
    }
  else
    {
    this->updateItemWidgetsEnableState(attItem, startRow, bEnabled);
    }
}

//----------------------------------------------------------------------------
void qtAttributeView::updateItemWidgetsEnableState(
  smtk::attribute::ItemPtr inData, int& startRow, bool enabled)
{
  QTableWidget* tableWidget = this->Internals->ValuesTable;
  if(inData->type() == smtk::attribute::Item::ATTRIBUTE_REF)
    {
    QWidget* cellWidget = tableWidget->cellWidget(startRow, 1);
    if(cellWidget)
      {
      cellWidget->setEnabled(enabled);
      }
    }
  else if(inData->type() == smtk::attribute::Item::VOID)
    {
    QWidget* cellWidget = tableWidget->cellWidget(startRow, 0);
    if(cellWidget)
      {
      cellWidget->setEnabled(enabled);
      }
    }
  else if(dynamic_pointer_cast<ValueItem>(inData))
    {
    smtk::attribute::ValueItemPtr linkedData = dynamic_pointer_cast<ValueItem>(inData);
    for(std::size_t row=0; row<linkedData->numberOfValues(); row++, startRow++)
      {
      QWidget* cellWidget = tableWidget->cellWidget(startRow, 1);
      if(cellWidget)
        {
        cellWidget->setEnabled(enabled);
        }
      }
    }
}

//----------------------------------------------------------------------------
void qtAttributeView::onCreateNew()
{
  smtk::view::AttributePtr aview =
    smtk::dynamic_pointer_cast<smtk::view::Attribute>(this->getObject());
  if(!aview || !aview->numberOfDefinitions())
    {
    return;
    }
  attribute::DefinitionPtr newAttDef = aview->definition(0);

  QString strDef = this->Internals->DefsCombo->currentText();
  foreach (attribute::DefinitionPtr attDef,
    this->Internals->getCurrentDefs(
      this->uiManager()->currentCategory().c_str()))
    {
    if(strDef == QString::fromUtf8(attDef->label().c_str()))
      {
      newAttDef = attDef;
      break;
      }
    }
  this->createNewAttribute(newAttDef);
}

//----------------------------------------------------------------------------
void qtAttributeView::createNewAttribute(
  smtk::attribute::DefinitionPtr attDef)
{
  if(!attDef)
    {
    return;
    }

  Manager *attManager = attDef->manager();

  smtk::attribute::AttributePtr newAtt = attManager->createAttribute(attDef->type());
  QTableWidgetItem* item = this->addAttributeListItem(newAtt);
  if(item)
    {
    this->Internals->ListTable->selectRow(item->row());
    }
  emit this->numOfAttriubtesChanged();
}

//----------------------------------------------------------------------------
void qtAttributeView::onCopySelected()
{
  smtk::attribute::AttributePtr selObject = this->getSelectedAttribute();
  if(selObject)
    {
    this->createNewAttribute(selObject->definition());

    //if(newAttribute)
    //  {
    //  QTableWidgetItem* item = this->addAttributeListItem(newAttribute);
    //  if(item)
    //    {
    //    this->Internals->ListTable->setCurrentItem(item);
    //    }
    //  }
    }
}
//----------------------------------------------------------------------------
void qtAttributeView::onDeleteSelected()
{
  smtk::attribute::AttributePtr selObject = this->getSelectedAttribute();
  if(selObject)
    {

    attribute::DefinitionPtr attDef = selObject->definition();
    Manager *attManager = attDef->manager();
    attManager->removeAttribute(selObject);

    QTableWidgetItem* selItem = this->getSelectedItem();
    this->Internals->ListTable->removeRow(selItem->row());
    emit this->numOfAttriubtesChanged();
    }
}
//----------------------------------------------------------------------------
void qtAttributeView::addAttributePropertyItems(
  smtk::attribute::AttributePtr childData)
{
  if(!childData)
    {
    return;
    }

  if(this->Internals->PropDefsCombo->currentText() !=
    QString::fromUtf8(childData->definition()->label().c_str()))
    {
    return;
    }

  this->Internals->AttFrame->setVisible(0);
  this->Internals->ValuesTable->setVisible(1);

  Qt::ItemFlags nonEditableFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  Qt::ItemFlags checkableFlags = nonEditableFlags | Qt::ItemIsUserCheckable;
  // Now lets process its items
  std::size_t i, n = childData->numberOfItems();
  for (i = 0; i < n; i++)
    {
    smtk::attribute::ItemPtr attItem = childData->item(static_cast<int>(i));
    if(this->uiManager()->passItemCategoryCheck(
        attItem->definition()) &&
      this->uiManager()->passAdvancedCheck(
      attItem->definition()->advanceLevel()))
      {
      // No User data, not editable
      std::string strItemLabel = attItem->label();
      QTableWidgetItem* item = new QTableWidgetItem(
        QString::fromUtf8(strItemLabel.c_str()),
        smtk_USER_DATA_TYPE);

      QVariant vdata;
      vdata.setValue(static_cast<void*>(attItem.get()));
      item->setData(Qt::UserRole, vdata);
      item->setFlags(checkableFlags);
      std::string keyName = childData->definition()->type() + attItem->name();
      if(!this->Internals->AttProperties.contains(keyName))
        {
        this->Internals->AttProperties[keyName] = Qt::Unchecked;
        }

      item->setCheckState(this->Internals->AttProperties[keyName]);

      int numRows = this->Internals->ListTable->rowCount();
      this->Internals->ListTable->setRowCount(++numRows);
      this->Internals->ListTable->setItem(numRows-1, 0, item);

      // add the type column too.
      QTableWidgetItem* defitem = new QTableWidgetItem(
        QString::fromUtf8(childData->definition()->label().c_str()),
        smtk_USER_DATA_TYPE);
      defitem->setFlags(nonEditableFlags);
      this->Internals->ListTable->setItem(numRows-1, 1, defitem);

      if(attItem->definition()->advanceLevel())
        {
        item->setFont(this->uiManager()->advancedFont());
        }
      }
    }
}

//----------------------------------------------------------------------------
QTableWidgetItem* qtAttributeView::addAttributeListItem(
  smtk::attribute::AttributePtr childData)
{
  QTableWidgetItem* item = new QTableWidgetItem(
      QString::fromUtf8(childData->name().c_str()),
      smtk_USER_DATA_TYPE);
  Qt::ItemFlags nonEditableFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable);

  QVariant vdata;
  vdata.setValue(static_cast<void*>(childData.get()));
  item->setData(Qt::UserRole, vdata);
  item->setFlags(nonEditableFlags | Qt::ItemIsEditable);

  int numRows = this->Internals->ListTable->rowCount();
  this->Internals->ListTable->setRowCount(++numRows);
  this->Internals->ListTable->setItem(numRows-1, 0, item);

  // add the type column too.
  QTableWidgetItem* defitem = new QTableWidgetItem(
    QString::fromUtf8(childData->definition()->label().c_str()),
    smtk_USER_DATA_TYPE);
  defitem->setFlags(nonEditableFlags);
  this->Internals->ListTable->setItem(numRows-1, 1, defitem);

  QTableWidgetItem* colorItem =  new QTableWidgetItem();
  this->Internals->ListTable->setItem(numRows-1, 2,colorItem);
  const double* rgba = childData->color();
  QBrush bgBrush(QColor::fromRgbF(rgba[0], rgba[1], rgba[2], rgba[3]));
  colorItem->setBackground(bgBrush);
  colorItem->setFlags(Qt::ItemIsEnabled);
  return item;
}

//----------------------------------------------------------------------------
void qtAttributeView::onViewBy(int viewBy)
{
  smtk::view::AttributePtr aview =
    smtk::dynamic_pointer_cast<smtk::view::Attribute>(this->getObject());
  if(!aview || !aview->numberOfDefinitions())
    {
    return;
    }

  QList<smtk::attribute::DefinitionPtr> currentDefs =
    this->Internals->getCurrentDefs(
    this->uiManager()->currentCategory().c_str());
  this->Internals->AddButton->setEnabled(currentDefs.count()>0);

  bool viewAtt = (viewBy == VIEWBY_Attribute);
  this->Internals->ButtonsFrame->setEnabled(viewAtt);
  this->Internals->ListTable->blockSignals(true);
  this->Internals->ListTable->clear();
  this->Internals->ListTable->setRowCount(0);

  int numCols = viewAtt ? 3 : 2;
  this->Internals->ListTable->setColumnCount(numCols);
  this->Internals->ListTable->setHorizontalHeaderItem(
    0, new QTableWidgetItem(viewAtt ? "Attribute" : "Property"));
  this->Internals->ListTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Type"));
  if(viewAtt)
    {
    this->Internals->ListTable->setHorizontalHeaderItem(2, new QTableWidgetItem("Color"));
    }
  this->Internals->DefsCombo->clear();
  this->Internals->PropDefsCombo->clear();
  foreach (attribute::DefinitionPtr attDef, currentDefs)
    {
    if(!attDef->isAbstract())
      {
      std::string txtDef = attDef->label().empty() ?
        attDef->type() : attDef->label();
      this->Internals->DefsCombo->addItem(
        QString::fromUtf8(txtDef.c_str()));
      this->Internals->PropDefsCombo->addItem(
        QString::fromUtf8(txtDef.c_str()));
      QVariant vdata;
      vdata.setValue(static_cast<void*>(attDef.get()));
      int idx = this->Internals->PropDefsCombo->count()-1;
      this->Internals->PropDefsCombo->setItemData(idx, vdata, Qt::UserRole);
      }
    }

  this->Internals->DefsCombo->setCurrentIndex(0);
  this->Internals->DefsCombo->setVisible(true);
  this->Internals->PropDefsCombo->setVisible(!viewAtt);
  this->Internals->PropDefsCombo->blockSignals(true);
  this->Internals->PropDefsCombo->setCurrentIndex(0);
  this->Internals->PropDefsCombo->blockSignals(false);

  foreach (attribute::DefinitionPtr attDef, currentDefs)
    {
    this->onViewByWithDefinition(viewBy, attDef);
    }
  this->Internals->ListTable->blockSignals(false);
  this->Internals->ListTable->resizeColumnsToContents();

  if(viewAtt)
    {
    if(this->Internals->ListTable->rowCount() && !this->getSelectedItem())
      {
      this->Internals->ListTable->selectRow(0);
      }
    else
      {
      this->onListBoxSelectionChanged();
      }
    }
  else
    {
    this->updateTableWithProperties();
    }
}

//----------------------------------------------------------------------------
void qtAttributeView::onViewByWithDefinition(
  int viewBy, smtk::attribute::DefinitionPtr attDef)
{
  std::vector<smtk::attribute::AttributePtr> result;
  Manager *attManager = attDef->manager();
  attManager->findAttributes(attDef, result);
  if(result.size())
    {
    //this->Internals->ButtonsFrame->setEnabled(true);
    if(viewBy == VIEWBY_Attribute)
      {
      std::vector<smtk::attribute::AttributePtr>::iterator it;
      for (it=result.begin(); it!=result.end(); ++it)
        {
        QTableWidgetItem* item = this->addAttributeListItem(*it);
        if((*it)->definition()->advanceLevel())
          {
          item->setFont(this->uiManager()->advancedFont());
          }
        }
      }
    else if(viewBy == VIEWBY_PROPERTY)
      {
      this->addAttributePropertyItems(result[0]);
      }
    }
}
//----------------------------------------------------------------------------
void qtAttributeView::onShowCategory()
{
  int viewBy = this->Internals->ViewByCombo->currentIndex();
  this->onViewBy(viewBy);
}

//----------------------------------------------------------------------------
void qtAttributeView::updateTableWithAttribute(
  smtk::attribute::AttributePtr att)
{
  this->Internals->ValuesTable->setVisible(0);
  this->Internals->AttFrame->setVisible(1);

  if(this->Internals->CurrentAtt && this->Internals->CurrentAtt->widget())
    {
    this->Internals->AttFrame->layout()->removeWidget(
      this->Internals->CurrentAtt->widget());
    delete this->Internals->CurrentAtt->widget();
    delete this->Internals->CurrentAtt;
    }
  this->Internals->CurrentAtt = new qtAttribute(att, this->Internals->AttFrame, this);
  if(this->Internals->CurrentAtt && this->Internals->CurrentAtt->widget())
    {
    this->Internals->AttFrame->layout()->addWidget(
      this->Internals->CurrentAtt->widget());
    }
/*
  QTableWidget* vtWidget = this->Internals->ValuesTable;
  vtWidget->setColumnCount(3);

  vtWidget->setHorizontalHeaderItem(0, new QTableWidgetItem("Property"));
  vtWidget->setHorizontalHeaderItem(1, new QTableWidgetItem("Value"));
  vtWidget->setHorizontalHeaderItem(2, new QTableWidgetItem("Units"));
  QString strValue;
  int numRows = 0;
  Qt::ItemFlags nonEditableFlags(
    Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  std::size_t i, n = att->numberOfItems();
  for (i = 0; i < n; i++)
    {
    smtk::attribute::ItemPtr attItem = att->item(static_cast<int>(i));
    const ItemDefinition* itemDef =
     dynamic_cast<const ItemDefinition*>(attItem->definition().get());
    if(!this->uiManager()->passAdvancedCheck(
      itemDef->advanceLevel()) ||
      !this->uiManager()->passItemCategoryCheck(
        attItem->definition()))
      {
      continue;
      }
    if(attItem->type() == smtk::attribute::Item::GROUP)
      {
      this->addTableGroupItems(
        dynamic_pointer_cast<GroupItem>(attItem), numRows);
      }
    else if(attItem->type() == smtk::attribute::Item::ATTRIBUTE_REF)
      {
      this->addTableAttRefItems(
        dynamic_pointer_cast<RefItem>(attItem), numRows,
        itemDef->label().c_str(), itemDef->advanceLevel());
      }
    else if(attItem->type() == smtk::attribute::Item::VOID)
      {
      this->addTableVoidItems(
        dynamic_pointer_cast<VoidItem>(attItem), numRows,
        itemDef->label().c_str(), itemDef->advanceLevel());
      }
    else if(dynamic_pointer_cast<ValueItem>(attItem))
      {
      this->addTableValueItems(
        dynamic_pointer_cast<ValueItem>(attItem), numRows);
      }
    }
  vtWidget->resizeRowsToContents();
*/
}

//----------------------------------------------------------------------------
void qtAttributeView::onPropertyDefSelected()
{
  if(this->Internals->ViewByCombo->currentIndex() == VIEWBY_Attribute)
    {
    return;
    }

  Definition* rawPtr = static_cast<Definition*>(
    this->Internals->PropDefsCombo->itemData(
    this->Internals->PropDefsCombo->currentIndex(), Qt::UserRole).value<void *>());
  if(!rawPtr)
    {
    return;
    }

  this->Internals->ListTable->blockSignals(true);
  this->Internals->ListTable->clear();
  this->Internals->ListTable->setRowCount(0);
  this->Internals->ListTable->setColumnCount(2);
  this->Internals->ListTable->setHorizontalHeaderItem(
    0, new QTableWidgetItem("Property"));
  this->Internals->ListTable->setHorizontalHeaderItem(1, new QTableWidgetItem("Type"));
  this->onViewByWithDefinition(VIEWBY_PROPERTY, rawPtr->pointer());
  this->Internals->ListTable->blockSignals(false);
  this->updateTableWithProperties();
}

//----------------------------------------------------------------------------
void qtAttributeView::removeComparativeProperty(const QString& propertyName)
{
  smtk::view::AttributePtr aview =
    smtk::dynamic_pointer_cast<smtk::view::Attribute>(this->getObject());
  if(!aview || !aview->numberOfDefinitions())
    {
    return;
    }

  QTableWidget* vtWidget = this->Internals->ValuesTable;
  for(int c=0; c< vtWidget->rowCount(); c++)
    {
    if(vtWidget->item(c, 0)->text() == propertyName)
      {
      vtWidget->removeRow(c);
      break;
      }
    }
//  vtWidget->resizeRowsToContents();
}
//----------------------------------------------------------------------------
void qtAttributeView::addComparativeProperty(
  QTableWidgetItem* current, smtk::attribute::DefinitionPtr attDef)
{
  smtk::view::AttributePtr aview =
    smtk::dynamic_pointer_cast<smtk::view::Attribute>(this->getObject());
  if(!aview || !aview->numberOfDefinitions())
    {
    return;
    }

  Definition* rawPtr = static_cast<Definition*>(
    this->Internals->PropDefsCombo->itemData(
    this->Internals->PropDefsCombo->currentIndex(), Qt::UserRole).value<void *>());
  if(rawPtr->pointer() != attDef)
    {
    return;
    }

  QTableWidget* vtWidget = this->Internals->ValuesTable;

  std::vector<smtk::attribute::AttributePtr> result;
  Manager *attManager = attDef->manager();
  attManager->findAttributes(attDef, result);

  int numRows = this->Internals->ValuesTable->rowCount();
  int insertRow = current->row() >= numRows ? numRows : current->row();
  vtWidget->insertRow(insertRow);
  vtWidget->setItem(insertRow, 0, new QTableWidgetItem(current->text()));
  vtWidget->item(insertRow, 0)->setFont(current->font());

  std::vector<smtk::attribute::AttributePtr>::iterator it;
  int col=1;
  for (it=result.begin(); it!=result.end(); ++it, ++col)
    {
    std::size_t i, n = (*it)->numberOfItems();
    for (i = 0; i < n; i++)// for each property
      {
      smtk::attribute::ItemPtr attItem = (*it)->item(static_cast<int>(i));
      const ItemDefinition* itemDef =
      dynamic_cast<const ItemDefinition*>(attItem->definition().get());
      if(!this->uiManager()->passAdvancedCheck(
          itemDef->advanceLevel()) ||
        !this->uiManager()->passItemCategoryCheck(
          attItem->definition()))
        {
        continue;
        }

      std::string strItemLabel = attItem->label();
      if(current->text() == strItemLabel.c_str())
        {
        qtItem* qItem = qtAttribute::createItem(attItem, NULL, this);
        qItem->setLabelVisible(false);
        vtWidget->setCellWidget(insertRow, col, qItem->widget());
        vtWidget->setItem(insertRow, col, new QTableWidgetItem());
/*
        if(attItem->type() == smtk::attribute::Item::GROUP)
          {
          this->addTableGroupItems(
            dynamic_pointer_cast<GroupItem>(attItem), numRows,
            (*it)->name().c_str());
          }
        else if(attItem->type() == smtk::attribute::Item::ATTRIBUTE_REF)
          {
          this->addTableAttRefItems(
            dynamic_pointer_cast<RefItem>(attItem), numRows,
            (*it)->name().c_str(), itemDef->advanceLevel());
          }
        else if(attItem->type() == smtk::attribute::Item::VOID)
          {
          this->addTableVoidItems(
            dynamic_pointer_cast<VoidItem>(attItem), numRows,
            (*it)->name().c_str(), itemDef->advanceLevel());
          }
        else if(dynamic_pointer_cast<ValueItem>(attItem))// value types
          {
          this->addTableValueItems(
            dynamic_pointer_cast<ValueItem>(attItem), numRows,
            (*it)->name().c_str(), itemDef->advanceLevel());
          }
*/
        }
      }
    }
}

//----------------------------------------------------------------------------
void qtAttributeView::addTableGroupItems(
  smtk::attribute::GroupItemPtr attItem, int& numRows, const char* /*strCommonLabel*/)
{
  const GroupItemDefinition *gItemDef =
    dynamic_cast<const GroupItemDefinition*>(attItem->definition().get());
  std::string strAttLabel = attItem->label();
  const char* attLabel = strAttLabel.empty() ? NULL : strAttLabel.c_str();
  int advanced = gItemDef->advanceLevel();
  // expecting one subgroup
  int numItems = static_cast<int>(attItem->numberOfItemsPerGroup());
  if(numItems > 0)
    {
    if(dynamic_pointer_cast<ValueItem>(attItem->item(0)))
      {
      this->addTableValueItems(dynamic_pointer_cast<ValueItem>(
        attItem->item(0)), numRows, attLabel, advanced);
      for (int j = 1; j < numItems; j++) // expecting one item for each column
        {
        this->addTableValueItems(dynamic_pointer_cast<ValueItem>(
          attItem->item(j)), numRows, NULL, 0);
        }
      }
    else if(attItem->item(0)->type() == smtk::attribute::Item::ATTRIBUTE_REF)
      {
      this->addTableAttRefItems(dynamic_pointer_cast<RefItem>(
        attItem->item(0)), numRows, attLabel, advanced);
      for (int j = 1; j < numItems; j++) // expecting one item for each column
        {
        this->addTableAttRefItems(dynamic_pointer_cast<RefItem>(
          attItem->item(j)), numRows, NULL, 0);
        }
      }
    else if(attItem->item(0)->type() == smtk::attribute::Item::VOID)
      {
      this->addTableVoidItems(dynamic_pointer_cast<VoidItem>(
        attItem->item(0)), numRows, attLabel, advanced);
      for (int j = 1; j < numItems; j++) // expecting one item for each column
        {
        this->addTableVoidItems(dynamic_pointer_cast<VoidItem>(
          attItem->item(j)), numRows, NULL, 0);
        }
      }
    }
}

//----------------------------------------------------------------------------
void qtAttributeView::addTableValueItems(
  smtk::attribute::ValueItemPtr attItem, int& numRows)
{
  if(!attItem)
    {
    return;
    }
  const ValueItemDefinition *vItemDef =
    dynamic_cast<const ValueItemDefinition*>(attItem->definition().get());
  std::string attLabel = attItem->label();
  this->addTableValueItems(
    attItem, numRows, attLabel.c_str(), vItemDef->advanceLevel());
}

//----------------------------------------------------------------------------
void qtAttributeView::addTableValueItems(smtk::attribute::ValueItemPtr attItem,
  int& numRows, const char* attLabel, int advanced)
{
  if(!attItem)
    {
    return;
    }
  QTableWidget* vtWidget = this->Internals->ValuesTable;
  const ValueItemDefinition *vItemDef =
    dynamic_cast<const ValueItemDefinition*>(attItem->definition().get());

  Qt::ItemFlags nonEditableFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  vtWidget->setRowCount(++numRows);
  QString labelText = attLabel ? attLabel : "";
  if(attItem->numberOfValues() > 1 && !vItemDef->valueLabel(0).empty())
    {
    if(labelText.length())
      {
      labelText.append(" ");
      }
    labelText.append(vItemDef->valueLabel(0).c_str());
    }
  QTableWidgetItem* labelitem = new QTableWidgetItem(labelText);
  if(advanced)
    {
    labelitem->setFont(this->uiManager()->advancedFont());
    }
  // add in BriefDescription as tooltip if available
  const std::string strBriefDescription = vItemDef->briefDescription();
  if(!strBriefDescription.empty())
    {
    labelitem->setToolTip(strBriefDescription.c_str());
    }

  labelitem->setFlags(nonEditableFlags);
  vtWidget->setItem(numRows-1, 0, labelitem);
  bool bEnabled = this->uiManager()->updateTableItemCheckState(
    labelitem, dynamic_pointer_cast<Item>(attItem));

  QString unitText = vItemDef && !vItemDef->units().empty() ? vItemDef->units().c_str() : "";
  QTableWidgetItem* unitItem = new QTableWidgetItem(unitText);
  unitItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  vtWidget->setItem(numRows-1, 2, unitItem);

  int numAdded = 0;
  std::size_t i, n = attItem->numberOfValues();
  for(i = 0; i < n; i++)
    {
    QWidget* inputWidget = this->uiManager()->createInputWidget(
      attItem, static_cast<int>(i), this->Widget, this);
    if(inputWidget)
      {
      numRows = numAdded>0 ? numRows+1 : numRows;
      vtWidget->setRowCount(numRows);
      inputWidget->setEnabled(bEnabled);
      vtWidget->setCellWidget(numRows-1, 1, inputWidget);
      vtWidget->setItem(numRows-1, 1, new QTableWidgetItem());

      // component labels
      std::string componentLabel = vItemDef->valueLabel(i);
      if(i > 0 && !componentLabel.empty())
        {
        labelText = (attLabel ? attLabel : "");
        labelText.append(" ").append(componentLabel.c_str());
        QTableWidgetItem* componentlabelitem = new QTableWidgetItem(labelText);
        if(advanced)
          {
          componentlabelitem->setFont(this->uiManager()->advancedFont());
          }
        componentlabelitem->setFlags(nonEditableFlags);
        // add in BriefDescription as tooltip if available
        const std::string bdStrBriefDescription = vItemDef->briefDescription();
        if(!bdStrBriefDescription.empty())
          {
          componentlabelitem->setToolTip(bdStrBriefDescription.c_str());
          }
        vtWidget->setItem(numRows-1, 0, componentlabelitem);
        }
      numAdded++;
      }
    }
}
//----------------------------------------------------------------------------
void qtAttributeView::addTableAttRefItems(
  smtk::attribute::RefItemPtr attItem, int& numRows,
  const char* attLabel, int advanced)
{
  if(!attItem)
    {
    return;
    }
  QTableWidget* vtWidget = this->Internals->ValuesTable;
  qtAttributeRefItem* refItem = qobject_cast<qtAttributeRefItem*>(
    qtAttribute::createAttributeRefItem(attItem, vtWidget, this));
  if(!refItem)
    {
    return;
    }
  refItem->setLabelVisible(false);
//  refItem->setAttributeEditorVisible(false);
//  refItem->setAttributeWidgetVisible(false);

  QString labelText = attItem->label().c_str();
  labelText = labelText.isEmpty() ? attLabel : labelText;

  Qt::ItemFlags nonEditableFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
  vtWidget->setRowCount(++numRows);
  QTableWidgetItem* labelitem = new QTableWidgetItem(labelText);
  if(advanced)
    {
    labelitem->setFont(this->uiManager()->advancedFont());
    }

  labelitem->setFlags(nonEditableFlags);
  vtWidget->setItem(numRows-1, 0, labelitem);

  bool bEnabled = this->uiManager()->updateTableItemCheckState(
    labelitem, dynamic_pointer_cast<Item>(attItem));
  refItem->widget()->setEnabled(bEnabled);
  vtWidget->setCellWidget(numRows-1, 1, refItem->widget());
  vtWidget->setItem(numRows-1, 1, new QTableWidgetItem());
//  vtWidget->update(vtWidget->visualItemRect(vtWidget->item(numRows-1, 1)));
}
//----------------------------------------------------------------------------
void qtAttributeView::addTableVoidItems(
  smtk::attribute::VoidItemPtr attItem, int& numRows,
  const char* /*attLabel*/, int advanced)
{
  if(!attItem)
    {
    return;
    }
  QTableWidget* vtWidget = this->Internals->ValuesTable;
  qtVoidItem* voidItem = qobject_cast<qtVoidItem*>(
    qtAttribute::createItem(attItem, vtWidget, this));
  if(!voidItem)
    {
    return;
    }

  vtWidget->setRowCount(++numRows);
  vtWidget->setCellWidget(numRows-1, 0, voidItem->widget());
  vtWidget->setItem(numRows-1, 0, new QTableWidgetItem());
  if(advanced && voidItem->widget())
    {
    voidItem->widget()->setFont(
      this->uiManager()->advancedFont());
    }
}

//----------------------------------------------------------------------------
int qtAttributeView::currentViewBy()
{
  return this->Internals->ViewByCombo->currentIndex();
}

//----------------------------------------------------------------------------
void qtAttributeView::getAllDefinitions()
{
  smtk::view::AttributePtr aview =
    smtk::dynamic_pointer_cast<smtk::view::Attribute>(this->getObject());
  if(!aview || !aview->numberOfDefinitions())
    {
    return;
    }
  std::size_t i, n = aview->numberOfDefinitions();
  for (i = 0; i < n; i++)
    {
    attribute::DefinitionPtr attDef = aview->definition(static_cast<int>(i));
    this->qtBaseView::getDefinitions(attDef, this->Internals->AllDefs);
    }

  foreach (smtk::attribute::DefinitionPtr adef, this->Internals->AllDefs)
    {
    foreach(QString category, this->Internals->AttDefMap.keys())
      {
      if(adef->isMemberOf(category.toStdString()) &&
        !this->Internals->AttDefMap[category].contains(adef))
        {
        this->Internals->AttDefMap[category].push_back(adef);
        }
      }
    }
}

//----------------------------------------------------------------------------
void qtAttributeView::onListBoxClicked(QTableWidgetItem* item)
{
  if(this->Internals->ViewByCombo->currentIndex() == VIEWBY_Attribute)
    {
     bool isColor = item->column() == 2;
    if(isColor)
      {
      QTableWidgetItem* selItem = this->Internals->ListTable->item(item->row(), 0);
      smtk::attribute::AttributePtr selAtt = this->getAttributeFromItem(selItem);
      QBrush bgBrush = item->background();
      QColor color = QColorDialog::getColor(bgBrush.color(), this->Widget);
      if(color.isValid() && color != bgBrush.color() && selAtt)
        {
        bgBrush.setColor(color);
        item->setBackground(bgBrush);
        selAtt->setColor(color.redF(), color.greenF(), color.blueF(), color.alphaF());
        emit this->attColorChanged();
        }
      if(!selItem->isSelected())
        {
        this->Internals->ListTable->setCurrentItem(selItem);
        selItem->setSelected(true);
        }
      }
    }
}
