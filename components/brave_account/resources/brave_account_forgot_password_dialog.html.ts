/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import './brave_account_dialog.js'
import { BraveAccountForgotPasswordDialogElement } from './brave_account_forgot_password_dialog.js'

export function getHtml(this: BraveAccountForgotPasswordDialogElement) {
  return html`<!--_html_template_start_-->
    <brave-account-dialog
      alert-message="$i18n{braveAccountAlertMessage}"
      dialog-description="$i18n{braveAccountForgotPasswordDialogDescription}"
      dialog-title="$i18n{braveAccountForgotPasswordDialogTitle}"
      show-back-button
    >
      <div slot="inputs">
        <leo-input
          placeholder="$i18n{braveAccountEmailInputPlaceholder}"
          @input=${this.onEmailInput}
        >
          <div
            class="label ${this.email.length !== 0 && !this.isEmailValid
              ? 'error'
              : ''}"
          >
            $i18n{braveAccountEmailInputLabel}
          </div>
        </leo-input>
      </div>
      <leo-button
        slot="buttons"
        ?isDisabled=${!this.isEmailValid}
      >
        $i18n{braveAccountResetPasswordButtonLabel}
      </leo-button>
    </brave-account-dialog>
    <!--_html_template_end_-->`
}
