/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import { onEyeIconClicked } from './brave_account_common.js'
import { BraveAccountCreateDialogElement } from './brave_account_create_dialog.js'

export function getHtml(this: BraveAccountCreateDialogElement) {
  return html`<!--_html_template_start_-->
    <brave-account-dialog
      dialog-description="$i18n{BRAVE_ACCOUNT_CREATE_DIALOG_DESCRIPTION}"
      dialog-title="$i18n{BRAVE_ACCOUNT_CREATE_DIALOG_TITLE}"
      show-back-button
    >
      <div slot="inputs">
        <leo-input
          placeholder="$i18n{BRAVE_ACCOUNT_EMAIL_INPUT_PLACEHOLDER}"
          showErrors
          @input=${this.onEmailInput}
        >
          <div
            class="label ${(this.email.length !== 0 && !this.isEmailValid)
            || (this.isEmailValid && this.isEmailBraveAlias)
              ? 'error'
              : ''}"
          >
            $i18n{BRAVE_ACCOUNT_EMAIL_INPUT_LABEL}
          </div>
          <div
            class="dropdown ${this.isEmailValid && this.isEmailBraveAlias
              ? 'visible'
              : ''}"
            id="brave-alias-dropdown"
            slot="errors"
          >
            <leo-icon name="warning-triangle-filled"></leo-icon>
            <div>$i18n{BRAVE_ACCOUNT_EMAIL_INPUT_ERROR_MESSAGE}</div>
          </div>
        </leo-input>
        <leo-input
          placeholder="$i18n{BRAVE_ACCOUNT_PASSWORD_INPUT_PLACEHOLDER}"
          showErrors
          type="password"
          @input=${this.onPasswordInput}
        >
          <div class="label">
            $i18n{BRAVE_ACCOUNT_CREATE_PASSWORD_INPUT_LABEL}
          </div>
          <leo-icon
            name="eye-off"
            slot="right-icon"
            @click=${onEyeIconClicked}
          >
          </leo-icon>
          <div
            slot="errors"
            class="dropdown ${this.passwordStrength !== 0 ? 'visible' : ''}"
            id="password-dropdown"
          >
            <password-strength-meter strength=${this.passwordStrength}>
            </password-strength-meter>
          </div>
        </leo-input>
        <leo-input
          placeholder="$i18n{BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_PLACEHOLDER}"
          showErrors
          type="password"
          @input=${this.onConfirmPasswordInput}
        >
          <div
            class="label ${this.passwordConfirmation.length !== 0
            && this.passwordConfirmation !== this.password
              ? 'error'
              : ''}"
          >
            $i18n{BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_LABEL}
          </div>
          <leo-icon
            name="eye-off"
            slot="right-icon"
            @click=${onEyeIconClicked}
          >
          </leo-icon>
          <div
            class="dropdown ${this.passwordConfirmation.length !== 0
              ? 'visible'
              : ''}"
            id="password-confirmation-dropdown"
            slot="errors"
          >
            <leo-icon name=${this.getIconName()}></leo-icon>
            <div>
              ${this.icon === 'check-circle-filled'
                ? html`$i18n{BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_SUCCESS_MESSAGE}`
                : html`$i18n{BRAVE_ACCOUNT_CONFIRM_PASSWORD_INPUT_ERROR_MESSAGE}`}
            </div>
          </div>
        </leo-input>
      </div>
      <leo-button
        slot="buttons"
        ?isDisabled=${!this.isEmailValid
        || (this.isEmailValid && this.isEmailBraveAlias)
        || this.passwordStrength !== 100
        || this.passwordConfirmation !== this.password}
        @click=${this.onCreateAccountButtonClicked}
      >
        $i18n{BRAVE_ACCOUNT_CREATE_ACCOUNT_BUTTON_LABEL}
      </leo-button>
    </brave-account-dialog>
    <!--_html_template_end_-->`
}
