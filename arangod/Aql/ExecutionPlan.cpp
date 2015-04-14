////////////////////////////////////////////////////////////////////////////////
/// @brief Aql, execution plan
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2014 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
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
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Jan Steemann
/// @author Copyright 2014, ArangoDB GmbH, Cologne, Germany
/// @author Copyright 2012-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "Aql/ExecutionPlan.h"
#include "Aql/Ast.h"
#include "Aql/AstNode.h"
#include "Aql/ExecutionNode.h"
#include "Aql/Expression.h"
#include "Aql/NodeFinder.h"
#include "Aql/Optimizer.h"
#include "Aql/Query.h"
#include "Aql/Variable.h"
#include "Aql/WalkerWorker.h"
#include "Basics/JsonHelper.h"
#include "Basics/Exceptions.h"

using namespace triagens::aql;
using namespace triagens::basics;
using JsonHelper = triagens::basics::JsonHelper;

// -----------------------------------------------------------------------------
// --SECTION--                                        constructors / destructors
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief create the plan
////////////////////////////////////////////////////////////////////////////////

ExecutionPlan::ExecutionPlan (Ast* ast) 
  : _ids(),
    _root(nullptr),
    _varUsageComputed(false),
    _nextId(0),
    _ast(ast),
    _lastLimitNode(nullptr),
    _subQueries() {

}

////////////////////////////////////////////////////////////////////////////////
/// @brief destroy the plan, frees all assigned nodes
////////////////////////////////////////////////////////////////////////////////

ExecutionPlan::~ExecutionPlan () {
  for (auto x : _ids){
    delete x.second;
  }
}

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan from an AST
////////////////////////////////////////////////////////////////////////////////

