////////////////////////////////////////////////////////////////////////////////
/// @brief Infrastructure for ExecutionPlans
///
/// @file arangod/Aql/ExecutionNode.cpp
///
/// DISCLAIMER
///
/// Copyright 2010-2014 triagens GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Max Neunhoeffer
/// @author Copyright 2014, triagens GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "Aql/ExecutionNode.h"
#include "Aql/Collection.h"
#include "Aql/ExecutionPlan.h"
#include "Aql/WalkerWorker.h"
#include "Aql/Ast.h"
#include "Basics/StringBuffer.h"

using namespace std;
using namespace triagens::basics;
using namespace triagens::aql;

const static bool Optional = true;

// -----------------------------------------------------------------------------
// --SECTION--                                             static initialization
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief maximum register id that can be assigned.
/// this is used for assertions
////////////////////////////////////////////////////////////////////////////////
    
RegisterId const ExecutionNode::MaxRegisterId = 1000;

////////////////////////////////////////////////////////////////////////////////
/// @brief type names
////////////////////////////////////////////////////////////////////////////////

std::unordered_map<int, std::string const> const ExecutionNode::TypeNames{ 
  { static_cast<int>(ILLEGAL),                      "ExecutionNode (abstract)" },
  { static_cast<int>(SINGLETON),                    "SingletonNode" },
  { static_cast<int>(ENUMERATE_COLLECTION),         "EnumerateCollectionNode" },
  { static_cast<int>(ENUMERATE_LIST),               "EnumerateListNode" },
  { static_cast<int>(INDEX_RANGE),                  "IndexRangeNode" },
  { static_cast<int>(LIMIT),                        "LimitNode" },
  { static_cast<int>(CALCULATION),                  "CalculationNode" },
  { static_cast<int>(SUBQUERY),                     "SubqueryNode" },
  { static_cast<int>(FILTER),                       "FilterNode" },
  { static_cast<int>(SORT),                         "SortNode" },
  { static_cast<int>(AGGREGATE),                    "AggregateNode" },
  { static_cast<int>(RETURN),                       "ReturnNode" },
  { static_cast<int>(REMOVE),                       "RemoveNode" },
  { static_cast<int>(INSERT),                       "InsertNode" },
  { static_cast<int>(UPDATE),                       "UpdateNode" },
  { static_cast<int>(REPLACE),                      "ReplaceNode" },
  { static_cast<int>(REMOTE),                       "RemoteNode" },
  { static_cast<int>(SCATTER),                      "ScatterNode" },
  { static_cast<int>(DISTRIBUTE),                   "DistributeNode" },
  { static_cast<int>(GATHER),                       "GatherNode" },
  { static_cast<int>(NORESULTS),                    "NoResultsNode" }
};
          
// -----------------------------------------------------------------------------
// --SECTION--                                          methods of ExecutionNode
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief returns the type name of the node
////////////////////////////////////////////////////////////////////////////////

std::string const& ExecutionNode::getTypeString () const {
  auto it = TypeNames.find(static_cast<int>(getType()));
  if (it != TypeNames.end()) {
    return (*it).second;
  }

  THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "missing type in TypeNames");
}

void ExecutionNode::validateType (int type) {
  auto it = TypeNames.find(static_cast<int>(type));
  if (it == TypeNames.end()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "unknown TypeID");
  }
}

void ExecutionNode::getSortElements (SortElementVector& elements,
                                     ExecutionPlan* plan,
                                     triagens::basics::Json const& oneNode,
                                     char const* which) {
  triagens::basics::Json jsonElements = oneNode.get("elements");
  if (! jsonElements.isArray()){
    std::string error = std::string("unexpected value for ") +
      std::string(which) + std::string(" elements");
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, error);
  }
  size_t len = jsonElements.size();
  elements.reserve(len);
  for (size_t i = 0; i < len; i++) {
    triagens::basics::Json oneJsonElement = jsonElements.at(static_cast<int>(i));
    bool ascending = JsonHelper::checkAndGetBooleanValue(oneJsonElement.json(), "ascending");
    Variable *v = varFromJson(plan->getAst(), oneJsonElement, "inVariable");
    elements.emplace_back(std::make_pair(v, ascending));
  }
}

ExecutionNode* ExecutionNode::fromJsonFactory (ExecutionPlan* plan,
                                               triagens::basics::Json const& oneNode) {
  auto JsonString = oneNode.toString();

  int nodeTypeID = JsonHelper::checkAndGetNumericValue<int>(oneNode.json(), "typeID");
  validateType(nodeTypeID);

  NodeType nodeType = (NodeType) nodeTypeID;
  switch (nodeType) {
    case SINGLETON:
      return new SingletonNode(plan, oneNode);
    case ENUMERATE_COLLECTION:
      return new EnumerateCollectionNode(plan, oneNode);
    case ENUMERATE_LIST:
      return new EnumerateListNode(plan, oneNode);
    case FILTER:
      return new FilterNode(plan, oneNode);
    case LIMIT:
      return new LimitNode(plan, oneNode);
    case CALCULATION:
      return new CalculationNode(plan, oneNode);
    case SUBQUERY: 
      return new SubqueryNode(plan, oneNode);
    case SORT: {
      SortElementVector elements;
      bool stable = JsonHelper::checkAndGetBooleanValue(oneNode.json(), "stable");
      getSortElements(elements, plan, oneNode, "SortNode");
      return new SortNode(plan, oneNode, elements, stable);
    }
    case AGGREGATE: {
      Variable* expressionVariable = varFromJson(plan->getAst(), oneNode, "expressionVariable", Optional);
      Variable* outVariable = varFromJson(plan->getAst(), oneNode, "outVariable", Optional);

      triagens::basics::Json jsonAggregates = oneNode.get("aggregates");
      if (! jsonAggregates.isArray()) {
        THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "missing node type in valueTypeNames"); 
      }

      std::vector<Variable const*> keepVariables;
      triagens::basics::Json jsonKeepVariables = oneNode.get("keepVariables");
      if (jsonKeepVariables.isArray()) {
        size_t const n = jsonKeepVariables.size();
        for (size_t i = 0; i < n; i++) {
          triagens::basics::Json keepVariable = jsonKeepVariables.at(static_cast<int>(i));
          Variable const* variable = varFromJson(plan->getAst(), keepVariable, "variable");
          keepVariables.push_back(variable);
        }
      }

      size_t const len = jsonAggregates.size();
      std::vector<std::pair<Variable const*, Variable const*>> aggregateVariables;

      aggregateVariables.reserve(len);
      for (size_t i = 0; i < len; i++) {
        triagens::basics::Json oneJsonAggregate = jsonAggregates.at(static_cast<int>(i));
        Variable* outVar = varFromJson(plan->getAst(), oneJsonAggregate, "outVariable");
        Variable* inVar =  varFromJson(plan->getAst(), oneJsonAggregate, "inVariable");

        aggregateVariables.emplace_back(std::make_pair(outVar, inVar));
      }

      triagens::basics::Json jsonCount = oneNode.get("count");
      bool countOnly = JsonHelper::checkAndGetBooleanValue(oneNode.json(), "count");

      return new AggregateNode(plan,
                               oneNode,
                               expressionVariable,
                               outVariable,
                               keepVariables,
                               plan->getAst()->variables()->variables(false),
                               aggregateVariables,  
                               countOnly);
    }
    case INSERT:
      return new InsertNode(plan, oneNode);
    case REMOVE:
      return new RemoveNode(plan, oneNode);
    case REPLACE:
      return new ReplaceNode(plan, oneNode);
    case UPDATE:
      return new UpdateNode(plan, oneNode);
    case RETURN:
      return new ReturnNode(plan, oneNode);
    case NORESULTS:
      return new NoResultsNode(plan, oneNode);
    case INDEX_RANGE:
      return new IndexRangeNode(plan, oneNode);
    case REMOTE:
      return new RemoteNode(plan, oneNode);
    case GATHER: {
      SortElementVector elements;
      getSortElements(elements, plan, oneNode, "GatherNode");
      return new GatherNode(plan, oneNode, elements);
    }
    case SCATTER: 
      return new ScatterNode(plan, oneNode);
    case DISTRIBUTE: 
      return new DistributeNode(plan, oneNode);
    case ILLEGAL: {
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, "invalid node type");
    }
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an ExecutionNode from JSON
////////////////////////////////////////////////////////////////////////////////

ExecutionNode::ExecutionNode (ExecutionPlan* plan,
                              triagens::basics::Json const& json) 
  : _id(JsonHelper::checkAndGetNumericValue<size_t>(json.json(), "id")),
    _estimatedCost(0.0), 
    _estimatedCostSet(false),
    _varUsageValid(true),
    _plan(plan),
    _depth(JsonHelper::checkAndGetNumericValue<int>(json.json(), "depth")) {

  TRI_ASSERT(_registerPlan.get() == nullptr); 
  _registerPlan.reset(new RegisterPlan());
  _registerPlan->clear();
  _registerPlan->depth = _depth;
  _registerPlan->totalNrRegs = JsonHelper::checkAndGetNumericValue<unsigned int>(json.json(), "totalNrRegs"); 

  auto jsonVarInfoList = json.get("varInfoList");
  if (! jsonVarInfoList.isArray()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "varInfoList needs to be a json list"); 
  }
 
  size_t len = jsonVarInfoList.size();
  _registerPlan->varInfo.reserve(len);

  for (size_t i = 0; i < len; i++) {
    auto jsonVarInfo = jsonVarInfoList.at(i);
    if (! jsonVarInfo.isObject()) {
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "one varInfoList item needs to be an object"); 
    }
    VariableId variableId = JsonHelper::checkAndGetNumericValue<VariableId>      (jsonVarInfo.json(), "VariableId");
    RegisterId registerId = JsonHelper::checkAndGetNumericValue<RegisterId>      (jsonVarInfo.json(), "RegisterId");
    unsigned int    depth = JsonHelper::checkAndGetNumericValue<unsigned int>(jsonVarInfo.json(), "depth");
  
    _registerPlan->varInfo.emplace(make_pair(variableId, VarInfo(depth, registerId)));
  }

  auto jsonNrRegsList = json.get("nrRegs");
  if (! jsonNrRegsList.isArray()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "nrRegs needs to be a json list"); 
  }

  len = jsonNrRegsList.size();
  _registerPlan->nrRegs.reserve(len);
  for (size_t i = 0; i < len; i++) {
    RegisterId oneReg = JsonHelper::getNumericValue<RegisterId>(jsonNrRegsList.at(i).json(), 0);
    _registerPlan->nrRegs.push_back(oneReg);
  }
  
  auto jsonNrRegsHereList = json.get("nrRegsHere");
  if (! jsonNrRegsHereList.isArray()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "nrRegsHere needs to be a json list"); 
  }

  len = jsonNrRegsHereList.size();
  _registerPlan->nrRegsHere.reserve(len);
  for (size_t i = 0; i < len; i++) {
    RegisterId oneReg = JsonHelper::getNumericValue<RegisterId>(jsonNrRegsHereList.at(i).json(), 0);
    _registerPlan->nrRegsHere.push_back(oneReg);
  }

  auto jsonRegsToClearList = json.get("regsToClear");
  if (! jsonRegsToClearList.isArray()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "regsToClear needs to be a json list"); 
  }

  len = jsonRegsToClearList.size();
  _regsToClear.reserve(len);
  for (size_t i = 0; i < len; i++) {
    RegisterId oneRegToClear = JsonHelper::getNumericValue<RegisterId>(jsonRegsToClearList.at(i).json(), 0);
    _regsToClear.insert(oneRegToClear);
  }

  auto allVars = plan->getAst()->variables();

  auto jsonvarsUsedLater = json.get("varsUsedLater");
  if (! jsonvarsUsedLater.isArray()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "varsUsedLater needs to be a json list"); 
  }

  len = jsonvarsUsedLater.size();
  _varsUsedLater.reserve(len);
  for (size_t i = 0; i < len; i++) {
    std::unique_ptr<Variable> oneVarUsedLater(new Variable(jsonvarsUsedLater.at(i)));
    Variable* oneVariable = allVars->getVariable(oneVarUsedLater->id);

    if (oneVariable == nullptr) {
      std::string errmsg = "varsUsedLater: ID not found in all-list: " + StringUtils::itoa(oneVarUsedLater->id);
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, errmsg); 
    }
    _varsUsedLater.insert(oneVariable);
  }

  auto jsonvarsValidList = json.get("varsValid");
  if (! jsonvarsValidList.isArray()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "varsValid needs to be a json list"); 
  }

  len = jsonvarsValidList.size();
  _varsValid.reserve(len);
  for (size_t i = 0; i < len; i++) {
    std::unique_ptr<Variable> oneVarValid(new Variable(jsonvarsValidList.at(i)));
    Variable* oneVariable = allVars->getVariable(oneVarValid->id);

    if (oneVariable == nullptr) {
      std::string errmsg = "varsValid: ID not found in all-list: " + StringUtils::itoa(oneVarValid->id);
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, errmsg); 
    }
    _varsValid.insert(oneVariable);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, export an ExecutionNode to JSON
