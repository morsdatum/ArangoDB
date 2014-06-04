!CHAPTER Collection Methods

`collection.document( document)`

The document method finds a document given its identifier. It returns the document. Note that the returned document contains two pseudo-attributes, namely _id and _rev. _id contains the document-handle and _rev the revision of the document.

An error is thrown if the _rev does not longer match the current revision of the document.

An error is also thrown if the document does not exist.

`collection.document( document-handle)`

As before. Instead of document a document-handle can be passed as first argument.

*Examples*

Returns the document for a document-handle:

  arango> db.example.document("1432124/2873916");
  { "_id" : "1432124/2873916", "_rev" : "2873916", "Hello" : "World" }

An error is raised if the document is unknown:

  arango> db.example.document("1432124/123456");
  JavaScript exception in file '(arango)' at 1,12:
  [ArangoError 1202: document not found: document not found]
  !db.example.document("1432124/123456");
  ! ^

An error is raised if the handle is invalid:

  arango> db.example.document("12345");
  JavaScript exception in file '(arango)' at 1,12:
  [ArangoError 10: bad parameter: <document-identifier> must be a document identifier]
  !db.example.document("12345");
  ! ^

`collection.exists( document)`

The exists method determines whether a document exists given its identifier. Instead of returning the found document or an error, this method will return either true or false. It can thus be used for easy existence checks.

The document method finds a document given its identifier. It returns the document. Note that the returned document contains two pseudo-attributes, namely _id and _rev. _id contains the document-handle and _rev the revision of the document.

No error will be thrown if the sought document or collection does not exist. Still this method will throw an error if used improperly, e.g. when called with a non-document handle, a non-document, or when a cross-collection request is performed.

`collection.exists( document-handle)`

As before. Instead of document a document-handle can be passed as first argument.


`collection.any()`

The any method returns a random document from the collection. It returns null if the collection is empty.

*Examples*

  arangod> db.example.any()
  { "_id" : "example/222716379559", "_rev" : "222716379559", "Hello" : "World" }

`collection.save( data)`

Creates a new document in the collection from the given data. The data must be a hash array. It must not contain attributes starting with _.

The method returns a document with the attributes _id and _rev. The attribute _id contains the document handle of the newly created document, the attribute _rev contains the document revision.

`collection.save( data, waitForSync)`

Creates a new document in the collection from the given data as above. The optional waitForSync parameter can be used to force synchronisation of the document creation operation to disk even in case that the waitForSync flag had been disabled for the entire collection. Thus, the waitForSync parameter can be used to force synchronisation of just specific operations. To use this, set the waitForSync parameter to true. If the waitForSync parameter is not specified or set to false, then the collection's default waitForSync behavior is applied. The waitForSync parameter cannot be used to disable synchronisation for collections that have a default waitForSync value of true.

*Examples*

  arango> db.example.save({ Hello : "World" })
  { "_id" : "example/3175486", "_key" : "3175486", "_rev" : "3175486" }

  arango> db.example.save({ Hello : "World" }, true)
  { "_id" : "example/3179437", "_key" : "3179437", "_rev" : "3179437" }

`collection.replace( document, data)`

Replaces an existing document. The document must be a document in the current collection. This document is then replaced with the data given as second argument.

The method returns a document with the attributes _id, _rev and _oldRev. The attribute _id contains the document handle of the updated document, the attribute _rev contains the document revision of the updated document, the attribute _oldRev contains the revision of the old (now replaced) document.

If there is a conflict, i. e. if the revision of the document does not match the revision in the collection, then an error is thrown.

`collection.replace( document, data, true)`

As before, but in case of a conflict, the conflict is ignored and the old document is overwritten.

`collection.replace( document, data, true, waitForSync)`

The optional waitForSync parameter can be used to force synchronisation of the document replacement operation to disk even in case that the waitForSync flag had been disabled for the entire collection. Thus, the waitForSync parameter can be used to force synchronisation of just specific operations. To use this, set the waitForSync parameter to true. If the waitForSync parameter is not specified or set to false, then the collection's default waitForSync behavior is applied. The waitForSync parameter cannot be used to disable synchronisation for collections that have a default waitForSync value of true.

`collection.replace( document-handle, data)`

As before. Instead of document a document-handle can be passed as first argument.

*Examples*

Create and update a document:

  arango> a1 = db.example.save({ a : 1 });
  { "_id" : "example/3903044", "_key" : "3903044", "_rev" : "3903044" }
  arango> a2 = db.example.replace(a1, { a : 2 });
  { "_id" : "example/3903044", "_key" : "3903044", "_rev" : "3968580", "_oldRev" : "3903044" }
  arango> a3 = db.example.replace(a1, { a : 3 });
  JavaScript exception in file '(arango)' at 1,17: [ArangoError 1200: conflict: cannot replace document]
  !a3 = db.example.replace(a1, { a : 3 })
  !                ^
  arango> a4 = db.example.replace(a1, { a : 4 }, true);
  { "_id" : "example/3903044", "_key" : "3903044", "_rev" : "4034116", "_oldRev" : "3968580" }

