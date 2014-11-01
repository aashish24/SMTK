//=========================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//=========================================================================
#include "smtk/io/ExportJSON.h"

#include "smtk/common/Version.h"

#include "smtk/model/BridgeRegistrar.h"
#include "smtk/model/BridgeIOJSON.h"
#include "smtk/model/Manager.h"
#include "smtk/model/Entity.h"
#include "smtk/model/ModelEntity.h"
#include "smtk/model/Operator.h"
#include "smtk/model/Tessellation.h"
#include "smtk/model/Arrangement.h"

#include "smtk/attribute/Attribute.h"
#include "smtk/attribute/Definition.h"
#include "smtk/attribute/System.h"

#include "smtk/io/AttributeWriter.h"
#include "smtk/io/Logger.h"

#include "cJSON.h"

#include <stdlib.h> // for free()

using namespace smtk::io;
using namespace smtk::common;
using namespace smtk::model;

// Some cJSON helpers
namespace {
  cJSON* cJSON_CreateLongArray(const long* ints, unsigned count);
  cJSON* cJSON_CreateStringArray(const std::string* strings, unsigned count);
  cJSON* cJSON_CreateUUIDArray(const smtk::common::UUID* uids, unsigned count);

  cJSON* cJSON_AddAttributeSpec(
    cJSON* opEntry,
    const char* tagName, // tag holding name of attribute
    const char* xmlTagName, // tag holding XML for attribute
    smtk::attribute::AttributePtr spec)
    {
    if (spec)
      {
      smtk::attribute::System tmpSys;
      tmpSys.setRefModelManager(spec->modelManager());
      tmpSys.copyAttribute(
        spec, smtk::attribute::System::FORCE_COPY_ASSOCIATIONS);
      smtk::io::Logger log;
      smtk::io::AttributeWriter wri;
      wri.includeDefinitions(false);
      wri.includeInstances(true);
      wri.includeModelInformation(false);
      wri.includeViews(false);
      std::string xml;
      bool err = wri.writeContents(tmpSys, xml, log, true);
      if (!err)
        {
        cJSON_AddItemToObject(opEntry, tagName,
          cJSON_CreateString(spec->name().c_str()));
        cJSON_AddItemToObject(opEntry, xmlTagName,
          cJSON_CreateString(xml.c_str()));
        }
      }
    return opEntry;
    }

  cJSON* cJSON_AddOperator(smtk::model::OperatorPtr op, cJSON* opEntry)
    {
    cJSON_AddItemToObject(opEntry, "name", cJSON_CreateString(op->name().c_str()));
    smtk::attribute::AttributePtr spec = op->specification();
    if (spec)
      {
      cJSON_AddAttributeSpec(opEntry, "spec", "specXML", spec);
      }
    /*
    cJSON_AddItemToObject(opEntry, "parameters",
      cJSON_CreateParameterArray(op->parameters()));
      */
    return opEntry;
    }

  /*
  cJSON* cJSON_CreateOperatorArray(const smtk::model::Operators& ops)
    {
    cJSON* a = cJSON_CreateArray();
    for (smtk::model::Operators::const_iterator it = ops.begin(); it != ops.end(); ++it)
      {
      cJSON* opEntry = cJSON_CreateObject();
      cJSON_AddItemToArray(a, opEntry);
      cJSON_AddOperator(*it, opEntry);
      }
    return a;
    }
    */

  cJSON* cJSON_CreateUUIDArray(const smtk::common::UUID* uids, unsigned count)
    {
    cJSON* a = cJSON_CreateArray();
    for (unsigned i = 0; i < count; ++i)
      {
      cJSON_AddItemToArray(a, cJSON_CreateString(uids[i].toString().c_str()));
      }
    return a;
    }

