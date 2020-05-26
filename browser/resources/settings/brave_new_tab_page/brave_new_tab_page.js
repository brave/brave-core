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
      isBinanceSupported_: Boolean,
      isBraveTogetherSupported_: Boolean,
    },

    /** @override */
    created: function() {
      this.browserProxy_ = settings.BraveNewTabBrowserProxyImpl.getInstance();
      this.isSuperReferralActive_ = false;
      this.isBinanceSupported_ = false;
      this.isBraveTogetherSupported_ = false;
    },

    /** @override */
    ready: function() {
      this.browserProxy_.getIsSuperReferralActive().then(isSuperReferralActive => {
        this.isSuperReferralActive_ = isSuperReferralActive;
      })
      this.browserProxy_.getIsBinanceSupported().then(isBinanceSupported => {
        this.isBinanceSupported_ = isBinanceSupported;
      })
      this.browserProxy_.getIsBraveTogetherSupported().then(isBraveTogetherSupported => {
        this.isBraveTogetherSupported_ = isBraveTogetherSupported;
      })

      this.addWebUIListener('super-referral-active-state-changed', (isSuperReferralActive) => {
        this.isSuperReferralActive_ = isSuperReferralActive;
      })
    },
  });
})();
