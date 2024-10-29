/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at https://mozilla.org/MPL/2.0/. */

import { html } from '//resources/lit/v3_0/lit.rollup.js'
import { SettingsBraveAccountCreateDialogElement } from './brave_account_create_dialog.js'

export function getHtml(this: SettingsBraveAccountCreateDialogElement) {
  return html`<!--_html_template_start_-->
    <settings-brave-account-dialog show-back-button
      header-text-top="Create your account"
      header-text-bottom="$i18n{braveSyncBraveAccountDesc}">
      <div slot="inputs">
        <leo-input id="email_address" placeholder="Enter your email address" @input=${this.onEmailAddressInput}>
          <div class="label">Email address</div>
        </leo-input>
        <leo-input id="account_name" placeholder="Enter a name for your account" @input=${this.onAccountNameInput}>
          <div class="label">Account name</div>
        </leo-input>
        <div>
          <leo-input id="create_password" type="password" placeholder="Enter your password" @input=${this.onCreatePasswordInput}>
            <div class="label">Create a password</div>
            <leo-icon slot="right-icon"
                      id="icon"
                      name="eye-off"
                      @click=${this.show}>
            </leo-icon>
          </leo-input>
          <div id="password_strength_indicator">
            <div class="password-strength-bar">
              <div id="password_strength_value"></div>
            </div>
            <div id="password_strength_category"></div>
          </div>
        </div>
        <leo-input id="confirm_password"
                   ?disabled=${!this.isCreatePasswordValid}
                   type="password"
                   placeholder="Confirm your password"
                   @input=${this.onConfirmPasswordInput}>
          <div class="label">Confirm password</div>
          <leo-icon slot="right-icon"
                    id="icon"
                    name="eye-off"
                    @click=${this.show}>
          </leo-icon>
        </leo-input>
        <leo-checkbox @change=${this.onChange}>
          <div>
            I have read and accept the  <a href="#">Terms of service</a> and <a href="#">Privacy agreement</a>.
          </div>
        </leo-checkbox>
      </div>
      <div slot="buttons">
        <leo-button size="medium" ?isDisabled=${!this.isEmailAddressValid || !this.isAccountNameValid || !this.isCreatePasswordValid || !this.isConfirmPasswordValid || !this.isChecked}>
          Create account
        </leo-button>
      </div>
    </settings-brave-account-dialog>
  <!--_html_template_end_-->`
}