  cJSON* cJSON_CreateStringArray(const std::string* strings, unsigned count)
    {
    cJSON* a = cJSON_CreateArray();
    for (unsigned i = 0; i < count; ++i)
      {
      cJSON_AddItemToArray(a, cJSON_CreateString(strings[i].c_str()));
      }
    return a;
    }
  cJSON* cJSON_CreateLongArray(const long* ints, unsigned count)
    {
    cJSON* a = cJSON_CreateArray();
    for (unsigned i = 0; i < count; ++i)
      {
      if (ints[i] > 9007199254740991.0) //== 2^53 - 1, max integer-accurate double
        {
        std::cerr << "Error exporting array: integer value " << i << " (" << ints[i] << ") out of range for cJSON\n";
        }
      cJSON_AddItemToArray(a, cJSON_CreateNumber(ints[i]));
      }
    return a;
    }
}

namespace smtk {
  namespace io {

using smtk::common::UUID;

cJSON* ExportJSON::fromUUIDs(const UUIDs& uids)
{
  cJSON* a = cJSON_CreateArray();
  for (UUIDs::const_iterator it = uids.begin(); it != uids.end(); ++it)
    {
    cJSON_AddItemToArray(a, cJSON_CreateString(it->toString().c_str()));
    }
  return a;
}

int ExportJSON::fromModel(cJSON* json, ManagerPtr modelMgr, JSONFlags sections)
{
  int status = 0;
  if (!json || !modelMgr)
    {
    std::cerr << "Invalid arguments.\n";
    return status;
    }

  cJSON* body = cJSON_CreateObject();
  cJSON* sess = cJSON_CreateObject();
  switch(json->type)
    {
  case cJSON_Object:
    cJSON_AddItemToObject(json, "topo", body);
    cJSON_AddItemToObject(json, "sessions", sess);
    break;
  case cJSON_Array:
    cJSON_AddItemToArray(json, body);
    cJSON_AddItemToArray(json, sess);
    break;
  case cJSON_NULL:
  case cJSON_Number:
  case cJSON_String:
  default:
    std::cerr << "Invalid toplevel JSON type (" << json->type << ").\n";
    return status;
    break;
    }

  cJSON* mtyp = cJSON_CreateString("Manager");
  cJSON_AddItemToObject(json, "type", mtyp);
  status = ExportJSON::forManager(body, sess, modelMgr, sections);

  return status;
}

std::string ExportJSON::fromModel(ManagerPtr modelMgr, JSONFlags sections)
{
  cJSON* top = cJSON_CreateObject();
  ExportJSON::fromModel(top, modelMgr, sections);
  char* json = cJSON_Print(top);
  std::string result(json);
  free(json);
  cJSON_Delete(top);
  return result;
}

int ExportJSON::forManager(
  cJSON* dict, cJSON* sess, ManagerPtr modelMgr, JSONFlags sections)
{
  if (!dict || !modelMgr)
    {
    return 0;
    }
  int status = 1;
  UUIDWithEntity it;

  if (sections == JSON_NOTHING)
    return status;

  for (it = modelMgr->topology().begin(); it != modelMgr->topology().end(); ++it)
    {
    cJSON* curChild = cJSON_CreateObject();
      {
      std::string suid = it->first.toString();
      cJSON_AddItemToObject(dict, suid.c_str(), curChild);
      }
    if (sections & JSON_ENTITIES)
      {
      status &= ExportJSON::forManagerEntity(it, curChild, modelMgr);
      status &= ExportJSON::forManagerArrangement(
        modelMgr->arrangements().find(it->first), curChild, modelMgr);
      }
    if (sections & JSON_TESSELLATIONS)
      status &= ExportJSON::forManagerTessellation(it->first, curChild, modelMgr);
    if (sections & JSON_PROPERTIES)
      {
      status &= ExportJSON::forManagerFloatProperties(it->first, curChild, modelMgr);
      status &= ExportJSON::forManagerStringProperties(it->first, curChild, modelMgr);
      status &= ExportJSON::forManagerIntegerProperties(it->first, curChild, modelMgr);
      }
    }

  if (sections & JSON_BRIDGES)
    {
    smtk::common::UUIDs bridgeSessions = modelMgr->bridgeSessions();
    for (smtk::common::UUIDs::iterator bit = bridgeSessions.begin(); bit != bridgeSessions.end(); ++bit)
      {
      status &= ExportJSON::forManagerBridgeSession(*bit, sess, modelMgr);
      }
    }
  return status;
}

int ExportJSON::forManagerEntity(
  UUIDWithEntity& entry, cJSON* entRec, ManagerPtr model)
{
  (void)model;
  cJSON* ent = cJSON_CreateNumber(entry->second.entityFlags());
  cJSON* dim = cJSON_CreateNumber(entry->second.dimension());
  cJSON_AddItemToObject(entRec, "e", ent);
  cJSON_AddItemToObject(entRec, "d", dim);
  if (!entry->second.relations().empty())
    {
    cJSON_AddItemToObject(entRec, "r",
      cJSON_CreateUUIDArray(
        &entry->second.relations()[0],
        static_cast<unsigned int>(entry->second.relations().size())));
    }
  /*
  if (entry->second.entityFlags() & MODEL_ENTITY)
    ExportJSON::forModelOperators(entry->first, entRec, model);
    */
  return 1;
}

int ExportJSON::forManagerArrangement(
  const UUIDWithArrangementDictionary& entry, cJSON* dict, ManagerPtr model)
{
  if (entry == model->arrangements().end())
    {
    return 0;
    }
  ArrangementKindWithArrangements it;
  cJSON* arrNode = cJSON_CreateObject();
  cJSON_AddItemToObject(dict, "a", arrNode);
  for (it = entry->second.begin(); it != entry->second.end(); ++it)
    {
    Arrangements& arr(it->second);
    if (!arr.empty())
      {
      cJSON* kindNode = cJSON_CreateArray();
      cJSON_AddItemToObject(arrNode, smtk::model::AbbreviationForArrangementKind(it->first).c_str(), kindNode);
      Arrangements::iterator ait;
      for (ait = arr.begin(); ait != arr.end(); ++ait)
        {
        if (!ait->details().empty())
          {
          cJSON_AddItemToArray(kindNode,
            cJSON_CreateIntArray(&(ait->details()[0]), static_cast<int>(ait->details().size())));
          }
        }
      }
    }
  return 1;
}

int ExportJSON::forManagerTessellation(
  const smtk::common::UUID& uid, cJSON* dict, ManagerPtr model)
{
  UUIDWithTessellation tessIt = model->tessellations().find(uid);
  if (
    tessIt == model->tessellations().end() ||
    tessIt->second.coords().empty()
    )
    { // No tessellation? Not a problem.
    return 1;
    }
  //  "metadata": { "formatVersion" : 3 },
  //  "vertices": [ 0,0,0, 0,0,1, 1,0,1, 1,0,0, ... ],
  //  "normals":  [ 0,1,0, ... ],
  //  "faces": [
  //    0, 0,1,2, // tri
  //    1, 0,1,2,3, // quad
  //    32, 0,1,2, // tri w/ per-vert norm
  cJSON* tess = cJSON_CreateObject();
  //cJSON* meta = cJSON_CreateObject();
  cJSON* fmt = cJSON_CreateObject();
  cJSON_AddItemToObject(fmt,"formatVersion", cJSON_CreateNumber(3));
  //cJSON_AddItemToObject(meta, "metadata", fmt);
  //cJSON_AddItemToObject(tess, "3js", meta);
  cJSON_AddItemToObject(tess, "metadata", fmt);
  cJSON_AddItemToObject(tess, "vertices", cJSON_CreateDoubleArray(
      &tessIt->second.coords()[0],
      static_cast<int>(tessIt->second.coords().size())));
  cJSON_AddItemToObject(tess, "faces", cJSON_CreateIntArray(
      tessIt->second.conn().empty() ? NULL : &tessIt->second.conn()[0],
      static_cast<int>(tessIt->second.conn().size())));
  cJSON_AddItemToObject(dict, "t", tess);
  return 1;
}

int ExportJSON::forManagerFloatProperties(const smtk::common::UUID& uid, cJSON* dict, ManagerPtr model)
{
  int status = 1;
  UUIDWithFloatProperties entIt = model->floatProperties().find(uid);
  if (entIt == model->floatProperties().end() || entIt->second.empty())
    { // No properties is not an error
    return status;
    }
  cJSON* pdict = cJSON_CreateObject();
  cJSON_AddItemToObject(dict, "f", pdict);
  PropertyNameWithFloats entry;
  for (entry = entIt->second.begin(); entry != entIt->second.end(); ++entry)
    {
    if (entry->second.empty())
      {
      continue;
      }
    cJSON_AddItemToObject(pdict, entry->first.c_str(),
      cJSON_CreateDoubleArray(
        &entry->second[0], static_cast<int>(entry->second.size())));
    }
  return status;
}

int ExportJSON::forManagerStringProperties(const smtk::common::UUID& uid, cJSON* dict, ManagerPtr model)
{
  int status = 1;
  UUIDWithStringProperties entIt = model->stringProperties().find(uid);
  if (entIt == model->stringProperties().end() || entIt->second.empty())
    { // No properties is not an error
    return status;
    }
  cJSON* pdict = cJSON_CreateObject();
  cJSON_AddItemToObject(dict, "s", pdict);
  PropertyNameWithStrings entry;
  for (entry = entIt->second.begin(); entry != entIt->second.end(); ++entry)
    {
    if (entry->second.empty())
      {
      continue;
      }
    cJSON_AddItemToObject(pdict, entry->first.c_str(),
      cJSON_CreateStringArray(
        &entry->second[0], static_cast<unsigned int>(entry->second.size())));
    }
  return status;
}

int ExportJSON::forManagerIntegerProperties(const smtk::common::UUID& uid, cJSON* dict, ManagerPtr model)
{
  int status = 1;
  UUIDWithIntegerProperties entIt = model->integerProperties().find(uid);
  if (entIt == model->integerProperties().end() || entIt->second.empty())
    { // No properties is not an error
    return status;
    }
  cJSON* pdict = cJSON_CreateObject();
  cJSON_AddItemToObject(dict, "i", pdict);
  PropertyNameWithIntegers entry;
  for (entry = entIt->second.begin(); entry != entIt->second.end(); ++entry)
    {
    if (entry->second.empty())
      {
      continue;
      }
    cJSON_AddItemToObject(pdict, entry->first.c_str(),
      cJSON_CreateLongArray(
        &entry->second[0], static_cast<unsigned int>(entry->second.size())));
    }
  return status;
}

int ExportJSON::forManagerBridgeSession(const smtk::common::UUID& uid, cJSON* node, ManagerPtr modelMgr)
{
  int status = 1;
  BridgePtr bridge = modelMgr->findBridgeSession(uid);
  if (!bridge)
    return status;

  cJSON* sess = cJSON_CreateObject();
  cJSON_AddItemToObject(node, uid.toString().c_str(), sess);
  cJSON_AddStringToObject(sess, "type", "bridge-session");
  cJSON_AddStringToObject(sess, "name", bridge->name().c_str());
  BridgeIOJSONPtr delegate =
    smtk::dynamic_pointer_cast<BridgeIOJSON>(
      bridge->createIODelegate("json"));
  if (delegate)
    status &= delegate->exportJSON(modelMgr, sess);
  status &= ExportJSON::forOperatorDefinitions(bridge->operatorSystem(), sess);
  return status;
}

/*
int ExportJSON::forModelOperators(const smtk::common::UUID& uid, cJSON* entRec, ManagerPtr modelMgr)
{
  smtk::model::ModelEntity mod(modelMgr, uid);
  smtk::model::Operators ops(mod.operators());
  cJSON_AddItemToObject(entRec, "ops",
    cJSON_CreateOperatorArray(ops));`
  return 1; // ExportJSON::forOperators(ops, entRec);
} */

int ExportJSON::forOperatorDefinitions(smtk::attribute::System* opSys, cJSON* entRec)
{
  smtk::io::Logger log;
  smtk::io::AttributeWriter wri;
  wri.includeDefinitions(true);
  wri.includeInstances(false);
  wri.includeModelInformation(false);
  wri.includeViews(false);
  std::string xml;
  bool err = wri.writeContents(*opSys, xml, log, true);
  if (!err)
    {
    cJSON_AddItemToObject(entRec, "ops",
      cJSON_CreateString(xml.c_str()));
    }
  /*
  std::vector<smtk::attribute::DefinitionPtr> ops;
  opSys.derivedDefinitions(
    opSys.findDefinition("operator"), ops);
  if (!ops.empty())
    {
    cJSON_AddItemToObject(entRec, "ops",
      cJSON_CreateOperatorArray(ops));
    }
    */
  return 1;
}

int ExportJSON::forOperator(OperatorPtr op, cJSON* entRec)
{
  cJSON_AddOperator(op, entRec);
  return 1;
}

int ExportJSON::forOperator(smtk::attribute::AttributePtr op, cJSON* entRec)
{
  cJSON_AddItemToObject(entRec, "name", cJSON_CreateString(op->type().c_str()));
  cJSON_AddAttributeSpec(entRec, "spec", "specXML", op);
  return 1;
}

int ExportJSON::forOperatorResult(OperatorResult res, cJSON* entRec)
{
  cJSON_AddItemToObject(entRec, "name", cJSON_CreateString(res->type().c_str()));
  cJSON_AddAttributeSpec(entRec, "result", "resultXML", res);
  return 1;
}

/// Serialize a bridge's list of dangling entities held in the given \a modelMgr.
int ExportJSON::forDanglingEntities(const smtk::common::UUID& bridgeSessionId, cJSON* node, ManagerPtr modelMgr)
{
  if (!modelMgr || !node || node->type != cJSON_Object)
    return 0;
  BridgePtr bridge = modelMgr->findBridgeSession(bridgeSessionId);
  if (!bridge)
    return 0;

  cJSON* danglers = cJSON_CreateObject();
  cJSON* darray = cJSON_CreateObject();
  cJSON_AddItemToObject(node, "danglingEntities", danglers);
  cJSON_AddItemToObject(danglers, "sessionId", cJSON_CreateString(bridgeSessionId.toString().c_str()));
  cJSON_AddItemToObject(danglers, "entities", darray);
  DanglingEntities::const_iterator it;
  for (it = bridge->danglingEntities().begin(); it != bridge->danglingEntities().end(); ++it)
    {
    if (it->first.manager() == modelMgr)
      cJSON_AddItemToObject(darray, it->first.entity().toString().c_str(), cJSON_CreateNumber(it->second));
    }
  return 1;
}

/**\brief Serialize a description of a Remus model worker.
  *
  * This populates an empty JSON Object (\a wdesc) with
  * data required for a Remus server to start an smtk-model-worker
  * process for use by a RemusRemoteBridge instance.
  */
int ExportJSON::forModelWorker(
    cJSON* wdesc,
    const std::string& meshTypeIn, const std::string& meshTypeOut,
    smtk::model::BridgePtr bridge, const std::string& engine,
    const std::string& site, const std::string& root,
    const std::string& workerPath, const std::string& requirementsFileName)
{
  if (!wdesc || wdesc->type != cJSON_Object)
    return 0;

  // Base worker requirements
  cJSON_AddItemToObject(wdesc, "InputType", cJSON_CreateString(meshTypeIn.c_str()));
  cJSON_AddItemToObject(wdesc, "OutputType", cJSON_CreateString(meshTypeOut.c_str()));
  cJSON_AddItemToObject(wdesc, "ExecutableName", cJSON_CreateString(workerPath.c_str()));

  // External requirements information
  cJSON_AddItemToObject(wdesc, "File", cJSON_CreateString(requirementsFileName.c_str()));
  cJSON_AddItemToObject(wdesc, "FileFormat", cJSON_CreateString("XML"));

  // Additional requirements for SMTK model worker
  cJSON* argArray = cJSON_CreateArray();
  cJSON_AddItemToArray(argArray, cJSON_CreateString("-rwfile=@SELF@"));
  cJSON_AddItemToArray(argArray, cJSON_CreateString(("-kernel=" + bridge->name()).c_str()));
  if (!engine.empty())
    cJSON_AddItemToArray(argArray, cJSON_CreateString(("-engine=" + engine).c_str()));
  if (!site.empty())
    cJSON_AddItemToArray(argArray, cJSON_CreateString(("-site=" + site).c_str()));
  if (!root.empty())
    cJSON_AddItemToArray(argArray, cJSON_CreateString(("-root=" + root).c_str()));
  cJSON_AddItemToObject(wdesc, "Arguments", argArray);

  // TODO: Handle workers that can support multiple kernels as well as
  //       multiple engines.
  cJSON* tag = cJSON_Parse(BridgeRegistrar::bridgeTags(bridge->name()).c_str());
  if (!tag)
    return 0;

  cJSON_AddItemToObject(wdesc, "Tag", tag);
  cJSON_AddItemToObject(tag, "default_kernel", cJSON_CreateString(bridge->name().c_str()));
  cJSON_AddItemToObject(tag, "default_engine", cJSON_CreateString(engine.c_str()));
  cJSON_AddItemToObject(tag, "site", cJSON_CreateString(site.c_str()));
  cJSON_AddItemToObject(tag, "smtk_version",
    cJSON_CreateString(smtk::common::Version::number().c_str()));
  return 1;
}

/**\brief Create a JSON-RPC request object.
  *
  * This variant stores a valid pointer in \a params for you to populate.
  */
cJSON* ExportJSON::createRPCRequest(const std::string& method, cJSON*& params, const std::string& reqId)
{
  cJSON* rpcReq = cJSON_CreateObject();
  cJSON_AddItemToObject(rpcReq, "method", cJSON_CreateString(method.c_str()));
  cJSON_AddItemToObject(rpcReq, "id", cJSON_CreateString(reqId.c_str()));
  if (&params)
    {
    params = cJSON_CreateArray();
    cJSON_AddItemToObject(rpcReq, "params", params);
    }
  return rpcReq;
}

/**\brief Create a JSON-RPC request object.
  *
  * This variant stores a single string parameter, \a params.
  */
cJSON* ExportJSON::createRPCRequest(const std::string& method, const std::string& params, const std::string& reqId)
{
  cJSON* paramObj;
  cJSON* rpcReq = ExportJSON::createRPCRequest(method, paramObj, reqId);
  cJSON_AddItemToArray(paramObj, cJSON_CreateString(params.c_str()));
  return rpcReq;
}

/**\brief Copy \a arr into a JSON array of strings.
  *
  * You are responsible for managing the memory allocated for the returned
  * object, either by calling cJSON_Delete on it or adding it to another
  * cJSON node that is eventually deleted.
  */
cJSON* ExportJSON::createStringArray(const std::vector<std::string>& arr)
{
  return cJSON_CreateStringArray(&arr[0], static_cast<unsigned>(arr.size()));
}

/**\brief Copy \a arr into a JSON array of UUIDs.
  *
  * You are responsible for managing the memory allocated for the returned
  * object, either by calling cJSON_Delete on it or adding it to another
  * cJSON node that is eventually deleted.
  */
cJSON* ExportJSON::createUUIDArray(const std::vector<smtk::common::UUID>& arr)
{
  return cJSON_CreateUUIDArray(&arr[0], static_cast<unsigned>(arr.size()));
}

/**\brief Copy \a arr into a JSON array of integers.
  *
  * You are responsible for managing the memory allocated for the returned
  * object, either by calling cJSON_Delete on it or adding it to another
  * cJSON node that is eventually deleted.
  */
cJSON* ExportJSON::createIntegerArray(const std::vector<long>& arr)
{
  return cJSON_CreateLongArray(&arr[0], static_cast<unsigned>(arr.size()));
}

  }
}