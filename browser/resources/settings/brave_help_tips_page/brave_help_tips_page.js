/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

(function() {
  'use strict';

  /**
   * 'settings-brave-help-tips-page' is the settings page containing
   * brave's help tips features.
   */
  Polymer({
    is: 'settings-brave-help-tips-page',

    /** @private {?settings.BraveHelpTipsBrowserProxy} */
    browserProxy_: null,

    /** @override */
    created: function() {
      this.browserProxy_ = settings.BraveHelpTipsBrowserProxyImpl.getInstance();
    },
  });
})();
