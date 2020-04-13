/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

 (function() {
  'use strict';

  /**
   * 'settings-brave-new-tab-page' is the settings page containing
   * brave's new tab features.
   */
  Polymer({
    is: 'settings-brave-new-tab-page',

    /** @private {?settings.BraveNewTabBrowserProxy} */
    browserProxy_: null,

    /** @override */
    created: function() {
      this.browserProxy_ = settings.BraveNewTabBrowserProxyImpl.getInstance();
    },

    toggleBrandedBackgroundOption_: function(isBackgroundEnabled, isBrandedBackgroundEnabled) {
      // If background image setting is not turned ON,
      // inform the back-end to also disable the branded wallpaper setting.
      // We will later disable interacting with the button as well.
      if (isBackgroundEnabled === false) {
        return {
          key: 'brave.new_tab_page.show_branded_background_image',
          type: 'BOOLEAN',
          value: false
        };
      }
      return isBrandedBackgroundEnabled;
    },
  });
})();