Use a document handle:

  arango> a5 = db.example.replace("1432124/3903044", { a : 5 });
  { "_id" : "example/3903044", "_key" : "3903044", "_rev" : "4099652", "_oldRev" : "4034116" }

`collection.update( document, data, overwrite, keepNull, waitForSync)`

Updates an existing document. The document must be a document in the current collection. This document is then patched with the data given as second argument. The optional overwrite parameter can be used to control the behavior in case of version conflicts (see below). The optional keepNull parameter can be used to modify the behavior when handling null values. Normally, null values are stored in the database. By setting the keepNull parameter to false, this behavior can be changed so that all attributes in data with null values will be removed from the target document.

The optional waitForSync parameter can be used to force synchronisation of the document update operation to disk even in case that the waitForSync flag had been disabled for the entire collection. Thus, the waitForSync parameter can be used to force synchronisation of just specific operations. To use this, set the waitForSync parameter to true. If the waitForSync parameter is not specified or set to false, then the collection's default waitForSync behavior is applied. The waitForSync parameter cannot be used to disable synchronisation for collections that have a default waitForSync value of true.

The method returns a document with the attributes _id, _rev and _oldRev. The attribute _id contains the document handle of the updated document, the attribute _rev contains the document revision of the updated document, the attribute _oldRev contains the revision of the old (now replaced) document.

If there is a conflict, i. e. if the revision of the document does not match the revision in the collection, then an error is thrown.

`collection.update( document, data, true)`

As before, but in case of a conflict, the conflict is ignored and the old document is overwritten.

`collection.update( document-handle, data)`

As before. Instead of document a document-handle can be passed as first argument.

*Examples*

Create and update a document:

  arango> a1 = db.example.save({"a" : 1});
  { "error" : false, "_id" : "example/18612115", "_key" : "18612115", "_rev" : "18612115" }
  arango> a2 = db.example.update(a1, {"b" : 2, "c" : 3});
  { "error" : false, "_id" : "example/18612115", "_key" : "18612115", "_rev" : "18874259" }
  arango> a3 = db.example.update(a1, {"d" : 4});
  JavaScript exception in file 'client/client.js' at 1876,7: [ArangoError 1200: precondition failed]
  !      throw new ArangoError(requestResult);
  !      ^
  arango> a4 = db.example.update(a2, {"e" : 5, "f" : 6 });
  { "error" : false, "_id" : "example/18612115", "_key" : "18612115", "_rev" : "19267475" }
  arango> db.example.document(a4);
  { "a" : 1, "b" : 2, "c" : 3, "e" : 5, "f" : 6, "_id" : "example/18612115", "_key" : "18612115", "_rev" : "19267475" }

  arango> a5 = db.example.update(a4, {"a" : 1, c : 9, e : 42 });
  { "error" : false, "_id" : "example/18612115", "_key" : "18612115", "_rev" : "19333011" }
  arango> db.example.document(a5);
  { "a" : 1, "b" : 2, "c" : 9, "e" : 42, "f" : 6, "_id" : "example/18612115", "_key" : "18612115", "_rev" : "19333011" }

Use a document handle:

  arango> a6 = db.example.update("example/18612115", { "x" : 1, "y" : 2 });
  { "error" : false, "_id" : "example/18612115", "_key" : "18612115", "_rev" : "19595155" }

Use the keepNull parameter to remove attributes with null values:

  arango> db.example.save({"a" : 1});
  { "error" : false, "_id" : "example/19988371", "_key" : "19988371", "_rev" : "19988371" }
  arango> db.example.update("example/19988371", { "b" : null, "c" : null, "d" : 3 });
  { "error" : false, "_id" : "example/19988371", "_key" : "19988371", "_rev" : "20119443" }
  arango> db.example.document("example/19988371");
  { "b" : null, "c" : null, "a" : 1, "d" : 3, "_id" : "example/19988371", "_key" : "19988371", "_rev" : "20119443" }

  arango> db.example.update("example/19988371", { "a" : null }, false, false);
  { "error" : false, "_id" : "example/19988371", "_key" : "19988371", "_rev" : "20250515" }
  arango> db.example.document("example/19988371");
  { "b" : null, "c" : null, "d" : 3, "_id" : "example/19988371", "_key" : "19988371", "_rev" : "20250515" }
  arango> db.example.update("example/19988371", { "b" : null, "c": null, "d" : null }, false, false);
  { "error" : false, "_id" : "example/19988371", "_key" : "19988371", "_rev" : "20381587" }
  arango> db.example.document("example/19988371");
  { "_id" : "example/19988371", "_key" : "19988371", "_rev" : "20381587" }

