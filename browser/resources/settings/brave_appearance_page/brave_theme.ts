// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement, html} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {WebUiListenerMixin, WebUiListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {routes} from '../route.js'
import {Router} from '../router.js'
import 'chrome://resources/cr_elements/md_select.css.js';
import '../settings_shared.css.js'
import '../settings_vars.css.js'
import {loadTimeData} from "../i18n_setup.js"
import {BraveAppearanceBrowserProxy,  BraveAppearanceBrowserProxyImpl} from './brave_appearance_browser_proxy.js'
import {BaseMixin} from '../base_mixin.js'
import {getTemplate} from './brave_theme.html.js'

export interface SettingsBraveAppearanceThemeElement {
  $: {
    braveThemeType: HTMLSelectElement
  }
}

const SettingsBraveAppearanceThemeElementBase =
  WebUiListenerMixin(BaseMixin(PolymerElement)) as {
  new (): PolymerElement & WebUiListenerMixinInterface
}

/**
 * 'settings-brave-appearance-theme' is the settings page area containing
 * brave's appearance related settings that located at the top of appearance
 * area.
 */
export class SettingsBraveAppearanceThemeElement extends SettingsBraveAppearanceThemeElementBase {
  static get is() {
    return 'settings-brave-appearance-theme'
  }

  static get template() {
    return getTemplate()
  }

  static get observers() {
    return [
      'updateSelected_(braveThemeType_, braveThemeList_)',
    ]
  }

  browserProxy_: BraveAppearanceBrowserProxy = BraveAppearanceBrowserProxyImpl.getInstance()
  braveThemeList_: chrome.braveTheme.ThemeItem[]
  braveThemeType_: number // index of current theme type in braveThemeList_

  override ready() {
    super.ready()

    this.addWebUiListener('brave-theme-type-changed', (type: number) => {
      this.braveThemeType_ = type;
    })
    this.browserProxy_.getBraveThemeList().then((list) => {
      this.braveThemeList_ = JSON.parse(list) as chrome.braveTheme.ThemeItem[];
    })
    this.browserProxy_.getBraveThemeType().then(type => {
      this.braveThemeType_ = type;
    })
  }

  private onBraveThemeTypeChange_() {
    this.browserProxy_.setBraveThemeType(Number(this.$.braveThemeType.value))
  }

  private braveThemeTypeEqual_(theme1: string, theme2: string) {
    return theme1 === theme2
  }

  private onThemeTap_() {
    Router.getInstance().navigateTo(routes.THEMES)
  }

  private updateSelected_() {
    // Wait for the dom-repeat to populate the <select> before setting
    // <select>#value so the correct option gets selected.
    setTimeout(() => {
      this.$.braveThemeType.value = String(this.braveThemeType_)
    })
  }

  useThemesSubPage_() {
    return loadTimeData.valueExists('superReferralThemeName') &&
      loadTimeData.getString('superReferralThemeName') !== ''
  }
}

customElements.define(
    SettingsBraveAppearanceThemeElement.is, SettingsBraveAppearanceThemeElement)
