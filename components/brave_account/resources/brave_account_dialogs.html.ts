/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import './brave_account_create_dialog.js'
import './brave_account_entry_dialog.js'
import './brave_account_error_dialog.js'
import './brave_account_forgot_password_dialog.js'
import './brave_account_sign_in_dialog.js'
import { BraveAccountDialogs } from './brave_account_dialogs.js'
import { Error } from './brave_account_common.js'

export function getHtml(this: BraveAccountDialogs) {
  switch (this.dialog.type) {
    case 'NONE':
      return nothing
    case 'ENTRY':
      return html`
        <brave-account-entry-dialog
          @close-dialog=${this.onCloseDialog}
          @create-button-clicked=${() => (this.dialog = { type: 'CREATE' })}
          @self-custody-button-clicked=${() => (this.dialog = { type: 'NONE' })}
          @sign-in-button-clicked=${() => (this.dialog = { type: 'SIGN_IN' })}
        >
        </brave-account-entry-dialog>
      `
    case 'CREATE':
      return html`
        <brave-account-create-dialog
          @back-button-clicked=${this.onBackButtonClicked}
          @close-dialog=${this.onCloseDialog}
          @error-occurred=${(e: CustomEvent<Error>) =>
            (this.dialog = {
              type: 'ERROR',
              error: e.detail,
            })}
        >
        </brave-account-create-dialog>
      `
    case 'SIGN_IN':
      return html`
        <brave-account-sign-in-dialog
          @back-button-clicked=${this.onBackButtonClicked}
          @close-dialog=${this.onCloseDialog}
          @forgot-password-button-clicked=${() =>
            (this.dialog = { type: 'FORGOT_PASSWORD' })}
          @sign-in-button-clicked=${() => {
            this.dialog = { type: 'NONE' }
            this.signedIn = true
          }}
        >
        </brave-account-sign-in-dialog>
      `
    case 'FORGOT_PASSWORD':
      return html`
        <brave-account-forgot-password-dialog
          @back-button-clicked=${this.onBackButtonClicked}
          @close-dialog=${this.onCloseDialog}
        </brave-account-forgot-password-dialog>
      `
    case 'ERROR':
      return html`
        <brave-account-error-dialog
          @back-button-clicked=${this.onBackButtonClicked}
          @close-dialog=${this.onCloseDialog}
          .error=${this.dialog.error}
        >
        </brave-account-error-dialog>
      `
  }
}
