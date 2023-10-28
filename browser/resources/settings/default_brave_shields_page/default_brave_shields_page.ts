/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import './brave_adblock_subpage.js';
import '//resources/cr_elements/md_select.css.js';

import {PolymerElement} from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {PrefsMixin} from 'chrome://resources/cr_components/settings_prefs/prefs_mixin.js';
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js';
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'

import {loadTimeData} from '../i18n_setup.js';
import {RouteObserverMixin, RouteObserverMixin, Router, Router} from '../router.js';

import {DefaultBraveShieldsBrowserProxyImpl, DefaultBraveShieldsBrowserProxyImpl} from './default_brave_shields_browser_proxy.js';
import {getTemplate} from './default_brave_shields_page.html.js'

const BraveShieldsPageBase = WebUiListenerMixin(I18nMixin(PrefsMixin(RouteObserverMixin(PolymerElement))))

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
      httpsUpgradeControlTypes_: {
          readOnly: true,
          type: Array,
          value: function () {
            return [
                { value: 'block',
                  name: loadTimeData.getString('strictHttpsUpgrade') },
                { value: 'block_third_party',
                  name: loadTimeData.getString('standardHttpsUpgrade') },
                { value: 'allow',
                  name: loadTimeData.getString('disabledHttpsUpgrade') }
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
      httpsUpgradeControlType_: String,
      isAdBlockRoute_: {
        type: Boolean,
        value: false
      },
      isDebounceFeatureEnabled_: {
        readOnly: true,
        type: Boolean,
        value: function () {
          return loadTimeData.getBoolean('isDebounceFeatureEnabled')
        }
      },
      isHttpsByDefaultEnabled_: {
        type: Boolean,
        value: loadTimeData.getBoolean('isHttpsByDefaultEnabled')
      },
      isForgetFirstPartyStorageFeatureEnabled_: {
        type: Boolean,
        value: loadTimeData.getBoolean('isForgetFirstPartyStorageFeatureEnabled')
      },
      isForgetFirstPartyStorageEnabled_: {
        type: Object,
        value: {
          key: '',
          type: chrome.settingsPrivate.PrefType.BOOLEAN,
          value: false,
        }
      }
    }
  }

  browserProxy_ = DefaultBraveShieldsBrowserProxyImpl.getInstance()

  ready () {
    super.ready()

    this.onShieldsSettingsChanged_()

    this.addWebUiListener('brave-shields-settings-changed',
      () => { this.onShieldsSettingsChanged_() })
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

  onShieldsSettingsChanged_ () {
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

    this.browserProxy_.getHttpsUpgradeControlType().then(value => {
      this.httpsUpgradeControlType_ = value
    })

    this.browserProxy_.getForgetFirstPartyStorageEnabled().then(value => {
      this.isForgetFirstPartyStorageEnabled_ = {
        key: '',
        type: chrome.settingsPrivate.PrefType.BOOLEAN,
        value: value,
      }
    })
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

  onHttpsUpgradeControlChange_ () {
    this.browserProxy_.setHttpsUpgradeControlType(
      this.$.httpsUpgradeControlType.value)
  }

  onNoScriptControlChange_ () {
    this.browserProxy_.setNoScriptControlType(this.$.noScriptControlType.checked)
  }

  onForgetFirstPartyStorageToggleChange_ () {
    this.browserProxy_.setForgetFirstPartyStorageEnabled(
      this.$.forgetFirstPartyStorageControlType.checked
    )
  }
}

customElements.define(
  BraveShieldsPage.is, BraveShieldsPage)
