![ArangoDB-Logo](https://www.arangodb.com/wp-content/uploads/2012/10/logo_arangodb_transp.png)

ArangoDB
========

1.4: [![Build Status](https://secure.travis-ci.org/arangodb/arangodb.png?branch=1.4)](http://travis-ci.org/arangodb/arangodb)
2.4: [![Build Status](https://secure.travis-ci.org/arangodb/arangodb.png?branch=2.4)](http://travis-ci.org/arangodb/arangodb)
2.5: [![Build Status](https://secure.travis-ci.org/arangodb/arangodb.png?branch=2.5)](http://travis-ci.org/arangodb/arangodb)
Master: [![Build Status](https://secure.travis-ci.org/arangodb/arangodb.png?branch=master)](http://travis-ci.org/arangodb/arangodb)

ArangoDB is a multi-model, open-source database with flexible data models for documents, graphs, and key-values. Build high performance applications using a convenient SQL-like query language or JavaScript extensions. Use ACID transactions if you require them. Scale horizontally and vertically with a few mouse clicks.

ArangoDB is Polyglot Persistence done right. The supported data models can be mixed in queries and allow ArangoDB to be the aggregation point for the data request you have in mind.

To get started, try one of our 10 Minutes [Tutorials](https://www.arangodb.com/tutorials) in your favourite programming language or try one of our [ArangoDB Cookbook recipes](https://docs.arangodb.com/cookbook).

For the impatient: [download](https://www.arangodb.com/download) and install ArangoDB. Start the server `arangod` and point your browser to `http://127.0.0.1:8529/`.

Key Features in ArangoDB
------------------------

**Multi-Model**
Documents, graphs and key-value pairs — model your data as you see fit for your application.

**Joins**
Conveniently join what belongs together for flexible ad-hoc querying, less data redundancy.

**Transactions**
Easy application development keeping your data consistent and safe. No hassle in your client.

Here is an AQL query that makes use of all those features:
![AQL Query Example](https://www.arangodb.com/wp-content/uploads/2015/03/query_join.png)

Joins and Transactions are key features for flexible, secure data designs, widely used in RDBMSs that you won’t want to miss in NoSQL products. You decide how and when to use Joins and strong consistency guarantees, keeping all the power for scaling and performance as choice. 

Furthermore, ArangoDB offers a microservice framework called [Foxx](https://www.arangodb.com/foxx) to build your own Rest API with a few lines of code.

Microservice Example
![Microservice Example](https://www.arangodb.com/wp-content/uploads/2015/03/microservice.png)

Next step, bundle your Foxx application as a [docker container](https://docs.arangodb.com/cookbook/UsingArangoDBNodeJSDocker.html) and get it running in the cloud.

Key features include:
---------------------

* **Schema-free schemata** let you combine the space efficiency of MySQL with the performance power of NoSQL
* Use a **data-centric microservices** approach with ArangoDB Foxx and fuse your application-logic and database together for maximal throughput
* JavaScript for all: **no language zoo**, you can use one language from your browser to your back-end
* ArangoDB is **multi-threaded** - exploit the power of all your cores
* **Flexible data modelling**: model your data as combination of key-value pairs, documents or graphs - perfect for social relations
* Free **index choice**: use the correct index for your problem, be it a skip list or a fulltext search
* Configurable **durability**: let the application decide if it needs more durability or more performance
* No-nonsense storage: ArangoDB uses all of the power of **modern storage hardware**, like SSD and large caches
* **Powerful query language** (AQL) to retrieve and modify data 
* **Transactions**: run queries on multiple documents or collections with optional transactional consistency and isolation
* **Replication** and **Sharding**: set up the database in a master-slave configuration or spread bigger datasets across multiple servers
* It is **open source** (Apache Licence 2.0)

For more in-depth information read the [design goals of ArangoDB](http://www.arangodb.com/2012/03/07/avocadodbs-design-objectives)

Latest Release - ArangoDB 2.5
-----------------

The full [changelog](https://docs.arangodb.com/) could be found in the documentation.

**Sparse Indizes**: In ArangoDB 2.5, hash and skiplist indexes can optionally be made sparse. Here is a [performance comparison](https://www.arangodb.com/2015/02/24/sparse-indexes-in-arangodb) that shows how you can benefit from great savings in memory and index creation CPU time declaring indexes as sparse.

We’ve added some small but useful **AQL language improvements** plus several **AQL optimizer improvements**.

**Object attribute names** in ArangoDB 2.5 can be specified using static string literals, bind parameters, and dynamic expressions.

Function **RANDOM_TOKEN(length)**: produces a pseudo-random string of the specified length. Such strings can be used for id or token generation.

One example implementation is the [API-Key management](https://www.arangodb.com/2015/03/05/using-api-keys) that restricts access to certain routes of an API. You can use this custom Foxx library to start charging your valuable data right away.
 
**Streamlined development process**: ArangoDB 2.5 improves the development process of data-centric microservices with the Foxx framework.

Installing an app is now as easy as it should be:
* install: get your Foxx app up and running
* uninstall: shut it down and erase it from disk

Developers can enable the new *development mode* for a single app. This deactivates caching and provides fine-graded debugging information.

More Information
----------------

Please check the [Installation Manual](https://docs.arangodb.com/Installing/README.html) for installation and compilation instructions.

The [User Manual](https://docs.arangodb.com/FirstSteps/README.html) has an introductory chapter showing the basic operations of ArangoDB.

Stay in Contact
---------------

Please note that there will be bugs and we'd really appreciate it if you report them:

[https://github.com/arangodb/arangodb/issues](https://github.com/arangodb/arangodb/issues)

You can use the Google group for improvements, feature requests, comments 

[http://www.arangodb.com/community](https://www.arangodb.com/community)
