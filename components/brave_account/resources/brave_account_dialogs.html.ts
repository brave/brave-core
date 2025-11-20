/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, TemplateResult } from '//resources/lit/v3_0/lit.rollup.js'

import './brave_account_create_dialog.js'
import './brave_account_entry_dialog.js'
import './brave_account_error_dialog.js'
import './brave_account_forgot_password_dialog.js'
import './brave_account_sign_in_dialog.js'
import { BraveAccountDialogs, Dialog } from './brave_account_dialogs.js'
import { Error } from './brave_account_common.js'

export function getHtml(this: BraveAccountDialogs) {
  const dialogHtml: Record<Dialog['type'], () => TemplateResult> = {
    ENTRY: () => html`
      <brave-account-entry-dialog
        @close-dialog=${this.onCloseDialog}
        @create-button-clicked=${() => (this.dialog = { type: 'CREATE' })}
        @self-custody-button-clicked=${() => (this.dialog = { type: 'ENTRY' })}
        @sign-in-button-clicked=${() => (this.dialog = { type: 'SIGN_IN' })}
      >
      </brave-account-entry-dialog>
    `,
    CREATE: () => html`
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
    `,
    SIGN_IN: () => html`
      <brave-account-sign-in-dialog
        @back-button-clicked=${this.onBackButtonClicked}
        @close-dialog=${this.onCloseDialog}
        @forgot-password-button-clicked=${() =>
          (this.dialog = { type: 'FORGOT_PASSWORD' })}
        @error-occurred=${(e: CustomEvent<Error>) =>
          (this.dialog = {
            type: 'ERROR',
            error: e.detail,
          })}
      >
      </brave-account-sign-in-dialog>
    `,
    FORGOT_PASSWORD: () => html`
      <brave-account-forgot-password-dialog
        @back-button-clicked=${this.onBackButtonClicked}
        @close-dialog=${this.onCloseDialog}
      </brave-account-forgot-password-dialog>
    `,
    ERROR: () => html`
      <brave-account-error-dialog
        @back-button-clicked=${this.onBackButtonClicked}
        @close-dialog=${this.onCloseDialog}
        .error=${(this.dialog as Extract<Dialog, { type: 'ERROR' }>).error}
      >
      </brave-account-error-dialog>
    `,
  }

  return dialogHtml[this.dialog.type]()
}
