/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import './brave_account_credentials_dialog.js'
import './brave_account_entry_dialog.js'
import './brave_account_otp_dialog.js'
import './brave_account_password_reset_dialog.js'
import './brave_account_sign_in_dialog.js'
import { BraveAccountDialogsElement } from './brave_account_dialogs.js'

export function getHtml(this: BraveAccountDialogsElement) {
  switch (this.dialog?.type) {
    case undefined:
      return nothing

    case 'ENTRY':
      return html`
        <brave-account-entry-dialog
          @create-button-clicked=${() =>
            (this.dialog = { type: 'CREDENTIALS' })}
          @sign-in-button-clicked=${() => (this.dialog = { type: 'SIGN_IN' })}
        >
        </brave-account-entry-dialog>
      `

    case 'SIGN_IN':
      return html`
        <brave-account-sign-in-dialog
          .isCapsLockOn=${this.isCapsLockOn}
          @forgot-password-button-clicked=${() =>
            (this.dialog = { type: 'PASSWORD_RESET' })}
        >
        </brave-account-sign-in-dialog>
      `

    case 'PASSWORD_RESET':
      return html`
        <brave-account-password-reset-dialog>
        </brave-account-password-reset-dialog>
      `

    case 'OTP':
      return html`
        <brave-account-otp-dialog .intent=${this.dialog.intent}>
        </brave-account-otp-dialog>
      `

    case 'CREDENTIALS':
      return html`
        <brave-account-credentials-dialog
          .isCapsLockOn=${this.isCapsLockOn}
          .verification=${this.dialog.verification}
        >
        </brave-account-credentials-dialog>
      `
  }
}
