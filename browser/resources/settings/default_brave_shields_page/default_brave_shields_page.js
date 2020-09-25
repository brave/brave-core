/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {Polymer, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import 'chrome://resources/cr_elements/md_select_css.m.js';
import '../settings_shared_css.m.js';
import '../settings_vars_css.m.js';
import {loadTimeData} from "../i18n_setup.js"
import {DefaultBraveShieldsBrowserProxy,  DefaultBraveShieldsBrowserProxyImpl} from './default_brave_shields_browser_proxy.js';


/**
 * 'settings-default-brave-shields-page' is the settings page containing brave's
 * default shields.
 */
Polymer({
  is: 'settings-default-brave-shields-page',

  _template: html`{__html_template__}`,

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

  /** @private {?DefaultBraveShieldsBrowserProxy} */
  browserProxy_: null,

  /** @override */
  created: function() {
    this.browserProxy_ = DefaultBraveShieldsBrowserProxyImpl.getInstance();
  },

  /** @override */
  ready: function() {
    this.onAdControlChange_= this.onAdControlChange_.bind(this)
    this.onCookieControlChange_ = this.onCookieControlChange_.bind(this)
    this.onFingerprintingControlChange_ = this.onFingerprintingControlChange_.bind(this)
    this.onHTTPSEverywhereControlChange_ = this.onHTTPSEverywhereControlChange_.bind(this)
    this.onNoScriptControlChange_ = this.onNoScriptControlChange_.bind(this)

    Promise.all([this.browserProxy_.getAdControlType(), this.browserProxy_.isFirstPartyCosmeticFilteringEnabled()])
        .then(([adControlType, hide1pContent]) => {
      this.adControlType_ = (adControlType === 'allow' ? 'allow' : (hide1pContent ? 'block' : 'block_third_party'))
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
