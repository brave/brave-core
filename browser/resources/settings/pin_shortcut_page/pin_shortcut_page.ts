/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import '../settings_shared.css.js'
import '../settings_vars.css.js'

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js'
import {SettingsPinShortcutPageBrowserProxyImpl} from './pin_shortcut_page_browser_proxy.js'
import {getTemplate} from './pin_shortcut_page.html.js'

const SettingsPinShortcutPageBase = WebUiListenerMixin(PolymerElement)

class SettingsPinShortcutPage extends SettingsPinShortcutPageBase {
  static get is() {
    return 'settings-pin-shortcut-page'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      pinned_: {
        readOnly: false,
        type: Boolean
      }
    }
  }

  private pinned_: boolean

  private browserProxy_: SettingsPinShortcutPageBrowserProxyImpl =
    SettingsPinShortcutPageBrowserProxyImpl.getInstance()

  override ready() {
    super.ready()
    this.pinned_ = false

    this.browserProxy_.checkShortcutPinState()
    this.addWebUiListener('shortcut-pin-state-changed',
      (pinned: boolean) => this.set('pinned_', pinned))
  }

  onPinShortcutTap_() {
    this.browserProxy_.pinShortcut()
  }
}

customElements.define(SettingsPinShortcutPage.is, SettingsPinShortcutPage)
