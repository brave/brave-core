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
import { BraveAccountDialogs, DialogType } from './brave_account_dialogs.js'
 
export function getHtml(this: BraveAccountDialogs) {
  switch (this.dialog.type) {
    case DialogType.NONE:
      return nothing
    case DialogType.ENTRY:
      return html`
        <brave-account-entry-dialog
          @close-button-clicked=${this.onCloseButtonClicked}
          @create-button-clicked=${() => (this.dialog = { type: 'CREATE' })}
          @self-custody-button-clicked=${() => (this.dialog = { type: 'NONE' })}
          @sign-in-button-clicked=${() => (this.dialog = { type: 'SIGN_IN' })}
        >
        </brave-account-entry-dialog>
       `
     case DialogType.CREATE:
       return html`
         <brave-account-create-dialog
           @back-button-clicked=${this.onBackButtonClicked}
           @close-button-clicked=${this.onCloseButtonClicked}
           @create-account-button-clicked=${() => (this.dialog = { type: 'NONE' })}
           @error-occurred=${(e: CustomEvent) => (
            this.dialog = {
              type: DialogType.ERROR,
              failure: { kind: 'register', reason: e.detail.error },
            })}
         >
         </brave-account-create-dialog>
       `
     case DialogType.SIGN_IN:
       return html`
         <brave-account-sign-in-dialog
           @back-button-clicked=${this.onBackButtonClicked}
           @close-button-clicked=${this.onCloseButtonClicked}
           @forgot-password-button-clicked=${() =>
             (this.dialog = { type: 'FORGOT_PASSWORD' })}
           @sign-in-button-clicked=${() => {
             this.dialog = { type: 'NONE' }
             this.signedIn = true
           }}
         >
         </brave-account-sign-in-dialog>
       `
     case DialogType.FORGOT_PASSWORD:
       return html`
         <brave-account-forgot-password-dialog
           @back-button-clicked=${this.onBackButtonClicked}
           @close-button-clicked=${this.onCloseButtonClicked}
         >
         </brave-account-forgot-password-dialog>
       `
     case DialogType.ERROR:
       return html`
         <brave-account-error-dialog
           @back-button-clicked=${this.onBackButtonClicked}
           @close-button-clicked=${this.onCloseButtonClicked}
           .failureReason=${this.dialog.failure}
         >
         </brave-account-error-dialog>
       `
   }
 }
