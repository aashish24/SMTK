//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================

#include "smtk/mesh/Collection.h"
#include "smtk/mesh/Manager.h"
#include "smtk/io/ImportMesh.h"
#include "smtk/io/WriteMesh.h"

#include "smtk/mesh/testing/cxx/helpers.h"

//force to use filesystem version 3
#define BOOST_FILESYSTEM_VERSION 3
#include <boost/filesystem.hpp>
using namespace boost::filesystem;


namespace
{

//SMTK_DATA_DIR is a define setup by cmake
std::string data_root = SMTK_DATA_DIR;
std::string write_root = data_root + "/mesh/tmp";

void cleanup( const std::string& file_path )
{
  //first verify the file exists
  ::boost::filesystem::path path( file_path );
  if( ::boost::filesystem::is_regular_file( path ) )
    {
    //remove the file_path if it exists.
    ::boost::filesystem::remove( path );
    }
}

//----------------------------------------------------------------------------
void verify_write_empty_collection()
{
  std::string file_path(data_root);
  file_path += "/mesh/output.h5m";

  std::string write_path(write_root);
  write_path += "/output.h5m";

  smtk::mesh::ManagerPtr manager = smtk::mesh::Manager::create();
  smtk::mesh::CollectionPtr c = manager->makeCollection();
  test( c->isValid(), "empty collection is empty");

  bool result = smtk::io::WriteMesh::entireCollection(write_path, c);

  //before we verify if the write was good, first remove the output file
  cleanup( write_path );
  test ( result == true, "Wrote empty collection to disk");
}

//----------------------------------------------------------------------------
void verify_write_null_collection()
{
  std::string write_path(write_root);
  write_path += "/output.h5m";

  //use a null collection ptr
  smtk::mesh::CollectionPtr c;

  bool result = smtk::io::WriteMesh::entireCollection(write_path, c);

  //before we verify if the write was good, first remove the output file
  cleanup( write_path );

  test ( result == false, "Can't save null collection to disk");
}

//----------------------------------------------------------------------------
void verify_write_valid_collection_hdf5()
{
  std::string file_path(data_root);
  file_path += "/mesh/twoassm_out.h5m";

  std::string write_path(write_root);
  write_path += "/twoassm_output.h5m";

  smtk::mesh::ManagerPtr manager = smtk::mesh::Manager::create();
  smtk::mesh::CollectionPtr c = smtk::io::ImportMesh::entireFile(file_path, manager);
  test( c->isValid(), "collection should be valid");

  //write out the mesh.
  bool result = smtk::io::WriteMesh::entireCollection(write_path, c);
  if(!result)
    {
    cleanup( write_path );
    test( result == true, "failed to properly write out a valid hdf5 collection");
    }

  //reload the written file and verify the number of meshes are the same as the
  //input mesh
  smtk::mesh::CollectionPtr c2 = smtk::io::ImportMesh::entireFile(write_path, manager);

  //remove the file from disk
  cleanup( write_path );

  //verify the meshes
  test( c2->isValid(), "collection should be valid");
  test( c2->name() == c->name() );
  test( c2->numberOfMeshes() == c->numberOfMeshes() );
  test( c2->associatedTypes() == c->associatedTypes() );
}

//----------------------------------------------------------------------------
void verify_write_valid_collection_exodus()
{
  std::string file_path(data_root);
  file_path += "/mesh/twoassm_out.h5m";

  std::string write_path(write_root);
  write_path += "/twoassm_output.exo";

  smtk::mesh::ManagerPtr manager = smtk::mesh::Manager::create();
  smtk::mesh::CollectionPtr c = smtk::io::ImportMesh::entireFile(file_path, manager);
  test( c->isValid(), "collection should be valid");

  //write out the mesh.
  bool result = smtk::io::WriteMesh::entireCollection(write_path, c);
  if(!result)
    {
    cleanup( write_path );
    test( result == true, "failed to properly write out a valid exodus collection");
    }

  //reload the written file and verify the number of meshes are the same as the
  //input mesh
  smtk::mesh::CollectionPtr c2 = smtk::io::ImportMesh::entireFile(write_path, manager);

  //remove the file from disk
  cleanup( write_path );

  //verify the meshes
  test( c2->isValid(), "collection should be valid");
  test( c2->name() == c->name() );
  test( c2->numberOfMeshes() == c->numberOfMeshes() );
  test( c2->associatedTypes() == c->associatedTypes() );
}

//----------------------------------------------------------------------------
void verify_write_onlyMaterial()
{
  std::string file_path(data_root);
  file_path += "/mesh/64bricks_12ktet.h5m";

  std::string write_path(write_root);
  write_path += "/64bricks_12ktet.h5m";

  smtk::mesh::ManagerPtr manager = smtk::mesh::Manager::create();
  smtk::mesh::CollectionPtr c = smtk::io::ImportMesh::entireFile(file_path, manager);
  test( c->isValid(), "collection should be valid");

  //write out the mesh.
  bool result = smtk::io::WriteMesh::onlyMaterial(write_path, c);
  if(!result)
    {
    cleanup( write_path );
    test( result == true, "failed to properly write out only Material");
    }

  //reload the written file and verify the number of meshes are the same as the
  //input mesh
  smtk::mesh::CollectionPtr c2 = smtk::io::ImportMesh::entireFile(write_path, manager);
  smtk::mesh::CollectionPtr c3 = smtk::io::ImportMesh::onlyMaterial(file_path, manager);

  // remove the file from disk
  cleanup( write_path );

  // //verify the meshes
  test( c2->isValid(), "collection should be valid");
  test( c2->name() == c3->name() );
  //need to dig into why we are getting a meshset on extraction that is not
  //part of the set
  test( c2->cells().size() == c3->cells().size() );
  test( c2->pointConnectivity().size() == c3->pointConnectivity().size() );
  test( c2->associatedTypes() == c3->associatedTypes() );
}

//----------------------------------------------------------------------------
void verify_write_onlyNeumann()
{
  std::string file_path(data_root);
  file_path += "/mesh/64bricks_12ktet.h5m";

  std::string write_path(write_root);
  write_path += "/64bricks_12ktet.h5m";

  smtk::mesh::ManagerPtr manager = smtk::mesh::Manager::create();
  smtk::mesh::CollectionPtr c = smtk::io::ImportMesh::entireFile(file_path, manager);
  test( c->isValid(), "collection should be valid");

  //write out the mesh.
  bool result = smtk::io::WriteMesh::onlyNeumann(write_path, c);
  if(!result)
    {
    cleanup( write_path );
    test( result == true, "failed to properly write out only Neumann");
    }

  //reload the written file and verify the number of meshes are the same as the
  //input mesh
  smtk::mesh::CollectionPtr c2 = smtk::io::ImportMesh::entireFile(write_path, manager);
  smtk::mesh::CollectionPtr c3 = smtk::io::ImportMesh::onlyNeumann(file_path, manager);

  // remove the file from disk
  cleanup( write_path );

  // //verify the meshes
  test( c2->isValid(), "collection should be valid");
  test( c2->name() == c3->name() );
  //need to dig into why we are getting a meshset on extraction that is not
  //part of the set
  test( c2->numberOfMeshes() + 1 == c3->numberOfMeshes() );
  test( c2->cells().size() == c3->cells().size() );
  test( c2->pointConnectivity().size() == c3->pointConnectivity().size() );
  test( c2->associatedTypes() == c3->associatedTypes() );


}

//----------------------------------------------------------------------------
void verify_write_onlyDirichlet()
{
  std::string file_path(data_root);
  file_path += "/mesh/64bricks_12ktet.h5m";

  std::string write_path(write_root);
  write_path += "/64bricks_12ktet.h5m";

  smtk::mesh::ManagerPtr manager = smtk::mesh::Manager::create();
  smtk::mesh::CollectionPtr c = smtk::io::ImportMesh::entireFile(file_path, manager);
  test( c->isValid(), "collection should be valid");

  //write out the mesh.
  bool result = smtk::io::WriteMesh::onlyDirichlet(write_path, c);
  if(!result)
    {
    cleanup( write_path );
    test( result == true, "failed to properly write out only Dirichlet");
    }

  //reload the written file and verify the number of meshes are the same as the
  //input mesh
  smtk::mesh::CollectionPtr c2 = smtk::io::ImportMesh::entireFile(write_path, manager);
  smtk::mesh::CollectionPtr c3 = smtk::io::ImportMesh::onlyDirichlet(file_path, manager);

  //remove the file from disk
  cleanup( write_path );

  //verify the meshes
  test( c2->isValid(), "collection should be valid");
  test( c2->name() == c3->name() );
  //need to dig into why we are getting a meshset on extraction that is not
  //part of the set
  test( c2->numberOfMeshes() + 1 == c3->numberOfMeshes() );
  test( c2->cells().size() == c3->cells().size() );
  test( c2->pointConnectivity().size() == c3->pointConnectivity().size() );
  test( c2->associatedTypes() == c3->associatedTypes() );
}


}

//----------------------------------------------------------------------------
int UnitTestWriteMesh(int argc, char** argv)
{
  verify_write_empty_collection();
  verify_write_null_collection();

  verify_write_valid_collection_hdf5();
  verify_write_valid_collection_exodus();

  verify_write_onlyMaterial();
  verify_write_onlyNeumann();
  verify_write_onlyDirichlet();

  return 0;
}