Patching array values:

  arango> db.example.save({"a" : { "one" : 1, "two" : 2, "three" : 3 }, "b" : { }});
  { "error" : false, "_id" : "example/20774803", "_key" : "20774803", "_rev" : "20774803" }
  arango> db.example.update("example/20774803", {"a" : { "four" : 4 }, "b" : { "b1" : 1 }});
  { "error" : false, "_id" : "example/20774803", "_key" : "20774803", "_rev" : "21168019" }
  arango> db.example.document("example/20774803");
  { "a" : { "one" : 1, "two" : 2, "three" : 3, "four" : 4 }, "b" : { "b1" : 1 }, "_id" : "example/20774803", "_key" : "20774803", "_rev" : "21168019" }

  arango> db.example.update("example/20774803", { "a" : { "one" : null }, "b" : null }, false, false);
  { "error" : false, "_id" : "example/20774803", "_key" : "20774803", "_rev" : "21364627" }
  arango> db.example.document("example/20774803");
  { "a" : { "two" : 2, "three" : 3, "four" : 4 }, "_id" : "example/20774803", "_key" : "20774803", "_rev" : "21364627" }

`collection.remove( document)`

Removes a document. If there is revision mismatch, then an error is thrown.

`collection.remove( document, true)`

Removes a document. If there is revision mismatch, then mismatch is ignored and document is deleted. The function returns true if the document existed and was deleted. It returns false, if the document was already deleted.

`collection.remove( document, true, waitForSync)`

The optional waitForSync parameter can be used to force synchronisation of the document deletion operation to disk even in case that the waitForSync flag had been disabled for the entire collection. Thus, the waitForSync parameter can be used to force synchronisation of just specific operations. To use this, set the waitForSync parameter to true. If the waitForSync parameter is not specified or set to false, then the collection's default waitForSync behavior is applied. The waitForSync parameter cannot be used to disable synchronisation for collections that have a default waitForSync value of true.

`collection.remove( document-handle, data)`

As before. Instead of document a document-handle can be passed as first argument.

*Examples*

Remove a document:

  arango> a1 = db.example.save({ a : 1 });
  { "_id" : "116308/3449537", "_rev" : "3449537" }
  arango> db.example.document(a1);
  { "_id" : "116308/3449537", "_rev" : "3449537", "a" : 1 }
  arango> db.example.remove(a1);
  true
  arango> db.example.document(a1);
  JavaScript exception in file '(arango)' at 1,12: [ArangoError 1202: document not found: document not found]
  !db.example.document(a1);
  ! ^

Remove a document with a conflict:

  arango> a1 = db.example.save({ a : 1 });
  { "_id" : "116308/3857139", "_rev" : "3857139" }
  arango> a2 = db.example.replace(a1, { a : 2 });
  { "_id" : "116308/3857139", "_rev" : "3922675", "_oldRev" : 3857139 }
  arango> db.example.remove(a1);
  JavaScript exception in file '(arango)' at 1,18: [ArangoError 1200: conflict: cannot remove document]
  !db.example.remove(a1);
  ! ^
  arango> db.example.remove(a1, true);
  true
  arango> db.example.document(a1);
  JavaScript exception in file '(arango)' at 1,12: [ArangoError 1202: document not found: document not found]
  !db.example.document(a1);
  ! ^

`collection.removeByExample( example)`

Removes all documents matching an example.

`collection.removeByExample( document, waitForSync)`

The optional waitForSync parameter can be used to force synchronisation of the document deletion operation to disk even in case that the waitForSync flag had been disabled for the entire collection. Thus, the waitForSync parameter can be used to force synchronisation of just specific operations. To use this, set the waitForSync parameter to true. If the waitForSync parameter is not specified or set to false, then the collection's default waitForSync behavior is applied. The waitForSync parameter cannot be used to disable synchronisation for collections that have a default waitForSync value of true.

`collection.removeByExample( document, waitForSync, limit)`

The optional limit parameter can be used to restrict the number of removals to the specified value. If limit is specified but less than the number of documents in the collection, it is undefined which documents are removed.

*Examples*

  arangod> db.content.removeByExample({ "domain": "de.celler" })

`collection.replaceByExample( example, newValue)`

Replaces all documents matching an example with a new document body. The entire document body of each document matching the example will be replaced with newValue. The document meta-attributes such as _id, _key, _from, _to will not be replaced.

`collection.replaceByExample( document, newValue, waitForSync)`

