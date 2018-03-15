//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#ifndef smtk_resource_Metadata_h
#define smtk_resource_Metadata_h

#include "smtk/CoreExports.h"
#include "smtk/PublicPointerDefs.h"
#include "smtk/common/UUID.h"
#include "smtk/resource/Resource.h"

#include <functional>
#include <set>
#include <string>

namespace smtk
{
namespace resource
{
class Manager;

/// Resources are registered to a resource manager at runtime with an instance
/// of smtk::resource::Metadata. Instances of this class must provide
/// <uniqueName>, a unique (to the manager) string used to describe the resource
/// within the manager. They may also provide functors for the creation and
/// serialization/deserialization of the resource to/from disk (<create>,
/// <write> and <read>, respectively).
class Metadata
{
  friend class Manager;

public:
  typedef std::function<void(const Metadata&)> Visitor;

  Metadata(const std::string& uniqueName, Resource::Index index,
    std::set<Resource::Index> parentIndices,
    std::function<ResourcePtr(const smtk::common::UUID&)> createFunctor)
    : create(createFunctor)
    , m_uniqueName(uniqueName)
    , m_index(index)
    , m_parentIndices(parentIndices)
  {
  }

  const std::string& uniqueName() const { return m_uniqueName; }
  const Resource::Index& index() const { return m_index; }

  // Resource metadata holds inheritence information for its resource as a set
  // of parent indices.
  bool isOfType(const Resource::Index& index) const
  {
    return m_parentIndices.find(index) != m_parentIndices.end();
  }

  std::function<ResourcePtr(const smtk::common::UUID&)> create = [](
    const smtk::common::UUID&) { return ResourcePtr(); };

private:
  std::string m_uniqueName;
  Resource::Index m_index;
  std::set<Resource::Index> m_parentIndices;
};
}
}

#endif // smtk_resource_Metadata_h