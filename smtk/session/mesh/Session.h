//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#ifndef __smtk_session_mesh_Session_h
#define __smtk_session_mesh_Session_h

#include "smtk/session/mesh/Exports.h"
#include "smtk/session/mesh/Facade.h"
#include "smtk/session/mesh/Topology.h"

#include "smtk/mesh/core/Collection.h"

#include "smtk/model/EntityRef.h"
#include "smtk/model/Session.h"

namespace smtk
{
namespace session
{
namespace mesh
{

class Resource;

class SMTKMESHSESSION_EXPORT Session : public smtk::model::Session
{
public:
  smtkTypeMacro(Session);
  smtkSuperclassMacro(smtk::model::Session);
  smtkSharedFromThisMacro(smtk::model::Session);
  smtkCreateMacro(smtk::model::Session);
  typedef smtk::model::SessionInfoBits SessionInfoBits;

  virtual ~Session() {}

  void addTopology(Topology t) { m_topologies.push_back(t); }
  Topology* topology(const std::shared_ptr<Resource>& modelResource);
  Topology* topology(const std::shared_ptr<const Resource>& modelResource);

  std::string defaultFileExtension(const smtk::model::Model&) const override { return ""; }

  Facade& facade() { return m_facade; }

protected:
  Session();

  SessionInfoBits transcribeInternal(
    const smtk::model::EntityRef& entity, SessionInfoBits requestedInfo, int depth = -1) override;

  std::vector<Topology> m_topologies;
  Facade m_facade;
};

} // namespace mesh
} // namespace session
} // namespace smtk

#endif // __smtk_session_mesh_Session_h
