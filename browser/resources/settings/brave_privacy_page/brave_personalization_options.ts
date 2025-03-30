// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {BaseMixin} from '../base_mixin.js'
import {loadTimeData} from '../i18n_setup.js'

import {getTemplate} from './brave_personalization_options.html.js'
import {BravePrivacyBrowserProxy, BravePrivacyBrowserProxyImpl} from './brave_privacy_page_browser_proxy.js'

import '../privacy_page/do_not_track_toggle.js'

const SettingsBravePersonalizationOptionsBase = BaseMixin(PolymerElement) as {
  new(): PolymerElement
}

export class SettingsBravePersonalizationOptions extends SettingsBravePersonalizationOptionsBase {
  static get is() {
    return 'settings-brave-personalization-options'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      webRTCPolicies_: {
        readOnly: true,
        type: Array,
        value: function () {
          return [
            { value: 'default', name: loadTimeData.getString('webRTCDefault') },
            { value: 'default_public_and_private_interfaces', name: loadTimeData.getString('defaultPublicAndPrivateInterfaces') },
            { value: 'default_public_interface_only', name: loadTimeData.getString('defaultPublicInterfaceOnly') },
            { value: 'disable_non_proxied_udp', name: loadTimeData.getString('disableNonProxiedUdp') }
          ]
        },
      },
      webRTCPolicy_: String,
      isDebounceFeatureEnabled_: {
        readOnly: true,
        type: Boolean,
        value: function () {
          return loadTimeData.getBoolean('isDebounceFeatureEnabled')
        }
      },
      isRequestOTRFeatureEnabled_: {
        readOnly: true,
        type: Boolean,
        value: function () {
          return loadTimeData.getBoolean('isRequestOTRFeatureEnabled')
        }
      },
      requestOTRActions_: {
        readOnly: true,
        type: Array,
        value: function () {
          return [
            { value: 0, name: loadTimeData.getString('requestOTRDefault') },
            { value: 1, name: loadTimeData.getString('requestOTRAlways') },
            { value: 2, name: loadTimeData.getString('requestOTRNever') },
          ]
        },
      },
      requestOTRAction_: String,
    }
  }

  private webRTCPolicies_: Object[]
  private webRTCPolicy_: String
  private isDebounceFeatureEnabled_: boolean
  private isRequestOTRFeatureEnabled_: boolean
  private requestOTRActions_: Object[]
  private requestOTRAction_: String

  browserProxy_: BravePrivacyBrowserProxy = BravePrivacyBrowserProxyImpl.getInstance()

  shouldShowRestart_(enabled: boolean) {
    return enabled != this.browserProxy_.wasPushMessagingEnabledAtStartup()
  }

  restartBrowser_(e: Event) {
    e.stopPropagation()
    window.open("chrome://restart", "_self")
  }

  override ready() {
    super.ready()
    // Add hr to the "Do not track" row.
    const doNotTrack = this.shadowRoot?.querySelector('#doNotTrack')
    if (doNotTrack) {
      const toggle = doNotTrack.shadowRoot?.querySelector('#toggle')
      if (toggle) {
        const toggleClass = toggle.getAttribute('class')
        toggle.setAttribute('class', toggleClass + ' hr')
      } else {
        console.log(
          '[Brave Settings Overrides] Could not find doNotTrack toggle')
      }
    } else {
      console.log(
        '[Brave Settings Overrides] Could not find element with id doNotTrack')
    }
  }
}

customElements.define(
  SettingsBravePersonalizationOptions.is, SettingsBravePersonalizationOptions)
