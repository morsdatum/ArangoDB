/*jshint strict: false, sub: true, maxlen: 500 */
/*global require, assertEqual, assertFalse, assertNull */

////////////////////////////////////////////////////////////////////////////////
/// @brief tests for query language, bind parameters
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

var internal = require("internal");
var db = require("org/arangodb").db;
var jsunity = require("jsunity");
var helper = require("org/arangodb/aql-helper");
var cluster = require("org/arangodb/cluster");
var getModifyQueryResults = helper.getModifyQueryResults;
var assertQueryError = helper.assertQueryError;
  
var sanitizeStats = function (stats) {
  // remove these members from the stats because they don't matter
  // for the comparisons
  delete stats.scannedFull;
  delete stats.scannedIndex;
  return stats;
};

////////////////////////////////////////////////////////////////////////////////
/// @brief test suite
////////////////////////////////////////////////////////////////////////////////

function ahuacatlModifySuite () {
  var errors = internal.errors;
  var cn1 = "UnitTestsAhuacatlModify1";
  var cn2 = "UnitTestsAhuacatlModify2";
  var c1, c2;

  return {

////////////////////////////////////////////////////////////////////////////////
/// @brief set up
////////////////////////////////////////////////////////////////////////////////

    setUp : function () {
      db._drop(cn1);
      db._drop(cn2);
      c1 = db._create(cn1);
      c2 = db._create(cn2);

      c1.save({ _key: "foo", a: 1 });
      c2.save({ _key: "foo", b: 1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief tear down
////////////////////////////////////////////////////////////////////////////////

    tearDown : function () {
      db._drop(cn1);
      db._drop(cn2);
      c1 = null;
      c2 = null;
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test subquery
////////////////////////////////////////////////////////////////////////////////

    testRemoveInSubquery : function () {
      assertQueryError(errors.ERROR_QUERY_MODIFY_IN_SUBQUERY.code, "FOR d IN @@cn LET x = (REMOVE d.foobar IN @@cn) RETURN d", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test subquery
////////////////////////////////////////////////////////////////////////////////

    testInsertInSubquery : function () {
      assertQueryError(errors.ERROR_QUERY_MODIFY_IN_SUBQUERY.code, "FOR d IN @@cn LET x = (INSERT { _key: 'test' } IN @@cn) RETURN d", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test subquery
////////////////////////////////////////////////////////////////////////////////

    testUpdateInSubquery : function () {
      assertQueryError(errors.ERROR_QUERY_MODIFY_IN_SUBQUERY.code, "FOR d IN @@cn LET x = (UPDATE { _key: 'test' } IN @@cn) RETURN d", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test subquery
////////////////////////////////////////////////////////////////////////////////

    testReplaceInSubquery : function () {
      assertQueryError(errors.ERROR_QUERY_MODIFY_IN_SUBQUERY.code, "FOR d IN @@cn LET x = (REPLACE { _key: 'test' } IN @@cn) RETURN d", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test subquery
////////////////////////////////////////////////////////////////////////////////

    testReplaceInSubquery2 : function () {
      assertQueryError(errors.ERROR_QUERY_MODIFY_IN_SUBQUERY.code, "FOR d IN @@cn LET x = (FOR i IN 1..2 REPLACE { _key: 'test' } IN @@cn) RETURN d", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test subquery
////////////////////////////////////////////////////////////////////////////////

    testMultiModify : function () {
      assertQueryError(errors.ERROR_QUERY_PARSE.code, "FOR d IN @@cn1 REMOVE d IN @@cn1 FOR e IN @@cn2 REMOVE e IN @@cn2", { "@cn1": cn1, "@cn2": cn2 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test subquery
////////////////////////////////////////////////////////////////////////////////

    testMultiModify2 : function () {
      assertQueryError(errors.ERROR_QUERY_PARSE.code, "FOR d IN @@cn1 FOR e IN @@cn2 REMOVE d IN @@cn1 REMOVE e IN @@cn2", { "@cn1": cn1, "@cn2": cn2 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test subquery
////////////////////////////////////////////////////////////////////////////////

    testDynamicOptions1 : function () {
      assertQueryError(errors.ERROR_QUERY_COMPILE_TIME_OPTIONS.code, "FOR d IN @@cn REMOVE d IN @@cn OPTIONS { foo: d }", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test subquery
////////////////////////////////////////////////////////////////////////////////

    testDynamicOptions2 : function () {
      assertQueryError(errors.ERROR_QUERY_COMPILE_TIME_OPTIONS.code, "FOR d IN @@cn REMOVE d IN @@cn OPTIONS { foo: MERGE(d, { }) }", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test subquery
////////////////////////////////////////////////////////////////////////////////

    testInvalidOptions : function () {
      assertQueryError(errors.ERROR_QUERY_PARSE.code, "FOR d IN @@cn REMOVE d IN @@cn OPTIONS 'foo'", { "@cn": cn1 });
    },
  };
}

////////////////////////////////////////////////////////////////////////////////
/// @brief test suite
////////////////////////////////////////////////////////////////////////////////

function ahuacatlRemoveSuite () {
  var errors = internal.errors;
  var cn1 = "UnitTestsAhuacatlRemove1";
  var cn2 = "UnitTestsAhuacatlRemove2";
  var c1;
  var c2;

  return {

////////////////////////////////////////////////////////////////////////////////
/// @brief set up
////////////////////////////////////////////////////////////////////////////////

    setUp : function () {
      var i;
      db._drop(cn1);
      db._drop(cn2);
      c1 = db._create(cn1);
      c2 = db._create(cn2);

      for (i = 0; i < 100; ++i) {
        c1.save({ _key: "test" + i, value1: i, value2: "test" + i });
      }
      for (i = 0; i < 50; ++i) {
        c2.save({ _key: "test" + i, value1: i, value2: "test" + i });
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief tear down
////////////////////////////////////////////////////////////////////////////////

    tearDown : function () {
      db._drop(cn1);
      db._drop(cn2);
      c1 = null;
      c2 = null;
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveNothing : function () {
      var expected = { writesExecuted: 0, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN " + cn1 + " FILTER d.value1 < 0 REMOVE d IN " + cn1, {});
    
      assertEqual(100, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveNothingBind : function () {
      var expected = { writesExecuted: 0, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn FILTER d.value1 < 0 REMOVE d IN @@cn", { "@cn": cn1 });

      assertEqual(100, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveInvalid1 : function () {
      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_TYPE_INVALID.code, "FOR d IN @@cn REMOVE d.foobar IN @@cn", { "@cn": cn1 });
      assertEqual(100, c1.count());
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveInvalid2 : function () {
      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_KEY_MISSING.code, "FOR d IN @@cn REMOVE { } IN @@cn", { "@cn": cn1 });
      assertEqual(100, c1.count());
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveInvalid3 : function () {
      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_NOT_FOUND.code, "FOR d IN @@cn REMOVE 'foobar' IN @@cn", { "@cn": cn1 });
      assertEqual(100, c1.count());
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveInvalid4 : function () {
      if (cluster.isCluster()) {
        // skip test in cluster as there are no distributed transactions yet
        return;
      }

      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_NOT_FOUND.code, "FOR i iN 0..100 REMOVE CONCAT('test', TO_STRING(i)) IN @@cn", { "@cn": cn1 });
      assertEqual(100, c1.count());
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveIgnore1 : function () {
      var expected = { writesExecuted: 0, writesIgnored: 100 };
      var actual = getModifyQueryResults("FOR d IN @@cn REMOVE 'foo' IN @@cn OPTIONS { ignoreErrors: true }", { "@cn": cn1 });

      assertEqual(100, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveIgnore2 : function () {
      var expected = { writesExecuted: 100, writesIgnored: 1 };
      var actual = getModifyQueryResults("FOR i IN 0..100 REMOVE CONCAT('test', TO_STRING(i)) IN @@cn OPTIONS { ignoreErrors: true }", { "@cn": cn1 });

      assertEqual(0, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveAll1 : function () {
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn REMOVE d IN @@cn", { "@cn": cn1 });

      assertEqual(0, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveAll2 : function () {
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn REMOVE d._key IN @@cn", { "@cn": cn1 });

      assertEqual(0, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveAll3 : function () {
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn REMOVE { _key: d._key } IN @@cn", { "@cn": cn1 });

      assertEqual(0, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveAll4 : function () {
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR i IN 0..99 REMOVE { _key: CONCAT('test', TO_STRING(i)) } IN @@cn", { "@cn": cn1 });

      assertEqual(0, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveAll5 : function () {
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn REMOVE d INTO @@cn", { "@cn": cn1 });

      assertEqual(0, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveHalf : function () {
      var expected = { writesExecuted: 50, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR i IN 0..99 FILTER i % 2 == 0 REMOVE { _key: CONCAT('test', TO_STRING(i)) } IN @@cn", { "@cn": cn1 });

      assertEqual(50, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testSingleNotFound : function () {
      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_NOT_FOUND.code, "REMOVE 'foobar' IN @@cn", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testSingle : function () {
      var expected = { writesExecuted: 1, writesIgnored: 0 };
      var actual = getModifyQueryResults("REMOVE 'test0' IN @@cn", { "@cn": cn1 });

      assertEqual(99, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testTwoCollectionsNotFound : function () {
      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_NOT_FOUND.code, "FOR d IN @@cn1 REMOVE { _key: d._key } IN @@cn2", { "@cn1": cn1, "@cn2": cn2 });

      assertEqual(100, c1.count());
      assertEqual(50, c2.count());
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testTwoCollectionsJoin1 : function () {
      var expected = { writesExecuted: 50, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn1 FILTER d.value1 < 50 REMOVE { _key: d._key } IN @@cn2", { "@cn1": cn1, "@cn2": cn2 });

      assertEqual(100, c1.count());
      assertEqual(0, c2.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testTwoCollectionsJoin2 : function () {
      var expected = { writesExecuted: 48, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn1 FILTER d.value1 >= 2 && d.value1 < 50 REMOVE { _key: d._key } IN @@cn2", { "@cn1": cn1, "@cn2": cn2 });

      assertEqual(100, c1.count());
      assertEqual(2, c2.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testTwoCollectionsIgnoreErrors1 : function () {
      var expected = { writesExecuted: 50, writesIgnored: 50 };
      var actual = getModifyQueryResults("FOR d IN @@cn1 REMOVE { _key: d._key } IN @@cn2 OPTIONS { ignoreErrors: true }", { "@cn1": cn1, "@cn2": cn2 });

      assertEqual(100, c1.count());
      assertEqual(0, c2.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testTwoCollectionsIgnoreErrors2 : function () {
      var expected = { writesExecuted: 0, writesIgnored: 100 };
      var actual = getModifyQueryResults("FOR d IN @@cn1 REMOVE { _key: CONCAT('foo', d._key) } IN @@cn2 OPTIONS { ignoreErrors: true }", { "@cn1": cn1, "@cn2": cn2 });

      assertEqual(100, c1.count());
      assertEqual(50, c2.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveWaitForSync : function () {
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn REMOVE d IN @@cn OPTIONS { waitForSync: true }", { "@cn": cn1 });

      assertEqual(0, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test remove
////////////////////////////////////////////////////////////////////////////////

    testRemoveEdge : function () {
      db._drop("UnitTestsAhuacatlEdge");
      var edge = db._createEdgeCollection("UnitTestsAhuacatlEdge"); 

      for (var i = 0; i < 100; ++i) {
        edge.save("UnitTestsAhuacatlRemove1/foo" + i, "UnitTestsAhuacatlRemove2/bar", { what: i, _key: "test" + i });
      }
      var expected = { writesExecuted: 10, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR i IN 0..9 REMOVE CONCAT('test', TO_STRING(i)) IN @@cn", { "@cn": edge.name() });

      assertEqual(100, c1.count());
      assertEqual(90, edge.count());
      assertEqual(expected, sanitizeStats(actual));
      db._drop("UnitTestsAhuacatlEdge");
    }

  };
}

////////////////////////////////////////////////////////////////////////////////
/// @brief test suite
////////////////////////////////////////////////////////////////////////////////

function ahuacatlInsertSuite () {
  var errors = internal.errors;
  var cn1 = "UnitTestsAhuacatlInsert1";
  var cn2 = "UnitTestsAhuacatlInsert2";
  var c1;
  var c2;

  return {

////////////////////////////////////////////////////////////////////////////////
/// @brief set up
////////////////////////////////////////////////////////////////////////////////

    setUp : function () {
      var i;
      db._drop(cn1);
      db._drop(cn2);
      c1 = db._create(cn1);
      c2 = db._create(cn2);

      for (i = 0; i < 100; ++i) {
        c1.save({ _key: "test" + i, value1: i, value2: "test" + i });
      }
      for (i = 0; i < 50; ++i) {
        c2.save({ _key: "test" + i, value1: i, value2: "test" + i });
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief tear down
////////////////////////////////////////////////////////////////////////////////

    tearDown : function () {
      db._drop(cn1);
      db._drop(cn2);
      c1 = null;
      c2 = null;
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertNothing : function () {
      var expected = { writesExecuted: 0, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN " + cn1 + " FILTER d.value1 < 0 INSERT { foxx: true } IN " + cn1, {});
    
      assertEqual(100, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertNothingBind : function () {
      var expected = { writesExecuted: 0, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn FILTER d.value1 < 0 INSERT { foxx: true } IN @@cn", { "@cn": cn1 });

      assertEqual(100, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertInvalid1 : function () {
      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_TYPE_INVALID.code, "FOR d IN @@cn INSERT d.foobar IN @@cn", { "@cn": cn1 });
      assertEqual(100, c1.count());
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertInvalid2 : function () {
      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_TYPE_INVALID.code, "FOR d IN @@cn INSERT [ ] IN @@cn", { "@cn": cn1 });
      assertEqual(100, c1.count());
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertInvalid3 : function () {
      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_TYPE_INVALID.code, "FOR d IN @@cn INSERT 'foo' IN @@cn", { "@cn": cn1 });
      assertEqual(100, c1.count());
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertUniqueConstraint1 : function () {
      assertQueryError(errors.ERROR_ARANGO_UNIQUE_CONSTRAINT_VIOLATED.code, "FOR d IN @@cn INSERT d IN @@cn", { "@cn": cn1 });
      assertEqual(100, c1.count());
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertUniqueConstraint2 : function () {
      assertQueryError(errors.ERROR_ARANGO_UNIQUE_CONSTRAINT_VIOLATED.code, "FOR i IN 0..100 INSERT { _key: CONCAT('test', TO_STRING(i)) } IN @@cn", { "@cn": cn2 });
      assertEqual(50, c2.count());
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertUniqueConstraint3 : function () {
      assertQueryError(errors.ERROR_ARANGO_UNIQUE_CONSTRAINT_VIOLATED.code, "FOR i IN 0..50 INSERT { _key: 'foo' } IN @@cn", { "@cn": cn1 });
      assertEqual(100, c1.count());
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertIgnore1 : function () {
      var expected = { writesExecuted: 0, writesIgnored: 100 };
      var actual = getModifyQueryResults("FOR d IN @@cn INSERT d IN @@cn OPTIONS { ignoreErrors: true }", { "@cn": cn1 });

      assertEqual(100, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertIgnore2 : function () {
      var expected = { writesExecuted: 1, writesIgnored: 50 };
      var actual = getModifyQueryResults("FOR i IN 50..100 INSERT { _key: CONCAT('test', TO_STRING(i)) } IN @@cn OPTIONS { ignoreErrors: true }", { "@cn": cn1 });

      assertEqual(101, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertIgnore3 : function () {
      var expected = { writesExecuted: 51, writesIgnored: 50 };
      var actual = getModifyQueryResults("FOR i IN 0..100 INSERT { _key: CONCAT('test', TO_STRING(i)) } IN @@cn OPTIONS { ignoreErrors: true }", { "@cn": cn2 });

      assertEqual(101, c2.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertIgnore4 : function () {
      var expected = { writesExecuted: 0, writesIgnored: 100 };
      var actual = getModifyQueryResults("FOR i IN 0..99 INSERT { _key: CONCAT('test', TO_STRING(i)) } IN @@cn OPTIONS { ignoreErrors: true }", { "@cn": cn1 });

      assertEqual(100, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertIgnore5 : function () {
      var expected = { writesExecuted: 50, writesIgnored: 50 };
      var actual = getModifyQueryResults("FOR i IN 0..99 INSERT { _key: CONCAT('test', TO_STRING(i)) } IN @@cn OPTIONS { ignoreErrors: true }", { "@cn": cn2 });

      assertEqual(100, c2.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertEmpty : function () {
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn INSERT { } IN @@cn", { "@cn": cn1 });

      assertEqual(200, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertCopy : function () {
      var expected = { writesExecuted: 50, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR i IN 50..99 INSERT { _key: CONCAT('test', TO_STRING(i)) } IN @@cn", { "@cn": cn2 });

      assertEqual(100, c1.count());
      assertEqual(100, c2.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testSingle : function () {
      var expected = { writesExecuted: 1, writesIgnored: 0 };
      var actual = getModifyQueryResults("INSERT { value: 'foobar', _key: 'test' } IN @@cn", { "@cn": cn1 });

      assertEqual(101, c1.count());
      assertEqual("foobar", c1.document("test").value);
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertWaitForSync : function () {
      var expected = { writesExecuted: 50, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR i IN 1..50 INSERT { value: i } INTO @@cn OPTIONS { waitForSync: true }", { "@cn": cn2 });

      assertEqual(100, c1.count());
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertEdgeInvalid : function () {
      db._drop("UnitTestsAhuacatlEdge");
      var edge = db._createEdgeCollection("UnitTestsAhuacatlEdge"); 

      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_HANDLE_BAD.code, "FOR i IN 1..50 INSERT { } INTO @@cn", { "@cn": edge.name() });
      assertEqual(0, edge.count());

      db._drop("UnitTestsAhuacatlEdge");
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertEdgeNoFrom : function () {
      db._drop("UnitTestsAhuacatlEdge");
      var edge = db._createEdgeCollection("UnitTestsAhuacatlEdge"); 

      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_HANDLE_BAD.code, "FOR i IN 1..50 INSERT { _to: CONCAT('UnitTestsAhuacatlInsert1/', TO_STRING(i)) } INTO @@cn", { "@cn": edge.name() });
      assertEqual(0, edge.count());

      db._drop("UnitTestsAhuacatlEdge");
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertEdgeNoTo : function () {
      db._drop("UnitTestsAhuacatlEdge");
      var edge = db._createEdgeCollection("UnitTestsAhuacatlEdge"); 

      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_HANDLE_BAD.code, "FOR i IN 1..50 INSERT { _from: CONCAT('UnitTestsAhuacatlInsert1/', TO_STRING(i)) } INTO @@cn", { "@cn": edge.name() });
      assertEqual(0, edge.count());

      db._drop("UnitTestsAhuacatlEdge");
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test insert
////////////////////////////////////////////////////////////////////////////////

    testInsertEdge : function () {
      db._drop("UnitTestsAhuacatlEdge");
      var edge = db._createEdgeCollection("UnitTestsAhuacatlEdge"); 

      var expected = { writesExecuted: 50, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR i IN 1..50 INSERT { _key: CONCAT('test', TO_STRING(i)), _from: CONCAT('UnitTestsAhuacatlInsert1/', TO_STRING(i)), _to: CONCAT('UnitTestsAhuacatlInsert2/', TO_STRING(i)), value: [ i ], sub: { foo: 'bar' } } INTO @@cn", { "@cn": edge.name() });

      assertEqual(expected, sanitizeStats(actual));
      assertEqual(50, edge.count());

      for (var i = 1; i <= 50; ++i) {
        var doc = edge.document("test" + i);
        assertEqual("UnitTestsAhuacatlInsert1/" + i, doc._from);
        assertEqual("UnitTestsAhuacatlInsert2/" + i, doc._to);
        assertEqual([ i ], doc.value);
        assertEqual({ foo: "bar" }, doc.sub);
      }
      db._drop("UnitTestsAhuacatlEdge");
    }

  };
}

////////////////////////////////////////////////////////////////////////////////
/// @brief test suite
////////////////////////////////////////////////////////////////////////////////

function ahuacatlUpdateSuite () {
  var errors = internal.errors;
  var cn1 = "UnitTestsAhuacatlUpdate1";
  var cn2 = "UnitTestsAhuacatlUpdate2";
  var c1;
  var c2;

  return {

////////////////////////////////////////////////////////////////////////////////
/// @brief set up
////////////////////////////////////////////////////////////////////////////////

    setUp : function () {
      var i;
      db._drop(cn1);
      db._drop(cn2);
      c1 = db._create(cn1);
      c2 = db._create(cn2);

      for (i = 0; i < 100; ++i) {
        c1.save({ _key: "test" + i, value1: i, value2: "test" + i });
      }
      for (i = 0; i < 50; ++i) {
        c2.save({ _key: "test" + i, value1: i, value2: "test" + i });
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief tear down
////////////////////////////////////////////////////////////////////////////////

    tearDown : function () {
      db._drop(cn1);
      db._drop(cn2);
      c1 = null;
      c2 = null;
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateNothing : function () {
      var expected = { writesExecuted: 0, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN " + cn1 + " FILTER d.value1 < 0 UPDATE { foxx: true } IN " + cn1, {});
    
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateNothingBind : function () {
      var expected = { writesExecuted: 0, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn FILTER d.value1 < 0 UPDATE { foxx: true } IN @@cn", { "@cn": cn1 });

      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateInvalid1 : function () {
      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_TYPE_INVALID.code, "FOR d IN @@cn UPDATE d.foobar IN @@cn", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateInvalid2 : function () {
      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_TYPE_INVALID.code, "FOR d IN @@cn UPDATE [ ] IN @@cn", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateInvalid3 : function () {
      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_TYPE_INVALID.code, "FOR d IN @@cn UPDATE 'foo' IN @@cn", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateUniqueConstraint1 : function () {
      c1.ensureUniqueConstraint("value1");
      assertQueryError(errors.ERROR_ARANGO_UNIQUE_CONSTRAINT_VIOLATED.code, "FOR d IN @@cn UPDATE d._key WITH { value1: 1 } IN @@cn", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateUniqueConstraint2 : function () {
      c1.ensureUniqueConstraint("value3");
      assertQueryError(errors.ERROR_ARANGO_UNIQUE_CONSTRAINT_VIOLATED.code, "FOR d IN @@cn UPDATE d._key WITH { value3: 1 } IN @@cn", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateIgnore1 : function () {
      c1.ensureUniqueConstraint("value3");
      var expected = { writesExecuted: 1, writesIgnored: 99 };
      var actual = getModifyQueryResults("FOR d IN @@cn UPDATE d WITH { value3: 1 } IN @@cn OPTIONS { ignoreErrors: true }", { "@cn": cn1 });

      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateIgnore2 : function () {
      c1.ensureUniqueConstraint("value1");
      var expected = { writesExecuted: 0, writesIgnored: 51 };
      var actual = getModifyQueryResults("FOR i IN 50..100 UPDATE { _key: CONCAT('test', TO_STRING(i)), value1: 1 } IN @@cn OPTIONS { ignoreErrors: true }", { "@cn": cn1 });

      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateEmpty1 : function () {
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn UPDATE { _key: d._key } IN @@cn", { "@cn": cn1 });

      assertEqual(expected, sanitizeStats(actual));
      for (var i = 0; i < 100; ++i) {
        var doc = c1.document("test" + i);
        assertEqual(i, doc.value1);
        assertEqual("test" + i, doc.value2);
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateEmpty2 : function () {
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn UPDATE d IN @@cn", { "@cn": cn1 });

      assertEqual(expected, sanitizeStats(actual));
      for (var i = 0; i < 100; ++i) {
        var doc = c1.document("test" + i);
        assertEqual(i, doc.value1);
        assertEqual("test" + i, doc.value2);
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testSingleNotFound : function () {
      assertQueryError(errors.ERROR_ARANGO_DOCUMENT_NOT_FOUND.code, "UPDATE { _key: 'foobar' } WITH { value1: 1 } IN @@cn", { "@cn": cn1 });
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testSingle : function () {
      var expected = { writesExecuted: 1, writesIgnored: 0 };
      var actual = getModifyQueryResults("UPDATE { value: 'foobar', _key: 'test17' } IN @@cn", { "@cn": cn1 });

      assertEqual("foobar", c1.document("test17").value);
      assertEqual(expected, sanitizeStats(actual));
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateOldValue : function () {
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn UPDATE { _key: d._key, value1: d.value2, value2: d.value1, value3: d.value1 + 5 } IN @@cn", { "@cn": cn1 });
      assertEqual(expected, sanitizeStats(actual));

      for (var i = 0; i < 100; ++i) {
        var doc = c1.document("test" + i);
        assertEqual("test" + i, doc.value1);
        assertEqual(i, doc.value2);
        assertEqual(i + 5, doc.value3);
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateWaitForSync : function () {
      var expected = { writesExecuted: 50, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR i IN 1..50 UPDATE { _key: CONCAT('test', TO_STRING(i)) } INTO @@cn OPTIONS { waitForSync: true }", { "@cn": cn1 });
      assertEqual(expected, sanitizeStats(actual));

      for (var i = 0; i < 100; ++i) {
        var doc = c1.document("test" + i);
        assertEqual(i, doc.value1);
        assertEqual("test" + i, doc.value2);
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateKeepNullDefault : function () {
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn UPDATE d._key WITH { value1: null, value3: 'foobar', value9: null } INTO @@cn", { "@cn": cn1 });
      assertEqual(expected, sanitizeStats(actual));

      for (var i = 0; i < 100; ++i) {
        var doc = c1.document("test" + i);
        assertNull(doc.value1);
        assertEqual("test" + i, doc.value2);
        assertEqual("foobar", doc.value3);
        assertNull(doc.value9);
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateKeepNullTrue : function () {
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn UPDATE d._key WITH { value1: null, value3: 'foobar', value9: null } INTO @@cn OPTIONS { keepNull: true }", { "@cn": cn1 });
      assertEqual(expected, sanitizeStats(actual));

      for (var i = 0; i < 100; ++i) {
        var doc = c1.document("test" + i);
        assertNull(doc.value1);
        assertEqual("test" + i, doc.value2);
        assertEqual("foobar", doc.value3);
        assertNull(doc.value9);
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateKeepNullFalse : function () {
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn UPDATE d._key WITH { value1: null, value3: 'foobar', value9: null } INTO @@cn OPTIONS { keepNull: false }", { "@cn": cn1 });
      assertEqual(expected, sanitizeStats(actual));

      for (var i = 0; i < 100; ++i) {
        var doc = c1.document("test" + i);
        assertFalse(doc.hasOwnProperty("value1"));
        assertEqual("test" + i, doc.value2);
        assertEqual("foobar", doc.value3);
        assertFalse(doc.hasOwnProperty("value9"));
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update mergeArrays
////////////////////////////////////////////////////////////////////////////////

    testUpdateMergeObjectsDefault : function () {
      c1.save({ _key: "something", values: { foo: 1, bar: 2, baz: 3 } });
      var expected = { writesExecuted: 1, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn FILTER d._key == 'something' UPDATE d._key WITH { values: { bar: 42, bumm: 23 } } INTO @@cn", { "@cn": cn1 });
      assertEqual(expected, sanitizeStats(actual));

      var doc = c1.document("something");
      assertEqual({ foo: 1, bar: 42, baz: 3, bumm : 23 }, doc.values);
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update mergeArrays
////////////////////////////////////////////////////////////////////////////////

    testUpdateMergeObjectsTrue : function () {
      c1.save({ _key: "something", values: { foo: 1, bar: 2, baz: 3 } });
      var expected = { writesExecuted: 1, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn FILTER d._key == 'something' UPDATE d._key WITH { values: { bar: 42, bumm: 23 } } INTO @@cn OPTIONS { mergeArrays: true }", { "@cn": cn1 });
      assertEqual(expected, sanitizeStats(actual));
      
      var doc = c1.document("something");
      assertEqual({ foo: 1, bar: 42, baz: 3, bumm : 23 }, doc.values);
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update mergeArrays
////////////////////////////////////////////////////////////////////////////////

    testUpdateMergeObjectsFalse : function () {
      c1.save({ _key: "something", values: { foo: 1, bar: 2, baz: 3 } });
      var expected = { writesExecuted: 1, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn FILTER d._key == 'something' UPDATE d._key WITH { values: { bar: 42, bumm: 23 } } INTO @@cn OPTIONS { mergeArrays: false }", { "@cn": cn1 });
      assertEqual(expected, sanitizeStats(actual));
      
      var doc = c1.document("something");
      assertEqual({ bar: 42, bumm: 23 }, doc.values);
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateFilter : function () {
      var expected = { writesExecuted: 50, writesIgnored: 0 };
      var actual = getModifyQueryResults("FOR d IN @@cn FILTER d.value1 % 2 == 0 UPDATE d._key WITH { value2: 100 } INTO @@cn", { "@cn": cn1 });
      assertEqual(expected, sanitizeStats(actual));

      for (var i = 0; i < 100; ++i) {
        var doc = c1.document("test" + i);
        if (i % 2 === 0) {
          assertEqual(100, doc.value2);
        }
        else {
          assertEqual("test" + i, doc.value2);
        }
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test update
////////////////////////////////////////////////////////////////////////////////

    testUpdateUpdate : function () {
      var i;
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      for (i = 0; i < 5; ++i) {
        var actual = getModifyQueryResults("FOR d IN @@cn UPDATE d._key WITH { counter: HAS(d, 'counter') ? d.counter + 1 : 1 } INTO @@cn", { "@cn": cn1 });
        assertEqual(expected, sanitizeStats(actual));
      }

      for (i = 0; i < 100; ++i) {
        var doc = c1.document("test" + i);
        assertEqual(5, doc.counter);
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test replace
////////////////////////////////////////////////////////////////////////////////

    testReplace1 : function () {
      var i;
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      for (i = 0; i < 5; ++i) {
        var actual = getModifyQueryResults("FOR d IN @@cn REPLACE d._key WITH { value4: 12 } INTO @@cn", { "@cn": cn1 });
        assertEqual(expected, sanitizeStats(actual));
      }

      for (i = 0; i < 100; ++i) {
        var doc = c1.document("test" + i);
        assertFalse(doc.hasOwnProperty("value1"));
        assertFalse(doc.hasOwnProperty("value2"));
        assertFalse(doc.hasOwnProperty("value3"));
        assertEqual(12, doc.value4);
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test replace
////////////////////////////////////////////////////////////////////////////////

    testReplace2 : function () {
      var i;
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      for (i = 0; i < 5; ++i) {
        var actual = getModifyQueryResults("FOR d IN @@cn REPLACE { _key: d._key, value4: 13 } INTO @@cn", { "@cn": cn1 });
        assertEqual(expected, sanitizeStats(actual));
      }

      for (i = 0; i < 100; ++i) {
        var doc = c1.document("test" + i);
        assertFalse(doc.hasOwnProperty("value1"));
        assertFalse(doc.hasOwnProperty("value2"));
        assertFalse(doc.hasOwnProperty("value3"));
        assertEqual(13, doc.value4);
      }
    },

////////////////////////////////////////////////////////////////////////////////
/// @brief test replace
////////////////////////////////////////////////////////////////////////////////

    testReplaceReplace : function () {
      var i;
      var expected = { writesExecuted: 100, writesIgnored: 0 };
      for (i = 0; i < 5; ++i) {
        var actual = getModifyQueryResults("FOR d IN @@cn REPLACE d._key WITH { value1: d.value1 + 1 } INTO @@cn", { "@cn": cn1 });
        assertEqual(expected, sanitizeStats(actual));
      }

      for (i = 0; i < 100; ++i) {
        var doc = c1.document("test" + i);
        assertEqual(i + 5, doc.value1);
        assertFalse(doc.hasOwnProperty("value2"));
      }
    }

  };
}

////////////////////////////////////////////////////////////////////////////////
/// @brief executes the test suites
////////////////////////////////////////////////////////////////////////////////

jsunity.run(ahuacatlModifySuite);
jsunity.run(ahuacatlRemoveSuite);
jsunity.run(ahuacatlInsertSuite);
jsunity.run(ahuacatlUpdateSuite);

return jsunity.done();

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// @addtogroup\\|// --SECTION--\\|/// @page\\|/// @}\\)"
// End:
