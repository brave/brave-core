/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

(function() {
'use strict';

/**
 * 'settings-brave-sync-page' is the settings page containing brave's
 * custom sync.
 */
Polymer({
  is: 'settings-brave-sync-page',

  properties: {},

  /** @private {?settings.DefaultBraveSyncBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ =
        settings.DefaultBraveSyncBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {},
});
})();
