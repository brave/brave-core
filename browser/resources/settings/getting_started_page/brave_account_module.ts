/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_dialog.js';
import '../settings_shared.css.js';
import '../settings_vars.css.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {WebUiListenerMixin} from 'chrome://resources/cr_elements/web_ui_listener_mixin.js';
import {getTemplate} from './brave_account_module.html.js'

const SettingsBraveAccountModuleBase = WebUiListenerMixin(PolymerElement)

class SettingsBraveAccountModule extends SettingsBraveAccountModuleBase {
  static get is() {
    return 'settings-brave-account-module'
  }

  static get template() {
    return getTemplate()
  }

  static get properties() {
    return {
      showBraveAccountDialog_: {
        type: Boolean,
        value: false,
      },
    }
  }

  private showBraveAccountDialog_: boolean;

  private onGetStartedButtonClicked_() {
    this.showBraveAccountDialog_ = true
  }

  private onBraveAccountDialogClosed_() {
    this.showBraveAccountDialog_ = false
  }
}

customElements.define(SettingsBraveAccountModule.is, SettingsBraveAccountModule);
