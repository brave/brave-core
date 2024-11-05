/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'
import { SettingsBraveAccountCreateDialogElement } from './brave_account_create_dialog.js'

export function getHtml(this: SettingsBraveAccountCreateDialogElement) {
  return html`<!--_html_template_start_-->
    <settings-brave-account-dialog show-back-button
                                   text-bottom="$i18n{braveSyncBraveAccountDesc}"
                                   text-top="Create your account">
      <div slot="inputs">
        <leo-input placeholder="Enter your email address" @input=${this.onEmailAddressInput}>
          <div class="label">Email address</div>
        </leo-input>
        <leo-input placeholder="Enter a name for your account" @input=${this.onAccountNameInput}>
          <div class="label">Account name</div>
        </leo-input>
        <leo-input showErrors
                   type="password"
                   placeholder="Enter your password"
                   @input=${this.onCreatePasswordInput}>
          <div class="label">Create a password</div>
          <leo-icon slot="right-icon"
                    name="eye-off"
                    @click=${this.OnEyeIconClick}>
          </leo-icon>
          <div id="password_strength_indicator"
               slot="errors"
               class="${this.score !== 0 ? 'visible' : ''} ${this.strength[this.score].toLocaleLowerCase()}">
            <div class="password-strength-bar">
              <div id="password_strength_value" style="width: ${`calc(100% * ${this.score}/${this.regexps.length})`}"></div>
            </div>
            <div id="password_strength_category">${this.strength[this.score]}</div>
          </div>
        </leo-input>
        <leo-input class=${this.passwordConfirmation.length !== 0 && this.passwordConfirmation !== this.password ? 'red-border' : ''}
                   showErrors
                   type="password"
                   placeholder="Confirm your password"
                   @input=${this.onConfirmPasswordInput}>
          <div class="label">Confirm password</div>
          <leo-icon slot="right-icon"
                    name="eye-off"
                    @click=${this.OnEyeIconClick}>
          </leo-icon>
          <div slot="errors" class="password-confirmation-result ${this.passwordConfirmation.length !== 0 ? 'visible' : ''}">
            <leo-icon name=${this.getIconName()}></leo-icon>
            <div>${`Passwords ${this.icon === 'check-circle-filled' ? '' : 'don\'t'} match`}</div>
          </div>
        </leo-input>
        <leo-checkbox @change=${this.onChange}>
          <div>
            I have read and accept the <a href="#">Terms of service</a> and <a href="#">Privacy agreement</a>.
          </div>
        </leo-checkbox>
      </div>
      <div slot="buttons">
        <leo-button size="medium" ?isDisabled=${!this.isEmailAddressValid || !this.isAccountNameValid || this.score !== this.regexps.length || this.passwordConfirmation !== this.password || !this.isChecked}>
          Create account
        </leo-button>
      </div>
    </settings-brave-account-dialog>
  <!--_html_template_end_-->`
}