The optional waitForSync parameter can be used to force synchronisation of the document replacement operation to disk even in case that the waitForSync flag had been disabled for the entire collection. Thus, the waitForSync parameter can be used to force synchronisation of just specific operations. To use this, set the waitForSync parameter to true. If the waitForSync parameter is not specified or set to false, then the collection's default waitForSync behavior is applied. The waitForSync parameter cannot be used to disable synchronisation for collections that have a default waitForSync value of true.

`collection.replaceByExample( document, newValue, waitForSync, limit)`

The optional limit parameter can be used to restrict the number of replacements to the specified value. If limit is specified but less than the number of documents in the collection, it is undefined which documents are replaced.

*Examples*

  arangod> db.content.replaceByExample({ "domain": "de.celler" }, { "foo": "someValue }, false, 5)

`collection.updateByExample( example, newValue)`

Partially updates all documents matching an example with a new document body. Specific attributes in the document body of each document matching the example will be updated with the values from newValue. The document meta-attributes such as _id, _key, _from, _to cannot be updated.

`collection.updateByExample( document, newValue, keepNull, waitForSync)`

The optional keepNull parameter can be used to modify the behavior when handling null values. Normally, null values are stored in the database. By setting the keepNull parameter to false, this behavior can be changed so that all attributes in data with null values will be removed from the target document.

The optional waitForSync parameter can be used to force synchronisation of the document replacement operation to disk even in case that the waitForSync flag had been disabled for the entire collection. Thus, the waitForSync parameter can be used to force synchronisation of just specific operations. To use this, set the waitForSync parameter to true. If the waitForSync parameter is not specified or set to false, then the collection's default waitForSync behavior is applied. The waitForSync parameter cannot be used to disable synchronisation for collections that have a default waitForSync value of true.

`collection.updateByExample( document, newValue, keepNull, waitForSync, limit)`

The optional limit parameter can be used to restrict the number of updates to the specified value. If limit is specified but less than the number of documents in the collection, it is undefined which documents are updated.

*Examples*

  arangod> db.content.updateByExample({ "domain": "de.celler" }, { "foo": "someValue, "domain": null }, false)

`collection.first( count)`

The first method returns the n first documents from the collection, in order of document insertion/update time.

If called with the count argument, the result is a list of up to count documents. If count is bigger than the number of documents in the collection, then the result will contain as many documents as there are in the collection. The result list is ordered, with the "oldest" documents being positioned at the beginning of the result list.

When called without an argument, the result is the first document from the collection. If the collection does not contain any documents, the result returned is null.

*Examples*

  arangod> db.example.first(1)
  [ { "_id" : "example/222716379559", "_rev" : "222716379559", "Hello" : "World" } ]
  arangod> db.example.first()
  { "_id" : "example/222716379559", "_rev" : "222716379559", "Hello" : "World" }

`collection.last( count)`

The first method returns the n last documents from the collection, in order of document insertion/update time.

If called with the count argument, the result is a list of up to count documents. If count is bigger than the number of documents in the collection, then the result will contain as many documents as there are in the collection. The result list is ordered, with the "latest" documents being positioned at the beginning of the result list.

When called without an argument, the result is the last document from the collection. If the collection does not contain any documents, the result returned is null.

*Examples*

  arangod> db.example.last(1)
  [ { "_id" : "example/222716379559", "_rev" : "222716379559", "Hello" : "World" } ]
  arangod> db.example.last()
  { "_id" : "example/222716379559", "_rev" : "222716379559", "Hello" : "World" }
 



<!--
@anchor HandlingDocumentsRead
@copydetails JS_DocumentVocbaseCol

@CLEARPAGE
@anchor HandlingDocumentsExists
@copydetails JS_ExistsVocbaseCol

@CLEARPAGE
@anchor HandlingDocumentsAny
@copydetails JS_AnyQuery

@CLEARPAGE
@anchor HandlingDocumentsCreate
@copydetails JS_SaveVocbaseCol

@CLEARPAGE
@anchor HandlingDocumentsReplace
@copydetails JS_ReplaceVocbaseCol

@CLEARPAGE
@anchor HandlingDocumentsUpdate
@copydetails JS_UpdateVocbaseCol

@CLEARPAGE
@anchor HandlingDocumentsRemove
@copydetails JS_RemoveVocbaseCol

@CLEARPAGE
@anchor HandlingDocumentsRemoveByExample
@copydetails JSF_ArangoCollection_prototype_removeByExample

@CLEARPAGE
@anchor HandlingDocumentsReplaceByExample
@copydetails JSF_ArangoCollection_prototype_replaceByExample

@CLEARPAGE
@anchor HandlingDocumentsUpdateByExample
@copydetails JSF_ArangoCollection_prototype_updateByExample

@CLEARPAGE
@anchor HandlingDocumentsFirst
@copydetails JSF_ArangoCollection_prototype_first

@CLEARPAGE
@anchor HandlingDocumentsLast
@copydetails JSF_ArangoCollection_prototype_last

-->