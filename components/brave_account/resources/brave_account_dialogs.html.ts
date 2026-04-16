/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import './brave_account_create_dialog.js'
import './brave_account_entry_dialog.js'
import './brave_account_forgot_password_dialog.js'
import './brave_account_otp_dialog.js'
import './brave_account_sign_in_dialog.js'
import { BraveAccountDialogsElement } from './brave_account_dialogs.js'

export function getHtml(this: BraveAccountDialogsElement) {
  switch (this.dialog) {
    case undefined:
      return nothing

    case 'ENTRY':
      return html`
        <brave-account-entry-dialog
          @create-button-clicked=${() => (this.dialog = 'CREATE')}
          @sign-in-button-clicked=${() => (this.dialog = 'SIGN_IN')}
        >
        </brave-account-entry-dialog>
      `

    case 'CREATE':
      return html`
        <brave-account-create-dialog .isCapsLockOn=${this.isCapsLockOn}>
        </brave-account-create-dialog>
      `

    case 'SIGN_IN':
      return html`
        <brave-account-sign-in-dialog
          .isCapsLockOn=${this.isCapsLockOn}
          @forgot-password-button-clicked=${() =>
            (this.dialog = 'FORGOT_PASSWORD')}
        >
        </brave-account-sign-in-dialog>
      `

    case 'FORGOT_PASSWORD':
      return html`
        <brave-account-forgot-password-dialog>
        </brave-account-forgot-password-dialog>
      `

    case 'OTP':
      return html` <brave-account-otp-dialog> </brave-account-otp-dialog> `
  }
}
