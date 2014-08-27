////////////////////////////////////////////////////////////////////////////////
/// @brief infrastructure for query optimizer
///
/// @file arangod/Aql/Optimizer.h
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

#ifndef ARANGOD_AQL_OPTIMIZER_H
#define ARANGOD_AQL_OPTIMIZER_H 1

#include <Basics/Common.h>

#include "Aql/ExecutionPlan.h"

namespace triagens {
  namespace aql {

// -----------------------------------------------------------------------------
// --SECTION--                                               the optimizer class
// -----------------------------------------------------------------------------

    class Optimizer {

      public:

////////////////////////////////////////////////////////////////////////////////
/// @brief the following struct keeps a list (deque) of ExecutionPlan*
/// and has some automatic convenience functions.
////////////////////////////////////////////////////////////////////////////////

        struct PlanList {
          std::deque<ExecutionPlan*> list;
          std::deque<int> levelDone;

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor
////////////////////////////////////////////////////////////////////////////////

          PlanList () {};

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor with a plan
////////////////////////////////////////////////////////////////////////////////

          PlanList (ExecutionPlan* p, int level) {
            push_back(p, level);
          }

////////////////////////////////////////////////////////////////////////////////
/// @brief destructor, deleting contents
////////////////////////////////////////////////////////////////////////////////

          ~PlanList () {
            for (auto p : list) {
              delete p;
            }
          }

////////////////////////////////////////////////////////////////////////////////
/// @brief get number of plans contained
////////////////////////////////////////////////////////////////////////////////

          size_t size () const {
            return list.size();
          }

////////////////////////////////////////////////////////////////////////////////
/// @brief check if empty
////////////////////////////////////////////////////////////////////////////////

          bool empty () const {
            return list.empty();
          }

////////////////////////////////////////////////////////////////////////////////
/// @brief pop the first one
////////////////////////////////////////////////////////////////////////////////

          ExecutionPlan* pop_front (int& levelDoneOut) {
            auto p = list.front();
            levelDoneOut = levelDone.front();
            list.pop_front();
            levelDone.pop_front();
            return p;
          }

////////////////////////////////////////////////////////////////////////////////
/// @brief push_back
////////////////////////////////////////////////////////////////////////////////

          void push_back (ExecutionPlan* p, int level) {
            list.push_back(p);
            try {
              levelDone.push_back(level);
            }
            catch (...) {
              list.pop_back();
              throw;
            }
          }

////////////////////////////////////////////////////////////////////////////////
/// @brief steals all the plans in b and clears b at the same time
////////////////////////////////////////////////////////////////////////////////

          void steal (PlanList& b) {
            list.swap(b.list);
            levelDone.swap(b.levelDone);
            for (auto p : b.list) {
              delete p;
            }
            b.list.clear();
            b.levelDone.clear();
          }

////////////////////////////////////////////////////////////////////////////////
/// @brief appends all the plans to the target and clears *this at the same time
////////////////////////////////////////////////////////////////////////////////

          void appendTo (PlanList& target) {
            while (list.size() > 0) {
              auto p = list.front();
              int level = levelDone.front();
              list.pop_front();
              levelDone.pop_front();
              try {
                target.push_back(p, level);
              }
              catch (...) {
                delete p;
                throw;
              }
            }
          }

////////////////////////////////////////////////////////////////////////////////
/// @brief clear, deletes all plans contained
////////////////////////////////////////////////////////////////////////////////

          void clear () {
            for (auto p : list) {
              delete p;
            }
            list.clear();
            levelDone.clear();
          }

        };

////////////////////////////////////////////////////////////////////////////////
/// @brief type of an optimizer rule function, the function gets an
/// optimiser, an ExecutionPlan, the current level of this rule and
/// has to append one or more plans to the resulting deque. This must
/// include the original plan if it ought to be kept. The rule has to
/// set the level of the appended plan to the largest level of rule
/// that ought to be considered as done to indicate which rule is to be
/// applied next.
////////////////////////////////////////////////////////////////////////////////

        typedef std::function<int(Optimizer* opt, 
                                  ExecutionPlan* plan,
                                  int level,
                                  PlanList& out)>
                RuleFunction;

////////////////////////////////////////////////////////////////////////////////
/// @brief type of an optimizer rule
////////////////////////////////////////////////////////////////////////////////

