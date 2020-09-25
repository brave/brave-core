/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

(function() {
'use strict';

/**
 * 'settings-default-brave-shields-page' is the settings page containing brave's
 * default shields.
 */
Polymer({
  is: 'settings-default-brave-shields-page',

  properties: {
    adControlTypes_: {
      readOnly: true,
      type: Array,
      value: function() {
        return [
          {value: 'block', name: loadTimeData.getString('blockAdsTrackersAggressive')},
          {value: 'block_third_party', name: loadTimeData.getString('blockAdsTrackersStandard')},
          {value: 'allow', name: loadTimeData.getString('allowAdsTrackers')}
        ]
      }
    },

    cookieControlTypes_: {
      readOnly: true,
      type: Array,
      value: function() {
        return [
          {value: 'block', name: loadTimeData.getString('blockAllCookies')},
          {value: 'block_third_party', name: loadTimeData.getString('block3rdPartyCookies')},
          {value: 'allow', name: loadTimeData.getString('allowAllCookies')}
        ]
      }
    },

    fingerprintingControlTypes_: {
      readOnly: true,
      type: Array,
      value: function() {
        return [
          {value: 'block', name: loadTimeData.getString('strictFingerprinting')},
          {value: 'default', name: loadTimeData.getString('standardFingerprinting')},
          {value: 'allow', name: loadTimeData.getString('allowAllFingerprinting')}
        ]
      }
    },

    adControlType_: String,
    cookieControlType_: String,
    fingerprintingControlType_: String,
  },

  /** @private {?settings.DefaultBraveShieldsBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = settings.DefaultBraveShieldsBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.onAdControlChange_= this.onAdControlChange_.bind(this)
    this.onCookieControlChange_ = this.onCookieControlChange_.bind(this)
    this.onFingerprintingControlChange_ = this.onFingerprintingControlChange_.bind(this)
    this.onHTTPSEverywhereControlChange_ = this.onHTTPSEverywhereControlChange_.bind(this)
    this.onNoScriptControlChange_ = this.onNoScriptControlChange_.bind(this)

    Promise.all([this.browserProxy_.isAdControlEnabled(), this.browserProxy_.isFirstPartyCosmeticFilteringEnabled()])
        .then(([adControlEnabled, hide1pContent]) => {
      if (adControlEnabled) {
        this.adControlType_ = hide1pContent ? 'block' : 'block_third_party'
      } else {
        this.adControlType_ = 'allow'
      }
    });
    this.browserProxy_.getCookieControlType().then(value => {
      this.cookieControlType_ = value;
    });
    this.browserProxy_.getFingerprintingControlType().then(value => {
      this.fingerprintingControlType_ = value;
    });
  },

  /**
   * @param {string} value
   * @param {string} value
   * @return {boolean}
   * @private
   */
  controlEqual: function(val1, val2) {
    return val1 === val2;
  },

  /**
   * The 'trackers & ads' setting controls both network filtering and cosmetic
   * filtering separately. However, network filtering is simply a boolean
   * toggle; there is no 1p/3p distinction.
   */
  onAdControlChange_: function() {
    const setting = this.$.adControlType.value;
    const adControlType = (setting !== 'allow');
    this.browserProxy_.setAdControlType(adControlType);
    this.browserProxy_.setCosmeticFilteringControlType(setting);
  },
  onCookieControlChange_: function() {
    this.browserProxy_.setCookieControlType(this.$.cookieControlType.value);
  },
  onFingerprintingControlChange_: function() {
    this.browserProxy_.setFingerprintingControlType(this.$.fingerprintingControlType.value);
  },
  onHTTPSEverywhereControlChange_: function() {
    this.browserProxy_.setHTTPSEverywhereEnabled(this.$.httpsEverywhereControlType.checked);
  },
  onNoScriptControlChange_: function() {
    this.browserProxy_.setNoScriptControlType(this.$.noScriptControlType.checked);
  }
});
})();
