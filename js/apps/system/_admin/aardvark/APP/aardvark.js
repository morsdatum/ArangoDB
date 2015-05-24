/*jshint globalstrict: true */
/*global require, applicationContext, parseInt*/

////////////////////////////////////////////////////////////////////////////////
/// @brief A Foxx.Controller to show all Foxx Applications
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2010-2013 triagens GmbH, Cologne, Germany
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
/// @author Michael Hackstein
/// @author Copyright 2011-2013, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

"use strict";

var FoxxController = require("org/arangodb/foxx").Controller,
  controller = new FoxxController(applicationContext),
  ArangoError = require("org/arangodb").ArangoError,
  underscore = require("underscore"),
  cluster = require("org/arangodb/cluster"),
  joi = require("joi"),
  notifications = require("org/arangodb/configuration").notifications,
  db = require("internal").db,
  foxxInstallKey = joi.string().required().description(
    "The _key attribute, where the information of this Foxx-Install is stored."
  );

var foxxes = new (require("lib/foxxes").Foxxes)();
var FoxxManager = require("org/arangodb/foxx/manager");

controller.get("/index.html", function(req, res) {
  res.status(301);
  if (cluster.dispatcherDisabled()) {
    res.set("Location", "standalone.html");
  } else {
    res.set("Location", "cluster.html");
  }
});

controller.get("/whoAmI", function(req, res) {
  res.json({
    name: req.user
  });
});

controller.get("/unauthorized", function() {
  throw new ArangoError();
}).errorResponse(ArangoError, 401, "unauthorized");

/** Is version check allowed
 *
 * Check if version check is allowed
 */
controller.get("shouldCheckVersion", function(req, res) {
  var versions = notifications.versions();

  if (!versions || versions.enableVersionNotification === false) {
    res.json(false);
  } else {
    res.json(true);
  }
});

/** Disable version check
 *
 * Disable the version check in web interface
 */
controller.post("disableVersionCheck", function(req, res) {
  notifications.setVersions({
    enableVersionNotification: false
  });
  res.json("ok");
});

/** Fetch a foxx from temp folder
 *
 * Makes a foxx uploaded to the temp folder
 * available for mounting.
 */

controller.post("/foxxes/inspect", function (req, res) {
  var content = JSON.parse(req.requestBody),
    path = content.filename;
  res.json(foxxes.inspect(path));
}).errorResponse(ArangoError, 500, "No valid app");

// .............................................................................
// install
// .............................................................................

controller.put("/foxxes/install", function (req, res) {
  var content = JSON.parse(req.requestBody),
    name = content.name,
    mount = content.mount,
    version = content.version,
    options = content.options;
    res.json(foxxes.install(name, mount, version, options));
}).summary("Installs a new foxx")
.notes("This function is used to install a new foxx.");

/** Remove an uninstalled Foxx
 *
 * Remove the Foxx with the given key.
 */
controller.del("/foxxes/purge/:key", function (req, res) {
  res.json(FoxxManager.purge(req.params("key")));
}).pathParam("key", {
  type: foxxInstallKey,
  allowMultiple: false
}).summary("Remove a Foxx.")
.notes("This function is used to remove a foxx.");

/** Remove all uninstalled Foxx versions
 *
 * Remove the Foxx with the given array of versions app ids.
 */

controller.del("/foxxes/purgeall/:key", function (req, res) {
  var name = req.params("key");

  var allFoxxes = foxxes.viewAll();
  var toDelete = [];

  underscore.each(allFoxxes, function(myFoxx) {
    if (myFoxx.name === name) {
      toDelete.push(myFoxx.app);
    }
  });

  underscore.each(toDelete, function(myFoxx) {
    FoxxManager.purge(myFoxx);
  });

  res.json({res: true});
}).pathParam("key", {
  type: foxxInstallKey,
  allowMultiple: false
}).summary("Remove a all existing Foxx versions .")
.notes("This function is used to remove all versions of a foxx.");

/** Get info about github information of an available Foxx
 *
 * Returns github information of Foxx
 */
controller.get("/foxxes/gitinfo/:key", function (req, res) {
  res.json(FoxxManager.gitinfo(req.params("key")));
}).pathParam("key", {
  type: foxxInstallKey,
  allowMultiple: false
}).summary("List git information of a Foxx")
.notes("This function is used to display all available github information of a foxx");


/** Get info about mount points of an installed Foxx
 *
 * Returns mount points of Foxx
 */

controller.get("/foxxes/mountinfo/:key", function (req, res) {
  res.json(FoxxManager.mountinfo(req.params("key")));
}).pathParam("key", {
  type: foxxInstallKey,
  allowMultiple: false
}).summary("List mount points of a Foxx")
.notes("This function is used to display all available mount points of a foxx");

/** Get info about all mount points of all installed Foxx
 *
 * Returns mount points of all Foxxes
 */

controller.get("/foxxes/mountinfo/", function (req, res) {
  res.json(FoxxManager.mountinfo());
}).pathParam("key", {
  type: foxxInstallKey,
  allowMultiple: false
}).summary("List mount points of all Foxx")
.notes("This function is used to display all available mount points of all foxxes");




