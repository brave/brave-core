/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

(function() {
'use strict';

Polymer({
  is: 'settings-brave-appearance-theme',

  behaviors: [
    WebUIListenerBehavior,
  ],

  properties: {
    braveThemeList_: Array,
    braveThemeType_: Number,
  },

  /** @private {?settings.BraveAppearanceBrowserProxy} */
  browserProxy_: null,

  observers: [
    'updateSelected_(braveThemeType_, braveThemeList_)',
  ],

  /** @override */
  created: function() {
    this.browserProxy_ = settings.BraveAppearanceBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.addWebUIListener('brave-theme-type-changed', (type) => {
      this.braveThemeType_ = type;
    })
    this.browserProxy_.getBraveThemeList().then(list => {
      this.braveThemeList_ = JSON.parse(list);
    })
    this.browserProxy_.getBraveThemeType().then(type => {
      this.braveThemeType_ = type;
    })
  },

  onBraveThemeTypeChange_: function() {
    this.browserProxy_.setBraveThemeType(Number(this.$.braveThemeType.value));
  },

  braveThemeTypeEqual_: function(theme1, theme2) {
    return theme1 === theme2;
  },

  // Wait for the dom-repeat to populate the <select> before setting
  // <select>#value so the correct option gets selected.
  updateSelected_: function() {
    this.async(() => {
      this.$.braveThemeType.value = this.braveThemeType_;
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