        struct Rule {
          std::string name;
          RuleFunction func;
          int level;

          Rule (std::string const& name, RuleFunction f, int l)
            : name(name), func(f), level(l) {
          }

////////////////////////////////////////////////////////////////////////////////
/// @brief operator< for two rules, used to stable_sort by pass 
////////////////////////////////////////////////////////////////////////////////

          bool operator< (Rule const& b) const {
            return level < b.level;
          }

        };

////////////////////////////////////////////////////////////////////////////////
/// @brief maximal number of plans to produce:
////////////////////////////////////////////////////////////////////////////////

        static int const maxNumberOfPlans = 1000;

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor, this will initialize the rules database
////////////////////////////////////////////////////////////////////////////////

        Optimizer ();   // the .cpp file includes Aql/OptimizerRules.h
                        // and add all methods there to the rules database

////////////////////////////////////////////////////////////////////////////////
/// @brief destructor
////////////////////////////////////////////////////////////////////////////////

        ~Optimizer () {
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief do the optimization, this does the optimization, the resulting
/// plans are all estimated, sorted by that estimate and can then be got
/// by getPlans, until the next initialize is called. Note that the optimizer
/// object takes ownership of the execution plan and will delete it 
/// automatically on destruction. It will also have ownership of all the
/// newly created plans it recalls and will automatically delete them.
/// If you need to extract the plans from the optimizer use stealBest or
/// stealPlans.
////////////////////////////////////////////////////////////////////////////////

        int createPlans (ExecutionPlan* p);

////////////////////////////////////////////////////////////////////////////////
/// @brief getBest, ownership of the plan remains with the optimizer
////////////////////////////////////////////////////////////////////////////////

        ExecutionPlan* getBest () {
          if (_plans.empty()) {
            return nullptr;
          }
          return _plans.list.front();
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief getPlans, ownership of the plans remains with the optimizer
////////////////////////////////////////////////////////////////////////////////

        std::deque<ExecutionPlan*>& getPlans () {
          return _plans.list;
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief stealBest, ownership of the plan is handed over to the caller,
/// all other plans are deleted
////////////////////////////////////////////////////////////////////////////////

        ExecutionPlan* stealBest () {
          if (_plans.empty()) {
            return nullptr;
          }
          auto res = _plans.list.front();
          for (size_t i = 1; i < _plans.size(); i++) {
            delete _plans.list[i];
          }
          _plans.list.clear();
          _plans.levelDone.clear();

          std::cout << res->toJson(TRI_UNKNOWN_MEM_ZONE, false).toString() << "\n";
          return res;
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief stealPlans, ownership of the plans is handed over to the caller,
/// the optimizer will forget about them!
////////////////////////////////////////////////////////////////////////////////

        std::deque<ExecutionPlan*> stealPlans () {
          std::deque<ExecutionPlan*> res;
          res.swap(_plans.list);
          _plans.levelDone.clear();
          return res;
        }

// -----------------------------------------------------------------------------
// --SECTION--                                                   private methods
// -----------------------------------------------------------------------------

      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief estimatePlans
////////////////////////////////////////////////////////////////////////////////

        void estimatePlans ();

////////////////////////////////////////////////////////////////////////////////
/// @brief sortPlans
////////////////////////////////////////////////////////////////////////////////

        void sortPlans ();

////////////////////////////////////////////////////////////////////////////////
/// @brief registerRule
////////////////////////////////////////////////////////////////////////////////

        static void registerRule (std::string const& name, 
                                  RuleFunction f, 
                                  int level) {
          _rules.emplace_back(name, f, level);
        }

////////////////////////////////////////////////////////////////////////////////
/// @brief set up the optimizer rules once and forever
////////////////////////////////////////////////////////////////////////////////

        static void setupRules ();

// -----------------------------------------------------------------------------
// --SECTION--                                                   private members
// -----------------------------------------------------------------------------
      
      private:

////////////////////////////////////////////////////////////////////////////////
/// @brief the rules database
////////////////////////////////////////////////////////////////////////////////

        static std::vector<Rule> _rules;

////////////////////////////////////////////////////////////////////////////////
/// @brief the current set of plans to be optimised
////////////////////////////////////////////////////////////////////////////////

        PlanList _plans;

    };

  }  // namespace aql
}  // namespace triagens
#endif

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|// --SECTION--\\|/// @\\}\\)"
// End:

