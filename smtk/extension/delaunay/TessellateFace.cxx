//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//=============================================================================
#include "smtk/extension/delaunay/TessellateFace.h"

#include "smtk/io/ModelToMesh.h"

#include "smtk/mesh/Collection.h"
#include "smtk/mesh/ExtractTessellation.h"
#include "smtk/mesh/Manager.h"

#include "smtk/model/Edge.h"
#include "smtk/model/Face.h"
#include "smtk/model/FaceUse.h"
#include "smtk/model/Loop.h"

#include "Discretization/ConstrainedDelaunayMesh.hh"
#include "Discretization/ExcisePolygon.hh"
#include "Mesh/Mesh.hh"
#include "Shape/Point.hh"
#include "Shape/Polygon.hh"
#include "Shape/PolygonUtilities.hh"

#include <algorithm>

namespace {

std::vector<Delaunay::Shape::Point> pointsInLoop(
  const smtk::model::Loop& loop, smtk::mesh::CollectionPtr& collection)
{
  std::int64_t connectivityLength= -1;
  std::int64_t numberOfCells = -1;
  std::int64_t numberOfPoints = -1;

  //query for all cells
  smtk::mesh::PreAllocatedTessellation::determineAllocationLengths(
    loop, collection, connectivityLength, numberOfCells, numberOfPoints);

  std::vector<std::int64_t> conn( connectivityLength );
  std::vector<float> fpoints(numberOfPoints * 3);

  smtk::mesh::PreAllocatedTessellation ftess(&conn[0], &fpoints[0]);

  ftess.disableVTKStyleConnectivity(true);
  ftess.disableVTKCellTypes(true);

  smtk::mesh::extractOrderedTessellation(loop, collection, ftess);

  std::vector<Delaunay::Shape::Point> points;

  for (std::size_t i=0;i<fpoints.size(); i+=3)
  {
    points.push_back(Delaunay::Shape::Point(fpoints[i], fpoints[i+1]));
  }
  // loops sometimes have a redundant point at the end. We need to remove it.
  if (points.front() == points.back())
  {
    points.pop_back();
  }

  return points;
}

template <typename Point, typename PointContainer>
int IndexOf(Point& point, const PointContainer& points)
{
  return static_cast<int>(std::distance(points.begin(),
                                        std::find(points.begin(),
                                                  points.end(), point)));
}

void ImportDelaunayMesh(const Delaunay::Mesh::Mesh& mesh,
                        smtk::model::Face& face)
{
  smtk::model::Tessellation* tess = face.resetTessellation();

  tess->coords().resize(mesh.GetVertices().size()*3);
  int index = 0;
  double xyz[3];
  for (auto& p : mesh.GetVertices())
    {
    index = IndexOf(p, mesh.GetVertices());
    xyz[0] = p.x;
    xyz[1] = p.y;
    xyz[2] = 0.;
    tess->setPoint(index, xyz);
    }

  index = 0;
  for (auto& t : mesh.GetTriangles())
    {
    tess->addTriangle(IndexOf(t.AB().A(), mesh.GetVertices()),
                      IndexOf(t.AB().B(), mesh.GetVertices()),
                      IndexOf(t.AC().B(), mesh.GetVertices()));
    }

  auto bounds = Delaunay::Shape::Bounds(mesh.GetPerimeter());
  const double bbox[6] = {bounds[0], bounds[1], bounds[2], bounds[3], 0., 0.};
  face.setBoundingBox(bbox);
}

}

namespace smtk {
  namespace model {

TessellateFace::TessellateFace()
{
}

bool TessellateFace::ableToOperate()
{
  smtk::model::EntityRef eRef =
    this->specification()->findModelEntity("face")->value();

  return
    this->Superclass::ableToOperate() &&
    eRef.isValid() &&
    eRef.isFace() &&
    eRef.owningModel().isValid();
}

OperatorResult TessellateFace::operateInternal()
{
  smtk::model::Face face =
    this->specification()->findModelEntity("face")->
    value().as<smtk::model::Face>();

  smtk::io::ModelToMesh convert;
  convert.setIsMerging(false);
  smtk::mesh::CollectionPtr collection = convert(this->session()->meshManager(),
                                                 this->session()->manager());

  // get the face use for the face
  smtk::model::FaceUse fu = face.positiveUse();

  // check if we have an exterior loop
  smtk::model::Loops exteriorLoops = fu.loops();
  if (exteriorLoops.size() == 0)
  {
    // if we don't have loops we are bailing out!
    smtkErrorMacro(this->log(), "No loops associated with this face.");
    return this->createResult(OPERATION_FAILED);
  }

  // the first loop is the exterior loop
  smtk::model::Loop exteriorLoop = exteriorLoops[0];

  // make a polygon from the points in the loop
  std::vector<Delaunay::Shape::Point> points =
    pointsInLoop(exteriorLoop, collection);
  Delaunay::Shape::Polygon p(points);
  // if the orientation is not ccw, flip the orientation
  if (Delaunay::Shape::Orientation(p) != 1)
  {
    p = Delaunay::Shape::Polygon(points.rbegin(), points.rend());
  }

  // discretize the polygon
  Delaunay::Discretization::ConstrainedDelaunayMesh discretize;
  Delaunay::Mesh::Mesh mesh;
  discretize(p, mesh);

  // then we excise each inner loop within the exterior loop
  Delaunay::Discretization::ExcisePolygon excise;
  for (auto& loop : exteriorLoop.containedLoops())
  {
    std::vector<Delaunay::Shape::Point> points_sub =
      pointsInLoop(loop, collection);
    Delaunay::Shape::Polygon p_sub(points_sub);
    // if the orientation is not ccw, flip the orientation
    if (Delaunay::Shape::Orientation(p_sub) != 1)
    {
      p_sub = Delaunay::Shape::Polygon(points_sub.rbegin(), points_sub.rend());
    }
    excise(p_sub, mesh);
  }

  // remove the original collection
  this->session()->meshManager()->removeCollection(collection);

  // Use the delaunay mesh to retessellate the face
  ImportDelaunayMesh(mesh, face);

  OperatorResult result = this->createResult(OPERATION_SUCCEEDED);
  this->addEntityToResult(result, face, MODIFIED);
  result->findModelEntity("tess_changed")->setValue(face);
  return result;
}

  } // namespace model
} // namespace smtk

#include "smtk/extension/delaunay/Exports.h"
#include "smtk/extension/delaunay/TessellateFace_xml.h"

smtkImplementsModelOperator(
  SMTKDELAUNAYEXT_EXPORT,
  smtk::model::TessellateFace,
  delaunay_tessellate_face,
  "tessellate face",
  TessellateFace_xml,
  smtk::model::Session);
