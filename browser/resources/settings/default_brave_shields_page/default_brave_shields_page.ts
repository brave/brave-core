/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import './brave_adblock_subpage.js';
import { PolymerElement } from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import '//resources/cr_elements/md_select.css.js';
import { DefaultBraveShieldsBrowserProxyImpl } from './default_brave_shields_browser_proxy.js';
import {Router, RouteObserverMixin} from '../router.js';

import {loadTimeData} from '../i18n_setup.js';
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js';
import {PrefsMixin} from '../prefs/prefs_mixin.js';
import {getTemplate} from './default_brave_shields_page.html.js'

const BraveShieldsPageBase = I18nMixin(PrefsMixin(RouteObserverMixin(PolymerElement)))

/**
* 'settings-default-brave-shields-page' is the settings page containing brave's
* default shields.
*/
class BraveShieldsPage extends BraveShieldsPageBase {
  static get is () {
    return 'settings-default-brave-shields-page'
  }

  static get template () {
    return getTemplate()
  }

  static get properties () {
    return {
      adControlTypes_: {
        readOnly: true,
        type: Array,
        value: function () {
          return [
              { value: 'block', name: loadTimeData.getString('blockAdsTrackersAggressive') },
              { value: 'block_third_party', name: loadTimeData.getString('blockAdsTrackersStandard') },
              { value: 'allow', name: loadTimeData.getString('allowAdsTrackers') }
          ];
        }
      },
      cookieControlTypes_: {
          readOnly: true,
          type: Array,
          value: function () {
            return [
                { value: 'block', name: loadTimeData.getString('blockAllCookies') },
                { value: 'block_third_party', name: loadTimeData.getString('block3rdPartyCookies') },
                { value: 'allow', name: loadTimeData.getString('allowAllCookies') }
            ];
          }
      },
      fingerprintingControlTypes_: {
          readOnly: true,
          type: Array,
          value: function () {
            return [
                { value: 'block', name: loadTimeData.getString('strictFingerprinting') },
                { value: 'default', name: loadTimeData.getString('standardFingerprinting') },
                { value: 'allow', name: loadTimeData.getString('allowAllFingerprinting') }
            ];
          }
      },
      isBraveRewardsSupported_: {
        readOnly: true,
        type: Boolean,
        value: function () {
          return loadTimeData.getBoolean('isBraveRewardsSupported')
        }
      },
      adControlType_: String,
      cookieControlType_: String,
      fingerprintingControlType_: String,
      isAdBlockRoute_: {
        type: Boolean,
        value: false
      }
    }
  }

  browserProxy_ = DefaultBraveShieldsBrowserProxyImpl.getInstance()

  ready () {
    super.ready()

    this.onAdControlChange_ = this.onAdControlChange_.bind(this)
    this.onCookieControlChange_ = this.onCookieControlChange_.bind(this)
    this.onFingerprintingControlChange_ = this.onFingerprintingControlChange_.bind(this)
    this.onHTTPSEverywhereControlChange_ = this.onHTTPSEverywhereControlChange_.bind(this)
    this.onNoScriptControlChange_ = this.onNoScriptControlChange_.bind(this)
    Promise.all([this.browserProxy_.isAdControlEnabled(), this.browserProxy_.isFirstPartyCosmeticFilteringEnabled()])
      .then(([adControlEnabled, hide1pContent]) => {
      if (adControlEnabled) {
          this.adControlType_ = hide1pContent ? 'block' : 'block_third_party'
      }
      else {
          this.adControlType_ = 'allow'
      }
    })

    this.browserProxy_.getCookieControlType().then(value => {
      this.cookieControlType_ = value
    })

    this.browserProxy_.getFingerprintingControlType().then(value => {
      this.fingerprintingControlType_ = value
    })
  }

  /** @protected */
  currentRouteChanged() {
    const router = Router.getInstance()
    this.isAdBlockRoute_ = (router.getCurrentRoute() == router.getRoutes().SHIELDS_ADBLOCK)
  }

  onAdblockPageClick_() {
    const router = Router.getInstance();
    router.navigateTo(router.getRoutes().SHIELDS_ADBLOCK);
  }

  controlEqual_ (val1, val2) {
    return val1 === val2
  }

  onAdControlChange_ () {
    const setting = this.$.adControlType.value
    const adControlType = (setting !== 'allow')
    this.browserProxy_.setAdControlType(adControlType)
    this.browserProxy_.setCosmeticFilteringControlType(setting)
  }

  onCookieControlChange_ () {
    this.browserProxy_.setCookieControlType(this.$.cookieControlType.value)
  }

  onFingerprintingControlChange_ () {
    this.browserProxy_.setFingerprintingControlType(this.$.fingerprintingControlType.value)
  }

  onHTTPSEverywhereControlChange_ () {
    this.browserProxy_.setHTTPSEverywhereEnabled(this.$.httpsEverywhereControlType.checked)
  }

  onNoScriptControlChange_ () {
    this.browserProxy_.setNoScriptControlType(this.$.noScriptControlType.checked)
  }
}

customElements.define(
  BraveShieldsPage.is, BraveShieldsPage)
