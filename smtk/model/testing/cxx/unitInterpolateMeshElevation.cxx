//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include "smtk/attribute/DoubleItem.h"
#include "smtk/attribute/GroupItem.h"
#include "smtk/attribute/IntItem.h"
#include "smtk/attribute/MeshItem.h"

#include "smtk/io/LoadJSON.h"
#include "smtk/io/ModelToMesh.h"
#include "smtk/io/ReadMesh.h"

#include "smtk/model/DefaultSession.h"

#include "smtk/mesh/Collection.h"
#include "smtk/mesh/ForEachTypes.h"
#include "smtk/mesh/Manager.h"

#include "smtk/model/Manager.h"
#include "smtk/model/Operator.h"

#include <array>
#include <fstream>

namespace
{
void create_simple_mesh_model(smtk::model::ManagerPtr mgr, std::string file_path)
{
  std::ifstream file(file_path.c_str());

  std::string json((std::istreambuf_iterator<char>(file)), (std::istreambuf_iterator<char>()));

  smtk::io::LoadJSON::intoModelManager(json.c_str(), mgr);
  mgr->assignDefaultNames();

  file.close();
}

class HistogramElevations : public smtk::mesh::PointForEach
{
public:
  HistogramElevations(std::size_t nBins, double min, double max)
    : m_min(min)
    , m_max(max)
  {
    m_hist.resize(nBins, 0);
  }

  void forPoints(
    const smtk::mesh::HandleRange& pointIds, std::vector<double>& xyz, bool& coordinatesModified)
  {
    (void)pointIds;
    coordinatesModified = false; //we are not modifying the coords

    for (std::size_t offset = 0; offset < xyz.size(); offset += 3)
    {
      std::size_t bin =
        static_cast<std::size_t>((xyz[offset + 2] - m_min) / (m_max - m_min) * (m_hist.size() + 1));
      ++m_hist[bin];
    }
  }

  const std::vector<std::size_t>& histogram() const { return m_hist; }

private:
  std::vector<std::size_t> m_hist;
  double m_min;
  double m_max;
};
}

// Load in a model, convert it to a mesh, and elevate that mesh using
// interpolation points. Then, histogram the elevation values of the mesh points
// and compare the histogram to expected values.

int main(int argc, char* argv[])
{
  if (argc == 1)
  {
    std::cout << "Must provide input file as argument" << std::endl;
    return 1;
  }

  std::ifstream file;
  file.open(argv[1]);
  if (!file.good())
  {
    std::cout << "Could not open file \"" << argv[1] << "\".\n\n";
    return 1;
  }

  // Create a model manager
  smtk::model::ManagerPtr manager = smtk::model::Manager::create();

  // Identify available sessions
  std::cout << "Available sessions\n";
  typedef smtk::model::StringList StringList;
  StringList sessions = manager->sessionTypeNames();
  smtk::mesh::ManagerPtr meshManager = manager->meshes();
  for (StringList::iterator it = sessions.begin(); it != sessions.end(); ++it)
    std::cout << "  " << *it << "\n";
  std::cout << "\n";

  // Create a new default session
  smtk::model::SessionRef sessRef = manager->createSession("native");

  // Identify available operators
  std::cout << "Available cmb operators\n";
  StringList opnames = sessRef.session()->operatorNames();
  for (StringList::iterator it = opnames.begin(); it != opnames.end(); ++it)
  {
    std::cout << "  " << *it << "\n";
  }
  std::cout << "\n";

  // Load in the model
  create_simple_mesh_model(manager, std::string(argv[1]));

  // Convert it t a mesh
  smtk::io::ModelToMesh convert;
  smtk::mesh::CollectionPtr c = convert(meshManager, manager);

  // Create an "Interpolate Mesh Elevation" operator
  smtk::model::OperatorPtr elevateMeshOp = sessRef.session()->op("interpolate mesh elevation");
  if (!elevateMeshOp)
  {
    std::cerr << "No \"interpolate mesh elevation\" operator\n";
    return 1;
  }

  // Set the operator's input mesh
  bool valueSet = elevateMeshOp->specification()->findMesh("mesh")->setValue(
    meshManager->collectionBegin()->second->meshes());

  if (!valueSet)
  {
    std::cerr << "Failed to set mesh value on operator\n";
    return 1;
  }

  // Set the operator's input power
  smtk::attribute::DoubleItemPtr power = elevateMeshOp->specification()->findDouble("power");

  if (!power)
  {
    std::cerr << "Failed to set power value on operator\n";
    return 1;
  }

  power->setValue(2.);

  // Set the operator's input points
  smtk::attribute::GroupItemPtr points = elevateMeshOp->specification()->findGroup("points");

  if (!points)
  {
    std::cerr << "No \"points\" item in specification\n";
    return 1;
  }

  std::size_t numberOfPoints = 4;
  double pointData[4][3] = { { -1., -1., 0. }, { -1., 6., 25. }, { 10., -1., 50. },
    { 10., 6., 40. } };

  points->setNumberOfGroups(numberOfPoints);
  for (std::size_t i = 0; i < numberOfPoints; i++)
  {
    if (i != 0)
    {
      points->appendGroup();
    }
    for (std::size_t j = 0; j < 3; j++)
    {
      smtk::dynamic_pointer_cast<smtk::attribute::DoubleItem>(points->item(i, 0))
        ->setValue(j, pointData[i][j]);
    }
  }

  // Execute "Interpolate Mesh Elevation" operator...
  smtk::model::OperatorResult elevateMeshOpResult = elevateMeshOp->operate();
  // ...and test the results for success.
  if (elevateMeshOpResult->findInt("outcome")->value() != smtk::model::OPERATION_SUCCEEDED)
  {
    std::cerr << "\"interpolate mesh elevation\" operator failed\n";
    return 1;
  }

  // Histogram the resulting points and compare against expected values.
  HistogramElevations histogramElevations(10, 0., 50.);
  smtk::mesh::for_each(c->points(), histogramElevations);

  std::array<std::size_t, 10> expected = { { 5, 3, 3, 6, 4, 3, 3, 3, 1, 1 } };

  std::size_t counter = 0;
  for (auto& bin : histogramElevations.histogram())
  {
    if (bin != expected[counter++])
    {
      std::cerr << "\"interpolate mesh elevation\" operator produced unexpected results\n";
      return 1;
    }
  }

  return 0;
}