/** Update a Foxx
 *
 * Update the Foxx with the given information.
 */

controller.put("/foxxes/:key", function (req, res) {
  var content = JSON.parse(req.requestBody),
    active = content.active;
  // TODO: Other changes applied to foxx! e.g. Mount
  if (active) {
    res.json(foxxes.activate());
  } else {
    res.json(foxxes.deactivate());
  }
}).pathParam("key", {
  type: foxxInstallKey,
  allowMultiple: false
}).summary("Update a foxx.")
  .notes("Used to either activate/deactivate a foxx, or change the mount point.");


/** List available Documentation
 *
 * Get the list of all running Foxxes with links to their documentation
 */
controller.get('/docus', function (req, res) {
  res.json(docus.list(req.protocol + "://" + req.headers.host + "/_db/" + req.database + req.path + "/"));
}).summary("List documentation of all foxxes.")
  .notes("This function simply returns the list of all running"
       + " foxxes and supplies the paths for the swagger documentation");
/** Get Documentation for one Foxx
 *
 * Get the complete documentation available for one Foxx
 *
 */

controller.get("/docu/:key",function (req, res) {
  var subPath = req.path.substr(0, req.path.lastIndexOf("[") - 1),
    key = req.params("key"),
    path = req.protocol + "://" + req.headers.host + 
           "/_db/" + encodeURIComponent(req.database) + subPath + "/" + encodeURIComponent(key) + "/";
  res.json(docus.listOne(path, key));
}).summary("List documentation of one foxxes.")
  .notes("This function simply returns one specific"
       + " foxx and supplies the paths for the swagger documentation");

 /** Subroutes for API Documentation
  *
  * Get the Elements of the API Documentation subroutes
  */
controller.get('/docu/:key/*', function(req, res) {
  var mountPoint = "";
    underscore.each(req.suffix, function(part) {
      mountPoint += "/" + part;
    });
  res.json(docus.show(mountPoint));
}).summary("List the API for one foxx")
  .notes("This function lists the API of the foxx"
       + " running under the given mount point");
 
/** Move Foxx to other Mount
 *
 * Move a running Foxx from one mount point to another
 *
 */
controller.put('/foxx/move/:key', function(req, res) {
  var body = req.body();
  var mountPoint = body.mount;
  var app = body.app;
  var key = req.params("key");
  var prefix = body.prefix;
  var result = foxxes.move(key, app, mountPoint, prefix);
  if (result.error) {
    res.status(result.status);
    res.body = result.message;
    return;
  }
  res.json(result);
})
.summary("Move one foxx to another moint point")
.notes ("This function moves one installed foxx"
  + " to a given mount point.");

/** Download stored queries
 *
 * Download and export all queries from the given username.
 *
 */

controller.post("/query/upload/:user", function(req, res) {
  var user = req.params("user");
  var response, queries, userColl, storedQueries, queriesToSave;

  queries = req.body();
  userColl = db._users.byExample({"user": user}).toArray()[0];
  queriesToSave = userColl.userData.queries || [ ];

  underscore.each(queries, function(newq) {
    var found = false, i;
    for (i = 0; i < queriesToSave.length; ++i) {
      if (queriesToSave[i].name === newq.name) {
        queriesToSave[i] = newq;
        found = true;
        break;
      }
    }
    if (! found) {
      queriesToSave.push(newq);
    }
  });

  var toUpdate = {
    userData: {
      queries: queriesToSave
    }
  }

  var result = db._users.update(userColl, toUpdate, true);
  res.json(result);
}).summary("Upload user queries")
  .notes("This function uploads all given user queries");

/** Download stored queries
 *
 * Download and export all queries from the given username.
 *
 */

controller.get("/query/download/:user", function(req, res) {
  var user = req.params("user");
  var result = db._users.byExample({"user": user}).toArray()[0];

  res.set("Content-Type", "application/json");
  res.set("Content-Disposition", "attachment; filename=queries.json");

  res.json(result.userData.queries);

}).summary("Download all user queries")
  .notes("This function downloads all user queries from the given user");

/** Download a query result
 *
 * Download and export all queries from the given username.
 *
 */

controller.get("/query/result/download/:query", function(req, res) {
  var query = req.params("query"),
  parsedQuery;

  var internal = require("internal");
  query = internal.base64Decode(query);
  try {
    parsedQuery = JSON.parse(query);
  }
  catch (ignore) {
  }

  var result = db._query(parsedQuery.query, parsedQuery.bindVars).toArray();
  res.set("Content-Type", "application/json");
  res.set("Content-Disposition", "attachment; filename=results.json");
  res.json(result);

}).summary("Download the result of a query")
  .notes("This function downloads the result of a user query.");

// -----------------------------------------------------------------------------
// --SECTION--                                                       END-OF-FILE
// -----------------------------------------------------------------------------

/// Local Variables:
/// mode: outline-minor
/// outline-regexp: "/// @brief\\|/// @addtogroup\\|/// @page\\|// --SECTION--\\|/// @\\}\\|/\\*jslint"
/// End:
