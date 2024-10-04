/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'

import { onEyeIconClicked } from './brave_account_common.js'
import { SettingsBraveAccountCreateDialogElement } from './brave_account_create_dialog.js'

export function getHtml(this: SettingsBraveAccountCreateDialogElement) {
  return html`<!--_html_template_start_-->
    <settings-brave-account-dialog
      dialog-description="$i18n{braveAccountCreateDialogDescription}"
      dialog-title="$i18n{braveAccountCreateDialogTitle}"
      show-back-button
    >
      <div slot="inputs">
        <leo-input placeholder="$i18n{braveAccountEmailInputPlaceholder}"
                   showErrors
                   @input=${this.onEmailInput}>
          <div class="label ${this.email.length !== 0 && !this.isEmailValid
                           || this.isEmailValid && this.isEmailBraveAlias ?
                              'error' : ''}">
            $i18n{braveAccountEmailInputLabel}
          </div>
          <div class="dropdown ${this.isEmailValid && this.isEmailBraveAlias ?
                                 'visible' : ''}"
               id="brave-alias-dropdown"
               slot="errors">
            <leo-icon name="warning-triangle-filled"></leo-icon>
            <div>$i18n{braveAccountEmailInputErrorMessage}</div>
          </div>
        </leo-input>
        <leo-input placeholder="$i18n{braveAccountAccountNameInputPlaceholder}"
                   @input=${this.onAccountNameInput}>
          <div class="label">$i18n{braveAccountAccountNameInputLabel}</div>
        </leo-input>
        <leo-input placeholder="$i18n{braveAccountPasswordInputPlaceholder}"
                   showErrors
                   type="password"
                   @input=${this.onPasswordInput}>
          <div class="label">$i18n{braveAccountCreatePasswordInputLabel}</div>
          <leo-icon name="eye-off"
                    slot="right-icon"
                    @click=${onEyeIconClicked}>
          </leo-icon>
          <div slot="errors" class="dropdown ${this.passwordStrength !== 0 ?
                                               'visible' : ''}"
                             id="password-dropdown">
            <password-strength-meter strength=${this.passwordStrength}>
            </password-strength-meter>
          </div>
        </leo-input>
        <leo-input placeholder="$i18n{braveAccountConfirmPasswordInputPlaceholder}"
                   showErrors
                   type="password"
                   @input=${this.onConfirmPasswordInput}>
          <div class="label ${this.passwordConfirmation.length !== 0
                           && this.passwordConfirmation !== this.password ?
                              'error' : ''}">
            $i18n{braveAccountConfirmPasswordInputLabel}
          </div>
          <leo-icon name="eye-off"
                    slot="right-icon"
                    @click=${onEyeIconClicked}>
          </leo-icon>
          <div class="dropdown ${this.passwordConfirmation.length !== 0 ?
                                 'visible' : ''}"
               id="password-confirmation-dropdown"
               slot="errors">
            <leo-icon name=${this.getIconName()}></leo-icon>
            <div>
              ${this.icon === 'check-circle-filled'
                ? html`$i18n{braveAccountConfirmPasswordInputSuccessMessage}`
                : html`$i18n{braveAccountConfirmPasswordInputErrorMessage}`
              }
            </div>
          </div>
        </leo-input>
        <leo-checkbox @change=${this.onCheckboxChanged}>
          <div>$i18nRaw{braveAccountConsentCheckboxLabel}</div>
        </leo-checkbox>
      </div>
      <div slot="buttons">
        <leo-button ?isDisabled=${!this.isEmailValid
                               || this.isEmailValid && this.isEmailBraveAlias
                               || !this.isAccountNameValid
                               || this.passwordStrength !== 100
                               || this.passwordConfirmation !== this.password
                               || !this.isCheckboxChecked}
                    @click=${() => this.fire('create-account-button-clicked')}>
          $i18n{braveAccountCreateAccountButtonLabel}
        </leo-button>
      </div>
    </settings-brave-account-dialog>
  <!--_html_template_end_-->`
}
