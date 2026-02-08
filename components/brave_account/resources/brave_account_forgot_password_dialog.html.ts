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
      alert-message="$i18n{BRAVE_ACCOUNT_ALERT_MESSAGE}"
      dialog-description="$i18n{BRAVE_ACCOUNT_FORGOT_PASSWORD_DIALOG_DESCRIPTION}"
      dialog-title="$i18n{BRAVE_ACCOUNT_FORGOT_PASSWORD_DIALOG_TITLE}"
      show-back-button
    >
      <div slot="inputs">
        <leo-input
          placeholder="$i18n{BRAVE_ACCOUNT_EMAIL_INPUT_PLACEHOLDER}"
          type="email"
          @input=${this.onEmailInput}
        >
          <div
            class="label ${this.email.length !== 0 && !this.isEmailValid
              ? 'error'
              : ''}"
          >
            $i18n{BRAVE_ACCOUNT_EMAIL_INPUT_LABEL}
          </div>
        </leo-input>
      </div>
      <leo-button
        slot="buttons"
        ?isDisabled=${!this.isEmailValid}
      >
        $i18n{BRAVE_ACCOUNT_RESET_PASSWORD_BUTTON_LABEL}
      </leo-button>
    </brave-account-dialog>
    <!--_html_template_end_-->`
}
