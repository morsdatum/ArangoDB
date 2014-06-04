!CHAPTER QUERIES 

`all()`

Selects all documents of a collection and returns a cursor. You can use toArray, next, or hasNext to access the result. The result can be limited using the skip and limit operator.

*Examples*

Use toArray to get all documents at once:

  arango> db.five.all().toArray();
  [
    { "_id" : "five/1798296", "_key" : "1798296", "_rev" : "1798296", "doc" : 3 },
    { "_id" : "five/1732760", "_key" : "1732760", "_rev" : "1732760", "doc" : 2 },
    { "_id" : "five/1863832", "_key" : "1863832", "_rev" : "1863832", "doc" : 4 },
    { "_id" : "five/1667224", "_key" : "1667224", "_rev" : "1667224", "doc" : 1 },
    { "_id" : "five/1929368", "_key" : "1929368", "_rev" : "1929368", "doc" : 5 }
  ]

Use next to loop over all documents:

  arango> var a = db.five.all();
  arango> while (a.hasNext()) print(a.next());
  { "_id" : "five/1798296", "_key" : "1798296", "_rev" : "1798296", "doc" : 3 }
  { "_id" : "five/1732760", "_key" : "1732760", "_rev" : "1732760", "doc" : 2 }
  { "_id" : "five/1863832", "_key" : "1863832", "_rev" : "1863832", "doc" : 4 }
  { "_id" : "five/1667224", "_key" : "1667224", "_rev" : "1667224", "doc" : 1 }
  { "_id" : "five/1929368", "_key" : "1929368", "_rev" : "1929368", "doc" : 5 }

`collection.byExample( example)`

Selects all documents of a collection that match the specified example and returns a cursor.

You can use toArray, next, or hasNext to access the result. The result can be limited using the skip and limit operator.

An attribute name of the form a.b is interpreted as attribute path, not as attribute. If you use

  { a : { c : 1 } }

as example, then you will find all documents, such that the attribute a contains a document of the form {c : 1 }. E.g., the document

  { a : { c : 1 }, b : 1 }

will match, but the document

  { a : { c : 1, b : 1 } }

will not.

However, if you use

  { a.c : 1 },

then you will find all documents, which contain a sub-document in a that has an attribute c of value 1. E.g., both documents

  { a : { c : 1 }, b : 1 } and
  { a : { c : 1, b : 1 } }

will match.

`collection.byExample( path1, value1, ...)`

As alternative you can supply a list of paths and values.

*Examples*

Use toArray to get all documents at once:

  arango> db.users.all().toArray();
  [ { "_id" : "users/554702285", "_key" : "554702285", "_rev" : "554702285", "id" : 323, "name" : "Peter" },
    { "_id" : "users/554636749", "_key" : "554636749", "_rev" : "554636749", "id" : 535, "name" : "Peter" },
    { "_id" : "users/554833357", "_key" : "554833357", "_rev" : "554833357", "id" : 25, "name" : "Vladimir" } ]

    arango> db.users.byExample({ "id" : 323 }).toArray();
    [ { "id" : 323, "name" : "Peter", "_id" : "users/554702285", "_key" : "554702285", "_rev" : "554702285" } ]

    arango> db.users.byExample({ "name" : "Peter" }).toArray();
    [ { "id" : 323, "name" : "Peter", "_id" : "users/554702285", "_key" : "554702285", "_rev" : "554702285" },
    { "id" : 535, "name" : "Peter", "_id" : "users/554636749", "_key" : "554636749", "_rev" : "554636749" } ]

    arango> db.users.byExample({ "name" : "Peter", "id" : 535 }).toArray();
    [ { "id" : 535, "name" : "Peter", "_id" : "users/554636749", "_key" : "554636749", "_rev" : "554636749" } ]

Use next to loop over all documents:

  arango> var a = db.users.byExample( {"name" : "Peter" } );
  arango> while (a.hasNext()) print(a.next());
  { "id" : 323, "name" : "Peter", "_id" : "users/554702285", "_key" : "554702285", "_rev" : "554702285" }
  { "id" : 535, "name" : "Peter", "_id" : "users/554636749", "_key" : "554636749", "_rev" : "554636749" }

`collection.firstExample( example)`

Returns the a document of a collection that match the specified example or null. The example must be specified as paths and values. See byExample for details.

`collection.firstExample( path1, value1, ...)`

As alternative you can supply a list of paths and values.

*Examples*

  arango> db.users.firstExample("name", 1237);
  { "_id" : "users/83049373", "_key" : "83049373", "_rev" : "83049373", "name" : 1237 }

`collection.range( attribute, left, right)`

Selects all documents of a collection such that the attribute is greater or equal than left and strictly less than right.

You can use toArray, next, or hasNext to access the result. The result can be limited using the skip and limit operator.

An attribute name of the form a.b is interpreted as attribute path, not as attribute.

For range queries it is required that a skiplist index is present for the queried attribute. If no skiplist index is present on the attribute, an error will be thrown.

*Examples*

Use toArray to get all documents at once:

  arangod> l = db.skip.range("age", 10, 13).toArray();
  [
    { "_id" : "skip/4260278", "_key" : "4260278", "_rev" : "4260278", "age" : 10 },
    { "_id" : "skip/4325814", "_key" : "4325814", "_rev" : "4325814", "age" : 11 },
    { "_id" : "skip/4391350", "_key" : "4391350", "_rev" : "4391350", "age" : 12 }
    ]

`collection.any()`

The any method returns a random document from the collection. It returns null if the collection is empty.

*Examples*

  arangod> db.example.any()
  { "_id" : "example/222716379559", "_rev" : "222716379559", "Hello" : "World" }

`collection.count()`

Returns the number of living documents in the collection.

*Examples*

  arango> db.users.count();
  10001

`collection.toArray()`

Converts the collection into an array of documents. Never use this call in a production environment.

<!--
@anchor SimpleQueryAll
@copydetails JSF_ArangoCollection_prototype_all

@CLEARPAGE
@anchor SimpleQueryByExample
@copydetails JSF_ArangoCollection_prototype_byExample

@CLEARPAGE
@anchor SimpleQueryFirstExample
@copydetails JSF_ArangoCollection_prototype_firstExample

@CLEARPAGE
@anchor SimpleQueryAny
@copydetails JSF_ArangoCollection_prototype_range

@CLEARPAGE
@anchor SimpleQueryRange
@copydetails JS_AnyQuery

@CLEARPAGE
@anchor SimpleQueryCollectionCount
@copydetails JS_CountVocbaseCol

@CLEARPAGE
@anchor SimpleQueryToArray
@copydetails JSF_ArangoCollection_prototype_toArray

@CLEARPAGE
@anchor SimpleQueryFirst
@copydetails JSF_ArangoCollection_prototype_first

@CLEARPAGE
@anchor SimpleQueryLast
@copydetails JSF_ArangoCollection_prototype_last
-->