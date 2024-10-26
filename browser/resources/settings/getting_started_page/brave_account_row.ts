/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import './brave_account_create_dialog.js'
import './brave_account_entry_dialog.js'
import './brave_account_forgot_password_dialog.js'
import './brave_account_sign_in_dialog.js'

import { PolymerElement } from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js'
import { getTemplate } from './brave_account_row.html.js'

export enum DialogType {
  NONE = 0,
  CREATE,
  ENTRY,
  FORGOT_PASSWORD,
  SIGN_IN
}

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

  static get properties() {
    return {
      dialogTypeEnum: {
        type: Object,
        value: DialogType,
      },
    }
  }

  private dialogType: DialogType

  private isDialogType(dialogType: DialogType) {
    return this.dialogType === dialogType
  }

  private onGetStartedButtonClicked_() {
    this.dialogType = DialogType.ENTRY
  }

  private onBraveAccountDialogClosed_() {
    this.dialogType = DialogType.NONE
  }

  private onCreateButtonClicked_() {
    this.dialogType = DialogType.CREATE
  }

  private onSignInButtonClicked_() {
    this.dialogType = DialogType.SIGN_IN
  }

  private onBackButtonClicked() {
    switch (this.dialogType) {
      case DialogType.CREATE:
        this.dialogType = DialogType.ENTRY
        break
      case DialogType.FORGOT_PASSWORD:
        this.dialogType = DialogType.SIGN_IN
        break
      case DialogType.SIGN_IN:
        this.dialogType = DialogType.ENTRY
        break
    }
  }

  private onForgotPassword() {
    this.dialogType = DialogType.FORGOT_PASSWORD
  }
}

customElements.define(SettingsBraveAccountRow.is, SettingsBraveAccountRow)
