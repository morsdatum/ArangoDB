/*jshint browser: true */
/*jshint unused: false */
/*global require, exports, Backbone, window */
window.StatisticsCollection = Backbone.Collection.extend({
  model: window.Statistics,
  url: "/_admin/statistics"
});
