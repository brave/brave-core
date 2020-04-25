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

    behaviors: [
      WebUIListenerBehavior,
    ],

    properties: {
      isSuperReferralActive_: Boolean,
    },

    /** @override */
    created: function() {
      this.browserProxy_ = settings.BraveNewTabBrowserProxyImpl.getInstance();
      isSuperReferralActive_ = false;
    },

    /** @override */
    ready: function() {
      this.browserProxy_.getIsSuperReferralActive().then(isSuperReferralActive => {
        this.isSuperReferralActive_ = isSuperReferralActive;
      })

      this.addWebUIListener('super-referral-active-state-changed', (isSuperReferralActive) => {
        this.isSuperReferralActive_ = isSuperReferralActive;
      })
    },
  });
})();
