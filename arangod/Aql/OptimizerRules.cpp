////////////////////////////////////////////////////////////////////////////////
/// @brief rules for the query optimizer
///
/// @file arangod/Aql/OptimizerRules.cpp
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

#include "Aql/OptimizerRules.h"
#include "Aql/Variable.h"

using namespace triagens::aql;

// -----------------------------------------------------------------------------
// --SECTION--                                           rules for the optimizer
// -----------------------------------------------------------------------------


////////////////////////////////////////////////////////////////////////////////
/// @brief remove all unnecessary filters
/// filters that are always true are removed
/// filters that are always false will be removed plus their dependent nodes
/// this modifies the plan in place
////////////////////////////////////////////////////////////////////////////////

int triagens::aql::removeUnnecessaryFiltersRule (Optimizer* opt, 
                                                 ExecutionPlan* plan, 
                                                 Optimizer::PlanList& out,
                                                 bool& keep) {
  
  keep = true;
  std::unordered_set<ExecutionNode*> toRemove;
  std::vector<ExecutionNode*> nodes = plan->findNodesOfType(triagens::aql::ExecutionNode::FILTER);
  
  for (auto n : nodes) {
    // filter has one input variable
    auto varsUsedHere = n->getVariablesUsedHere();
    TRI_ASSERT(varsUsedHere.size() == 1);

    // now check who introduced our variable
    auto variable = varsUsedHere[0];
    auto setter = plan->getVarSetBy(variable->id);

    if (setter != nullptr && 
        setter->getType() == triagens::aql::ExecutionNode::CALCULATION) {
      // if it was a CalculationNode, check its expression
      auto s = static_cast<CalculationNode*>(setter);
      auto root = s->expression()->node();

      if (root->isConstant()) {
        // the expression is a constant value
        if (root->toBoolean()) {
          // filter is always true
          // remove filter node and merge with following node
          toRemove.insert(n);
        }
        else {
          // filter is always false
          /*
          // get all dependent nodes of the filter node
          std::vector<ExecutionNode*> stack;
          stack.push_back(n);

          while (! stack.empty()) {
            // pop a node from the stack
            auto current = stack.back();
            stack.pop_back();

            bool removeNode = true;

            if (toRemove.find(current) != toRemove.end()) {
              // detected a cycle. TODO: decide whether a cycle is an error here or 
              // if it is valid
              break;
            }

            if (current->getType() == triagens::aql::ExecutionNode::SINGLETON) {
              // stop at a singleton node
              break;
            }

            if (current->getType() == triagens::aql::ExecutionNode::CALCULATION) {
              auto c = static_cast<CalculationNode*>(current);
              if (c->expression()->node()->canThrow()) {
                // the calculation may throw an exception. we must not remove it
                // because its removal might change the query result
                removeNode = false;
                std::cout << "FOUND A CALCULATION THAT CAN THROW\n";
              }
            }

            auto deps = current->getDependencies();
            for (auto it = deps.begin(); it != deps.end(); ++it) {
              stack.push_back((*it));
            }

            if (removeNode) {
              std::cout << "INSERTING NODE OF TYPE: " << current->getTypeString() << "\n";
              toRemove.insert(current);
            }
          }
          */
        }
      }
    }
  }
  
  if (! toRemove.empty()) {
    std::cout << "Removing " << toRemove.size() << " unnecessary "
                 "nodes..." << std::endl;
    plan->removeNodes(toRemove);
  }
  
  return TRI_ERROR_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief moves calculations up in the plan
/// this modifies the plan in place
////////////////////////////////////////////////////////////////////////////////

int triagens::aql::moveCalculationsUpRule (Optimizer* opt, 
                                           ExecutionPlan* plan, 
                                           Optimizer::PlanList& out,
                                           bool& keep) {
  std::vector<ExecutionNode*> nodes = plan->findNodesOfType(triagens::aql::ExecutionNode::CALCULATION);

  for (auto n : nodes) {
    auto nn = static_cast<CalculationNode*>(n);
    if (nn->expression()->canThrow()) {
      // we will only move expressions up that cannot throw
      continue;
    }
      
    auto neededVars = n->getVariablesUsedHere();
    // sort the list of variables that the expression needs as its input
    // (sorting is needed for intersection later)
    std::sort(neededVars.begin(), neededVars.end(), &Variable::Comparator);

    std::vector<ExecutionNode*> stack;
    auto deps = n->getDependencies();
    
    for (auto it = deps.begin(); it != deps.end(); ++it) {
      stack.push_back((*it));
    }

    while (! stack.empty()) {
      auto current = stack.back();
      stack.pop_back();

      auto deps = current->getDependencies();

      if (deps.size() != 1) {
        // node either has no or more than one dependency. we don't know what to do and must abort
        // note that this will also handle Singleton nodes
        break;
      }

      // check which variables the current node defines
      auto dependencyVars = current->getVariablesSetHere();
      // sort the variables (sorting needed for intersection)
      std::sort(dependencyVars.begin(), dependencyVars.end(), &Variable::Comparator);
    
      // create the intersection of variables
      std::vector<Variable const*> shared;
      std::set_intersection(neededVars.begin(), neededVars.end(), 
                            dependencyVars.begin(), dependencyVars.end(),
                            std::back_inserter(shared));

      if (! shared.empty()) {
        // shared variables found, meaning that the current node introduces a variable needed
        // by our calculation. we cannot move the calculation up the chain
        break;
      }
      
      // no shared variables found. we can move the calculation up the dependency chain

      // first, delete the calculation from the plan
      plan->removeNode(n);

      // fiddle dependencies of calculation node
      n->removeDependencies();
      n->addDependency(deps[0]);
      n->invalidateVarUsage();

      // fiddle dependencies of current node
      current->removeDependency(deps[0]);
      current->addDependency(n);
      current->invalidateVarUsage();
      deps[0]->invalidateVarUsage();
            
      for (auto it = deps.begin(); it != deps.end(); ++it) {
        stack.push_back((*it));
      }
    }
  }

  plan->findVarUsage();

  keep = true; 
  return TRI_ERROR_NO_ERROR;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief remove CalculationNode(s) that are never needed
////////////////////////////////////////////////////////////////////////////////

int triagens::aql::removeUnnecessaryCalculationsRule (Optimizer* opt, 
                                                      ExecutionPlan* plan, 
                                                      Optimizer::PlanList& out, 
                                                      bool& keep) {
  std::vector<ExecutionNode*> nodes
    = plan->findNodesOfType(triagens::aql::ExecutionNode::CALCULATION);
  std::unordered_set<ExecutionNode*> toRemove;
  for (auto n : nodes) {
    auto nn = static_cast<CalculationNode*>(n);
    if (nn->expression()->canThrow()) {
      // If this node can throw, we must not optimize it away!
      continue;
    }

    auto outvar = n->getVariablesSetHere();
    TRI_ASSERT(outvar.size() == 1);
    auto varsUsedLater = n->getVarsUsedLater();

    if (varsUsedLater.find(outvar[0]) == varsUsedLater.end()) {
      // The variable whose value is calculated here is not used at
      // all further down the pipeline! We remove the whole
      // calculation node, 
      toRemove.insert(n);
    }
  }

  if (! toRemove.empty()) {
    std::cout << "Removing " << toRemove.size() << " unnecessary "
                 "CalculationNodes..." << std::endl;
    plan->removeNodes(toRemove);
    out.push_back(plan);
    keep = false;
  }
  else {
    keep = true;
  }
  return TRI_ERROR_NO_ERROR;
}


// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|// --SECTION--\\|/// @\\}\\)"
// End:
