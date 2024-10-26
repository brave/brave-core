/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import { CrLitElement } from '//resources/lit/v3_0/lit.rollup.js'
import { getCss } from './brave_account_row.css.js'
import { getHtml } from './brave_account_row.html.js'

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

export class SettingsBraveAccountRow extends CrLitElement {
  static get is() {
    return 'settings-brave-account-row'
  }

  static override get styles() {
    return getCss()
  }

  override render() {
    return getHtml.bind(this)()
  }

  static override get properties() {
    return {
      dialogType: { type: DialogType },
    }
  }

  protected dialogType: DialogType = DialogType.NONE

  protected onBackButtonClicked() {
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
}

customElements.define(SettingsBraveAccountRow.is, SettingsBraveAccountRow)
