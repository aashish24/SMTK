//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#ifndef smtk_extension_paraview_appcomponents_pqSMTKCloseResourceBehavior_h
#define smtk_extension_paraview_appcomponents_pqSMTKCloseResourceBehavior_h

#include "smtk/extension/paraview/appcomponents/Exports.h"

#include "smtk/PublicPointerDefs.h"

#include "pqReaction.h"

#include <QObject>

class QMenu;

/// A reaction for creating a new SMTK Resource.
class pqCloseResourceReaction : public pqReaction
{
  Q_OBJECT
  typedef pqReaction Superclass;

public:
  /**
  * Constructor. Parent cannot be NULL.
  */
  pqCloseResourceReaction(QAction* parent);

  static void closeResource();

public slots:
  /**
  * Updates the enabled state. Applications need not explicitly call
  * this.
  */
  void updateEnableState() override;

protected:
  /**
  * Called when the action is triggered.
  */
  void onTriggered() override { this->closeResource(); }

private:
  Q_DISABLE_COPY(pqCloseResourceReaction)
};

/// Create a menu item under "File" for closing an SMTK resource. The behavior
/// executes the "RemoveResource" operation on the server with the active
/// resource as its input.
class SMTKPQCOMPONENTSEXT_EXPORT pqSMTKCloseResourceBehavior : public QObject
{
  Q_OBJECT
  using Superclass = QObject;

public:
  static pqSMTKCloseResourceBehavior* instance(QObject* parent = nullptr);
  ~pqSMTKCloseResourceBehavior() override;

protected:
  pqSMTKCloseResourceBehavior(QObject* parent = nullptr);

private:
  Q_DISABLE_COPY(pqSMTKCloseResourceBehavior);

  QMenu* m_newMenu;
};

#endif // smtk_extension_paraview_appcomponents_pqSMTKCloseResourceBehavior_h
