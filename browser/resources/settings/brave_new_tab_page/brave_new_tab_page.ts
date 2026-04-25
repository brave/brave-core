// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {WebUiListenerMixin, WebUiListenerMixinInterface} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {BaseMixin} from '../base_mixin.js'
import {NewTabOption, BraveNewTabBrowserProxy, BraveNewTabBrowserProxyImpl} from './brave_new_tab_browser_proxy.js'
import {getTemplate} from './brave_new_tab_page.html.js'

/**
 * 'settings-brave-new-tab-page' is the settings page containing
 * brave's new tab features.
 */

const SettingsBraveNewTabPageElementBase =
  WebUiListenerMixin(BaseMixin(PolymerElement)) as {
    new(): PolymerElement & WebUiListenerMixinInterface
  }

export class SettingsBraveNewTabPageElement extends SettingsBraveNewTabPageElementBase {
  static get is() {
    return 'settings-brave-new-tab-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      newTabShowOptions_: Array,
      showNewTabDashboardSettings_: Boolean,
    };
  }

  private declare newTabShowOptions_: NewTabOption[];
  private declare showNewTabDashboardSettings_: boolean;

  browserProxy_: BraveNewTabBrowserProxy = BraveNewTabBrowserProxyImpl.getInstance();

  override ready() {
    super.ready()
    this.openNewTabPage_ = this.openNewTabPage_.bind(this)

    this.browserProxy_.getNewTabShowsOptionsList().then((list: NewTabOption[]) => {
      this.newTabShowOptions_ = list
    })
    this.browserProxy_.shouldShowNewTabDashboardSettings().then((show: boolean) => {
      this.showNewTabDashboardSettings_ = show
    })
    this.addWebUiListener(
      'show-new-tab-dashboard-settings-changed', (show: boolean) => {
        this.showNewTabDashboardSettings_ = show
    })
  }

  openNewTabPage_() {
    window.open("chrome://newTab?openSettings=1", "_self")
  }
}

customElements.define(
  SettingsBraveNewTabPageElement.is, SettingsBraveNewTabPageElement)
