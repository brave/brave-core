/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import 'chrome://resources/cr_elements/cr_button/cr_button.js'
import 'chrome://resources/cr_elements/icons.html.js'

import {PrefsMixin} from '/shared/settings/prefs/prefs_mixin.js';
import {I18nMixin} from 'chrome://resources/cr_elements/i18n_mixin.js'
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'

import {BaseMixin} from '../base_mixin.js'
import {loadTimeData} from '../i18n_setup.js'

import {BraveDefaultExtensionsBrowserProxyImpl, ExtensionV2} from './brave_default_extensions_browser_proxy.js'
import {getTemplate} from './brave_extensions_manifest_v2_subpage.html.js'

const BraveExtensionsV2SubpageBase =
  WebUiListenerMixin(PrefsMixin(I18nMixin(BaseMixin(PolymerElement))))

class BraveExtensionsV2Subpage extends BraveExtensionsV2SubpageBase {
  static get is() {
    return 'brave-extensions-v2-subpage'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      braveV2Extensions_: {
        type: Array,
        value: []
      },
      showErrorToast_: {
        type: Boolean,
        value: false,
      },
      toastMessage_: String,
      installInProgress_: Boolean,
    }
  }

  browserProxy_ = BraveDefaultExtensionsBrowserProxyImpl.getInstance()
  braveV2Extensions_: Array<ExtensionV2>
  showErrorToast_: boolean
  toastMessage_: string
  installInProgress_: boolean

  override ready() {
    super.ready()

    if (!loadTimeData.getBoolean('extensionsManifestV2Feature')) {
      return;
    }

    if (loadTimeData.getBoolean('shouldExposeElementsForTesting')) {
      window.testing = window.testing || {}
      window.testing[`extensionsV2Subpage`] = this.shadowRoot
    }

    this.addWebUiListener('brave-extension-manifest-v2-changed',
      () => { this.onExtensionV2Changed_() })
    this.getExtensions_()
  }

  getExtensions_() {
    this.browserProxy_.getExtensionsManifestV2().then((extensions) => {
      this.braveV2Extensions_ = extensions
    })
  }

  onExtensionV2Changed_() {
    this.getExtensions_()
  }

  onExtensionV2EnabledChange_(e: any) {
    e.stopPropagation()

    this.closeToast_()
    this.installInProgress_ = e.target.checked;
    this.browserProxy_
      .enableExtensionManifestV2(e.target.id, e.target.checked)
      .catch((reason: string) => {
        console.log(reason)
        this.toastMessage_ = reason
        this.showErrorToast_ = true
      })
      .finally(() => {
        this.installInProgress_ = false
        this.getExtensions_()
      })
  }

  removeExtension_(e: any) {
    e.stopPropagation()
    this.browserProxy_.removeExtensionManifestV2(e.target.id)
  }

  showRemoveButton_(ext: ExtensionV2): boolean {
    return ext.installed && !ext.enabled
  }

  itemPref_(enabled: boolean) {
    return {
      key: '',
      type: chrome.settingsPrivate.PrefType.BOOLEAN,
      value: enabled,
    }
  }

  closeToast_() {
    this.showErrorToast_ = false
  }
}

customElements.define(BraveExtensionsV2Subpage.is, BraveExtensionsV2Subpage)