////////////////////////////////////////////////////////////////////////////////

triagens::basics::Json ExecutionNode::toJson (TRI_memory_zone_t* zone,
                                              bool verbose) const {
  triagens::basics::Json json;
  triagens::basics::Json nodes;
  try {
    nodes = triagens::basics::Json(triagens::basics::Json::Array, 10);
    toJsonHelper(nodes, zone, verbose);
    json = triagens::basics::Json(triagens::basics::Json::Object, 1)
             ("nodes", nodes);
  }
  catch (std::exception&) {
    return triagens::basics::Json();
  }

  return json;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief execution Node clone utility to be called by derives
////////////////////////////////////////////////////////////////////////////////

void ExecutionNode::CloneHelper (ExecutionNode* other,
                                 ExecutionPlan* plan,
                                 bool withDependencies,
                                 bool withProperties) const {
  plan->registerNode(other);

  if (withProperties) {
    other->_regsToClear = _regsToClear;
    other->_depth = _depth;
    other->_varUsageValid = _varUsageValid;

    auto allVars = plan->getAst()->variables();
    // Create new structures on the new AST...
    other->_varsUsedLater.reserve(_varsUsedLater.size());
    for (auto orgVar: _varsUsedLater) {
      auto var = allVars->getVariable(orgVar->id);
      TRI_ASSERT(var != nullptr);
      other->_varsUsedLater.insert(var);
    }

    other->_varsValid.reserve(_varsValid.size());
    for (auto orgVar: _varsValid) {
      auto var = allVars->getVariable(orgVar->id);
      TRI_ASSERT(var != nullptr);
      other->_varsValid.insert(var);
    }

    if (_registerPlan.get() != nullptr) {
      auto otherRegisterPlan = std::shared_ptr<RegisterPlan>(_registerPlan->clone(plan, _plan));
      other->_registerPlan = otherRegisterPlan;
    }
  }
  else {
    // point to current AST -> don't do deep copies.
    other->_depth = _depth;
    other->_regsToClear = _regsToClear;
    other->_varUsageValid = _varUsageValid;
    other->_varsUsedLater = _varsUsedLater;
    other->_varsValid = _varsValid;
    other->_registerPlan = _registerPlan;
  }

  if (withDependencies) {
    cloneDependencies(plan, other, withProperties);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief helper for cloning, use virtual clone methods for dependencies
////////////////////////////////////////////////////////////////////////////////

void ExecutionNode::cloneDependencies (ExecutionPlan* plan,
                                       ExecutionNode* theClone,
                                       bool withProperties) const {
  auto it = _dependencies.begin();
  while (it != _dependencies.end()) {
    auto c = (*it)->clone(plan, true, withProperties);
    try {
      c->_parents.emplace_back(theClone);
      theClone->_dependencies.emplace_back(c);
    }
    catch (...) {
      delete c;
      throw;
    }
    ++it;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief convert to a string, basically for debugging purposes
////////////////////////////////////////////////////////////////////////////////

void ExecutionNode::appendAsString (std::string& st, int indent) {
  for (int i = 0; i < indent; i++) {
    st.push_back(' ');
  }
  
  st.push_back('<');
  st.append(getTypeString());
  if (_dependencies.size() != 0) {
    st.push_back('\n');
    for (size_t i = 0; i < _dependencies.size(); i++) {
      _dependencies[i]->appendAsString(st, indent + 2);
      if (i != _dependencies.size() - 1) {
        st.push_back(',');
      }
      else {
        st.push_back(' ');
      }
    }
  }
  st.push_back('>');
}

////////////////////////////////////////////////////////////////////////////////
/// @brief inspect one index; only skiplist indexes which match attrs in sequence.
/// returns a qualification how good they match;
/// match->index == nullptr means no match at all.
////////////////////////////////////////////////////////////////////////////////

ExecutionNode::IndexMatch ExecutionNode::CompareIndex (ExecutionNode const* node,
                                                       Index const* idx,
                                                       ExecutionNode::IndexMatchVec const& attrs) {
  IndexMatch match;

  if (attrs.empty()) {
    return match;
  }
  
  // check 
  std::unordered_set<std::string> equalityLookupAttributes;

  if (node->getType() == INDEX_RANGE) {
    // found an index range node...
    // now check, regardless of the type of index, which attributes are only used in
    // equality lookups 
    auto ranges = static_cast<IndexRangeNode const*>(node)->ranges();

    // check for OR
    if (ranges.size() == 1) {
      // no OR
   
      // check for equality-lookup ranges and note them for later
      for (auto const& r : ranges[0]) {
        if (r.is1ValueRangeInfo()) {
          // found an equality lookup    
          equalityLookupAttributes.emplace(r._attr);
        }
      }
    }
  }
  
  // check index type
  if (idx->type != TRI_IDX_TYPE_SKIPLIST_INDEX) {
    // no skiplist... that means we might have found the primary index or a hash index
    // with no guaranteed sort order.
    // still we can optimize away the sort if (and only if) all index attributes are used
    // in the sort criteria, and all index attributes are used for constant equality
    // lookups (e.g. doc.value1 == 1 && doc.value2 == 2 SORT doc.value1, doc.value2)
 
    for (auto const& attr : attrs) {
      if (equalityLookupAttributes.find(attr.first) == equalityLookupAttributes.end()) {
        return match;
      }
    }
 
    // when we get here we will be able to optimize away the sort
    match.doesMatch = true;
    return match;
  }


  TRI_ASSERT(idx->type == TRI_IDX_TYPE_SKIPLIST_INDEX);

  size_t const idxFields = idx->fields.size();
  size_t const n = attrs.size();
  match.doesMatch = (idxFields >= n);

  size_t interestingCount = 0;
  size_t forwardCount = 0;
  size_t backwardCount = 0;
  size_t i = 0;
  size_t j = 0;

  for (; (i < idxFields && j < n); i++) {
    if (equalityLookupAttributes.find(idx->fields[i]) != equalityLookupAttributes.end()) {
      // found an attribute in the sort criterion that is used in an equality lookup, too...
      // (e.g. doc.value == 1 && SORT doc.value1)
      // in this case, we can ignore the sorting for this particular attribute, as the index
      // will only return constant values for it
      match.matches.push_back(FORWARD_MATCH); // doesn't matter here if FORWARD or BACKWARD
      ++interestingCount;
      if (attrs[j].first == idx->fields[i]) {
        ++j;
      }
      continue;
    }

    if (attrs[j].first == idx->fields[i]) {
      if (attrs[j].second) {
        // ascending
        match.matches.push_back(FORWARD_MATCH);
        ++forwardCount;
        if (backwardCount > 0) {
          match.doesMatch = false;
        }
      }
      else {
        // descending
        match.matches.push_back(REVERSE_MATCH);
        ++backwardCount;
        if (forwardCount > 0) {
          match.doesMatch = false;
        }
        match.reverse = true;
      }
      ++interestingCount;
    }
    else {
      match.matches.push_back(NO_MATCH);
      match.doesMatch = false;
    }
    ++j;
  }

  if (interestingCount > 0) {
    match.index = idx;

    if (i < idxFields) { // more index fields
      for (; i < idxFields; i++) {
        match.matches.push_back(NOT_COVERED_IDX);
      }
    }
    else if (j < attrs.size()) { // more sorts
      for (; j < attrs.size(); j++) {
        match.matches.push_back(NOT_COVERED_ATTR);
      }
      match.doesMatch = false;
    }
  }

  return match;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief functionality to walk an execution plan recursively
////////////////////////////////////////////////////////////////////////////////

bool ExecutionNode::walk (WalkerWorker<ExecutionNode>* worker) {
  // Only do every node exactly once:
  if (worker->done(this)) {
    return false;
  }

  if (worker->before(this)) {
    return true;
  }
  
  // Now the children in their natural order:
  for (auto it : _dependencies) {
    if (it->walk(worker)) {
      return true;
    }
  }
  
  // Now handle a subquery:
  if (getType() == SUBQUERY) {
    auto p = static_cast<SubqueryNode*>(this);
    if (worker->enterSubquery(this, p->getSubquery())) {
      bool abort = p->getSubquery()->walk(worker);
      worker->leaveSubquery(this, p->getSubquery());
      if (abort) {
        return true;
      }
    }
  }

  worker->after(this);

  return false;
}

// -----------------------------------------------------------------------------
// --SECTION--                                                 protected methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief factory for (optional) variables from json.
////////////////////////////////////////////////////////////////////////////////

Variable* ExecutionNode::varFromJson (Ast* ast,
                                      triagens::basics::Json const& base,
                                      const char* variableName,
                                      bool optional) {
  triagens::basics::Json variableJson = base.get(variableName);

  if (variableJson.isEmpty()) {
    if (optional) {
      return nullptr;
    }
    else {
      std::string msg;
      msg += "Mandatory variable \"" + std::string(variableName) + "\" not found.";
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, msg.c_str());
    }
  }
  else {
    return ast->variables()->createVariable(variableJson);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJsonHelper, for a generic node
////////////////////////////////////////////////////////////////////////////////

triagens::basics::Json ExecutionNode::toJsonHelperGeneric (triagens::basics::Json& nodes,
                                                           TRI_memory_zone_t* zone,
                                                           bool verbose) const {
  size_t const n = _dependencies.size();
  for (size_t i = 0; i < n; i++) {
    _dependencies[i]->toJsonHelper(nodes, zone, verbose);
  }

  triagens::basics::Json json(triagens::basics::Json::Object, 5);
  json("type", triagens::basics::Json(getTypeString()));

  if (verbose) {
    json("typeID", triagens::basics::Json(static_cast<int>(getType())));
  }

  triagens::basics::Json deps(triagens::basics::Json::Array, n);
  for (size_t i = 0; i < n; i++) {
    deps(triagens::basics::Json(static_cast<double>(_dependencies[i]->id())));
  }
  json("dependencies", deps);

  if (verbose) {
    triagens::basics::Json parents(triagens::basics::Json::Array, _parents.size());
    for (size_t i = 0; i < _parents.size(); i++) {
      parents(triagens::basics::Json(static_cast<double>(_parents[i]->id())));
    }
    json("parents", parents);
  }

  json("id", triagens::basics::Json(static_cast<double>(id())));
  size_t nrItems = 0;
  json("estimatedCost", triagens::basics::Json(getCost(nrItems)));
  json("estimatedNrItems", triagens::basics::Json(static_cast<double>(nrItems)));

  if (verbose) {
    json("depth", triagens::basics::Json(static_cast<double>(_depth)));
 
    if (_registerPlan) {
      triagens::basics::Json jsonVarInfoList(triagens::basics::Json::Array, _registerPlan->varInfo.size());
      for (auto oneVarInfo: _registerPlan->varInfo) {
        triagens::basics::Json jsonOneVarInfoArray(triagens::basics::Json::Object, 2);
        jsonOneVarInfoArray
          ("VariableId", triagens::basics::Json(static_cast<double>(oneVarInfo.first)))
          ("depth", triagens::basics::Json(static_cast<double>(oneVarInfo.second.depth)))
          ("RegisterId", triagens::basics::Json(static_cast<double>(oneVarInfo.second.registerId)))
          ;
        jsonVarInfoList(jsonOneVarInfoArray);
      }
      json("varInfoList", jsonVarInfoList);

      triagens::basics::Json jsonNRRegsList(triagens::basics::Json::Array, _registerPlan->nrRegs.size());
      for (auto oneRegisterID: _registerPlan->nrRegs) {
        jsonNRRegsList(triagens::basics::Json(static_cast<double>(oneRegisterID)));
      }
      json("nrRegs", jsonNRRegsList);
    
      triagens::basics::Json jsonNRRegsHereList(triagens::basics::Json::Array, _registerPlan->nrRegsHere.size());
      for (auto oneRegisterID: _registerPlan->nrRegsHere) {
        jsonNRRegsHereList(triagens::basics::Json(static_cast<double>(oneRegisterID)));
      }
      json("nrRegsHere", jsonNRRegsHereList);
      json("totalNrRegs", triagens::basics::Json(static_cast<double>(_registerPlan->totalNrRegs)));
    }
    else {
      json("varInfoList", triagens::basics::Json(triagens::basics::Json::Array));
      json("nrRegs", triagens::basics::Json(triagens::basics::Json::Array));
      json("nrRegsHere", triagens::basics::Json(triagens::basics::Json::Array));
      json("totalNrRegs", triagens::basics::Json(0.0));
    }

    triagens::basics::Json jsonRegsToClearList(triagens::basics::Json::Array, _regsToClear.size());
    for (auto oneRegisterID : _regsToClear) {
      jsonRegsToClearList(triagens::basics::Json(static_cast<double>(oneRegisterID)));
    }
    json("regsToClear", jsonRegsToClearList);

    triagens::basics::Json jsonVarsUsedLaterList(triagens::basics::Json::Array, _varsUsedLater.size());
    for (auto oneVarUsedLater: _varsUsedLater) {
      jsonVarsUsedLaterList.add(oneVarUsedLater->toJson());
    }

    json("varsUsedLater", jsonVarsUsedLaterList);

    triagens::basics::Json jsonvarsValidList(triagens::basics::Json::Array, _varsValid.size());
    for (auto oneVarUsedLater: _varsValid) {
      jsonvarsValidList.add(oneVarUsedLater->toJson());
    }

    json("varsValid", jsonvarsValidList);
  }
  return json;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief static analysis debugger
////////////////////////////////////////////////////////////////////////////////

struct RegisterPlanningDebugger : public WalkerWorker<ExecutionNode> {
  RegisterPlanningDebugger () : indent(0) {};
  ~RegisterPlanningDebugger () {};

  int indent;

  bool enterSubquery (ExecutionNode*, ExecutionNode*) override final {
    indent++;
    return true;
  }

  void leaveSubquery (ExecutionNode*, ExecutionNode*) override final {
    indent--;
  }

  void after (ExecutionNode* ep) override final {
    for (int i = 0; i < indent; i++) {
      std::cout << " ";
    }
    std::cout << ep->getTypeString() << " ";
    std::cout << "regsUsedHere: ";
    for (auto v : ep->getVariablesUsedHere()) {
      std::cout << ep->getRegisterPlan()->varInfo.find(v->id)->second.registerId
                << " ";
    }
    std::cout << "regsSetHere: ";
    for (auto v : ep->getVariablesSetHere()) {
      std::cout << ep->getRegisterPlan()->varInfo.find(v->id)->second.registerId
                << " ";
    }
    std::cout << "regsToClear: ";
    for (auto r : ep->getRegsToClear()) {
      std::cout << r << " ";
    }
    std::cout << std::endl;
  }
};

////////////////////////////////////////////////////////////////////////////////
/// @brief planRegisters
////////////////////////////////////////////////////////////////////////////////

void ExecutionNode::planRegisters (ExecutionNode* super) {
  // The super is only for the case of subqueries.
  shared_ptr<RegisterPlan> v;
  if (super == nullptr) {
    v.reset(new RegisterPlan());
  }
  else {
    v.reset(new RegisterPlan(*(super->_registerPlan), super->_depth));
  }
  v->setSharedPtr(&v);

  walk(v.get());
  // Now handle the subqueries:
  for (auto s : v->subQueryNodes) {
    auto sq = static_cast<SubqueryNode*>(s);
    sq->getSubquery()->planRegisters(s);
  }
  v->reset();

  // Just for debugging:
  /*
  std::cout << std::endl;
  RegisterPlanningDebugger debugger;
  walk(&debugger);
  std::cout << std::endl;
  */
}

// -----------------------------------------------------------------------------
// --SECTION--                                struct ExecutionNode::RegisterPlan
// -----------------------------------------------------------------------------

// Copy constructor used for a subquery:
ExecutionNode::RegisterPlan::RegisterPlan (RegisterPlan const& v,
                                           unsigned int newdepth)
  : varInfo(v.varInfo),
    nrRegsHere(v.nrRegsHere),
    nrRegs(v.nrRegs),
    subQueryNodes(),
    depth(newdepth + 1),
    totalNrRegs(v.nrRegs[newdepth]),
    me(nullptr) {
  nrRegs.resize(depth);
  nrRegsHere.resize(depth);
  nrRegsHere.push_back(0);

  RegisterId registerId = nrRegs.back();
  nrRegs.push_back(registerId);
}

void ExecutionNode::RegisterPlan::clear () {
  varInfo.clear();
  nrRegsHere.clear();
  nrRegs.clear();
  subQueryNodes.clear();
  depth = 0;
  totalNrRegs = 0;
}

ExecutionNode::RegisterPlan* ExecutionNode::RegisterPlan::clone (ExecutionPlan* otherPlan, ExecutionPlan* plan) {
  std::unique_ptr<RegisterPlan> other(new RegisterPlan());

  other->nrRegsHere  = nrRegsHere;
  other->nrRegs      = nrRegs;
  other->depth       = depth;
  other->totalNrRegs = totalNrRegs;

  other->varInfo = varInfo;

  // No need to clone subQueryNodes because this was only used during
  // the buildup.

  return other.release();
}

void ExecutionNode::RegisterPlan::after (ExecutionNode *en) {
  switch (en->getType()) {
    case ExecutionNode::ENUMERATE_COLLECTION: 
    case ExecutionNode::INDEX_RANGE: {
      depth++;
      nrRegsHere.push_back(1);
      RegisterId registerId = 1 + nrRegs.back();
      nrRegs.push_back(registerId);
      auto ep = static_cast<EnumerateCollectionNode const*>(en);
      TRI_ASSERT(ep != nullptr);
      varInfo.emplace(make_pair(ep->_outVariable->id,
                               VarInfo(depth, totalNrRegs)));
      totalNrRegs++;
      break;
    }
    case ExecutionNode::ENUMERATE_LIST: {
      depth++;
      nrRegsHere.push_back(1);
      RegisterId registerId = 1 + nrRegs.back();
      nrRegs.push_back(registerId);
      auto ep = static_cast<EnumerateListNode const*>(en);
      TRI_ASSERT(ep != nullptr);
      varInfo.emplace(make_pair(ep->_outVariable->id,
                               VarInfo(depth, totalNrRegs)));
      totalNrRegs++;
      break;
    }
    case ExecutionNode::CALCULATION: {
      nrRegsHere[depth]++;
      nrRegs[depth]++;
      auto ep = static_cast<CalculationNode const*>(en);
      TRI_ASSERT(ep != nullptr);
      varInfo.emplace(make_pair(ep->_outVariable->id,
                               VarInfo(depth, totalNrRegs)));
      totalNrRegs++;
      break;
    }

    case ExecutionNode::SUBQUERY: {
      nrRegsHere[depth]++;
      nrRegs[depth]++;
      auto ep = static_cast<SubqueryNode const*>(en);
      TRI_ASSERT(ep != nullptr);
      varInfo.emplace(make_pair(ep->_outVariable->id,
                               VarInfo(depth, totalNrRegs)));
      totalNrRegs++;
      subQueryNodes.push_back(en);
      break;
    }

    case ExecutionNode::AGGREGATE: {
      depth++;
      nrRegsHere.push_back(0);
      RegisterId registerId = nrRegs.back();
      nrRegs.push_back(registerId);

      auto ep = static_cast<AggregateNode const*>(en);
      for (auto p : ep->_aggregateVariables) {
        // p is std::pair<Variable const*,Variable const*>
        // and the first is the to be assigned output variable
        // for which we need to create a register in the current
        // frame:
        nrRegsHere[depth]++;
        nrRegs[depth]++;
        varInfo.emplace(make_pair(p.first->id,
                                 VarInfo(depth, totalNrRegs)));
        totalNrRegs++;
      }
      if (ep->_outVariable != nullptr) {
        nrRegsHere[depth]++;
        nrRegs[depth]++;
        varInfo.emplace(make_pair(ep->_outVariable->id,
                                 VarInfo(depth, totalNrRegs)));
        totalNrRegs++;
      }
      break;
    }

    case ExecutionNode::SORT: {
      // sort sorts in place and does not produce new registers
      break;
    }
    
    case ExecutionNode::RETURN: {
      // return is special. it produces a result but is the last step in the pipeline
      break;
    }

    case ExecutionNode::REMOVE: {
      /*
      auto ep = static_cast<RemoveNode const*>(en);
      if (ep->_outVariable != nullptr) {
        nrRegsHere[depth]++;
        nrRegs[depth]++;
        varInfo.emplace(make_pair(ep->_outVariable->id,
                                 VarInfo(depth, totalNrRegs)));
        totalNrRegs++;
      }
      */
      break;
    }

    case ExecutionNode::INSERT: {
      /*
      auto ep = static_cast<InsertNode const*>(en);
      if (ep->_outVariable != nullptr) {
        nrRegsHere[depth]++;
        nrRegs[depth]++;
        varInfo.emplace(make_pair(ep->_outVariable->id,
                                 VarInfo(depth, totalNrRegs)));
        totalNrRegs++;
      }
      */
      break;
    }

    case ExecutionNode::UPDATE: {
      /*
      auto ep = static_cast<UpdateNode const*>(en);
      if (ep->_outVariable != nullptr) {
        nrRegsHere[depth]++;
        nrRegs[depth]++;
        varInfo.emplace(make_pair(ep->_outVariable->id,
                                 VarInfo(depth, totalNrRegs)));
        totalNrRegs++;
      }
      */
      break;
    }

    case ExecutionNode::REPLACE: {
      /*
      auto ep = static_cast<ReplaceNode const*>(en);
      if (ep->_outVariable != nullptr) {
        nrRegsHere[depth]++;
        nrRegs[depth]++;
        varInfo.emplace(make_pair(ep->_outVariable->id,
                                 VarInfo(depth, totalNrRegs)));
        totalNrRegs++;
      }
      */
      break;
    }

    case ExecutionNode::SINGLETON:
    case ExecutionNode::FILTER:
    case ExecutionNode::LIMIT:
    case ExecutionNode::SCATTER: 
    case ExecutionNode::DISTRIBUTE: 
    case ExecutionNode::GATHER: 
    case ExecutionNode::REMOTE: 
    case ExecutionNode::NORESULTS: {
      // these node types do not produce any new registers
      break;
    }

    case ExecutionNode::ILLEGAL: {
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_NOT_IMPLEMENTED, "node type not implemented");
    }
  }

  en->_depth = depth;
  en->_registerPlan = *me;

  // Now find out which registers ought to be erased after this node:
  if (en->getType() != ExecutionNode::RETURN) {
    // ReturnNodes are special, since they return a single column anyway
    std::unordered_set<Variable const*> const& varsUsedLater = en->getVarsUsedLater();
    std::vector<Variable const*> const& varsUsedHere = en->getVariablesUsedHere();
    
    // We need to delete those variables that have been used here but are not
    // used any more later:
    std::unordered_set<RegisterId> regsToClear;
    for (auto v : varsUsedHere) {
      auto it = varsUsedLater.find(v);
      if (it == varsUsedLater.end()) {
        auto it2 = varInfo.find(v->id);
        TRI_ASSERT(it2 != varInfo.end());
        RegisterId r = it2->second.registerId;
        regsToClear.insert(r);
      }
    }
    en->setRegsToClear(regsToClear);
  }
}

// -----------------------------------------------------------------------------
// --SECTION--                                          methods of SingletonNode
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for SingletonNode
////////////////////////////////////////////////////////////////////////////////

SingletonNode::SingletonNode (ExecutionPlan* plan, 
                              triagens::basics::Json const& base)
  : ExecutionNode(plan, base) {
}

void SingletonNode::toJsonHelper (triagens::basics::Json& nodes,
                                  TRI_memory_zone_t* zone,
                                  bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method
  if (json.isEmpty()) {
    return;
  }

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief the cost of a singleton is 1, it produces one item only
////////////////////////////////////////////////////////////////////////////////
        
double SingletonNode::estimateCost (size_t& nrItems) const {
  nrItems = 1;
  return 1.0;
}

// -----------------------------------------------------------------------------
// --SECTION--                                methods of EnumerateCollectionNode
// -----------------------------------------------------------------------------

EnumerateCollectionNode::EnumerateCollectionNode (ExecutionPlan* plan,
                                                  triagens::basics::Json const& base)
  : ExecutionNode(plan, base),
    _vocbase(plan->getAst()->query()->vocbase()),
    _collection(plan->getAst()->query()->collections()->get(JsonHelper::checkAndGetStringValue(base.json(), "collection"))),
    _outVariable(varFromJson(plan->getAst(), base, "outVariable")),
    _random(JsonHelper::checkAndGetBooleanValue(base.json(), "random")) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for EnumerateCollectionNode
////////////////////////////////////////////////////////////////////////////////

void EnumerateCollectionNode::toJsonHelper (triagens::basics::Json& nodes,
                                            TRI_memory_zone_t* zone,
                                            bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method

  if (json.isEmpty()) {
    return;
  }

  // Now put info about vocbase and cid in there
  json("database", triagens::basics::Json(_vocbase->_name))
      ("collection", triagens::basics::Json(_collection->getName()))
      ("outVariable", _outVariable->toJson())
      ("random", triagens::basics::Json(_random));

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief clone ExecutionNode recursively
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* EnumerateCollectionNode::clone (ExecutionPlan* plan,
                                               bool withDependencies,
                                               bool withProperties) const {
  auto outVariable = _outVariable;
  if (withProperties) {
    outVariable = plan->getAst()->variables()->createVariable(outVariable);
    TRI_ASSERT(outVariable != nullptr);
  }
  auto c = new EnumerateCollectionNode(plan, _id, _vocbase, _collection, outVariable, _random);

  CloneHelper(c, plan, withDependencies, withProperties);

  return static_cast<ExecutionNode*>(c);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief get the number of usable fields from the index (according to the
/// attributes passed)
////////////////////////////////////////////////////////////////////////////////

size_t EnumerateCollectionNode::getUsableFieldsOfIndex (Index const* idx,
                                                        std::unordered_set<std::string> const& attrs) const {
  size_t count = 0;
  for (size_t i = 0; i < idx->fields.size(); i++) {
    if (attrs.find(idx->fields[i]) == attrs.end()) {
      break;
    }

    ++count;
  }
  
  return count;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief get vector of indexes with fields <attrs> 
////////////////////////////////////////////////////////////////////////////////

// checks if a subset of <attrs> is a prefix of <idx->_fields> for every index
// of the collection of this node, modifies its arguments <idxs>, and <prefixes>
// so that . . . 

void EnumerateCollectionNode::getIndexesForIndexRangeNode (std::unordered_set<std::string> const& attrs, 
                                                           std::vector<Index*>& idxs, 
                                                           std::vector<size_t>& prefixes) const {

  auto&& indexes = _collection->getIndexes();

  for (auto idx : indexes) {
    TRI_ASSERT(idx != nullptr);

    auto const idxType = idx->type;

    if (idxType != TRI_IDX_TYPE_PRIMARY_INDEX &&
        idxType != TRI_IDX_TYPE_HASH_INDEX &&
        idxType != TRI_IDX_TYPE_SKIPLIST_INDEX &&
        idxType != TRI_IDX_TYPE_EDGE_INDEX) {
      // only these index types can be used
      continue;
    }

    size_t prefix = 0;

    if (idxType == TRI_IDX_TYPE_PRIMARY_INDEX) {
      // primary index allows lookups on both "_id" and "_key" in isolation
      if (attrs.find(std::string(TRI_VOC_ATTRIBUTE_ID)) != attrs.end() ||
          attrs.find(std::string(TRI_VOC_ATTRIBUTE_KEY)) != attrs.end()) {
        // can use index
        idxs.push_back(idx);
        // <prefixes> not used for this type of index
        prefixes.push_back(0);
      }
    }

    else if (idxType == TRI_IDX_TYPE_HASH_INDEX) {
      prefix = getUsableFieldsOfIndex(idx, attrs);

      if (prefix == idx->fields.size()) {
        // can use index
        idxs.push_back(idx);
        // <prefixes> not used for this type of index
        prefixes.push_back(0);
      } 
    }

    else if (idxType == TRI_IDX_TYPE_SKIPLIST_INDEX) {
      prefix = getUsableFieldsOfIndex(idx, attrs);

      if (prefix > 0) {
        // can use index
        idxs.push_back(idx);
        prefixes.push_back(prefix);
      }
    }
    
    else if (idxType == TRI_IDX_TYPE_EDGE_INDEX) {
      // edge index allows lookups on both "_from" and "_to" in isolation
      if (attrs.find(std::string(TRI_VOC_ATTRIBUTE_FROM)) != attrs.end() ||
          attrs.find(std::string(TRI_VOC_ATTRIBUTE_TO)) != attrs.end()) {
        // can use index
        idxs.push_back(idx);
        // <prefixes> not used for this type of index
        prefixes.push_back(0);
      }
    }
    
    else {
      TRI_ASSERT(false);
    }
  }
}

std::vector<EnumerateCollectionNode::IndexMatch> 
    EnumerateCollectionNode::getIndicesOrdered (IndexMatchVec const& attrs) const {

  std::vector<IndexMatch> out;
  auto&& indexes = _collection->getIndexes();

  for (auto idx : indexes) {
    if (idx->sparse) {
      // sparse indexes cannot be used for replacing an EnumerateCollection node
      continue;
    }

    IndexMatch match = CompareIndex(this, idx, attrs);

    if (match.index != nullptr) {
      out.push_back(match);
    }
  }

  return out;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief the cost of an enumerate collection node is a multiple of the cost of
/// its unique dependency
////////////////////////////////////////////////////////////////////////////////
        
double EnumerateCollectionNode::estimateCost (size_t& nrItems) const { 
  size_t incoming;
  double depCost = _dependencies.at(0)->getCost(incoming);
  size_t count = _collection->count();
  nrItems = incoming * count;
  // We do a full collection scan for each incoming item.
  // random iteration is slightly more expensive than linear iteration
  return depCost + nrItems * (_random ? 1.005 : 1.0);
}

// -----------------------------------------------------------------------------
// --SECTION--                                      methods of EnumerateListNode
// -----------------------------------------------------------------------------

EnumerateListNode::EnumerateListNode (ExecutionPlan* plan,
                                      triagens::basics::Json const& base)
  : ExecutionNode(plan, base),
    _inVariable(varFromJson(plan->getAst(), base, "inVariable")),
    _outVariable(varFromJson(plan->getAst(), base, "outVariable")) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for EnumerateListNode
////////////////////////////////////////////////////////////////////////////////

void EnumerateListNode::toJsonHelper (triagens::basics::Json& nodes,
                                      TRI_memory_zone_t* zone,
                                      bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method
  if (json.isEmpty()) {
    return;
  }
  json("inVariable",  _inVariable->toJson())
      ("outVariable", _outVariable->toJson());

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief clone ExecutionNode recursively
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* EnumerateListNode::clone (ExecutionPlan* plan,
                                         bool withDependencies,
                                         bool withProperties) const {
  auto outVariable = _outVariable;
  auto inVariable = _inVariable;

  if (withProperties) {
    outVariable = plan->getAst()->variables()->createVariable(outVariable);
    inVariable = plan->getAst()->variables()->createVariable(inVariable);
  }

  auto c = new EnumerateListNode(plan, _id, inVariable, outVariable);

  CloneHelper(c, plan, withDependencies, withProperties);

  return static_cast<ExecutionNode*>(c);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief the cost of an enumerate list node
////////////////////////////////////////////////////////////////////////////////
        
double EnumerateListNode::estimateCost (size_t& nrItems) const {
  size_t incoming = 0;
  double depCost = _dependencies.at(0)->getCost(incoming);

  // Well, what can we say? The length of the list can in general
  // only be determined at runtime... If we were to know that this
  // list is constant, then we could maybe multiply by the length
  // here... For the time being, we assume 100
  size_t length = 100;
 
  auto setter = _plan->getVarSetBy(_inVariable->id);

  if (setter != nullptr) {
    if (setter->getType() == ExecutionNode::CALCULATION) {
      // list variable introduced by a calculation
      auto expression = static_cast<CalculationNode*>(setter)->expression();

      if (expression != nullptr) {
        auto node = expression->node();

        if (node->type == NODE_TYPE_ARRAY) {
          // this one is easy
          length = node->numMembers();
        }
        if (node->type == NODE_TYPE_RANGE) {
          auto low = node->getMember(0); 
          auto high = node->getMember(1); 

          if (low->isConstant() && 
              high->isConstant() &&
              (low->isValueType(VALUE_TYPE_INT) || low->isValueType(VALUE_TYPE_DOUBLE)) &&
              (high->isValueType(VALUE_TYPE_INT) || high->isValueType(VALUE_TYPE_DOUBLE))) {
            // create a temporary range to determine the size
            Range range(low->getIntValue(), high->getIntValue());

            length = range.size();
          }
        }
      }
    }
    else if (setter->getType() == ExecutionNode::SUBQUERY) {
      // length will be set by the subquery's cost estimator
      static_cast<SubqueryNode const*>(setter)->getSubquery()->estimateCost(length);
    }
  }

  nrItems = length * incoming;
  return depCost + static_cast<double>(length) * incoming; 
}

// -----------------------------------------------------------------------------
// --SECTION--                                         methods of IndexRangeNode
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for IndexRangeNode
////////////////////////////////////////////////////////////////////////////////

void IndexRangeNode::toJsonHelper (triagens::basics::Json& nodes,
                                   TRI_memory_zone_t* zone,
                                   bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));
  // call base class method

  if (json.isEmpty()) {
    return;
  }

  // put together the range info . . .
  triagens::basics::Json ranges(triagens::basics::Json::Array, _ranges.size());

  for (auto x : _ranges) {
    triagens::basics::Json range(triagens::basics::Json::Array, x.size());
    for(auto y : x) {
      range.add(y.toJson());
    }
    ranges.add(range);
  }

  // Now put info about vocbase and cid in there
  json("database", triagens::basics::Json(_vocbase->_name))
      ("collection", triagens::basics::Json(_collection->getName()))
      ("outVariable", _outVariable->toJson())
      ("ranges", ranges);
 
  json("index", _index->toJson()); 
  json("reverse", triagens::basics::Json(_reverse));

  // And add it:
  nodes(json);
}

ExecutionNode* IndexRangeNode::clone (ExecutionPlan* plan,
                                      bool withDependencies,
                                      bool withProperties) const {
  std::vector<std::vector<RangeInfo>> ranges;
  for (size_t i = 0; i < _ranges.size(); i++){
    ranges.push_back(std::vector<RangeInfo>());
    
    for (auto x: _ranges.at(i)){
      ranges.at(i).push_back(x);
    }
  }

  auto outVariable = _outVariable;

  if (withProperties) {
    outVariable = plan->getAst()->variables()->createVariable(outVariable);
  }

  auto c = new IndexRangeNode(plan, _id, _vocbase, _collection, 
                              outVariable, _index, ranges, _reverse);

  CloneHelper(c, plan, withDependencies, withProperties);

  return static_cast<ExecutionNode*>(c);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor for IndexRangeNode from Json
////////////////////////////////////////////////////////////////////////////////

IndexRangeNode::IndexRangeNode (ExecutionPlan* plan,
                                triagens::basics::Json const& json)
  : ExecutionNode(plan, json),
    _vocbase(plan->getAst()->query()->vocbase()),
    _collection(plan->getAst()->query()->collections()->get(JsonHelper::checkAndGetStringValue(json.json(), "collection"))),
    _outVariable(varFromJson(plan->getAst(), json, "outVariable")),
    _index(nullptr), 
    _ranges(),
    _reverse(false) {

  triagens::basics::Json rangeArrayJson(TRI_UNKNOWN_MEM_ZONE, JsonHelper::checkAndGetArrayValue(json.json(), "ranges"));

  for (size_t i = 0; i < rangeArrayJson.size(); i++) { //loop over the ranges . . .
    _ranges.emplace_back();

    triagens::basics::Json rangeJson(rangeArrayJson.at(static_cast<int>(i)));
    for (size_t j = 0; j < rangeJson.size(); j++) {
      _ranges.at(i).emplace_back(rangeJson.at(static_cast<int>(j)));
    }
  }

  // now the index . . . 
  // TODO the following could be a constructor method for
  // an Index object when these are actually used
  auto index = JsonHelper::checkAndGetObjectValue(json.json(), "index");
  auto iid   = JsonHelper::checkAndGetStringValue(index, "id");

  _index = _collection->getIndex(iid);
  _reverse = JsonHelper::checkAndGetBooleanValue(json.json(), "reverse");

  if (_index == nullptr) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, "index not found");
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief check whether the pattern matches this node's index
////////////////////////////////////////////////////////////////////////////////

ExecutionNode::IndexMatch IndexRangeNode::matchesIndex (IndexMatchVec const& pattern) const {
  return CompareIndex(this, _index, pattern);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief the cost of an index range node is a multiple of the cost of
/// its unique dependency
////////////////////////////////////////////////////////////////////////////////
 
double IndexRangeNode::estimateCost (size_t& nrItems) const { 
  static double const EqualityReductionFactor = 100.0;

  size_t incoming = 0;
  double const dependencyCost = _dependencies.at(0)->getCost(incoming);
  size_t docCount = _collection->count();

  TRI_ASSERT(! _ranges.empty());
  
  if (_index->type == TRI_IDX_TYPE_PRIMARY_INDEX) {
    // always an equality lookup

    // selectivity of primary index is always 1
    nrItems = incoming * _ranges.size();
    return dependencyCost + nrItems;
  }
  
  if (_index->type == TRI_IDX_TYPE_EDGE_INDEX) {
    // always an equality lookup
    
    // check if the index can provide a selectivity estimate
    if (! estimateItemsWithIndexSelectivity(incoming, nrItems)) {
      // use hard-coded heuristic
      nrItems = incoming * _ranges.size() * docCount / static_cast<size_t>(EqualityReductionFactor);
    }
        
    nrItems = (std::max)(nrItems, static_cast<size_t>(1));

    return dependencyCost + nrItems;
  }

  if (_index->type == TRI_IDX_TYPE_HASH_INDEX) {
    // always an equality lookup

    // check if the index can provide a selectivity estimate
    if (! estimateItemsWithIndexSelectivity(incoming, nrItems)) {
      // use hard-coded heuristic
      if (_index->unique) {
        nrItems = incoming * _ranges.size();
      }
      else {
        double cost = static_cast<double>(docCount) * incoming * _ranges.size();
        // the more attributes are contained in the index, the more specific the lookup will be
        for (size_t i = 0; i < _ranges.at(0).size(); ++i) { 
          cost /= EqualityReductionFactor; 
        }
    
        nrItems = static_cast<size_t>(cost);
      }
    }
        
    nrItems = (std::max)(nrItems, static_cast<size_t>(1));
    // the more attributes an index matches, the better it is
    double matchLengthFactor = _ranges.at(0).size() * 0.01;

    // this is to prefer the hash index over skiplists if everything else is equal
    return dependencyCost + ((static_cast<double>(nrItems) - matchLengthFactor) * 0.9999995);
  }

  if (_index->type == TRI_IDX_TYPE_SKIPLIST_INDEX) {
    auto const count = _ranges.at(0).size();
    
    if (count == 0) {
      // no ranges? so this is unlimited -> has to be more expensive
      nrItems = incoming * docCount;
      return dependencyCost + nrItems;
    }

    if (_index->unique) {
      bool allEquality = true;
      for (auto const& x : _ranges) {
        // check if we are using all indexed attributes in the query
        if (x.size() != _index->fields.size()) {
          allEquality = false;
          break;
        }

        // check if this is an equality comparison
        if (x.empty() || ! x.back().is1ValueRangeInfo()) {
          allEquality = false;
          break;
        }
      }

      if (allEquality) {
        // unique index, all attributes compared using eq (==) operator
        nrItems = incoming * _ranges.size();
        return dependencyCost + nrItems;
      }
    }

    // build a total cost for the index usage by peeking into all ranges
    double totalCost = 0.0;

    for (auto const& x : _ranges) {
      double cost = static_cast<double>(docCount) * incoming;

      for (auto const& y : x) { //only doing the 1-d case so far
        if (y.is1ValueRangeInfo()) {
          // equality lookup
          cost /= EqualityReductionFactor;
          continue;
        }

        bool hasLowerBound = false;
        bool hasUpperBound = false;

        if (y._lowConst.isDefined() || y._lows.size() > 0) {
          hasLowerBound = true;
        }
        if (y._highConst.isDefined() || y._highs.size() > 0) {
          hasUpperBound = true;
        }

        if (hasLowerBound && hasUpperBound) {
          // both lower and upper bounds defined
          cost /= 10.0;
        }
        else if (hasLowerBound || hasUpperBound) {
          // either only low or high bound defined
          cost /= 2.0;
        }

        // each bound (const and dynamic) counts!
        size_t const numBounds = y._lows.size() + 
                                 y._highs.size() + 
                                 (y._lowConst.isDefined() ? 1 : 0) + 
                                 (y._highConst.isDefined() ? 1 : 0);

        for (size_t j = 0; j < numBounds; ++j) {
          // each dynamic bound again reduces the cost
          cost *= 0.95;
        }
      }

      totalCost += cost;
    }

    totalCost = static_cast<double>((std::max)(static_cast<size_t>(totalCost), static_cast<size_t>(1))); 

    nrItems = static_cast<size_t>(totalCost);

    return dependencyCost + totalCost;
  }

  // no index
  nrItems = incoming * docCount;
  return dependencyCost + nrItems;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief getVariablesUsedHere
////////////////////////////////////////////////////////////////////////////////

std::vector<Variable const*> IndexRangeNode::getVariablesUsedHere () const {
  std::vector<Variable const*> v;
  std::unordered_set<Variable const*> s;

  for (auto const& x : _ranges) {
    for (RangeInfo const& y : x) {
      auto inserter = [&] (RangeInfoBound const& b) -> void {
        AstNode const* a = b.getExpressionAst(_plan->getAst());
        std::unordered_set<Variable*> vars
            = Ast::getReferencedVariables(a);
        for (auto vv : vars) {
          s.insert(vv);
        }
      };

      for (RangeInfoBound const& z : y._lows) {
        inserter(z);
      }
      for (RangeInfoBound const& z : y._highs) {
        inserter(z);
      }
    }
  }

  // Copy set elements into vector:
  v.reserve(s.size());

  for (auto vv : s) {
    v.push_back(vv);
  }
  return v;
}

// -----------------------------------------------------------------------------
// --SECTION--                                                   private methods
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief provide an estimate for the number of items, using the index
/// selectivity info (if present)
////////////////////////////////////////////////////////////////////////////////

bool IndexRangeNode::estimateItemsWithIndexSelectivity (size_t incoming,
                                                        size_t& nrItems) const {
  // check if the index can provide a selectivity estimate
  if (! _index->hasSelectivityEstimate()) {
    return false; 
  }

  // use index selectivity estimate
  double estimate = _index->selectivityEstimate();

  if (estimate <= 0.0) {
    // avoid DIV0
    return false;
  }

  nrItems = static_cast<size_t>(incoming * _ranges.size() * (1.0 / estimate));
  return true;
}

// -----------------------------------------------------------------------------
// --SECTION--                                              methods of LimitNode
// -----------------------------------------------------------------------------

LimitNode::LimitNode (ExecutionPlan* plan, 
                      triagens::basics::Json const& base)
  : ExecutionNode(plan, base),
    _offset(JsonHelper::checkAndGetNumericValue<decltype(_offset)>(base.json(), "offset")),
    _limit(JsonHelper::checkAndGetNumericValue<decltype(_limit)>(base.json(), "limit")),
    _fullCount(JsonHelper::checkAndGetBooleanValue(base.json(), "fullCount")) {

}

////////////////////////////////////////////////////////////////////////////////
// @brief toJson, for LimitNode
////////////////////////////////////////////////////////////////////////////////

void LimitNode::toJsonHelper (triagens::basics::Json& nodes,
                              TRI_memory_zone_t* zone,
                              bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method
  if (json.isEmpty()) {
    return;
  }
  // Now put info about offset and limit in
  json("offset", triagens::basics::Json(static_cast<double>(_offset)))
      ("limit", triagens::basics::Json(static_cast<double>(_limit)))
      ("fullCount", triagens::basics::Json(_fullCount));

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimateCost
////////////////////////////////////////////////////////////////////////////////
        
double LimitNode::estimateCost (size_t& nrItems) const {
  size_t incoming = 0;
  double depCost = _dependencies.at(0)->getCost(incoming);
  nrItems = (std::min)(_limit, (std::max)(static_cast<size_t>(0), 
                                          incoming - _offset));
  return depCost + nrItems;
}

// -----------------------------------------------------------------------------
// --SECTION--                                        methods of CalculationNode
// -----------------------------------------------------------------------------

CalculationNode::CalculationNode (ExecutionPlan* plan,
                                  triagens::basics::Json const& base)
  : ExecutionNode(plan, base),
    _conditionVariable(varFromJson(plan->getAst(), base, "conditionVariable", true)),
    _outVariable(varFromJson(plan->getAst(), base, "outVariable")),
    _expression(new Expression(plan->getAst(), base)) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for CalculationNode
////////////////////////////////////////////////////////////////////////////////

void CalculationNode::toJsonHelper (triagens::basics::Json& nodes,
                                    TRI_memory_zone_t* zone,
                                    bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method

  if (json.isEmpty()) {
    return;
  }
  
  json("expression", _expression->toJson(TRI_UNKNOWN_MEM_ZONE, verbose))
      ("outVariable", _outVariable->toJson())
      ("canThrow", triagens::basics::Json(_expression->canThrow()));

  if (_conditionVariable != nullptr) {
    json("conditionVariable", _conditionVariable->toJson());
  }

  // And add it:
  nodes(json);
}

ExecutionNode* CalculationNode::clone (ExecutionPlan* plan,
                                       bool withDependencies,
                                       bool withProperties) const {
  auto conditionVariable = _conditionVariable;
  auto outVariable = _outVariable;

  if (withProperties) {
    if (_conditionVariable != nullptr) {
      conditionVariable = plan->getAst()->variables()->createVariable(conditionVariable);
    }
    outVariable = plan->getAst()->variables()->createVariable(outVariable);
  }

  auto c = new CalculationNode(plan, _id, _expression->clone(), conditionVariable, outVariable);

  CloneHelper(c, plan, withDependencies, withProperties);

  return static_cast<ExecutionNode*>(c);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimateCost
////////////////////////////////////////////////////////////////////////////////
        
double CalculationNode::estimateCost (size_t& nrItems) const {
  TRI_ASSERT(! _dependencies.empty());
  double depCost = _dependencies.at(0)->getCost(nrItems);
  return depCost + nrItems;
}

// -----------------------------------------------------------------------------
// --SECTION--                                           methods of SubqueryNode
// -----------------------------------------------------------------------------

SubqueryNode::SubqueryNode (ExecutionPlan* plan,
                            triagens::basics::Json const& base)
  : ExecutionNode(plan, base),
    _subquery(nullptr),
    _outVariable(varFromJson(plan->getAst(), base, "outVariable")) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for SubqueryNode
////////////////////////////////////////////////////////////////////////////////

void SubqueryNode::toJsonHelper (triagens::basics::Json& nodes,
                                 TRI_memory_zone_t* zone,
                                 bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method
  if (json.isEmpty()) {
    return;
  }
  json("subquery",  _subquery->toJson(TRI_UNKNOWN_MEM_ZONE, verbose))
      ("outVariable", _outVariable->toJson());

  // And add it:
  nodes(json);
}

ExecutionNode* SubqueryNode::clone (ExecutionPlan* plan,
                                    bool withDependencies,
                                    bool withProperties) const {
  auto outVariable = _outVariable;

  if (withProperties) {
    outVariable = plan->getAst()->variables()->createVariable(outVariable);
  }
  auto c = new SubqueryNode(plan, _id, _subquery->clone(plan, true, withProperties),
                            outVariable);

  CloneHelper(c, plan, withDependencies, withProperties);

  return static_cast<ExecutionNode*>(c);
}


void SubqueryNode::replaceOutVariable(Variable const* var) {
  _outVariable = var;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimateCost
////////////////////////////////////////////////////////////////////////////////
        
double SubqueryNode::estimateCost (size_t& nrItems) const {
  double depCost = _dependencies.at(0)->getCost(nrItems);
  size_t nrItemsSubquery;
  double subCost = _subquery->getCost(nrItemsSubquery);
  return depCost + nrItems * subCost;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief helper struct to find all (outer) variables used in a SubqueryNode
////////////////////////////////////////////////////////////////////////////////

struct SubqueryVarUsageFinder : public WalkerWorker<ExecutionNode> {
  std::unordered_set<Variable const*> _usedLater;
  std::unordered_set<Variable const*> _valid;

  SubqueryVarUsageFinder () {
  }

  ~SubqueryVarUsageFinder () {
  }

  bool before (ExecutionNode* en) override final {
    // Add variables used here to _usedLater:
    auto&& usedHere = en->getVariablesUsedHere();
    for (auto v : usedHere) {
      _usedLater.insert(v);
    }
    return false;
  }

  void after (ExecutionNode* en) override final {
    // Add variables set here to _valid:
    auto&& setHere = en->getVariablesSetHere();
    for (auto v : setHere) {
      _valid.insert(v);
    }
  }

  bool enterSubquery (ExecutionNode*, ExecutionNode* sub) override final {
    SubqueryVarUsageFinder subfinder;
    sub->walk(&subfinder);

    // keep track of all variables used by a (dependent) subquery
    // this is, all variables in the subqueries _usedLater that are not in _valid
     
    // create the set difference. note: cannot use std::set_difference as our sets are NOT sorted
    for (auto it = subfinder._usedLater.begin(); it != subfinder._usedLater.end(); ++it) {
      if (_valid.find(*it) != _valid.end()) {
        _usedLater.insert((*it));
      }
    }
    return false;
  }
};

////////////////////////////////////////////////////////////////////////////////
/// @brief getVariablesUsedHere
////////////////////////////////////////////////////////////////////////////////

std::vector<Variable const*> SubqueryNode::getVariablesUsedHere () const {
  SubqueryVarUsageFinder finder;
  _subquery->walk(&finder);
      
  std::vector<Variable const*> v;
  for (auto it = finder._usedLater.begin(); it != finder._usedLater.end(); ++it) {
    if (finder._valid.find(*it) == finder._valid.end()) {
      v.push_back((*it));
    }
  }
  return v;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief can the node throw? We have to find whether any node in the 
/// subquery plan can throw.
////////////////////////////////////////////////////////////////////////////////

struct CanThrowFinder : public WalkerWorker<ExecutionNode> {
  bool _canThrow;

  CanThrowFinder () 
    : _canThrow(false) {
  }

  ~CanThrowFinder () {
  }

  bool enterSubquery (ExecutionNode*, ExecutionNode*) override final {
    return false;
  }

  bool before (ExecutionNode* node) override final {
    if (node->canThrow()) {
      _canThrow = true;
      return true;
    }
    return false;
  }

};

bool SubqueryNode::canThrow () {
  CanThrowFinder finder;
  _subquery->walk(&finder);
  return finder._canThrow;
}

// -----------------------------------------------------------------------------
// --SECTION--                                             methods of FilterNode
// -----------------------------------------------------------------------------

FilterNode::FilterNode (ExecutionPlan* plan, 
                        triagens::basics::Json const& base)
  : ExecutionNode(plan, base),
    _inVariable(varFromJson(plan->getAst(), base, "inVariable")) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for FilterNode
////////////////////////////////////////////////////////////////////////////////

void FilterNode::toJsonHelper (triagens::basics::Json& nodes,
                               TRI_memory_zone_t* zone,
                               bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method
  if (json.isEmpty()) {
    return;
  }
  
  json("inVariable", _inVariable->toJson());

  // And add it:
  nodes(json);
}

ExecutionNode* FilterNode::clone (ExecutionPlan* plan,
                                  bool withDependencies,
                                  bool withProperties) const {
  auto inVariable = _inVariable;

  if (withProperties) {
    inVariable = plan->getAst()->variables()->createVariable(inVariable);
  }
  auto c = new FilterNode(plan, _id, inVariable);

  CloneHelper(c, plan, withDependencies, withProperties);

  return static_cast<ExecutionNode*>(c);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimateCost
////////////////////////////////////////////////////////////////////////////////
        
double FilterNode::estimateCost (size_t& nrItems) const {
  double depCost = _dependencies.at(0)->getCost(nrItems);
  // We are pessimistic here by not reducing the nrItems. However, in the
  // worst case the filter does not reduce the items at all. Furthermore,
  // no optimiser rule introduces FilterNodes, thus it is not important
  // that they appear to lower the costs. Note that contrary to this,
  // an IndexRangeNode does lower the costs, it also has a better idea
  // to what extent the number of items is reduced. On the other hand it
  // is important that a FilterNode produces additional costs, otherwise
  // the rule throwing away a FilterNode that is already covered by an
  // IndexRangeNode cannot reduce the costs.
  return depCost + nrItems;
}

// -----------------------------------------------------------------------------
// --SECTION--                                               methods of SortNode
// -----------------------------------------------------------------------------

SortNode::SortNode (ExecutionPlan* plan,
                    triagens::basics::Json const& base,
                    SortElementVector const& elements,
                    bool stable)
  : ExecutionNode(plan, base),
    _elements(elements),
    _stable(stable) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for SortNode
////////////////////////////////////////////////////////////////////////////////

void SortNode::toJsonHelper (triagens::basics::Json& nodes,
                             TRI_memory_zone_t* zone,
                             bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method

  if (json.isEmpty()) {
    return;
  }
  triagens::basics::Json values(triagens::basics::Json::Array, _elements.size());
  for (auto it = _elements.begin(); it != _elements.end(); ++it) {
    triagens::basics::Json element(triagens::basics::Json::Object);
    element("inVariable", (*it).first->toJson())
      ("ascending", triagens::basics::Json((*it).second));
    values(element);
  }
  json("elements", values);
  json("stable", triagens::basics::Json(_stable));

  // And add it:
  nodes(json);
}

class SortNodeFindMyExpressions : public WalkerWorker<ExecutionNode> {

public:
  size_t _foundCalcNodes;
  SortElementVector _elms;
  std::vector<std::pair<ExecutionNode*, bool>> _myVars;

  SortNodeFindMyExpressions(SortNode* me)
    : _foundCalcNodes(0),
      _elms(me->getElements()) {
    _myVars.resize(_elms.size());
  }

  bool before (ExecutionNode* en) override final {
    auto vars = en->getVariablesSetHere();
    for (auto v : vars) {
      for (size_t n = 0; n < _elms.size(); n++) {
        if (_elms[n].first->id == v->id) {
          _myVars[n] = std::make_pair(en, _elms[n].second);
          _foundCalcNodes ++;
          break;
        }
      }
    }
    return _foundCalcNodes >= _elms.size();
  }
};

std::vector<std::pair<ExecutionNode*, bool>> SortNode::getCalcNodePairs () {
  SortNodeFindMyExpressions findExp(this);
  _dependencies[0]->walk(&findExp);
  if (findExp._foundCalcNodes < _elements.size()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL,
                                   "SortNode wasn't able to locate all its CalculationNodes");
  }
  return findExp._myVars;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief returns all sort information 
////////////////////////////////////////////////////////////////////////////////

SortInformation SortNode::getSortInformation (ExecutionPlan* plan,
                                              triagens::basics::StringBuffer* buffer) const {
  SortInformation result;

  auto elements = getElements();
  for (auto it = elements.begin(); it != elements.end(); ++it) {
    auto variable = (*it).first;
    TRI_ASSERT(variable != nullptr);
    auto setter = _plan->getVarSetBy(variable->id);
      
    if (setter == nullptr) {
      result.isValid = false;
      break;
    }
      
    if (! result.canThrow && setter->canThrow()) {
      result.canThrow = true;
    }

    if (setter->getType() == ExecutionNode::CALCULATION) {
      // variable introduced by a calculation
      auto expression = static_cast<CalculationNode*>(setter)->expression();

      if (! expression->isDeterministic()) {
        result.isDeterministic = false;
      }

      if (! expression->isAttributeAccess() &&
          ! expression->isReference()) {
        result.isComplex = true;
        break;
      }

      expression->stringify(buffer);
      result.criteria.emplace_back(std::make_tuple(setter, buffer->c_str(), (*it).second));
      buffer->reset();
    }
    else {
      // use variable only. note that we cannot use the variable's name as it is not
      // necessarily unique in one query (yes, COLLECT, you are to blame!)
      result.criteria.emplace_back(std::make_tuple(setter, std::to_string(variable->id), (*it).second));
    }
  }

  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimateCost
////////////////////////////////////////////////////////////////////////////////
        
double SortNode::estimateCost (size_t& nrItems) const {
  double depCost = _dependencies.at(0)->getCost(nrItems);
  if (nrItems <= 3.0) {
    return depCost + nrItems;
  }
  return depCost + nrItems * log(nrItems);
}

// -----------------------------------------------------------------------------
// --SECTION--                                          methods of AggregateNode
// -----------------------------------------------------------------------------

AggregateNode::AggregateNode (ExecutionPlan* plan,
                              triagens::basics::Json const& base,
                              Variable const* expressionVariable,
                              Variable const* outVariable,
                              std::vector<Variable const*> const& keepVariables,
                              std::unordered_map<VariableId, std::string const> const& variableMap,
                              std::vector<std::pair<Variable const*, Variable const*>> const& aggregateVariables,
                              bool countOnly)
  : ExecutionNode(plan, base),
    _aggregateVariables(aggregateVariables), 
    _expressionVariable(expressionVariable),
    _outVariable(outVariable),
    _keepVariables(keepVariables),
    _variableMap(variableMap),
    _countOnly(countOnly) {

}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for AggregateNode
////////////////////////////////////////////////////////////////////////////////

void AggregateNode::toJsonHelper (triagens::basics::Json& nodes,
                                  TRI_memory_zone_t* zone,
                                  bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method

  if (json.isEmpty()) {
    return;
  }

  triagens::basics::Json values(triagens::basics::Json::Array, _aggregateVariables.size());

  for (auto it = _aggregateVariables.begin(); it != _aggregateVariables.end(); ++it) {
    triagens::basics::Json variable(triagens::basics::Json::Object);
    variable("outVariable", (*it).first->toJson())
            ("inVariable", (*it).second->toJson());
    values(variable);
  }
  json("aggregates", values);

  // expression variable might be empty
  if (_expressionVariable != nullptr) {
    json("expressionVariable", _expressionVariable->toJson());
  }

  // output variable might be empty
  if (_outVariable != nullptr) {
    json("outVariable", _outVariable->toJson());
  }

  if (! _keepVariables.empty()) {
    triagens::basics::Json values(triagens::basics::Json::Array, _keepVariables.size());
    for (auto it = _keepVariables.begin(); it != _keepVariables.end(); ++it) {
      triagens::basics::Json variable(triagens::basics::Json::Object);
      variable("variable", (*it)->toJson());
      values(variable);
    }
    json("keepVariables", values);
  }

  json("count", triagens::basics::Json(_countOnly));

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief clone ExecutionNode recursively
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* AggregateNode::clone (ExecutionPlan* plan,
                                     bool withDependencies,
                                     bool withProperties) const {
  auto outVariable = _outVariable;
  auto expressionVariable = _expressionVariable;
  auto aggregateVariables = _aggregateVariables;

  if (withProperties) {
    if (expressionVariable != nullptr) {
      expressionVariable = plan->getAst()->variables()->createVariable(expressionVariable);
    }

    if (outVariable != nullptr) {
      outVariable = plan->getAst()->variables()->createVariable(outVariable);
    }

    for (auto oneAggregate: _aggregateVariables) {
      auto in  = plan->getAst()->variables()->createVariable(oneAggregate.first);
      auto out = plan->getAst()->variables()->createVariable(oneAggregate.second);
      aggregateVariables.emplace_back(std::make_pair(in, out));
    }

  }

  auto c = new AggregateNode(plan, 
                             _id, 
                             aggregateVariables, 
                             expressionVariable, 
                             outVariable, 
                             _keepVariables, 
                             _variableMap, 
                             _countOnly);

  CloneHelper(c, plan, withDependencies, withProperties);

  return static_cast<ExecutionNode*>(c);
}


////////////////////////////////////////////////////////////////////////////////
/// @brief getVariablesUsedHere
////////////////////////////////////////////////////////////////////////////////

struct UserVarFinder : public WalkerWorker<ExecutionNode> {
  UserVarFinder (int mindepth) : mindepth(mindepth), depth(-1) {};
  ~UserVarFinder () {};
  std::vector<Variable const*> userVars;
  int mindepth;   // minimal depth to consider
  int depth;

  bool enterSubquery (ExecutionNode*, ExecutionNode*) override final {
    return false;
  }

  void after (ExecutionNode* en) override final {
    if (en->getType() == ExecutionNode::SINGLETON) {
      depth = 0;
    }
    else if (en->getType() == ExecutionNode::ENUMERATE_COLLECTION ||
             en->getType() == ExecutionNode::INDEX_RANGE ||
             en->getType() == ExecutionNode::ENUMERATE_LIST ||
             en->getType() == ExecutionNode::AGGREGATE) {
      depth += 1;
    }
    // Now depth is set correct for this node.
    if (depth >= mindepth) {
      auto vars = en->getVariablesSetHere();
      for (auto v : vars) {
        if (v->isUserDefined()) {
          userVars.push_back(v);
        }
      }
    }
  }
};

std::vector<Variable const*> AggregateNode::getVariablesUsedHere () const {
  std::unordered_set<Variable const*> v;
  for (auto p : _aggregateVariables) {
    v.insert(p.second);
  }

  if (_expressionVariable != nullptr) {
    v.insert(_expressionVariable);
  }

  if (_outVariable != nullptr && ! _countOnly) {
    if (_keepVariables.empty()) {
      // Here we have to find all user defined variables in this query
      // amongst our dependencies:
      UserVarFinder finder(1);
      auto myselfAsNonConst = const_cast<AggregateNode*>(this);
      myselfAsNonConst->walk(&finder);
      if (finder.depth == 1) {
        // we are toplevel, let's run again with mindepth = 0
        finder.userVars.clear();
        finder.mindepth = 0;
        finder.depth = -1;
        finder.reset();
        myselfAsNonConst->walk(&finder);
      }
      for (auto x : finder.userVars) {
        v.insert(x);
      }
    }
    else {
      for (auto x : _keepVariables) {
        v.insert(x);
      }
    }
  }

  std::vector<Variable const*> vv;
  vv.reserve(v.size());
  for (auto x : v) {
    vv.push_back(x);
  }
  return vv;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimateCost
////////////////////////////////////////////////////////////////////////////////
        
double AggregateNode::estimateCost (size_t& nrItems) const {
  double depCost = _dependencies.at(0)->getCost(nrItems);

  if (_countOnly && _aggregateVariables.empty()) {
    // we are known to only produce a single output row
    nrItems = 1;
  }

  // As in the FilterNode case, we are pessimistic here by not reducing the
  // nrItems, since this is the worst case. We have to look at all incoming
  // items, and in particular in the COLLECT ... INTO ... case, we have
  // to actually hand on all data anyway, albeit not as separate items.
  // Nevertheless, the optimiser does not do much with AggregateNodes
  // and thus this overestimation does not really matter.
  return depCost + nrItems;
}

// -----------------------------------------------------------------------------
// --SECTION--                                             methods of ReturnNode
// -----------------------------------------------------------------------------

ReturnNode::ReturnNode (ExecutionPlan* plan, 
                        triagens::basics::Json const& base)
  : ExecutionNode(plan, base),
    _inVariable(varFromJson(plan->getAst(), base, "inVariable")) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for ReturnNode
////////////////////////////////////////////////////////////////////////////////

void ReturnNode::toJsonHelper (triagens::basics::Json& nodes,
                               TRI_memory_zone_t* zone,
                               bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method

  if (json.isEmpty()) {
    return;
  }
      
  json("inVariable", _inVariable->toJson());

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief clone ExecutionNode recursively
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ReturnNode::clone (ExecutionPlan* plan,
                                  bool withDependencies,
                                  bool withProperties) const {
  auto inVariable = _inVariable;

  if (withProperties) {
    inVariable = plan->getAst()->variables()->createVariable(inVariable);
  }

  auto c = new ReturnNode(plan, _id, inVariable);

  CloneHelper(c, plan, withDependencies, withProperties);

  return static_cast<ExecutionNode*>(c);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimateCost
////////////////////////////////////////////////////////////////////////////////
        
double ReturnNode::estimateCost (size_t& nrItems) const {
  double depCost = _dependencies.at(0)->getCost(nrItems);
  return depCost + nrItems;
}

// -----------------------------------------------------------------------------
// --SECTION--                                                  ModificationNode
// -----------------------------------------------------------------------------

ModificationNode::ModificationNode (ExecutionPlan* plan,
                                    triagens::basics::Json const& base)
  : ExecutionNode(plan, base), 
    _vocbase(plan->getAst()->query()->vocbase()),
    _collection(plan->getAst()->query()->collections()->get(JsonHelper::checkAndGetStringValue(base.json(), "collection"))),
    _options(base),
    _outVariable(varFromJson(plan->getAst(), base, "outVariable", Optional)) {
  TRI_ASSERT(_vocbase != nullptr);
  TRI_ASSERT(_collection != nullptr);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson
////////////////////////////////////////////////////////////////////////////////

void ModificationNode::toJsonHelper (triagens::basics::Json& json,
                                     TRI_memory_zone_t* zone,
                                     bool) const {

  // Now put info about vocbase and cid in there
  json("database", triagens::basics::Json(_vocbase->_name))
      ("collection", triagens::basics::Json(_collection->getName()));

  _options.toJson(json, zone);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimateCost
/// Note that all the modifying nodes use this estimateCost method which is
/// why we can make it final here.
////////////////////////////////////////////////////////////////////////////////
        
double ModificationNode::estimateCost (size_t& nrItems) const {
  size_t incoming = 0;
  double depCost = _dependencies.at(0)->getCost(incoming);
  nrItems = 0;
  return depCost + incoming;
}


// -----------------------------------------------------------------------------
// --SECTION--                                             methods of RemoveNode
// -----------------------------------------------------------------------------

RemoveNode::RemoveNode (ExecutionPlan* plan, 
                        triagens::basics::Json const& base)
  : ModificationNode(plan, base),
    _inVariable(varFromJson(plan->getAst(), base, "inVariable")) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson
////////////////////////////////////////////////////////////////////////////////

void RemoveNode::toJsonHelper (triagens::basics::Json& nodes,
                               TRI_memory_zone_t* zone,
                               bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method

  if (json.isEmpty()) {
    return;
  }

  // Now put info about vocbase and cid in there
  json("inVariable", _inVariable->toJson());
  
  ModificationNode::toJsonHelper(json, zone, verbose);

  // output variable might be empty
  if (_outVariable != nullptr) {
    json("outVariable", _outVariable->toJson());
  }

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief clone ExecutionNode recursively
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* RemoveNode::clone (ExecutionPlan* plan,
                                  bool withDependencies,
                                  bool withProperties) const {
  auto outVariable = _outVariable;
  auto inVariable = _inVariable;

  if (withProperties) {
    if (_outVariable != nullptr) {
      outVariable = plan->getAst()->variables()->createVariable(outVariable);
    }
    inVariable = plan->getAst()->variables()->createVariable(inVariable);
  }

  auto c = new RemoveNode(plan, _id, _vocbase, _collection, _options, inVariable, outVariable);

  CloneHelper(c, plan, withDependencies, withProperties);

  return static_cast<ExecutionNode*>(c);
}

// -----------------------------------------------------------------------------
// --SECTION--                                             methods of InsertNode
// -----------------------------------------------------------------------------

InsertNode::InsertNode (ExecutionPlan* plan, 
                        triagens::basics::Json const& base)
  : ModificationNode(plan, base),
    _inVariable(varFromJson(plan->getAst(), base, "inVariable")) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson
////////////////////////////////////////////////////////////////////////////////

void InsertNode::toJsonHelper (triagens::basics::Json& nodes,
                               TRI_memory_zone_t* zone,
                               bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method

  if (json.isEmpty()) {
    return;
  }

  // Now put info about vocbase and cid in there
  json("inVariable", _inVariable->toJson()); 

  ModificationNode::toJsonHelper(json, zone, verbose);

  // output variable might be empty
  if (_outVariable != nullptr) {
    json("outVariable", _outVariable->toJson());
  }

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief clone ExecutionNode recursively
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* InsertNode::clone (ExecutionPlan* plan,
                                  bool withDependencies,
                                  bool withProperties) const {
  auto outVariable = _outVariable;
  auto inVariable = _inVariable;

  if (withProperties) {
    if (_outVariable != nullptr) {
      outVariable = plan->getAst()->variables()->createVariable(outVariable);
    }
    inVariable = plan->getAst()->variables()->createVariable(inVariable);
  }

  auto c = new InsertNode(plan, _id, _vocbase, _collection,
                          _options, inVariable, outVariable);

  CloneHelper(c, plan, withDependencies, withProperties);

  return static_cast<ExecutionNode*>(c);
}


// -----------------------------------------------------------------------------
// --SECTION--                                             methods of UpdateNode
// -----------------------------------------------------------------------------

UpdateNode::UpdateNode (ExecutionPlan* plan, 
                        triagens::basics::Json const& base)
  : ModificationNode(plan, base),
    _inDocVariable(varFromJson(plan->getAst(), base, "inDocVariable")),
    _inKeyVariable(varFromJson(plan->getAst(), base, "inKeyVariable", Optional)) {

  if (_outVariable != nullptr) {
    _returnNewValues = JsonHelper::checkAndGetBooleanValue(base.json(), "returnNewValues");
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson
////////////////////////////////////////////////////////////////////////////////

void UpdateNode::toJsonHelper (triagens::basics::Json& nodes,
                               TRI_memory_zone_t* zone,
                               bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method

  if (json.isEmpty()) {
    return;
  }

  // Now put info about vocbase and cid in there
  json("inDocVariable", _inDocVariable->toJson());
  
  ModificationNode::toJsonHelper(json, zone, verbose);

  // inKeyVariable might be empty
  if (_inKeyVariable != nullptr) {
    json("inKeyVariable", _inKeyVariable->toJson());
  }
  
  // output variable might be empty
  if (_outVariable != nullptr) {
    json("outVariable", _outVariable->toJson());
    json("returnNewValues", triagens::basics::Json(_returnNewValues));
  }

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief clone ExecutionNode recursively
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* UpdateNode::clone (ExecutionPlan* plan,
                                  bool withDependencies,
                                  bool withProperties) const {
  auto outVariable = _outVariable;
  auto inKeyVariable = _inKeyVariable;
  auto inDocVariable = _inDocVariable;

  if (withProperties) {
    if (_outVariable != nullptr) {
      outVariable = plan->getAst()->variables()->createVariable(outVariable);
    }
    if (inKeyVariable != nullptr) {
      inKeyVariable = plan->getAst()->variables()->createVariable(inKeyVariable);
    }
    inDocVariable = plan->getAst()->variables()->createVariable(inDocVariable);
  }

  auto c = new UpdateNode(plan, _id, _vocbase, _collection, _options, inDocVariable, inKeyVariable, outVariable, _returnNewValues);

  CloneHelper(c, plan, withDependencies, withProperties);

  return static_cast<ExecutionNode*>(c);
}

// -----------------------------------------------------------------------------
// --SECTION--                                            methods of ReplaceNode
// -----------------------------------------------------------------------------

ReplaceNode::ReplaceNode (ExecutionPlan* plan, 
                          triagens::basics::Json const& base)
  : ModificationNode(plan, base),
    _inDocVariable(varFromJson(plan->getAst(), base, "inDocVariable")),
    _inKeyVariable(varFromJson(plan->getAst(), base, "inKeyVariable", Optional)) {
  if (_outVariable != nullptr) {
    _returnNewValues = JsonHelper::checkAndGetBooleanValue(base.json(), "returnNewValues");
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson
////////////////////////////////////////////////////////////////////////////////

void ReplaceNode::toJsonHelper (triagens::basics::Json& nodes,
                                TRI_memory_zone_t* zone,
                                bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method

  if (json.isEmpty()) {
    return;
  }

  // Now put info about vocbase and cid in there
  json("inDocVariable", _inDocVariable->toJson());

  ModificationNode::toJsonHelper(json, zone, verbose);

  // inKeyVariable might be empty
  if (_inKeyVariable != nullptr) {
    json("inKeyVariable", _inKeyVariable->toJson());
  }
  
  // output variable might be empty
  if (_outVariable != nullptr) {
    json("outVariable", _outVariable->toJson());
    json("returnNewValues", triagens::basics::Json(_returnNewValues));
  }

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief clone ExecutionNode recursively
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ReplaceNode::clone (ExecutionPlan* plan,
                                   bool withDependencies,
                                   bool withProperties) const {
  auto outVariable = _outVariable;
  auto inKeyVariable = _inKeyVariable;
  auto inDocVariable = _inDocVariable;

  if (withProperties) {
    if (_outVariable != nullptr) {
      outVariable = plan->getAst()->variables()->createVariable(outVariable);
    }
    if (inKeyVariable != nullptr) {
      inKeyVariable = plan->getAst()->variables()->createVariable(inKeyVariable);
    }
    inDocVariable = plan->getAst()->variables()->createVariable(inDocVariable);
  }

  auto c = new ReplaceNode(plan, _id, _vocbase, _collection, 
                           _options, inDocVariable, inKeyVariable,
                           outVariable, _returnNewValues);

  CloneHelper(c, plan, withDependencies, withProperties);

  return static_cast<ExecutionNode*>(c);
}

// -----------------------------------------------------------------------------
// --SECTION--                                          methods of NoResultsNode
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for NoResultsNode
////////////////////////////////////////////////////////////////////////////////

void NoResultsNode::toJsonHelper (triagens::basics::Json& nodes,
                                  TRI_memory_zone_t* zone,
                                  bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method
  if (json.isEmpty()) {
    return;
  }

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimateCost, the cost of a NoResults is nearly 0
////////////////////////////////////////////////////////////////////////////////
        
double NoResultsNode::estimateCost (size_t& nrItems) const {
  nrItems = 0;
  return 0.5;  // just to make it non-zero
}

// -----------------------------------------------------------------------------
// --SECTION--                                             methods of RemoteNode
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor for RemoteNode from Json
////////////////////////////////////////////////////////////////////////////////

RemoteNode::RemoteNode (ExecutionPlan* plan,
                        triagens::basics::Json const& base)
  : ExecutionNode(plan, base),
    _vocbase(plan->getAst()->query()->vocbase()),
    _collection(plan->getAst()->query()->collections()->get(JsonHelper::checkAndGetStringValue(base.json(), "collection"))),
    _server(JsonHelper::checkAndGetStringValue(base.json(), "server")), 
    _ownName(JsonHelper::checkAndGetStringValue(base.json(), "ownName")), 
    _queryId(JsonHelper::checkAndGetStringValue(base.json(), "queryId")) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for RemoteNode
////////////////////////////////////////////////////////////////////////////////

void RemoteNode::toJsonHelper (triagens::basics::Json& nodes,
                               TRI_memory_zone_t* zone,
                               bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method
  if (json.isEmpty()) {
    return;
  }
  
  json("database", triagens::basics::Json(_vocbase->_name))
      ("collection", triagens::basics::Json(_collection->getName()))
      ("server", triagens::basics::Json(_server))
      ("ownName", triagens::basics::Json(_ownName))
      ("queryId", triagens::basics::Json(_queryId));

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimateCost
////////////////////////////////////////////////////////////////////////////////
        
double RemoteNode::estimateCost (size_t& nrItems) const {
  double depCost;
  if (_dependencies.size() == 1) {
    // This will usually be the case, however, in the context of the
    // instantiation it is possible that there is no dependency...
    depCost = _dependencies[0]->estimateCost(nrItems);
    return depCost + nrItems;   // we need to process them all
  }
  // We really should not get here, but if so, do something bordering on 
  // sensible:
  nrItems = 1;
  return 1.0;
}

// -----------------------------------------------------------------------------
// --SECTION--                                            methods of ScatterNode
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief construct a scatter node from JSON
////////////////////////////////////////////////////////////////////////////////

ScatterNode::ScatterNode (ExecutionPlan* plan, 
                          triagens::basics::Json const& base)
  : ExecutionNode(plan, base),
    _vocbase(plan->getAst()->query()->vocbase()),
    _collection(plan->getAst()->query()->collections()->get(JsonHelper::checkAndGetStringValue(base.json(), "collection"))) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for ScatterNode
////////////////////////////////////////////////////////////////////////////////

void ScatterNode::toJsonHelper (triagens::basics::Json& nodes,
                                TRI_memory_zone_t* zone,
                                bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method
  if (json.isEmpty()) {
    return;
  }
  
  json("database", triagens::basics::Json(_vocbase->_name))
      ("collection", triagens::basics::Json(_collection->getName()));

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimateCost
////////////////////////////////////////////////////////////////////////////////
        
double ScatterNode::estimateCost (size_t& nrItems) const {
  double depCost = _dependencies[0]->getCost(nrItems);
  std::vector<std::string> shardIds = _collection->shardIds();
  size_t nrShards = shardIds.size();
  return depCost + nrItems * nrShards;
}

// -----------------------------------------------------------------------------
// --SECTION--                                         methods of DistributeNode
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief construct a distribute node from JSON
////////////////////////////////////////////////////////////////////////////////

DistributeNode::DistributeNode (ExecutionPlan* plan, 
                                triagens::basics::Json const& base)
  : ExecutionNode(plan, base),
    _vocbase(plan->getAst()->query()->vocbase()),
    _collection(plan->getAst()->query()->collections()->get(JsonHelper::checkAndGetStringValue(base.json(), "collection"))),
    _varId(JsonHelper::checkAndGetNumericValue<VariableId>(base.json(), "varId")),
    _createKeys(JsonHelper::checkAndGetBooleanValue(base.json(), "createKeys")) {
}

void DistributeNode::toJsonHelper (triagens::basics::Json& nodes,
                                TRI_memory_zone_t* zone,
                                bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone,
        verbose));  // call base class method
  if (json.isEmpty()) {
    return;
  }
  
  json("database", triagens::basics::Json(_vocbase->_name))
      ("collection", triagens::basics::Json(_collection->getName()))
      ("varId", triagens::basics::Json(static_cast<int>(_varId)))
      ("createKeys", triagens::basics::Json(_createKeys));

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimateCost
////////////////////////////////////////////////////////////////////////////////
        
double DistributeNode::estimateCost (size_t& nrItems) const {
  double depCost = _dependencies[0]->getCost(nrItems);
  return depCost + nrItems;
}

// -----------------------------------------------------------------------------
// --SECTION--                                             methods of GatherNode
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief construct a gather node from JSON
////////////////////////////////////////////////////////////////////////////////

GatherNode::GatherNode (ExecutionPlan* plan, 
                        triagens::basics::Json const& base,
                        SortElementVector const& elements)
  : ExecutionNode(plan, base),
    _elements(elements),
    _vocbase(plan->getAst()->query()->vocbase()),
    _collection(plan->getAst()->query()->collections()->get(JsonHelper::checkAndGetStringValue(base.json(), "collection"))) {
}

////////////////////////////////////////////////////////////////////////////////
/// @brief toJson, for GatherNode
////////////////////////////////////////////////////////////////////////////////

void GatherNode::toJsonHelper (triagens::basics::Json& nodes,
                               TRI_memory_zone_t* zone,
                               bool verbose) const {
  triagens::basics::Json json(ExecutionNode::toJsonHelperGeneric(nodes, zone, verbose));  // call base class method
  if (json.isEmpty()) {
    return;
  }
  
  json("database", triagens::basics::Json(_vocbase->_name))
      ("collection", triagens::basics::Json(_collection->getName()));

  triagens::basics::Json values(triagens::basics::Json::Array, _elements.size());
  for (auto it = _elements.begin(); it != _elements.end(); ++it) {
    triagens::basics::Json element(triagens::basics::Json::Object);
    element("inVariable", (*it).first->toJson())
           ("ascending", triagens::basics::Json((*it).second));
    values(element);
  }
  json("elements", values);

  // And add it:
  nodes(json);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief estimateCost
////////////////////////////////////////////////////////////////////////////////
        
double GatherNode::estimateCost (size_t& nrItems) const {
  double depCost = _dependencies[0]->getCost(nrItems);
  return depCost + nrItems;
}

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|// --SECTION--\\|/// @\\}\\)"
// End:

