!CHAPTER Address of a Collection 

All collections in ArangoDB have an unique identifier and an unique
name. ArangoDB internally uses the collection's unique identifier to look up
collections. This identifier, however, is managed by ArangoDB and the user has
no control over it. In order to allow users to use their own names, each collection
  also has an unique name which is specified by the user. To access a collection
from the user perspective, the collection name should be used, i.e.:

*db._collection(collection-name)*

A collection is created by a ["db._create"](../Collections/DatabaseMethods.md) call.

For example: Assume that the collection identifier is `7254820` and the name is
`demo`, then the collection can be accessed as:

    db._collection("demo")

If no collection with such a name exists, then *null* is returned.

There is a short-cut that can be used for non-system collections:

*db.collection-name*

This call will either return the collection named *db.collection-name* or create
a new one with that name and a set of default properties.

Note: Creating a collection on the fly using *db.collection-name* is
not recommend and does not work in _arangosh_. To create a new collection, please
use

*db._create(collection-name)*