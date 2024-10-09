/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

// @ts-nocheck TODO(petemill): Define types and remove ts-nocheck

import '../settings_shared.css.js';
import '../settings_vars.css.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {SettingsBraveAccountPageBrowserProxyImpl} from './brave_account_page_browser_proxy.js';
import {getTemplate} from './brave_account_page.html.js'

const SettingsBraveAccountPageBase = WebUiListenerMixin(PolymerElement)

class SettingsBraveAccountPage extends SettingsBraveAccountPageBase {
  static get is() {
    return 'settings-brave-account-page'
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

  browserProxy_ = SettingsBraveAccountPageBrowserProxyImpl.getInstance()

  ready() {
    super.ready()
    this.pinned_ = false

    this.browserProxy_.checkShortcutPinState()
    this.addWebUiListener(
      'shortcut-pin-state-changed', pinned => this.set('pinned_', pinned))
  }

  onPinShortcutTap_() {
    this.browserProxy_.pinShortcut()
  }
}

customElements.define(SettingsBraveAccountPage.is, SettingsBraveAccountPage);
