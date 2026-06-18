/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html, nothing } from '//resources/lit/v3_0/lit.rollup.js'

import './brave_account_email_input.js'
import './brave_account_password_input.js'
import { BraveAccountCredentialsDialogElement } from './brave_account_credentials_dialog.js'
import type { EmailInputEventDetail } from './brave_account_email_input.js'
import type { PasswordInputEventDetail } from './brave_account_password_input.js'
import type { PasswordStrengthChangedEventDetail } from './brave_account_password_strength_meter.js'

export function getHtml(this: BraveAccountCredentialsDialogElement) {
  return html`<!--_html_template_start_-->
    <brave-account-dialog
      .dialogDescription=${this.verification
        ? '$i18n{BRAVE_ACCOUNT_SET_NEW_PASSWORD_DIALOG_DESCRIPTION}'
        : '$i18n{BRAVE_ACCOUNT_DESCRIPTION}'}
      .dialogTitle=${this.verification
        ? '$i18n{BRAVE_ACCOUNT_SET_NEW_PASSWORD_DIALOG_TITLE}'
        : '$i18n{BRAVE_ACCOUNT_CREATE_DIALOG_TITLE}'}
      ?show-back-button=${!this.verification}
    >
      <div slot="inputs">
        ${this.verification
          ? nothing
          : html`<brave-account-email-input
              block-brave-alias
              @email-input=${(e: CustomEvent<EmailInputEventDetail>) => {
                this.email = e.detail.email
                this.isEmailValid = e.detail.isValid
              }}
            >
            </brave-account-email-input>`}
        <brave-account-password-input
          .config=${{
            mode: 'strength',
            onPasswordStrengthChanged: (
              detail: PasswordStrengthChangedEventDetail,
            ) => {
              this.isPasswordStrongEnough = detail.isStrongEnough
            },
          }}
          .isCapsLockOn=${this.isCapsLockOn}
          label="$i18n{BRAVE_ACCOUNT_CREATE_PASSWORD_INPUT_LABEL}"
          placeholder="$i18n{BRAVE_ACCOUNT_PASSWORD_INPUT_PLACEHOLDER}"
          @password-input=${(e: CustomEvent<PasswordInputEventDetail>) => {
            this.password = e.detail.password
            this.isPasswordValid = e.detail.isValid
          }}
        >
        </brave-account-password-input>
        <brave-account-password-input
          .config=${{ mode: 'confirmation', confirmPassword: this.password }}
          .isCapsLockOn=${this.isCapsLockOn}
          label="$i18n{BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_LABEL}"
          placeholder="$i18n{BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_PLACEHOLDER}"
          @password-input=${(e: CustomEvent<PasswordInputEventDetail>) => {
            this.passwordConfirmation = e.detail.password
          }}
        >
        </brave-account-password-input>
      </div>
      <leo-button
        slot="buttons"
        ?isDisabled=${(!this.verification && !this.isEmailValid)
        || !this.isPasswordValid
        || !this.isPasswordStrongEnough
        || this.passwordConfirmation !== this.password
        || this.isSubmitting}
        @click=${this.onSubmitButtonClicked}
      >
        ${this.verification
          ? '$i18n{BRAVE_ACCOUNT_SET_NEW_PASSWORD_BUTTON_LABEL}'
          : '$i18n{BRAVE_ACCOUNT_CREATE_ACCOUNT_BUTTON_LABEL}'}
      </leo-button>
    </brave-account-dialog>
    <!--_html_template_end_-->`
}
