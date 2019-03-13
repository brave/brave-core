/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

(function() {
'use strict';

Polymer({
  is: 'settings-brave-appearance-theme',

  properties: {
    braveThemeList_: [],
  },

  /** @private {?settings.BraveAppearanceBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.BraveAppearanceBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.browserProxy_.getBraveThemeList().then(list => {
      this.braveThemeList_ = JSON.parse(list);
    });
  },
});

/**
 * 'settings-brave-appearance-toolbar' is the settings page area containing
 * brave's appearance settings related to the toolbar.
 */
Polymer({
  is: 'settings-brave-appearance-toolbar',
});
})();
