/*jslint indent: 2, nomen: true, maxlen: 200, sloppy: true, vars: true, white: true, plusplus: true */
/*global require, exports, assertTrue, assertEqual, AQL_EXECUTE, AQL_EXPLAIN, fail, loopmax */
////////////////////////////////////////////////////////////////////////////////
/// @brief tests for optimizer rules
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2010-2012 triagens GmbH, Cologne, Germany
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
/// @author Jan Steemann
/// @author Copyright 2012, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

var jsunity = require("jsunity");
var errors = require("internal").errors;
var helper = require("org/arangodb/aql-helper");
var getQueryResults = helper.getQueryResults2;
var assertQueryError = helper.assertQueryError2;
var isEqual = helper.isEqual;

////////////////////////////////////////////////////////////////////////////////
/// @brief test suite
////////////////////////////////////////////////////////////////////////////////

function optimizerRuleTestSuite () {
  var ruleName = "remove-redundant-sorts";

  // various choices to control the optimizer: 
  var paramNone     = { optimizer: { rules: [ "-all" ] } };
  var paramEnabled  = { optimizer: { rules: [ "-all", "+" + ruleName ] } };
  var paramDisabled = { optimizer: { rules: [ "+all", "-" + ruleName ] } };

  return {

////////////////////////////////////////////////////////////////////////////////
/// @brief set up
////////////////////////////////////////////////////////////////////////////////

    setUp : function () {
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief tear down
////////////////////////////////////////////////////////////////////////////////

    tearDown : function () {
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test that rule has no effect when explicitly disabled
////////////////////////////////////////////////////////////////////////////////

    testRuleDisabled : function () {
      var queries = [
        "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a SORT i.a RETURN i",
        "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a FILTER i.a == 1 SORT i.a RETURN i",
        "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a LIMIT 1 SORT i.a RETURN i"
      ];

      queries.forEach(function(query) {
        var result = AQL_EXPLAIN(query, { }, paramNone);
        assertEqual([ ], result.plan.rules);
      });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test that rule has no effect
////////////////////////////////////////////////////////////////////////////////

    testRuleNoEffect : function () {
      var queries = [ 
        "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a RETURN i",
        "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a SORT i.b RETURN i",
        "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a SORT i.b RETURN i",
        "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a SORT i.c RETURN i"
      ];

      queries.forEach(function(query) {
        var result = AQL_EXPLAIN(query, { }, paramEnabled);
        assertEqual([ ], result.plan.rules, query);
      });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test that rule has an effect
////////////////////////////////////////////////////////////////////////////////

    testRuleHasEffect : function () {
      var queries = [ 
        "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a SORT i.a RETURN i",
        "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a, i.b SORT i.a, i.b RETURN i",
        "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a DESC SORT i.a DESC RETURN i",
        "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a, i.b DESC SORT i.a, i.b DESC RETURN i",
        "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a FILTER i.a == 1 SORT i.a RETURN i",
        "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a DESC FILTER i.a == 1 SORT i.a DESC RETURN i"
      ];

      queries.forEach(function(query) {
        var result = AQL_EXPLAIN(query, { }, paramEnabled);
          //require("internal").print(result);
        assertEqual([ ruleName ], result.plan.rules);
      });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test generated plans
////////////////////////////////////////////////////////////////////////////////


    testPlans : function () {
      var plans = [
      ]; 

      plans.forEach(function(plan) {
          var result = AQL_EXPLAIN(plan[0], { }, paramEnabled);
          assertEqual([ ruleName ], result.plan.rules, plan[0]);
          //require("internal").print(helper.getCompactPlan(result).map(function(node) { return node.type; }));
          assertEqual(plan[1], helper.getCompactPlan(result).map(function(node) { return node.type; }), plan[0]);
      });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test results
////////////////////////////////////////////////////////////////////////////////

    testResults : function () {
      var queries = [ 
        [ "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a SORT i.a RETURN i", [ { a: 1 }, { a: 2 }, { a: 3 } ] ],
        [ "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a DESC SORT i.a DESC RETURN i", [ { a: 3 }, { a: 2 }, { a: 1 } ] ],
        [ "FOR i IN [ { a: 1, b: 1 }, { a: 1, b: 2 }, { a: 2, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 1 }, { a: 3, b: 2 } ] SORT i.a, i.b ASC SORT i.a, i.b ASC RETURN i", [ { a: 1, b: 1 }, { a: 1, b: 2 }, { a: 2, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 1 }, { a: 3, b: 2 } ] ],
        [ "FOR i IN [ { a: 1, b: 1 }, { a: 1, b: 2 }, { a: 2, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 1 }, { a: 3, b: 2 } ] SORT i.a, i.b DESC SORT i.a, i.b DESC RETURN i", [ { a: 1, b: 2 }, { a: 1, b: 1 }, { a: 2, b: 2 }, { a: 2, b: 1 }, { a: 3, b: 2 }, { a: 3, b: 1 } ] ],
        [ "FOR i IN [ { a: 1, b: 1 }, { a: 1, b: 2 }, { a: 2, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 1 }, { a: 3, b: 2 } ] SORT i.a DESC, i.b DESC SORT i.a DESC, i.b DESC RETURN i", [ { a: 3, b: 2 }, { a: 3, b: 1 }, { a: 2, b: 2 }, { a: 2, b: 1 }, { a: 1, b: 2 }, { a: 1, b: 1 } ] ]
      ];

      // note: the optimizer currently cannot optimize these queries:
      // [ "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a DESC SORT i.a RETURN i", [ { a: 1 }, { a: 2 }, { a: 3 } ] ],
      // [ "FOR i IN [ { a: 1 }, { a: 2 }, { a: 3 } ] SORT i.a ASC SORT i.a DESC RETURN i", [ { a: 3 }, { a: 2 }, { a: 1 } ] ],
      // [ "FOR i IN [ { a: 1, b: 1 }, { a: 1, b: 2 }, { a: 2, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 1 }, { a: 3, b: 2 } ] SORT i.a, i.b DESC SORT i.a, i.b ASC RETURN i", [ { a: 1, b: 1 }, { a: 1, b: 2 }, { a: 2, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 1 }, { a: 3, b: 2 } ] ],
      // [ "FOR i IN [ { a: 1, b: 1 }, { a: 1, b: 2 }, { a: 2, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 1 }, { a: 3, b: 2 } ] SORT i.a, i.b ASC SORT i.a, i.b DESC RETURN i", [ { a: 1, b: 2 }, { a: 1, b: 1 }, { a: 2, b: 2 }, { a: 2, b: 1 }, { a: 3, b: 2 }, { a: 3, b: 1 } ] ],
      // [ "FOR i IN [ { a: 1, b: 1 }, { a: 1, b: 2 }, { a: 2, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 1 }, { a: 3, b: 2 } ] SORT i.a, i.b ASC SORT i.a DESC, i.b DESC RETURN i", [ { a: 3, b: 2 }, { a: 3, b: 1 }, { a: 2, b: 2 }, { a: 2, b: 1 }, { a: 1, b: 2 }, { a: 1, b: 1 } ] ],

      queries.forEach(function(query) {
        var planDisabled   = AQL_EXPLAIN(query[0], { }, paramDisabled);
        var planEnabled    = AQL_EXPLAIN(query[0], { }, paramEnabled);
        var resultDisabled = AQL_EXECUTE(query[0], { }, paramDisabled).json;
        var resultEnabled  = AQL_EXECUTE(query[0], { }, paramEnabled).json;

        assertTrue(isEqual(resultDisabled, resultEnabled), query[0]);

        assertTrue(planDisabled.plan.rules.indexOf(ruleName) === -1, query[0]);
        assertTrue(planEnabled.plan.rules.indexOf(ruleName) !== -1, query[0]);

        assertEqual(resultDisabled, query[1]);
        assertEqual(resultEnabled, query[1]);
      });
    }

  };
}

////////////////////////////////////////////////////////////////////////////////
/// @brief executes the test suite
////////////////////////////////////////////////////////////////////////////////

jsunity.run(optimizerRuleTestSuite);

return jsunity.done();

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// @addtogroup\\|// --SECTION--\\|/// @page\\|/// @}\\)"
// End: