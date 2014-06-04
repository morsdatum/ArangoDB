!CHAPTER Collection Methods

`collection.drop()`
Drops a collection and all its indexes.

*Examples*

Drops a collection:

  arango> col = db.examples;
  [ArangoCollection 109757, "examples" (status unloaded)]
  arango> col.drop()
  arango> col;
  [ArangoCollection 109757, "examples" (status deleted)]

`collection.truncate()`

Truncates a collection, removing all documents but keeping all its indexes.

*Examples*

Truncates a collection:

arango> col = db.examples;
[ArangoCollection 91022, "examples" (status new born)]
arango> col.save({ "Hello" : "World" });
{ "_id" : "91022/1532814", "_rev" : 1532814 }
arango> col.count();
1
arango> col.truncate();
arango> col.count();
0

`collection.properties()`

Returns an object containing all collection properties.

* waitForSync: If true creating a document will only return after the data was synced to disk.
* journalSize : The size of the journal in bytes.
* isVolatile: If true then the collection data will be kept in memory only and ArangoDB will not write or sync the data to disk.
* keyOptions (optional) additional options for key generation. This is a JSON array containing the following attributes (note: some of the attributes are optional):
* type: the type of the key generator used for the collection.
* allowUserKeys: if set to true, then it is allowed to supply own key values in the _key attribute of a document. If set to false, then the key generator will solely be responsible for generating keys and supplying own key values in the _key attribute of documents is considered an error.
* increment: increment value for autoincrement key generator. Not used for other key generator types.
* offset: initial offset value for autoincrement key generator. Not used for other key generator types.
* collection.properties( properties)

Changes the collection properties. properties must be a object with one or more of the following attribute(s):

* waitForSync: If true creating a document will only return after the data was synced to disk.
* journalSize : The size of the journal in bytes.

Note that it is not possible to change the journal size after the journal or datafile has been created. Changing this parameter will only effect newly created journals. Also note that you cannot lower the journal size to less then size of the largest document already stored in the collection.

Note: some other collection properties, such as type, isVolatile, or keyOptions cannot be changed once the collection is created.

*Examples*

Read all properties

  arango> db.examples.properties()
  { "waitForSync" : false, "journalSize" : 33554432, "isVolatile" : false }

Change a property

  arango> db.examples.properties({ waitForSync : false })
  { "waitForSync" : false, "journalSize" : 33554432, "isVolatile" : false }

`collection.figures()`

Returns an object containing all collection figures.

* alive.count: The number of living documents.
* alive.size: The total size in bytes used by all living documents.
* dead.count: The number of dead documents.
* dead.size: The total size in bytes used by all dead documents.
* dead.deletion: The total number of deletion markers.
* datafiles.count: The number of active datafiles.
* datafiles.fileSize: The total filesize of the active datafiles.
* journals.count: The number of journal files.
* journals.fileSize: The total filesize of the journal files.
* compactors.count: The number of compactor files.
* compactors.fileSize: The total filesize of the compactor files.
* shapefiles.count: The number of shape files.
* shapefiles.fileSize: The total filesize of the shape files.
* shapes.count: The total number of shapes used in the collection (this includes shapes that are not in use anymore)
* shapes.fileSize: The total filesize of the shapes files.
* attributes.count: The total number of attributes used in the collection (this includes attributes that are not in use anymore)

*Examples*

  arango> db.demo.figures()
  { "alive" : { "count" : 1000, "size" : 45000 }, "dead" : { "count" : 0, "size" : 0, "deletion" : 0 }, "datafiles" : { "count" : 0, "fileSize" : 0 }, "journals" : { "count" : 1, "fileSize" : 33554432 }, "shapes" : { "count" : 7 } }

`collection.load()`

Loads a collection into memory.

*Examples*

  arango> col = db.example;
  [ArangoCollection 164208316, "example" (status unloading)]
  arango> col.load();
  arango> col;
  [ArangoCollection 164208316, "example" (status loaded)]

`collection.reserve( number)`

Sends a resize hint to the indexes in the collection. The resize hint allows indexes to reserve space for additional documents (specified by number) in one go.

The reserve hint can be sent before a mass insertion into the collection is started. It allows indexes to allocate the required memory at once and avoids re-allocations and possible re-locations.

Not all indexes implement the reserve function at the moment. The indexes that don't implement it will simply ignore the request. returns the revision id of a collection

`collection.revision()`

Returns the revision id of the collection

The revision id is updated when the document data is modified, either by inserting, deleting, updating or replacing documents in it.

The revision id of a collection can be used by clients to check whether data in a collection has changed or if it is still unmodified since a previous fetch of the revision id.

The revision id returned is a string value. Clients should treat this value as an opaque string, and only use it for equality/non-equality comparisons.


`collection.checksum( withRevisions, withData)`

The checksum operation calculates a CRC32 checksum of the keys contained in collection collection.

If the optional argument withRevisions is set to true, then the revision ids of the documents are also included in the checksumming.

If the optional argument withData is set to true, then the actual document data is also checksummed. Including the document data in checksumming will make the calculation slower, but is more accurate.

`collection.unload()`
Starts unloading a collection from memory. Note that unloading is deferred until all query have finished.

*Examples*

  arango> col = db.example;
  [ArangoCollection 164208316, "example" (status loaded)]
  arango> col.unload();
  arango> col;
  [ArangoCollection 164208316, "example" (status unloaded)]

`collection.rename( new-name)`

Renames a collection using the new-name. The new-name must not already be used for a different collection. new-name must also be a valid collection name. For more information on valid collection names please refer to Naming Conventions in ArangoDB.

If renaming fails for any reason, an error is thrown.

*Examples*

  arango> c = db.example;
  [ArangoCollection 68519, "example" (status new born)]
  arango> c.rename("better-example");
  arango> c;
  [ArangoCollection 68519, "better-example" (status new born)]

`collection.rotate()`

Rotates the current journal of a collection (i.e. makes the journal a read-only datafile). The purpose of the rotation is to include the datafile in a following compaction run and perform earlier garbage collection.



<!--
@anchor HandlingCollectionsDrop
@copydetails JS_DropVocbaseCol

@CLEARPAGE
@anchor HandlingCollectionsTruncate
@copydetails JSF_ArangoCollection_prototype_truncate

@CLEARPAGE
@anchor HandlingCollectionsProperties
@copydetails JS_PropertiesVocbaseCol

@CLEARPAGE
@anchor HandlingCollectionsFigures
@copydetails JS_FiguresVocbaseCol

@CLEARPAGE
@anchor HandlingCollectionsLoad
@copydetails JS_LoadVocbaseCol

@CLEARPAGE
@anchor HandlingCollectionsRevision
@copydetails JS_RevisionVocbaseCol

@CLEARPAGE
@anchor HandlingCollectionsChecksum
@copydetails JS_ChecksumCollection

@CLEARPAGE
@anchor HandlingCollectionsUnload
@copydetails JS_UnloadVocbaseCol

@CLEARPAGE
@anchor HandlingCollectionsRename
@copydetails JS_RenameVocbaseCol

@CLEARPAGE
@anchor HandlingCollectionsRotate
@copydetails JS_RotateVocbaseCol
-->