ExecutionPlan* ExecutionPlan::instanciateFromAst (Ast* ast) {
  TRI_ASSERT(ast != nullptr);

  auto root = ast->root();
  TRI_ASSERT(root != nullptr);
  TRI_ASSERT(root->type == NODE_TYPE_ROOT);

  auto plan = new ExecutionPlan(ast);

  try {
    plan->_root = plan->fromNode(root);
    // insert fullCount flag
    if (plan->_lastLimitNode != nullptr && ast->query()->getBooleanOption("fullCount", false)) {
      static_cast<LimitNode*>(plan->_lastLimitNode)->setFullCount();
    }
    plan->findVarUsage();
    return plan;
    // just for debugging
  }
  catch (...) {
    delete plan;
    throw;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan from JSON
////////////////////////////////////////////////////////////////////////////////

void ExecutionPlan::getCollectionsFromJson (Ast* ast, 
                                            triagens::basics::Json const& json) {
  Json jsonCollectionList = json.get("collections");

  if (! jsonCollectionList.isArray()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, "json node \"collections\" not found or not an array");
  }

  auto const size = jsonCollectionList.size();
    
  for (size_t i = 0; i < size; i++) {
    Json oneJsonCollection = jsonCollectionList.at(static_cast<int>(i));
    auto typeStr = triagens::basics::JsonHelper::checkAndGetStringValue(oneJsonCollection.json(), "type");
      
    ast->query()->collections()->add(
      triagens::basics::JsonHelper::checkAndGetStringValue(oneJsonCollection.json(), "name"),
      TRI_GetTransactionTypeFromStr(triagens::basics::JsonHelper::checkAndGetStringValue(oneJsonCollection.json(), "type").c_str())
    );
  }
}

ExecutionPlan* ExecutionPlan::instanciateFromJson (Ast* ast,
                                                   triagens::basics::Json const& json) {
  auto plan = new ExecutionPlan(ast);

  try {
    plan->_root = plan->fromJson(json);
    plan->_varUsageComputed = true;
    return plan;
  }
  catch (...) {
    delete plan;
    throw;
  }
}

ExecutionPlan* ExecutionPlan::clone (Query& onThatQuery) {
  std::unique_ptr<ExecutionPlan> otherPlan(new ExecutionPlan(onThatQuery.ast()));

  for (auto it: _ids) {
    otherPlan->registerNode(it.second->clone(otherPlan.get(), false, true));
  }

  return otherPlan.release();
}

////////////////////////////////////////////////////////////////////////////////
/// @brief export to JSON, returns an AUTOFREE Json object
////////////////////////////////////////////////////////////////////////////////

triagens::basics::Json ExecutionPlan::toJson (Ast* ast,
                                              TRI_memory_zone_t* zone,
                                              bool verbose) const {
  triagens::basics::Json result = _root->toJson(zone, verbose); 
 
  // set up rules 
  auto const&& appliedRules = Optimizer::translateRules(_appliedRules);
  triagens::basics::Json rules(Json::Array, appliedRules.size());

  for (auto r : appliedRules) {
    rules.add(triagens::basics::Json(r));
  }
  result.set("rules", rules);

  auto usedCollections = *ast->query()->collections()->collections();
  triagens::basics::Json jsonCollectionList(Json::Array, usedCollections.size());

  for (auto c : usedCollections) {
    Json json(Json::Object);

    jsonCollectionList(json("name", Json(c.first))
                           ("type", Json(TRI_TransactionTypeGetStr(c.second->accessType))));
  }

  result.set("collections", jsonCollectionList);
  result.set("variables", ast->variables()->toJson(TRI_UNKNOWN_MEM_ZONE));
  size_t nrItems = 0;
  result.set("estimatedCost", triagens::basics::Json(_root->getCost(nrItems)));
  result.set("estimatedNrItems", triagens::basics::Json(static_cast<double>(nrItems)));

  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief get a list of all applied rules
////////////////////////////////////////////////////////////////////////////////

std::vector<std::string> ExecutionPlan::getAppliedRules () const {
  return Optimizer::translateRules(_appliedRules);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief get a node by its id
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::getNodeById (size_t id) const {
  auto it = _ids.find(id);
  
  if (it != _ids.end()) {
    // node found
    return (*it).second;
  }

  std::string msg =  std::string("node [") + std::to_string(id) + std::string("] wasn't found");
  // node unknown
  THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, msg);
}

// -----------------------------------------------------------------------------
// --SECTION--                                                 private functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @brief create modification options from an AST node
////////////////////////////////////////////////////////////////////////////////

ModificationOptions ExecutionPlan::createOptions (AstNode const* node) {
  ModificationOptions options;

  // parse the modification options we got
  if (node != nullptr && 
      node->type == NODE_TYPE_OBJECT) {
    size_t n = node->numMembers();

    for (size_t i = 0; i < n; ++i) {
      auto member = node->getMember(i);

      if (member != nullptr && 
          member->type == NODE_TYPE_OBJECT_ELEMENT) {
        auto name = member->getStringValue();
        auto value = member->getMember(0);

        TRI_ASSERT(value->isConstant());

        if (strcmp(name, "waitForSync") == 0) {
          options.waitForSync = value->isTrue();
        }
        else if (strcmp(name, "ignoreErrors") == 0) {
          options.ignoreErrors = value->isTrue();
        }
        else if (strcmp(name, "keepNull") == 0) {
          // nullMeansRemove is the opposite of keepNull
          options.nullMeansRemove = value->isFalse();
        }
        else if (strcmp(name, "mergeObjects") == 0) {
          options.mergeObjects = value->isTrue();
        }
      }
    }
  }

  options.readCompleteInput = true;

  if (! _ast->functionsMayAccessDocuments()) {
    // no functions in the query can access document data...
    bool isReadWrite = false;

    auto const collections = _ast->query()->collections();
    for (auto it : *(collections->collections())) {
      if (it.second->isReadWrite) {
        isReadWrite = true;
        break;
      }
    }

    if (! isReadWrite) {
      // no collection is used in both read and write
      // this means the query's write operation can use read & write in lockstep
      options.readCompleteInput = false;
    }
  }

  return options;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief register a node with the plan, will delete node if addition fails
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::registerNode (ExecutionNode* node) {
  TRI_ASSERT(node != nullptr);
  TRI_ASSERT(node->id() > 0);

  try {
    _ids.emplace(std::make_pair(node->id(), node));
  }
  catch (...) {
    delete node;
    throw;
  }
  return node;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief creates a calculation node for an arbitrary expression
////////////////////////////////////////////////////////////////////////////////

CalculationNode* ExecutionPlan::createTemporaryCalculation (AstNode const* expression) {
  // generate a temporary variable
  auto out = _ast->variables()->createTemporaryVariable();
  TRI_ASSERT(out != nullptr);

  // generate a temporary calculation node
  std::unique_ptr<Expression> expr(new Expression(_ast, const_cast<AstNode*>(expression)));

  auto en = new CalculationNode(this, nextId(), expr.get(), out);
  expr.release();
 
  registerNode(reinterpret_cast<ExecutionNode*>(en));
  return en;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief adds "previous" as dependency to "plan", returns "plan"
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::addDependency (ExecutionNode* previous,
                                             ExecutionNode* plan) {
  TRI_ASSERT(previous != nullptr);
  TRI_ASSERT(plan != nullptr);

  try {
    plan->addDependency(previous);
    return plan;
  }
  catch (...) {
    // prevent memleak
    delete plan;
    throw;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan element from an AST FOR node
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromNodeFor (ExecutionNode* previous,
                                           AstNode const* node) {
  TRI_ASSERT(node != nullptr && node->type == NODE_TYPE_FOR);
  TRI_ASSERT(node->numMembers() == 2);

  auto variable = node->getMember(0);
  auto expression = node->getMember(1);

  // fetch 1st operand (out variable name)
  TRI_ASSERT(variable->type == NODE_TYPE_VARIABLE);
  auto v = static_cast<Variable*>(variable->getData());
  TRI_ASSERT(v != nullptr);
  
  ExecutionNode* en = nullptr;

  // peek at second operand
  if (expression->type == NODE_TYPE_COLLECTION) {
    // second operand is a collection
    char const* collectionName = expression->getStringValue();
    auto collections = _ast->query()->collections();
    auto collection = collections->get(collectionName);

    if (collection == nullptr) {
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, "no collection for EnumerateCollection");
    }
    en = registerNode(new EnumerateCollectionNode(this, nextId(), _ast->query()->vocbase(), collection, v, false));
  }
  else if (expression->type == NODE_TYPE_REFERENCE) {
    // second operand is already a variable
    auto inVariable = static_cast<Variable*>(expression->getData());
    TRI_ASSERT(inVariable != nullptr);
    en = registerNode(new EnumerateListNode(this, nextId(), inVariable, v));
  }
  else {
    // second operand is some misc. expression
    auto calc = createTemporaryCalculation(expression);

    calc->addDependency(previous);
    en = registerNode(new EnumerateListNode(this, nextId(), calc->outVariable(), v));
    previous = calc;
  }

  TRI_ASSERT(en != nullptr);
  
  return addDependency(previous, en);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan element from an AST FILTER node
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromNodeFilter (ExecutionNode* previous,
                                              AstNode const* node) {
  TRI_ASSERT(node != nullptr && node->type == NODE_TYPE_FILTER);
  TRI_ASSERT(node->numMembers() == 1);
  
  auto expression = node->getMember(0);

  ExecutionNode* en = nullptr;
  
  if (expression->type == NODE_TYPE_REFERENCE) {
    // operand is already a variable
    auto v = static_cast<Variable*>(expression->getData());
    TRI_ASSERT(v != nullptr);
    en = registerNode(new FilterNode(this, nextId(), v));
  }
  else {
    // operand is some misc expression
    auto calc = createTemporaryCalculation(expression);

    calc->addDependency(previous);
    en = registerNode(new FilterNode(this, nextId(), calc->outVariable()));
    previous = calc;
  }

  return addDependency(previous, en);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan element from an AST LET node
/// this also includes handling of subqueries (as subqueries can only occur 
/// inside LET nodes)
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromNodeLet (ExecutionNode* previous,
                                           AstNode const* node) {
  TRI_ASSERT(node != nullptr && node->type == NODE_TYPE_LET);
  TRI_ASSERT(node->numMembers() >= 2);

  AstNode const* variable = node->getMember(0);
  AstNode const* expression = node->getMember(1);

  Variable const* conditionVariable = nullptr;

  if (node->numMembers() > 2) {
    // a LET with an IF condition
    auto condition = createTemporaryCalculation(node->getMember(2));
    condition->addDependency(previous);
    previous = condition;

    conditionVariable = condition->outVariable();
  }

  auto v = static_cast<Variable*>(variable->getData());
  
  ExecutionNode* en = nullptr;

  if (expression->type == NODE_TYPE_SUBQUERY) {
    // operand is a subquery...
    auto subquery = fromNode(expression);

    if (subquery == nullptr) {
      THROW_ARANGO_EXCEPTION(TRI_ERROR_OUT_OF_MEMORY);
    }

    en = registerNode(new SubqueryNode(this, nextId(), subquery, v));
    _subQueries[static_cast<SubqueryNode*>(en)->outVariable()->id] = en;
  }
  else {
    if (expression->type == NODE_TYPE_REFERENCE) {
      // the right hand side of the LET is just a reference to an existing variable
      auto referencedVariable = static_cast<Variable const*>(expression->getData());

      TRI_ASSERT(referencedVariable != nullptr);

      if (! referencedVariable->isUserDefined()) {
        // if the variable on the right is an internal variable, check if we can get
        // away without the LET
        auto it = _subQueries.find(referencedVariable->id);
        if (it != _subQueries.end()) {
          // optimization: if the LET a = ... references a variable created by a subquery,
          // change the output variable of the (anonymous) subquery to be the outvariable of
          // the LET. and don't create the LET

          auto sn = static_cast<SubqueryNode*>((*it).second);
          sn->replaceOutVariable(v);
          return sn;
        }
      }

      // otherwise fall-through to normal behavior
    }

    // operand is some misc expression, potentially including references to other variables
    std::unique_ptr<Expression> expr(new Expression(_ast, const_cast<AstNode*>(expression)));

    auto calc = new CalculationNode(this, nextId(), expr.get(), conditionVariable, v);
    expr.release();
    en = registerNode(calc);
  }
    
  return addDependency(previous, en);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan element from an AST SORT node
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromNodeSort (ExecutionNode* previous,
                                            AstNode const* node) {
  TRI_ASSERT(node != nullptr && node->type == NODE_TYPE_SORT);
  TRI_ASSERT(node->numMembers() == 1);

  auto list = node->getMember(0);
  TRI_ASSERT(list->type == NODE_TYPE_ARRAY);

  std::vector<std::pair<Variable const*, bool>> elements;
  std::vector<CalculationNode*> temp;

  try {
    size_t const n = list->numMembers();
    elements.reserve(n);

    for (size_t i = 0; i < n; ++i) {
      auto element = list->getMember(i);
      TRI_ASSERT(element != nullptr);
      TRI_ASSERT(element->type == NODE_TYPE_SORT_ELEMENT);
      TRI_ASSERT(element->numMembers() == 2);

      auto expression = element->getMember(0);

      if (expression->isConstant()) {
        // expression is constant, so sorting with not provide any benefit 
        continue;
      }

      auto ascending = element->getMember(1);

      // get sort order
      bool isAscending;
      bool handled = false;
      if (ascending->type == NODE_TYPE_VALUE) {
        if (ascending->value.type == VALUE_TYPE_STRING) {
          // special treatment for string values ASC/DESC
          if (TRI_CaseEqualString(ascending->value.value._string, "ASC")) {
            isAscending = true;
            handled = true;
          }
          else if (TRI_CaseEqualString(ascending->value.value._string, "DESC")) {
            isAscending = false;
            handled = true;
          }
        }
      }

      if (! handled) {
        // if no sort order is set, ensure we have one
        auto ascendingNode = ascending->castToBool(_ast);
        if (ascendingNode->type == NODE_TYPE_VALUE && 
            ascendingNode->value.type == VALUE_TYPE_BOOL) {
          isAscending = ascendingNode->value.value._bool;
        }
        else {
          // must have an order
          isAscending = true;
        }
      }

      if (expression->type == NODE_TYPE_REFERENCE) {
        // sort operand is a variable
        auto v = static_cast<Variable*>(expression->getData());
        TRI_ASSERT(v != nullptr);
        elements.emplace_back(std::make_pair(v, isAscending));
      }
      else {
        // sort operand is some misc expression
        auto calc = createTemporaryCalculation(expression);
        temp.push_back(calc);
        elements.emplace_back(std::make_pair(calc->outVariable(), isAscending));
      }
    }
  }
  catch (...) {
    // prevent memleak
    for (auto it = temp.begin(); it != temp.end(); ++it) {
      delete (*it);
    }
    throw;
  }


  if (elements.empty()) {
    // no sort criterion remained - this can only happen if all sort
    // criteria were constants 
    return previous;
  }


  // at least one sort criterion remained 
  TRI_ASSERT(! elements.empty());

  // properly link the temporary calculations in the plan
  for (auto it = temp.begin(); it != temp.end(); ++it) {
    (*it)->addDependency(previous);
    previous = (*it);
  }

  auto en = registerNode(new SortNode(this, nextId(), elements, false));

  return addDependency(previous, en);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan element from an AST COLLECT node
/// note that also a sort plan node will be added in front of the collect plan
/// node
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromNodeCollect (ExecutionNode* previous,
                                               AstNode const* node) {
  TRI_ASSERT(node != nullptr && 
             node->type == NODE_TYPE_COLLECT);
  size_t const n = node->numMembers();

  TRI_ASSERT(n >= 1);

  auto list = node->getMember(0);
  size_t const numVars = list->numMembers();
  
  std::vector<std::pair<Variable const*, bool>> sortElements;

  std::vector<std::pair<Variable const*, Variable const*>> aggregateVariables;
  aggregateVariables.reserve(numVars);
  for (size_t i = 0; i < numVars; ++i) {
    auto assigner = list->getMember(i);

    if (assigner == nullptr) {
      continue;
    }

    TRI_ASSERT(assigner->type == NODE_TYPE_ASSIGN);
    auto out = assigner->getMember(0);
    TRI_ASSERT(out != nullptr);
    auto v = static_cast<Variable*>(out->getData());
    TRI_ASSERT(v != nullptr);
   
    auto expression = assigner->getMember(1);
      
    if (expression->type == NODE_TYPE_REFERENCE) {
      // operand is a variable
      auto e = static_cast<Variable*>(expression->getData());
      aggregateVariables.push_back(std::make_pair(v, e));
      sortElements.push_back(std::make_pair(e, true));
    }
    else {
      // operand is some misc expression
      auto calc = createTemporaryCalculation(expression);

      calc->addDependency(previous);
      previous = calc;

      aggregateVariables.emplace_back(std::make_pair(v, calc->outVariable()));
      sortElements.emplace_back(std::make_pair(calc->outVariable(), true));
    }
  }

  // inject a sort node for all expressions / variables that we just picked up...
  // note that this sort is stable
  auto sort = registerNode(new SortNode(this, nextId(), sortElements, true));
  sort->addDependency(previous);
  previous = sort;

  // handle out variable
  Variable* outVariable = nullptr;
  std::vector<Variable const*> keepVariables;

  if (n >= 2) {
    // collect with an output variable!
    auto v = node->getMember(1);
    outVariable = static_cast<Variable*>(v->getData());

    if (n >= 3) {
      auto vars = node->getMember(2);
      TRI_ASSERT(vars->type == NODE_TYPE_ARRAY);
      size_t const keepVarsSize = vars->numMembers();
      keepVariables.reserve(keepVarsSize);
      for (size_t i = 0; i < keepVarsSize; ++i) {
        auto ref = vars->getMember(i);
        TRI_ASSERT(ref->type == NODE_TYPE_REFERENCE);
        keepVariables.push_back(static_cast<Variable const*>(ref->getData()));
      }
    }
  }

  auto en = registerNode(new AggregateNode(this, nextId(), aggregateVariables, nullptr, 
                  outVariable, keepVariables, _ast->variables()->variables(false), false));

  return addDependency(previous, en);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan element from an AST COLLECT node
/// note that also a sort plan node will be added in front of the collect plan
/// node
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromNodeCollectExpression (ExecutionNode* previous,
                                                         AstNode const* node) {
  TRI_ASSERT(node != nullptr && 
             node->type == NODE_TYPE_COLLECT_EXPRESSION);
  TRI_ASSERT(node->numMembers() == 3);

  auto list = node->getMember(0);
  size_t const numVars = list->numMembers();
  
  std::vector<std::pair<Variable const*, bool>> sortElements;

  std::vector<std::pair<Variable const*, Variable const*>> aggregateVariables;
  aggregateVariables.reserve(numVars);
  for (size_t i = 0; i < numVars; ++i) {
    auto assigner = list->getMember(i);

    if (assigner == nullptr) {
      continue;
    }

    TRI_ASSERT(assigner->type == NODE_TYPE_ASSIGN);
    auto out = assigner->getMember(0);
    TRI_ASSERT(out != nullptr);
    auto v = static_cast<Variable*>(out->getData());
    TRI_ASSERT(v != nullptr);
   
    auto expression = assigner->getMember(1);
      
    if (expression->type == NODE_TYPE_REFERENCE) {
      // operand is a variable
      auto e = static_cast<Variable*>(expression->getData());
      aggregateVariables.push_back(std::make_pair(v, e));
      sortElements.push_back(std::make_pair(e, true));
    }
    else {
      // operand is some misc expression
      auto calc = createTemporaryCalculation(expression);

      calc->addDependency(previous);
      previous = calc;

      aggregateVariables.emplace_back(std::make_pair(v, calc->outVariable()));
      sortElements.emplace_back(std::make_pair(calc->outVariable(), true));
    }
  }

  
  Variable const* expressionVariable = nullptr;
  auto expression = node->getMember(2);
  if (expression->type == NODE_TYPE_REFERENCE) {
    // expression is already a variable
    auto variable = static_cast<Variable*>(expression->getData());
    TRI_ASSERT(variable != nullptr);
    expressionVariable = variable;
  }
  else {
    // expression is some misc expression
    auto calc = createTemporaryCalculation(expression);
    calc->addDependency(previous);
    previous = calc;
    expressionVariable = calc->outVariable();
  }

  // inject a sort node for all expressions / variables that we just picked up...
  // note that this sort is stable
  auto sort = registerNode(new SortNode(this, nextId(), sortElements, true));
  sort->addDependency(previous);
  previous = sort;

  // output variable
  auto v = node->getMember(1);
  Variable* outVariable = static_cast<Variable*>(v->getData());
        
  std::unordered_map<VariableId, std::string const> variableMap;
        
  auto en = registerNode(new AggregateNode(this, nextId(), aggregateVariables, 
              expressionVariable, outVariable, std::vector<Variable const*>(), variableMap, false));

  return addDependency(previous, en);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan element from an AST COLLECT node, COUNT
/// note that also a sort plan node will be added in front of the collect plan
/// node
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromNodeCollectCount (ExecutionNode* previous,
                                                    AstNode const* node) {
  TRI_ASSERT(node != nullptr && 
             node->type == NODE_TYPE_COLLECT_COUNT);
  TRI_ASSERT(node->numMembers() == 2);

  auto list = node->getMember(0);
  size_t const numVars = list->numMembers();
  
  std::vector<std::pair<Variable const*, bool>> sortElements;

  std::vector<std::pair<Variable const*, Variable const*>> aggregateVariables;
  aggregateVariables.reserve(numVars);
  for (size_t i = 0; i < numVars; ++i) {
    auto assigner = list->getMember(i);

    if (assigner == nullptr) {
      continue;
    }

    TRI_ASSERT(assigner->type == NODE_TYPE_ASSIGN);
    auto out = assigner->getMember(0);
    TRI_ASSERT(out != nullptr);
    auto v = static_cast<Variable*>(out->getData());
    TRI_ASSERT(v != nullptr);
   
    auto expression = assigner->getMember(1);
      
    if (expression->type == NODE_TYPE_REFERENCE) {
      // operand is a variable
      auto e = static_cast<Variable*>(expression->getData());
      aggregateVariables.emplace_back(std::make_pair(v, e));
      sortElements.emplace_back(std::make_pair(e, true));
    }
    else {
      // operand is some misc expression
      auto calc = createTemporaryCalculation(expression);

      calc->addDependency(previous);
      previous = calc;

      aggregateVariables.push_back(std::make_pair(v, calc->outVariable()));
      sortElements.push_back(std::make_pair(calc->outVariable(), true));
    }
  }

  // inject a sort node for all expressions / variables that we just picked up...
  // note that this sort is stable
  if (numVars > 0) {
    // a SortNode is only required if we have grouping criteria.
    // for example, COLLECT x = ... WITH COUNT INTO g has grouping criteria x = ...
    // but the following statement doesn't have any: COLLECT WITH COUNT INTO g
    auto sort = registerNode(new SortNode(this, nextId(), sortElements, true));
    sort->addDependency(previous);
    previous = sort;
  }

  // output variable
  auto v = node->getMember(1);
  // handle out variable
  Variable* outVariable = static_cast<Variable*>(v->getData());

  auto en = registerNode(new AggregateNode(this, nextId(), aggregateVariables, nullptr, 
                  outVariable, std::vector<Variable const*>(), _ast->variables()->variables(false), true));

  return addDependency(previous, en);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan element from an AST LIMIT node
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromNodeLimit (ExecutionNode* previous,
                                             AstNode const* node) {
  TRI_ASSERT(node != nullptr && node->type == NODE_TYPE_LIMIT);
  TRI_ASSERT(node->numMembers() == 2);

  auto offset = node->getMember(0);
  auto count  = node->getMember(1);

  TRI_ASSERT(offset->type == NODE_TYPE_VALUE);
  TRI_ASSERT(count->type == NODE_TYPE_VALUE);
    
  if ((offset->value.type != VALUE_TYPE_INT && 
       offset->value.type != VALUE_TYPE_DOUBLE) ||
      offset->getIntValue() < 0) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_QUERY_NUMBER_OUT_OF_RANGE, "LIMIT value is not a number or out of range");
  }
  if ((count->value.type != VALUE_TYPE_INT && 
       count->value.type != VALUE_TYPE_DOUBLE) ||
      count->getIntValue() < 0) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_QUERY_NUMBER_OUT_OF_RANGE, "LIMIT value is not a number or out of range");
  }

  auto en = registerNode(new LimitNode(this, nextId(), static_cast<size_t>(offset->getIntValue()), static_cast<size_t>(count->getIntValue())));

  _lastLimitNode = en;

  return addDependency(previous, en);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan element from an AST RETURN node
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromNodeReturn (ExecutionNode* previous,
                                              AstNode const* node) {
  TRI_ASSERT(node != nullptr && node->type == NODE_TYPE_RETURN);
  TRI_ASSERT(node->numMembers() == 1);
  
  auto expression = node->getMember(0);

  ExecutionNode* en = nullptr;
  
  if (expression->type == NODE_TYPE_REFERENCE) {
    // operand is already a variable
    auto v = static_cast<Variable*>(expression->getData());
    TRI_ASSERT(v != nullptr);
    en = registerNode(new ReturnNode(this, nextId(), v));
  }
  else {
    // operand is some misc expression
    auto calc = createTemporaryCalculation(expression);
    calc->addDependency(previous);
    en = registerNode(new ReturnNode(this, nextId(), calc->outVariable()));
    previous = calc;
  }

  return addDependency(previous, en);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan element from an AST REMOVE node
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromNodeRemove (ExecutionNode* previous,
                                              AstNode const* node) {
  TRI_ASSERT(node != nullptr && node->type == NODE_TYPE_REMOVE);
  TRI_ASSERT(node->numMembers() >= 3);
  
  auto options = createOptions(node->getMember(0));
  char const* collectionName = node->getMember(1)->getStringValue();
  auto collections = _ast->query()->collections();
  auto collection = collections->get(collectionName);

  if (collection == nullptr) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, "no collection for RemoveNode");
  }

  auto expression = node->getMember(2);
  ExecutionNode* en = nullptr;

  Variable const* outVariable = nullptr;

  if (node->numMembers() > 3) {
    auto returnVarNode = node->getMember(3);
    outVariable = static_cast<Variable*>(returnVarNode->getData());
  }

  if (expression->type == NODE_TYPE_REFERENCE) {
    // operand is already a variable
    auto v = static_cast<Variable*>(expression->getData());
    TRI_ASSERT(v != nullptr);
    en = registerNode(new RemoveNode(this, nextId(), _ast->query()->vocbase(), collection, options, v, outVariable));
  }
  else {
    // operand is some misc expression
    auto calc = createTemporaryCalculation(expression);
    calc->addDependency(previous);
    en = registerNode(new RemoveNode(this, nextId(), _ast->query()->vocbase(), collection, options, calc->outVariable(), outVariable));
    previous = calc;
  }

  return addDependency(previous, en);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan element from an AST INSERT node
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromNodeInsert (ExecutionNode* previous,
                                              AstNode const* node) {
  TRI_ASSERT(node != nullptr && node->type == NODE_TYPE_INSERT);
  TRI_ASSERT(node->numMembers() <= 4);
  
  auto options = createOptions(node->getMember(0));
  char const* collectionName = node->getMember(1)->getStringValue();
  auto collections = _ast->query()->collections();
  auto collection = collections->get(collectionName);
  auto expression = node->getMember(2);

  Variable const* outVariable = nullptr;

  if (node->numMembers() > 3) {
    auto returnVarNode = node->getMember(3);
    outVariable = static_cast<Variable*>(returnVarNode->getData());
  }

  ExecutionNode* en = nullptr;

  if (expression->type == NODE_TYPE_REFERENCE) {
    // operand is already a variable
    auto v = static_cast<Variable*>(expression->getData());
    TRI_ASSERT(v != nullptr);
    en = registerNode(new InsertNode(this, nextId(), _ast->query()->vocbase(),
                                     collection, options, v, outVariable));
  }
  else {
    // operand is some misc expression
    auto calc = createTemporaryCalculation(expression);
    calc->addDependency(previous);
    en = registerNode(new InsertNode(this, nextId(), _ast->query()->vocbase(),
                      collection, options, calc->outVariable(), outVariable));
    previous = calc;
  }

  return addDependency(previous, en);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan element from an AST UPDATE node
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromNodeUpdate (ExecutionNode* previous,
                                              AstNode const* node) {
  TRI_ASSERT(node != nullptr && node->type == NODE_TYPE_UPDATE);
  TRI_ASSERT(node->numMembers() >= 4);
  
  auto options = createOptions(node->getMember(0));
  char const* collectionName = node->getMember(1)->getStringValue();
  auto collections = _ast->query()->collections();
  auto collection = collections->get(collectionName);
  auto docExpression = node->getMember(2);
  auto keyExpression = node->getMember(3);
  Variable const* keyVariable = nullptr;
  ExecutionNode* en = nullptr;

  Variable const* outVariable = nullptr;
  bool returnNewValues = true;
  auto returnVarNode = node->getOptionalMember(4);
  if (returnVarNode != nullptr) {
    outVariable = static_cast<Variable*>(returnVarNode->getData());
    returnNewValues = node->getMember(5)->getBoolValue ();
  }

  if (keyExpression->type == NODE_TYPE_NOP) {
    keyExpression = nullptr;
  }

  if (keyExpression != nullptr) {
    if (keyExpression->type == NODE_TYPE_REFERENCE) {
      // key operand is already a variable
      auto v = static_cast<Variable*>(keyExpression->getData());
      TRI_ASSERT(v != nullptr);
      keyVariable = v;
    }
    else {
      // key operand is some misc expression
      auto calc = createTemporaryCalculation(keyExpression);
      calc->addDependency(previous);
      previous = calc;
      keyVariable = calc->outVariable();
    }
  }

  if (docExpression->type == NODE_TYPE_REFERENCE) {
    // document operand is already a variable
    auto v = static_cast<Variable*>(docExpression->getData());
    TRI_ASSERT(v != nullptr);
    en = registerNode(new UpdateNode(this, nextId(), _ast->query()->vocbase(),
                                     collection, options, v,
                                     keyVariable, outVariable, returnNewValues));
  }
  else {
    // document operand is some misc expression
    auto calc = createTemporaryCalculation(docExpression);
    calc->addDependency(previous);
    en = registerNode(new UpdateNode(this, nextId(), _ast->query()->vocbase(),
                                     collection, options, calc->outVariable(),
                                     keyVariable, outVariable, returnNewValues));
    previous = calc;
  }

  return addDependency(previous, en);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan element from an AST REPLACE node
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromNodeReplace (ExecutionNode* previous,
                                               AstNode const* node) {
  TRI_ASSERT(node != nullptr && node->type == NODE_TYPE_REPLACE);
  TRI_ASSERT(node->numMembers() >= 4);

  auto options        = createOptions(node->getMember(0));
  auto collectionName = node->getMember(1)->getStringValue();
  auto collections    = _ast->query()->collections();
  auto collection     = collections->get(collectionName);
  auto docExpression  = node->getMember(2);
  auto keyExpression  = node->getMember(3);

  Variable const* keyVariable = nullptr;
  ExecutionNode* en = nullptr;

  Variable const* outVariable = nullptr;
  bool returnNewValues = true;
  auto returnVarNode = node->getOptionalMember(4);
  if (returnVarNode != nullptr) {
    outVariable = static_cast<Variable*>(returnVarNode->getData());
    returnNewValues = node->getMember(5)->getBoolValue ();
  }
  
  if (keyExpression->type == NODE_TYPE_NOP) {
    keyExpression = nullptr;
  }

  if (keyExpression != nullptr) {
    if (keyExpression->type == NODE_TYPE_REFERENCE) {
      // key operand is already a variable
      auto v = static_cast<Variable*>(keyExpression->getData());
      TRI_ASSERT(v != nullptr);
      keyVariable = v;
    }
    else {
      // key operand is some misc expression
      auto calc = createTemporaryCalculation(keyExpression);
      calc->addDependency(previous);
      previous = calc;
      keyVariable = calc->outVariable();
    }
  }
  
  if (docExpression->type == NODE_TYPE_REFERENCE) {
    // operand is already a variable
    auto v = static_cast<Variable*>(docExpression->getData());
    TRI_ASSERT(v != nullptr);
    en = registerNode(new ReplaceNode(this, nextId(), _ast->query()->vocbase(),
                                      collection, options, v,
                                      keyVariable, outVariable, returnNewValues));
  }
  else {
    // operand is some misc expression
    auto calc = createTemporaryCalculation(docExpression);
    calc->addDependency(previous);
    en = registerNode(new ReplaceNode(this, nextId(), _ast->query()->vocbase(),
                                      collection, options, calc->outVariable(),
                                      keyVariable, outVariable, returnNewValues));
    previous = calc;
  }

  return addDependency(previous, en);
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create an execution plan from an abstract syntax tree node
////////////////////////////////////////////////////////////////////////////////
  
ExecutionNode* ExecutionPlan::fromNode (AstNode const* node) {
  TRI_ASSERT(node != nullptr);

  ExecutionNode* en = registerNode(new SingletonNode(this, nextId()));

  size_t const n = node->numMembers();

  for (size_t i = 0; i < n; ++i) {
    auto member = node->getMember(i);

    if (member == nullptr || member->type == NODE_TYPE_NOP) {
      continue;
    }

    switch (member->type) {
      case NODE_TYPE_FOR: {
        en = fromNodeFor(en, member);
        break;
      }

      case NODE_TYPE_FILTER: {
        en = fromNodeFilter(en, member);
        break;
      }

      case NODE_TYPE_LET: {
        en = fromNodeLet(en, member);
        break;
      }
    
      case NODE_TYPE_SORT: {
        en = fromNodeSort(en, member);
        break;
      }
    
      case NODE_TYPE_COLLECT: {
        en = fromNodeCollect(en, member);
        break;
      }

      case NODE_TYPE_COLLECT_EXPRESSION: {
        en = fromNodeCollectExpression(en, member);
        break;
      }

      case NODE_TYPE_COLLECT_COUNT: {
        en = fromNodeCollectCount(en, member);
        break;
      }
      
      case NODE_TYPE_LIMIT: {
        en = fromNodeLimit(en, member);
        break;
      }
    
      case NODE_TYPE_RETURN: {
        en = fromNodeReturn(en, member);
        break;
      }
    
      case NODE_TYPE_REMOVE: {
        en = fromNodeRemove(en, member);
        break;
      }
    
      case NODE_TYPE_INSERT: {
        en = fromNodeInsert(en, member);
        break;
      }
    
      case NODE_TYPE_UPDATE: {
        en = fromNodeUpdate(en, member);
        break;
      }
    
      case NODE_TYPE_REPLACE: {
        en = fromNodeReplace(en, member);
        break;
      }

      default: {
        // node type not implemented
        en = nullptr;
        break;
      }
    }

    if (en == nullptr) {
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, "type not handled");
    }
  }

  return en;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief find nodes of a certain type
////////////////////////////////////////////////////////////////////////////////

std::vector<ExecutionNode*> ExecutionPlan::findNodesOfType (
                                  ExecutionNode::NodeType type,
                                  bool enterSubqueries) {

  std::vector<ExecutionNode*> result;
  NodeFinder<ExecutionNode::NodeType> finder(type, result, enterSubqueries);
  root()->walk(&finder);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief find nodes of a certain types
////////////////////////////////////////////////////////////////////////////////

std::vector<ExecutionNode*> ExecutionPlan::findNodesOfType (
                                  std::vector<ExecutionNode::NodeType> const& types,
                                  bool enterSubqueries) {

  std::vector<ExecutionNode*> result;
  NodeFinder<std::vector<ExecutionNode::NodeType>> finder(types, result, enterSubqueries);
  root()->walk(&finder);
  return result;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief check linkage of execution plan
////////////////////////////////////////////////////////////////////////////////

#if 0
class LinkChecker : public WalkerWorker<ExecutionNode> {

  public:
    LinkChecker () {
    }

    bool before (ExecutionNode* en) {
      auto deps = en->getDependencies();
      for (auto x : deps) {
        auto parents = x->getParents();
        bool ok = false;
        for (auto it = parents.begin(); it != parents.end(); ++it) {
          if (*it == en) {
            ok = true;
            break;
          }
        }
        if (! ok) {
          std::cout << "Found dependency which does not have us as a parent!"
                    << std::endl;
        }
      }
      auto parents = en->getParents();
      if (parents.size() > 1) {
        std::cout << "Found a node with more than one parent!" << std::endl;
      }
      for (auto x : parents) {
        auto deps = x->getDependencies();
        bool ok = false;
        for (auto it = deps.begin(); it != deps.end(); ++it) {
          if (*it == en) {
            ok = true;
            break;
          }
        }
        if (! ok) {
          std::cout << "Found parent which does not have us as a dependency!"
                    << std::endl;
        }
      }
      return false;
    }
};

void ExecutionPlan::checkLinkage () {
  LinkChecker checker;
  root()->walk(&checker);
}

#endif

////////////////////////////////////////////////////////////////////////////////
/// @brief helper struct for findVarUsage
////////////////////////////////////////////////////////////////////////////////

struct VarUsageFinder : public WalkerWorker<ExecutionNode> {
    std::unordered_set<Variable const*> _usedLater;
    std::unordered_set<Variable const*> _valid;
    std::unordered_map<VariableId, ExecutionNode*>* _varSetBy;
    bool const _ownsVarSetBy;

    VarUsageFinder () 
      : _varSetBy(new std::unordered_map<VariableId, ExecutionNode*>()),
        _ownsVarSetBy(true) {
      
      TRI_ASSERT(_varSetBy != nullptr);
    }
    
    explicit VarUsageFinder (std::unordered_map<VariableId, ExecutionNode*>* varSetBy) 
      : _varSetBy(varSetBy),
        _ownsVarSetBy(false) {
        
      TRI_ASSERT(_varSetBy != nullptr);
    }
    
    ~VarUsageFinder () {
      if (_ownsVarSetBy) {
        TRI_ASSERT(_varSetBy != nullptr);
        delete _varSetBy;
      }
    }

    bool before (ExecutionNode* en) override final {
      en->invalidateVarUsage();
      en->setVarsUsedLater(_usedLater);
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
        _varSetBy->emplace(std::make_pair(v->id, en));
      }
      en->setVarsValid(_valid);
      en->setVarUsageValid();
    }

    bool enterSubquery (ExecutionNode*, ExecutionNode* sub) override final {
      VarUsageFinder subfinder(_varSetBy);
      subfinder._valid = _valid;  // need a copy for the subquery!
      sub->walk(&subfinder);
      
      // we've fully processed the subquery
      return false;
    }
};

////////////////////////////////////////////////////////////////////////////////
/// @brief determine and set _varsUsedLater in all nodes
////////////////////////////////////////////////////////////////////////////////

void ExecutionPlan::findVarUsage () {
  ::VarUsageFinder finder;
  root()->walk(&finder);
  _varSetBy = *finder._varSetBy;
  _varUsageComputed = true;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief determine if the above are already set
////////////////////////////////////////////////////////////////////////////////

bool ExecutionPlan::varUsageComputed () const {
  return _varUsageComputed;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief unlinkNodes, note that this does not delete the removed
/// nodes and that one cannot remove the root node of the plan.
////////////////////////////////////////////////////////////////////////////////

void ExecutionPlan::unlinkNodes (std::unordered_set<ExecutionNode*>& toRemove) {
  for (auto x : toRemove) {
    unlinkNode(x);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief unlinkNode, note that this does not delete the removed
/// node and that one cannot remove the root node of the plan.
////////////////////////////////////////////////////////////////////////////////

void ExecutionPlan::unlinkNode (ExecutionNode* node,
                                bool allowUnlinkingRoot) {
  auto parents = node->getParents();
  if (parents.empty()) {
    if (! allowUnlinkingRoot) {
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL,
                                     "Cannot unlink root node of plan");
    }
    // adjust root node. the caller needs to make sure that a new root node gets inserted
    _root = nullptr;
  }

  auto dep = node->getDependencies();  // Intentionally copy the vector!
  for (auto* p : parents) {
    p->removeDependency(node);
    for (auto* x : dep) {
      p->addDependency(x);
    }
  }
  for (auto* x : dep) {
    node->removeDependency(x);
  }
  _varUsageComputed = false;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief replaceNode, note that <newNode> must be registered with the plan
/// before this method is called, also this does not delete the old
/// node and that one cannot replace the root node of the plan.
////////////////////////////////////////////////////////////////////////////////

void ExecutionPlan::replaceNode (ExecutionNode* oldNode, 
                                 ExecutionNode* newNode) { 
  TRI_ASSERT(oldNode->id() != newNode->id());
  TRI_ASSERT(newNode->getDependencies().empty());
  TRI_ASSERT(oldNode != _root);

  std::vector<ExecutionNode*> deps = oldNode->getDependencies();
    // Intentional copy
 
  for (auto* x : deps) {
    newNode->addDependency(x);
    oldNode->removeDependency(x);
  }
  
  auto oldNodeParents = oldNode->getParents();  // Intentional copy
  for (auto* oldNodeParent : oldNodeParents) {
    if (! oldNodeParent->replaceDependency(oldNode, newNode)){
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL,
                  "Could not replace dependencies of an old node");
    }
  }
  _varUsageComputed = false;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief insert <newNode> as a new (the first!) dependency of
/// <oldNode> and make the former first dependency of <oldNode> a
/// dependency of <newNode> (and no longer a direct dependency of
/// <oldNode>).
/// <newNode> must be registered with the plan before this method is called.
////////////////////////////////////////////////////////////////////////////////

void ExecutionPlan::insertDependency (ExecutionNode* oldNode, 
                                      ExecutionNode* newNode) {
  TRI_ASSERT(oldNode->id() != newNode->id());
  TRI_ASSERT(newNode->getDependencies().empty());
  TRI_ASSERT(oldNode->getDependencies().size() == 1);

  auto oldDeps = oldNode->getDependencies();  // Intentional copy
  if (! oldNode->replaceDependency(oldDeps[0], newNode)) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL,
                "Could not replace dependencies of an old node");
  }

  newNode->removeDependencies();
  newNode->addDependency(oldDeps[0]);
  _varUsageComputed = false;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief clone the plan by recursively cloning starting from the root
////////////////////////////////////////////////////////////////////////////////

class CloneNodeAdder : public WalkerWorker<ExecutionNode> {
    ExecutionPlan* _plan;
  
  public:

    bool success;

    CloneNodeAdder (ExecutionPlan* plan) : _plan(plan), success(true) {}
    
    ~CloneNodeAdder (){}

    bool before (ExecutionNode* node){
      // We need to catch exceptions because the walk has to finish
      // and either register the nodes or delete them.
      try {
        _plan->registerNode(node);
      }
      catch (...) {
        success = false;
      }
      return false;
    }
};

////////////////////////////////////////////////////////////////////////////////
/// @brief clone an existing execution plan
////////////////////////////////////////////////////////////////////////////////

ExecutionPlan* ExecutionPlan::clone () {
  auto plan = new ExecutionPlan(_ast);

  try {
    plan->_root = _root->clone(plan, true, false);
    plan->_nextId = _nextId;
    plan->_appliedRules = _appliedRules;
    CloneNodeAdder adder(plan);
    plan->_root->walk(&adder);
    if (! adder.success) {
      delete plan;
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, "Could not clone plan");
    }
    // plan->findVarUsage();
    // Let's not do it here, because supposedly the plan is modified as
    // the very next thing anyway!
    return plan;
  }
  catch (...) {
    delete plan;
    throw;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief create a plan from the JSON provided
////////////////////////////////////////////////////////////////////////////////

ExecutionNode* ExecutionPlan::fromJson (Json const& json) {
  ExecutionNode* ret = nullptr;
  Json nodes = json.get("nodes");
  //std::cout << nodes.toString() << "\n";

  if (! nodes.isArray()) {
    THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, "nodes is not an array");
  }

  // first, re-create all nodes from the JSON, using the node ids
  // no dependency links will be set up in this step
  auto const size = nodes.size();

  for (size_t i = 0; i < size; i++) {
    Json oneJsonNode = nodes.at(static_cast<int>(i));

    if (! oneJsonNode.isObject()) {
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, "json node is not an object");
    }
    ret = ExecutionNode::fromJsonFactory(this, oneJsonNode);

    registerNode(ret);

    TRI_ASSERT(ret != nullptr);

    if (ret->getType() == triagens::aql::ExecutionNode::SUBQUERY) {
      // found a subquery node. now do magick here
      Json subquery = oneJsonNode.get("subquery");
      // create the subquery nodes from the "subquery" sub-node
      auto subqueryNode = fromJson(subquery);
    
      // register the just created subquery 
      static_cast<SubqueryNode*>(ret)->setSubquery(subqueryNode); 
    }
  }

  // all nodes have been created. now add the dependencies

  for (size_t i = 0; i < size; i++) {
    Json oneJsonNode = nodes.at(static_cast<int>(i));

    if (! oneJsonNode.isObject()) {
      THROW_ARANGO_EXCEPTION_MESSAGE(TRI_ERROR_INTERNAL, "json node is not an object");
    }
   
    // read the node's own id 
    auto thisId = triagens::basics::JsonHelper::checkAndGetNumericValue<size_t>(oneJsonNode.json(), "id");
    auto thisNode = getNodeById(thisId);

    // now re-link the dependencies
    Json dependencies = oneJsonNode.get("dependencies");
    if (triagens::basics::JsonHelper::isArray(dependencies.json())) {
      size_t const nDependencies = dependencies.size();

      for (size_t j = 0; j < nDependencies; j ++) {
        if (triagens::basics::JsonHelper::isNumber(dependencies.at(static_cast<int>(j)).json())) {
          auto depId = triagens::basics::JsonHelper::getNumericValue<size_t>(dependencies.at(static_cast<int>(j)).json(), 0);
          thisNode->addDependency(getNodeById(depId));
        }
      }
    }
  }

  return ret;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief returns true if a plan is so simple that optimizations would
/// probably cost more than simply executing the plan
////////////////////////////////////////////////////////////////////////////////

bool ExecutionPlan::isDeadSimple () const {
  auto current = _root;
  while (current != nullptr) {
    auto deps = current->getDependencies();

    if (deps.size() != 1) {
      break;
    }

    auto const nodeType = current->getType();

    if (nodeType == ExecutionNode::SUBQUERY ||
        nodeType == ExecutionNode::ENUMERATE_COLLECTION ||
        nodeType == ExecutionNode::ENUMERATE_LIST ||
        nodeType == ExecutionNode::INDEX_RANGE) {
      // these node types are not simple
      return false;
    }

    current = deps[0];
  }

  return true;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief show an overview over the plan
////////////////////////////////////////////////////////////////////////////////

struct Shower : public WalkerWorker<ExecutionNode> {
  int indent;

  Shower () : indent(0) {
  }

  ~Shower () {
  }

  bool enterSubquery (ExecutionNode*, ExecutionNode*) override final {
    indent++;
    return true;
  }

  void leaveSubquery (ExecutionNode*, ExecutionNode*) override final {
    indent--;
  }

  void after (ExecutionNode* en) override final {
    for (int i = 0; i < indent; i++) {
      std::cout << ' ';
    }
    std::cout << en->getTypeString() << std::endl;
  }
};

////////////////////////////////////////////////////////////////////////////////
/// @brief show an overview over the plan
////////////////////////////////////////////////////////////////////////////////

void ExecutionPlan::show () {
  Shower shower;
  _root->walk(&shower);
}

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

// Local Variables:
// mode: outline-minor
// outline-regexp: "/// @brief\\|/// {@inheritDoc}\\|/// @page\\|// --SECTION--\\|/// @\\}"
// End:
