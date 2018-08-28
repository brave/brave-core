/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

(function() {
'use strict';

/**
 * 'settings-brave-appearance-page' is the settings page containing brave's
 * appearance settings.
 */
Polymer({
  is: 'settings-brave-appearance-page',

  properties: {
    braveThemeTypes_: {
      readOnly: true,
      type: Array,
      value: [
        'Default',
        'Light',
        'Dark',
      ],
    },
    braveThemeType_: String,
  },

  /** @private {?settings.BraveAppearanceBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.BraveAppearanceBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.browserProxy_.getBraveThemeType().then(theme => {
      this.braveThemeType_ = theme;
    });
  },

  /**
   * @param {string} theme1
   * @param {string} theme2
   * @return {boolean}
   * @private
   */
  braveThemeTypeEqual_: function(theme1, theme2) {
    return theme1 === theme2;
  },

  onBraveThemeTypeChange_: function() {
    this.browserProxy_.setBraveThemeType(this.$.braveThemeType.value);
  },
});
})();
