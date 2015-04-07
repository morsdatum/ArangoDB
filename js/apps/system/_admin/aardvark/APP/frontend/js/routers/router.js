/*jshint unused: false */
/*global window, $, Backbone, document, arangoCollectionModel*/
/*global arangoHelper,dashboardView,arangoDatabase, _*/

(function () {
  "use strict";

  window.Router = Backbone.Router.extend({
    routes: {
      "": "dashboard",
      "dashboard": "dashboard",
      "collections": "collections",
      "new": "newCollection",
      "login": "login",
      "collection/:colid/documents/:pageid": "documents",
      "collection/:colid/:docid": "document",
      "shell": "shell",
      "query": "query",
      "queryManagement": "queryManagement",
      "api": "api",
      "databases": "databases",
      "applications": "applications",
      "applications/:mount": "applicationDetail",
      "application/documentation/:mount": "appDocumentation",
      "graph": "graphManagement",
      "userManagement": "userManagement",
      "userProfile": "userProfile",
      "logs": "logs"
    },

    logs: function () {
      if (!this.logsView) {
        var newLogsAllCollection = new window.ArangoLogs(
          {upto: true, loglevel: 4}
        ),
        newLogsDebugCollection = new window.ArangoLogs(
          {loglevel: 4}
        ),
        newLogsInfoCollection = new window.ArangoLogs(
          {loglevel: 3}
        ),
        newLogsWarningCollection = new window.ArangoLogs(
          {loglevel: 2}
        ),
        newLogsErrorCollection = new window.ArangoLogs(
          {loglevel: 1}
        );
        this.logsView = new window.LogsView({
          logall: newLogsAllCollection,
          logdebug: newLogsDebugCollection,
          loginfo: newLogsInfoCollection,
          logwarning: newLogsWarningCollection,
          logerror: newLogsErrorCollection
        });
      }
      this.logsView.render();
      this.naviView.selectMenuItem('tools-menu');
    },

    initialize: function () {
      // This should be the only global object
      window.modalView = new window.ModalView();

      this.foxxList = new window.FoxxCollection();
      window.foxxInstallView = new window.FoxxInstallView({
        collection: this.foxxList
      });
      window.progressView = new window.ProgressView();
      var self = this;

      this.arangoDatabase = new window.ArangoDatabase();

      this.currentDB = new window.CurrentDatabase();
      this.currentDB.fetch({
        async: false
      });

      this.userCollection = new window.ArangoUsers();

      this.arangoCollectionsStore = new window.arangoCollections();
      this.arangoDocumentStore = new window.arangoDocument();
      arangoHelper.setDocumentStore(this.arangoDocumentStore);

      this.arangoCollectionsStore.fetch({async: false});

      this.footerView = new window.FooterView();
      this.notificationList = new window.NotificationCollection();
      this.naviView = new window.NavigationView({
        database: this.arangoDatabase,
        currentDB: this.currentDB,
        notificationCollection: self.notificationList,
        userCollection: this.userCollection
      });

      this.queryCollection = new window.ArangoQueries();

      this.footerView.render();
      this.naviView.render();

      $(window).resize(function () {
        self.handleResize();
      });
      window.checkVersion();

    },

    checkUser: function () {
      if (this.userCollection.models.length === 0) {
        this.navigate("login", {trigger: true});
        return false;
      }
      return true;
    },

    applicationDetail: function (mount) {
      this.naviView.selectMenuItem('applications-menu');

      if (this.foxxList.length === 0) {
        this.foxxList.fetch({ async: false });
      }
      if (!this.hasOwnProperty('applicationDetailView')) {
        this.applicationDetailView = new window.ApplicationDetailView({
          model: this.foxxList.get(decodeURIComponent(mount))
        });
      }

      this.applicationDetailView.model = this.foxxList.get(decodeURIComponent(mount));
      this.applicationDetailView.render();
    },

    login: function () {
      if (!this.loginView) {
        this.loginView = new window.loginView({
          collection: this.userCollection
        });
      }
      this.loginView.render();
      this.naviView.selectMenuItem('');
    },

    collections: function () {
      var naviView = this.naviView, self = this;
      if (!this.collectionsView) {
        this.collectionsView = new window.CollectionsView({
          collection: this.arangoCollectionsStore
        });
      }
      this.arangoCollectionsStore.fetch({
        success: function () {
          self.collectionsView.render();
          naviView.selectMenuItem('collections-menu');
        }
      });
    },

    documents: function (colid, pageid) {
      if (!this.documentsView) {
        this.documentsView = new window.DocumentsView({
          collection: new window.arangoDocuments(),
          documentStore: this.arangoDocumentStore,
          collectionsStore: this.arangoCollectionsStore
        });
      }
      this.documentsView.setCollectionId(colid, pageid);
      this.documentsView.render();

    },

    document: function (colid, docid) {
      if (!this.documentView) {
        this.documentView = new window.DocumentView({
          collection: this.arangoDocumentStore
        });
      }
      this.documentView.colid = colid;
      this.documentView.docid = docid;
      this.documentView.render();
      var type = arangoHelper.collectionApiType(colid);
      this.documentView.setType(type);
    },

    shell: function () {
      if (!this.shellView) {
        this.shellView = new window.shellView();
      }
      this.shellView.render();
      this.naviView.selectMenuItem('tools-menu');
    },

    query: function () {
      if (!this.queryView) {
        this.queryView = new window.queryView({
          collection: this.queryCollection
        });
      }
      this.queryView.render();
      this.naviView.selectMenuItem('query-menu');
    },

    queryManagement: function () {
      if (!this.queryManagementView) {
        this.queryManagementView = new window.queryManagementView({
          collection: undefined
        });
      }
      this.queryManagementView.render();
      this.naviView.selectMenuItem('tools-menu');
    },

    api: function () {
      if (!this.apiView) {
        this.apiView = new window.ApiView();
      }
      this.apiView.render();
      this.naviView.selectMenuItem('tools-menu');
    },

    databases: function () {
      if (arangoHelper.databaseAllowed() === true) {
        if (! this.databaseView) {
          this.databaseView = new window.databaseView({
            users: this.userCollection,
            collection: this.arangoDatabase
          });
          }
          this.databaseView.render();
          this.naviView.selectMenuItem('databases-menu');
        }
        else {
          this.navigate("#", {trigger: true});
          this.naviView.selectMenuItem('dashboard-menu');
          $('#databaseNavi').css('display', 'none');
          $('#databaseNaviSelect').css('display', 'none');
        }
      },

      dashboard: function () {
        this.naviView.selectMenuItem('dashboard-menu');
        if (this.dashboardView === undefined) {
          this.dashboardView = new window.DashboardView({
            dygraphConfig: window.dygraphConfig,
            database: this.arangoDatabase
          });
        }
        this.dashboardView.render();
      },

      graphManagement: function () {
        if (!this.graphManagementView) {
          this.graphManagementView =
          new window.GraphManagementView(
            {
              collection: new window.GraphCollection(),
              collectionCollection: this.arangoCollectionsStore
            }
          );
        }
        this.graphManagementView.render();
        this.naviView.selectMenuItem('graphviewer-menu');
      },

      applications: function () {
        if (this.applicationsView === undefined) {
          this.applicationsView = new window.ApplicationsView({
            collection: this.foxxList
          });
        }
        this.applicationsView.reload();
        this.naviView.selectMenuItem('applications-menu');
      },

      appDocumentation: function (mount) {
        var docuView = new window.AppDocumentationView({mount: mount});
        docuView.render();
        this.naviView.selectMenuItem('applications-menu');
      },

      handleSelectDatabase: function () {
        this.naviView.handleSelectDatabase();
      },

      handleResize: function () {
        if (this.dashboardView) {
          this.dashboardView.resize();
        }
        if (this.graphManagementView) {
          this.graphManagementView.handleResize($("#content").width());
        }
        if (this.queryView) {
          this.queryView.resize();
        }
      },

      userManagement: function () {
        if (!this.userManagementView) {
          this.userManagementView = new window.userManagementView({
            collection: this.userCollection
          });
        }
        this.userManagementView.render();
        this.naviView.selectMenuItem('tools-menu');
      },

      userProfile: function () {
        if (!this.userManagementView) {
          this.userManagementView = new window.userManagementView({
            collection: this.userCollection
          });
        }
        this.userManagementView.render(true);
        this.naviView.selectMenuItem('tools-menu');
      }
    });

  }());
