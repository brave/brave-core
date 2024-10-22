/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_dialog.js';
import 'chrome://resources/brave/leo.bundle.js'
import 'chrome://resources/polymer/v3_0/iron-flex-layout/iron-flex-layout-classes.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {getTemplate} from './brave_account_entry_dialog.html.js'

/**
 * @fileoverview
 * 'settings-brave-account-dialog'...
 */

class SettingsBraveAccountEntryDialogElement extends PolymerElement {
  static get is() {
    return 'settings-brave-account-entry-dialog'
  }

  static get template() {
    return getTemplate()
  }

  private onSignInButtonClicked() {
    this.dispatchEvent(new CustomEvent('sign-in-button-clicked'))
  }

  private onSelfCustodyButtonClicked() {
    this.dispatchEvent(new CustomEvent('self-custody-button-clicked'))
  }
}

customElements.define(
  SettingsBraveAccountEntryDialogElement.is, SettingsBraveAccountEntryDialogElement)
