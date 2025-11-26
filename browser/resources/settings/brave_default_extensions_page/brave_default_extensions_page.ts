// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './brave_extensions_manifest_v2_subpage.js';

import {PrefsMixin, PrefsMixinInterface} from '/shared/settings/prefs/prefs_mixin.js';
import {WebUiListenerMixin, WebUiListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import type {CrViewManagerElement} from 'chrome://resources/cr_elements/cr_view_manager/cr_view_manager.js';

import {SettingsCheckboxElement} from '../controls/settings_checkbox.js';
import {loadTimeData} from '../i18n_setup.js';
import {Router} from '../router.js';

import {SettingsViewMixin, SettingsViewMixinInterface} from '../settings_page/settings_view_mixin.js';
import {SearchableViewContainerMixin, SearchableViewContainerMixinInterface} from '../settings_page/searchable_view_container_mixin.js';

import {BraveDefaultExtensionsBrowserProxyImpl} from './brave_default_extensions_browser_proxy.js'
import {getTemplate} from './brave_default_extensions_page.html.js'

const SettingBraveDefaultExtensionsPageElementBase =
  SearchableViewContainerMixin(SettingsViewMixin(WebUiListenerMixin(
    PrefsMixin(PolymerElement)))) as new () =>
      PolymerElement & WebUiListenerMixinInterface & PrefsMixinInterface &
        SearchableViewContainerMixinInterface & SettingsViewMixinInterface

export interface SettingBraveDefaultExtensionsPageElement {
  $: {
    widevineEnabled: SettingsCheckboxElement,
    viewManager: CrViewManagerElement,
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
      widevineEnabledPref_: {
        type: Object,
        value() {
          // TODO(dbeam): this is basically only to appease PrefControlMixin.
          // Maybe add a no-validate attribute instead? This makes little sense.
          return {}
        },
      },
      isExtensionsManifestV2FeatureEnabled_: {
        value: () => loadTimeData.getBoolean('extensionsManifestV2Feature'),
      }
    }
  }

  private browserProxy_ = BraveDefaultExtensionsBrowserProxyImpl.getInstance()
  declare showRestartToast_: boolean
  declare widevineEnabledPref_: chrome.settingsPrivate.PrefObject
  declare isExtensionsManifestV2FeatureEnabled_: boolean

  override ready() {
    super.ready()

    this.addWebUiListener(
      'brave-needs-restart-changed', (needsRestart: boolean) => {
        this.showRestartToast_ = needsRestart
    })
    this.browserProxy_.getRestartNeeded().then(show => {
      this.showRestartToast_ = show
    })

    // PrefControlMixin checks for a pref being valid, so have to fake it,
    // same as upstream.
    const setWidevineEnabledPref = (enabled: boolean) => this.setWidevineEnabledPref_(enabled)
    this.addWebUiListener('widevine-enabled-changed', setWidevineEnabledPref)
    this.browserProxy_.isWidevineEnabled().then(setWidevineEnabledPref)
  }

  override getAssociatedControlFor(childViewId: string): HTMLElement {
    switch (childViewId) {
      case 'manageV2Extensions':
        return this.shadowRoot!.querySelector('#manageV2Extensions')!;
      default:
        throw new Error(`Unknown child view id: ${childViewId}`)
    }
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

  openManageV2ExtensionPage_() {
    const router = Router.getInstance();
    router.navigateTo(router.getRoutes().EXTENSIONS_V2);
  }

  openExtensionsPage_() {
    window.open("chrome://extensions", "_self")
  }

  openKeyboardShortcutsPage_() {
    window.open("chrome://extensions/shortcuts", "_self")
  }

  openWebStoreUrl_() {
    window.open(loadTimeData.getString('getMoreExtensionsUrl'), undefined, 'noreferrer')
  }

  shouldShowRestartForGoogleLogin_(value: boolean) {
    return this.browserProxy_.wasSignInEnabledAtStartup() !== value
  }

  shouldShowRestartForMediaRouter_(value: boolean) {
    return this.browserProxy_.isMediaRouterEnabled() !== value
  }
}

customElements.define(
  SettingBraveDefaultExtensionsPageElement.is, SettingBraveDefaultExtensionsPageElement)
