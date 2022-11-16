// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { WebUIListenerMixin, WebUIListenerMixinInterface } from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {SettingsCheckboxElement} from '../controls/settings_checkbox.js';
import { loadTimeData } from '../i18n_setup.js';
import { PrefsMixin, PrefsMixinInterface } from '../prefs/prefs_mixin.js'
import { BraveDefaultExtensionsBrowserProxyImpl } from './brave_default_extensions_browser_proxy.js'
import { getTemplate } from './brave_default_extensions_page.html.js'

const SettingBraveDefaultExtensionsPageElementBase = WebUIListenerMixin(PrefsMixin(PolymerElement)) as {
  new (): PolymerElement & WebUIListenerMixinInterface & PrefsMixinInterface
}

export interface SettingBraveDefaultExtensionsPageElement {
  $: {
    widevineEnabled: SettingsCheckboxElement,
    webTorrentEnabled: SettingsCheckboxElement,
    hangoutsEnabled: SettingsCheckboxElement
  }
}


/**
 * 'settings-brave-default-extensions-page' is the settings page containing
 * brave's default extensions.
 */
export class SettingBraveDefaultExtensionsPageElement extends SettingBraveDefaultExtensionsPageElementBase {
  static get is() {
    return 'settings-brave-default-extensions-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      showRestartToast_: Boolean,
      unstoppableDomainsResolveMethod_: Array,
      ensResolveMethod_: Array,
      ensOffchainResolveMethod_: Array,
      showEnsOffchainLookupRow_: {
        type: Boolean,
        computed: 'computeShowEnsOffchainLookupRow_(prefs.*)',
      },
      widevineEnabledPref_: {
        type: Object,
        value() {
          // TODO(dbeam): this is basically only to appease PrefControlMixin.
          // Maybe add a no-validate attribute instead? This makes little sense.
          return {}
        },
      }
    }
  }

  private browserProxy_ = BraveDefaultExtensionsBrowserProxyImpl.getInstance()
  showRestartToast_: boolean
  unstoppableDomainsResolveMethod_: any[] // TODO(petemill): Define type
  ensResolveMethod_: any[] // TODO(petemill): Define type
  ensOffchainResolveMethod_: any[] // TODO(petemill): Define type
  widevineEnabledPref_: chrome.settingsPrivate.PrefObject

  override ready() {
    super.ready()

    this.addWebUIListener('brave-needs-restart-changed', (needsRestart: boolean) => {
      this.showRestartToast_ = needsRestart
    })

    this.browserProxy_.getRestartNeeded().then(show => {
      this.showRestartToast_ = show
    })
    this.browserProxy_.getDecentralizedDnsResolveMethodList().then(list => {
      this.unstoppableDomainsResolveMethod_ = list
    })
    this.browserProxy_.getDecentralizedDnsResolveMethodList().then(list => {
      this.ensResolveMethod_ = list
    })
    this.browserProxy_.getEnsOffchainResolveMethodList().then(list => {
      this.ensOffchainResolveMethod_ = list
    })

    // PrefControlMixin checks for a pref being valid, so have to fake it,
    // same as upstream.
    const setWidevineEnabledPref = (enabled: boolean) => this.setWidevineEnabledPref_(enabled)
    this.addWebUIListener('widevine-enabled-changed', setWidevineEnabledPref)
    this.browserProxy_.isWidevineEnabled().then(setWidevineEnabledPref)
  }

  onWebTorrentEnabledChange_() {
    this.browserProxy_.setWebTorrentEnabled(this.$.webTorrentEnabled.checked)
  }

  onHangoutsEnabledChange_() {
    this.browserProxy_.setHangoutsEnabled(this.$.hangoutsEnabled.checked)
  }

  restartBrowser_(e: Event) {
    e.stopPropagation()
    window.open("chrome://restart", "_self")
  }

  setWidevineEnabledPref_(enabled: boolean) {
    const pref = {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    }
    this.widevineEnabledPref_ = pref
  }

  onWidevineEnabledChange_() {
    this.browserProxy_.setWidevineEnabled(this.$.widevineEnabled.checked)
  }

  openExtensionsPage_() {
    window.open("chrome://extensions", "_self")
  }

  openKeyboardShortcutsPage_() {
    window.open("chrome://extensions/shortcuts", "_self")
  }

  openWebStoreUrl_() {
    window.open(loadTimeData.getString('getMoreExtensionsUrl'))
  }

  shouldShowRestartForGoogleLogin_(value: boolean) {
    return this.browserProxy_.wasSignInEnabledAtStartup() != value
  }

  shouldShowRestartForMediaRouter_(value: boolean) {
    return this.browserProxy_.isMediaRouterEnabled() != value
  }

  computeShowEnsOffchainLookupRow_() {
    if (!this.browserProxy_.isENSL2Enabled())
      return false
    return !!this.prefs && this.getPref('brave.ens.resolve_method').value === 3
  }
}

customElements.define(
  SettingBraveDefaultExtensionsPageElement.is, SettingBraveDefaultExtensionsPageElement)
