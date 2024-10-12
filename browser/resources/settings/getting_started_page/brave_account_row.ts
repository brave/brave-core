/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_dialog.js';
import '../settings_shared.css.js';
import 'chrome://resources/cr_elements/cr_shared_style.css.js';
import 'chrome://resources/polymer/v3_0/iron-flex-layout/iron-flex-layout-classes.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {getTemplate} from './brave_account_row.html.js'

/**
 * @fileoverview
 * 'settings-brave-account-row'...
 */

class SettingsBraveAccountRow extends PolymerElement {
  static get is() {
    return 'settings-brave-account-row'
  }

  static get template() {
    return getTemplate()
  }

  private dialogType: 'main' | 'sign-in' | null;

  isDialogType(askingType: string) {
    return (this.dialogType === askingType)
  }

  private onGetStartedButtonClicked_() {
    this.dialogType = 'main'
  }

  private onBraveAccountDialogClosed_() {
    this.dialogType = null
  }

  private onSignInButtonClicked_() {
    this.dialogType = 'sign-in'
  }

  private onBraveAccountSignInDialogClosed_() {
  }
}

customElements.define(SettingsBraveAccountRow.is, SettingsBraveAccountRow);
