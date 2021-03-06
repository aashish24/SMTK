//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#include "smtk/extension/qt/qtAttribute.h"

#include "smtk/extension/qt/qtBaseView.h"
#include "smtk/extension/qt/qtUIManager.h"

#include "smtk/attribute/Attribute.h"
#include "smtk/attribute/ComponentItem.h"
#include "smtk/attribute/DateTimeItem.h"
#include "smtk/attribute/DirectoryItem.h"
#include "smtk/attribute/DoubleItem.h"
#include "smtk/attribute/FileItem.h"
#include "smtk/attribute/GroupItem.h"
#include "smtk/attribute/IntItem.h"
#include "smtk/attribute/MeshItem.h"
#include "smtk/attribute/MeshSelectionItem.h"
#include "smtk/attribute/ReferenceItem.h"
#include "smtk/attribute/ValueItem.h"
#include "smtk/attribute/VoidItem.h"

#include "smtk/io/Logger.h"

#include <QFrame>
#include <QLabel>
#include <QPointer>
#include <QVBoxLayout>

#include <stdlib.h> // for atexit()

using namespace smtk::attribute;
using namespace smtk::extension;

class qtAttributeInternals
{
public:
  qtAttributeInternals(smtk::attribute::AttributePtr myAttribute,
    const smtk::view::View::Component& comp, QWidget* p, qtBaseView* myView)
  {
    m_parentWidget = p;
    m_attribute = myAttribute;
    m_view = myView;
    m_attComp = comp;

    // Does the component reprsenting the attribute contain a Style block?
    int sindex = comp.findChild("ItemViews");
    if (sindex == -1)
    {
      return;
    }
    auto iviews = m_attComp.child(sindex);
    std::string iname;
    std::size_t i, n = iviews.numberOfChildren();
    for (i = 0; i < n; i++)
    {
      // Do we have an item attribute?
      if (iviews.child(i).attribute("Item", iname))
      {
        m_itemViewMap[iname] = iviews.child(i);
      }
    }
  }

  ~qtAttributeInternals() {}
  smtk::attribute::WeakAttributePtr m_attribute;
  QPointer<QWidget> m_parentWidget;
  QList<smtk::extension::qtItem*> m_items;
  QPointer<qtBaseView> m_view;
  smtk::view::View::Component m_attComp;
  std::map<std::string, smtk::view::View::Component> m_itemViewMap;
};

qtAttribute::qtAttribute(smtk::attribute::AttributePtr myAttribute,
  const smtk::view::View::Component& comp, QWidget* p, qtBaseView* myView)
{
  m_internals = new qtAttributeInternals(myAttribute, comp, p, myView);
  m_widget = NULL;
  m_useSelectionManager = false;
  this->createWidget();
  m_isEmpty = true;
}

qtAttribute::~qtAttribute()
{
  // First Clear all the items
  for (int i = 0; i < m_internals->m_items.count(); i++)
  {
    delete m_internals->m_items.value(i);
  }

  m_internals->m_items.clear();
  if (m_widget)
  {
    delete m_widget;
  }

  delete m_internals;
}

void qtAttribute::createWidget()
{
  // Initially there are no items being displayed
  m_isEmpty = true;
  auto att = this->attribute();
  if ((att == nullptr) || ((att->numberOfItems() == 0) && (att->associations() == nullptr)))
  {
    return;
  }

  int numShowItems = 0;
  std::size_t i, n = att->numberOfItems();
  if (m_internals->m_view)
  {
    for (i = 0; i < n; i++)
    {
      if (m_internals->m_view->displayItem(att->item(static_cast<int>(i))))
      {
        numShowItems++;
      }
    }
    // also check associations
    if (m_internals->m_view->displayItem(att->associations()))
    {
      numShowItems++;
    }
  }
  else // show everything
  {
    numShowItems = static_cast<int>(att->associations() ? n + 1 : n);
  }
  if (numShowItems == 0)
  {
    return;
  }
  m_isEmpty = false;
  QFrame* attFrame = new QFrame(this->parentWidget());
  attFrame->setFrameShape(QFrame::Box);
  m_widget = attFrame;

  QVBoxLayout* layout = new QVBoxLayout(m_widget);
  layout->setMargin(3);
  m_widget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
}

void qtAttribute::addItem(qtItem* child)
{
  if (!m_internals->m_items.contains(child))
  {
    m_internals->m_items.append(child);
    // When the item is modified so is the attribute that uses it
    connect(child, SIGNAL(modified()), this, SLOT(onItemModified()));
  }
}

QList<qtItem*>& qtAttribute::items() const
{
  return m_internals->m_items;
}

void qtAttribute::showAdvanceLevelOverlay(bool show)
{
  for (int i = 0; i < m_internals->m_items.count(); i++)
  {
    m_internals->m_items.value(i)->showAdvanceLevelOverlay(show);
  }
}

void qtAttribute::createBasicLayout(bool includeAssociations)
{
  // Initially we have not displayed any items
  m_isEmpty = true;
  //If there is no main widget there is nothing to show
  if (!m_widget)
  {
    return;
  }
  QLayout* layout = m_widget->layout();
  qtItem* qItem = NULL;
  smtk::attribute::AttributePtr att = this->attribute();
  auto uiManager = m_internals->m_view->uiManager();
  // If there are model assocications for the attribute, create UI for them if requested.
  // This will be the same widget used for ModelEntityItem.
  if (includeAssociations && att->associations())
  {
    smtk::view::View::Component comp; // not currently used but will be
    AttributeItemInfo info(att->associations(), comp, m_widget, m_internals->m_view);
    qItem = uiManager->createItem(info);
    if (qItem && qItem->widget())
    {
      m_isEmpty = false;
      layout->addWidget(qItem->widget());
      this->addItem(qItem);
    }
  }
  // Now go through all child items and create ui components.
  std::size_t i, n = att->numberOfItems();
  for (i = 0; i < n; i++)
  {
    // Does this item have a style associated with it?
    auto item = att->item(static_cast<int>(i));
    auto it = m_internals->m_itemViewMap.find(item->name());
    if (it != m_internals->m_itemViewMap.end())
    {
      AttributeItemInfo info(item, it->second, m_widget, m_internals->m_view);
      qItem = uiManager->createItem(info);
    }
    else
    {
      smtk::view::View::Component comp; // not current used but will be
      AttributeItemInfo info(item, comp, m_widget, m_internals->m_view);
      qItem = uiManager->createItem(info);
    }
    if (qItem && qItem->widget())
    {
      m_isEmpty = false;
      layout->addWidget(qItem->widget());
      this->addItem(qItem);
    }
  }
}

smtk::attribute::AttributePtr qtAttribute::attribute()
{
  return m_internals->m_attribute.lock();
}

QWidget* qtAttribute::parentWidget()
{
  return m_internals->m_parentWidget;
}

/* Slot for properly emitting signals when an attribute's item is modified */
void qtAttribute::onItemModified()
{
  // are we here due to a signal?
  QObject* sobject = this->sender();
  if (sobject == NULL)
  {
    return;
  }
  auto iobject = qobject_cast<smtk::extension::qtItem*>(sobject);
  if (iobject == NULL)
  {
    return;
  }
  emit this->itemModified(iobject);
  emit this->modified();
}

bool qtAttribute::isEmpty() const
{
  return m_isEmpty;
}
