//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#ifndef pybind_smtk_session_rgg_operators_RemoveMaterial_h
#define pybind_smtk_session_rgg_operators_RemoveMaterial_h

#include <pybind11/pybind11.h>

#include "smtk/session/rgg/operators/RemoveMaterial.h"

#include "smtk/operation/XMLOperation.h"

namespace py = pybind11;

PySharedPtrClass< smtk::session::rgg::RemoveMaterial, smtk::operation::XMLOperation > pybind11_init_smtk_session_rgg_RemoveMaterial(py::module &m)
{
  PySharedPtrClass< smtk::session::rgg::RemoveMaterial, smtk::operation::XMLOperation > instance(m, "RemoveMaterial");
  instance
    .def(py::init<::smtk::session::rgg::RemoveMaterial const &>())
    .def(py::init<>())
    .def("deepcopy", (smtk::session::rgg::RemoveMaterial & (smtk::session::rgg::RemoveMaterial::*)(::smtk::session::rgg::RemoveMaterial const &)) &smtk::session::rgg::RemoveMaterial::operator=)
    .def_static("create", (std::shared_ptr<smtk::session::rgg::RemoveMaterial> (*)()) &smtk::session::rgg::RemoveMaterial::create)
    .def_static("create", (std::shared_ptr<smtk::session::rgg::RemoveMaterial> (*)(::std::shared_ptr<smtk::session::rgg::RemoveMaterial> &)) &smtk::session::rgg::RemoveMaterial::create, py::arg("ref"))
    .def("shared_from_this", (std::shared_ptr<smtk::session::rgg::RemoveMaterial> (smtk::session::rgg::RemoveMaterial::*)()) &smtk::session::rgg::RemoveMaterial::shared_from_this)
    .def("shared_from_this", (std::shared_ptr<const smtk::session::rgg::RemoveMaterial> (smtk::session::rgg::RemoveMaterial::*)() const) &smtk::session::rgg::RemoveMaterial::shared_from_this)
    ;
  return instance;
}

#endif
