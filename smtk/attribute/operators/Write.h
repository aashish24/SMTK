//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#ifndef __smtk_attribute_operators_Write_h
#define __smtk_attribute_operators_Write_h

#include "smtk/operation/XMLOperation.h"

namespace smtk
{
namespace attribute
{

/**\brief Write an attribute resource.
  */
class SMTKCORE_EXPORT Write : public smtk::operation::XMLOperation
{
public:
  smtkTypeMacro(smtk::attribute::Write);
  smtkCreateMacro(Write);
  smtkSharedFromThisMacro(smtk::operation::Operation);
  smtkSuperclassMacro(smtk::operation::XMLOperation);

protected:
  Result operateInternal() override;
  virtual const char* xmlDescription() const override;
  void markModifiedResources(Result&) override;
};

SMTKCORE_EXPORT bool write(const smtk::resource::ResourcePtr&);
}
}

#endif // __smtk_attribute_operators_Write_h
